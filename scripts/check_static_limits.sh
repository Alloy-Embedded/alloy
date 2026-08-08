#!/usr/bin/env bash
# The evidence behind docs/reference/safety.md.
#
# Builds real firmware, then asks the LINKED image four questions: is there a
# libc heap, is there C++ exception or RTTI machinery, is there recursion, and
# how deep can the stack get. Each answer comes with a negative control — a
# deliberately broken build that MUST be caught — because a checker that
# always says "clean" is worth nothing.
#
#   scripts/check_static_limits.sh [<example> <board>]...
#
# Defaults to a spread that covers the interesting cases: bare metal, the
# async executor, littlefs, and lwIP. Needs arm-none-eabi-gcc on PATH.
set -uo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
fail=0
tmp="$(mktemp -d "${TMPDIR:-/tmp}/alloy-limits.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT

alloy() { (cd "$1" && shift && uv run --project "$root/tools/alloy" alloy "$@"); }
limits() { python3 "$root/scripts/static_limits.py" "$@"; }

targets=("blink nucleo_g071rb" "async_blink nucleo_g071rb"
         "fs nucleo_g0b1re" "http_server nucleo_f767zi")
if [ $# -gt 0 ]; then
    targets=()
    while [ $# -gt 1 ]; do targets+=("$1 $2"); shift 2; done
fi

# ---------------------------------------------------------------------------
# The images themselves.
# ---------------------------------------------------------------------------
for target in "${targets[@]}"; do
    set -- $target
    example="$1" board="$2"
    dir="$root/examples/$example"
    echo "== $example@$board"
    if ! alloy "$dir" build --board "$board" >"$tmp/build.log" 2>&1; then
        echo "FAIL: build failed — see $tmp/build.log"; tail -5 "$tmp/build.log"
        fail=1; continue
    fi
    elf="$dir/.alloy/build-tree/$board/out/$example.elf"
    if ! limits "$elf" --json > "$tmp/report.json"; then
        echo "FAIL: $example@$board — see the report below"
        limits "$elf"; fail=1; continue
    fi
    python3 - "$tmp/report.json" "$example@$board" <<'PY' || fail=1
import json, sys
report = json.load(open(sys.argv[1]))
label = sys.argv[2]
bad = []
if not report["recursion"]["clean"]:
    bad.append(f"recursion: {report['recursion']['cycles']}")
if report["heap_symbols"]:
    bad.append(f"heap symbols: {report['heap_symbols']}")
if report["exception_symbols"]:
    bad.append(f"exception machinery: {report['exception_symbols']}")
if report["rtti_symbols"]:
    bad.append(f"rtti: {report['rtti_symbols'][:4]}")
if report["eh_frame_bytes"]:
    bad.append(f".eh_frame is {report['eh_frame_bytes']} bytes")
stack = report["stack"]
for line in bad:
    print(f"FAIL {label}: {line}")
if not bad:
    print(f"ok   {label}: no heap, no exceptions/RTTI, no recursion; "
          f"peak stack {stack['peak_bytes']} bytes "
          f"(thread {stack['thread']['bytes']} + handler "
          f"{(stack['worst_handler'] or {}).get('bytes', 0)} + "
          f"{stack['exception_entry_bytes']} entry), "
          f"{report['indirect_calls']['total']} indirect call(s) not followed, "
          f"{len(report['frames']['estimated_from_prologue'])} frame(s) estimated, "
          f"{len(report['vtable_symbols'])} vtable(s)")
sys.exit(1 if bad else 0)
PY
done

# ---------------------------------------------------------------------------
# Negative control 1: recursion and a heap, in a real alloy project.
# ---------------------------------------------------------------------------
echo "== negative control: a project that DOES recurse and DOES allocate"
mkdir -p "$tmp/nc/src"
cat > "$tmp/nc/alloy.toml" <<EOF
[project]
name = "nc"

[board]
id = "nucleo_g071rb"

[alloy]
root = "$root"
EOF
cat > "$tmp/nc/src/main.cpp" <<'EOF'
#include <alloy/board.hpp>
#include <alloy/time.hpp>
#include <cstdlib>

// noipa: -Os would otherwise turn this pair into a loop and the control would
// pass for the wrong reason.
__attribute__((noinline, noipa)) int pong(int n);
__attribute__((noinline, noipa)) int ping(int n) { return n <= 0 ? 0 : pong(n - 1) + 1; }
__attribute__((noinline, noipa)) int pong(int n) { return n <= 0 ? 0 : ping(n - 1) + 1; }

volatile int sink;

int main() {
    board::init();
    sink = ping(7);
    void* p = std::malloc(64);
    sink += (p != nullptr);
    std::free(p);
    for (;;) { alloy::sleep_for(std::chrono::microseconds{1000}); }
}
EOF
if ! alloy "$tmp/nc" build >"$tmp/nc.log" 2>&1; then
    echo "FAIL: the negative-control project did not build"; tail -5 "$tmp/nc.log"; fail=1
else
    limits "$tmp/nc/.alloy/build-tree/nucleo_g071rb/out/nc.elf" --json > "$tmp/nc.json"
    python3 - "$tmp/nc.json" <<'PY' || fail=1
import json, sys
r = json.load(open(sys.argv[1]))
cycles = r["recursion"]["cycles"]
heap = r["heap_symbols"]
ok = True
if not cycles:
    print("FAIL: mutual recursion was NOT detected — the recursion check is decoration")
    ok = False
else:
    print("ok   negative control: recursion detected — " + " -> ".join(cycles[0]))
if "malloc" not in heap:
    print(f"FAIL: malloc was NOT detected — the heap check is decoration ({heap})")
    ok = False
else:
    print(f"ok   negative control: heap detected — {heap}")
sys.exit(0 if ok else 1)
PY
fi

# ---------------------------------------------------------------------------
# Negative control 2: exceptions and RTTI are refused at COMPILE time, which
# is a stronger statement than "the image happens to contain no unwinder".
# ---------------------------------------------------------------------------
echo "== negative control: -fno-exceptions / -fno-rtti actually refuse the code"
cat > "$tmp/eh.cpp" <<'EOF'
#include <typeinfo>
struct B { virtual ~B() = default; };
struct D : B {};
int uses_rtti(B& b) { return typeid(b) == typeid(D); }
int uses_exceptions() { try { throw 1; } catch (int e) { return e; } }
EOF
gxx="$(command -v arm-none-eabi-g++ || true)"
if [ -z "$gxx" ]; then
    echo "SKIP: arm-none-eabi-g++ not on PATH"
else
    if "$gxx" -std=c++23 -c "$tmp/eh.cpp" -o /dev/null >/dev/null 2>&1; then
        echo "ok   control: the same code compiles when the flags are absent"
    else
        echo "FAIL: the control TU does not compile even WITH exceptions/RTTI"
        fail=1
    fi
    if "$gxx" -std=c++23 -fno-exceptions -fno-rtti -c "$tmp/eh.cpp" -o /dev/null \
        >"$tmp/eh.log" 2>&1; then
        echo "FAIL: -fno-exceptions -fno-rtti accepted throw and typeid"
        fail=1
    else
        echo "ok   negative control: refused —" \
             "$(grep -c 'error:' "$tmp/eh.log") compile error(s)"
    fi
fi

[ "$fail" -eq 0 ] && echo "static limits: every claim on docs/reference/safety.md holds" \
                  || echo "static limits: FAILED"
exit "$fail"
