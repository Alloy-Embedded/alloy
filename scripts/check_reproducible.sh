#!/usr/bin/env bash
# Reproducible-build gate: the same source must produce the same bytes,
# wherever it happens to live on disk.
#
# The method is deliberately adversarial about the one variable that used to
# leak into the artefact — the absolute path. Two copies of the SAME framework
# tree are made at two deliberately different absolute paths (different depth
# AND different length, so a stray path could not accidentally hash the same),
# the example is built in each, and the artefacts are compared by sha256.
#
#   scripts/check_reproducible.sh [example] [board]
#
# Scope of the claim, stated honestly:
#   .elf .bin .hex   MUST match — these are what ships and what gets archived.
#   .map             is NOT compared. It records the CMake object-file paths,
#                    and CMake derives those from the absolute source path for
#                    any source outside its own tree. That is a property of the
#                    build directory, not of the firmware; nothing in the .map
#                    reaches the device. See docs/guide/supply-chain.md.
#
# The device database is shared between the two copies on purpose: alloy-devices
# is read as data (YAML) and its path never enters a compiler command line.
set -euo pipefail

example="${1:-blink}"
board="${2:-nucleo_g071rb}"

# macOS ships shasum, Linux ships sha256sum; this gate runs on both.
if command -v sha256sum >/dev/null 2>&1; then
    sha256() { sha256sum | cut -d' ' -f1; }
else
    sha256() { shasum -a 256 | cut -d' ' -f1; }
fi

root="$(cd "$(dirname "$0")/.." && pwd)"
work="$(mktemp -d "${TMPDIR:-/tmp}/alloy-repro.XXXXXX")"
trap 'rm -rf "$work"' EXIT

# Two roots that differ in length and in depth.
one="$work/a"
two="$work/bb/ccc/a-considerably-longer-second-path"
mkdir -p "$one" "$two"

echo "== copying the framework tree to two different absolute paths"
for dest in "$one" "$two"; do
    rsync -a --exclude '.git' --exclude '.alloy' --exclude '.venv' \
        --exclude '__pycache__' --exclude 'compile_commands.json' \
        "$root/" "$dest/alloy/"
done

devices="${ALLOY_DEVICES_ROOT:-$(cd "$root/.." && pwd)/alloy-devices}"
if [ ! -d "$devices/chips" ]; then
    echo "FAIL: no alloy-devices database at $devices (set ALLOY_DEVICES_ROOT)" >&2
    exit 1
fi

for dest in "$one" "$two"; do
    echo "== building $example@$board under $dest"
    (
        cd "$dest/alloy/examples/$example"
        ALLOY_ROOT="$dest/alloy" ALLOY_DEVICES_ROOT="$devices" \
            uv run --project "$root/tools/alloy" alloy build --board "$board" >/dev/null
    )
done

status=0
for ext in elf bin hex; do
    a="$one/alloy/examples/$example/.alloy/build-tree/$board/out/$example.$ext"
    b="$two/alloy/examples/$example/.alloy/build-tree/$board/out/$example.$ext"
    if [ ! -f "$a" ] || [ ! -f "$b" ]; then
        # .hex/.bin are not emitted for every architecture (xtensa images are
        # multi-segment ELFs). Missing on BOTH sides is fine; on one only is not.
        if [ -f "$a" ] || [ -f "$b" ]; then
            echo "FAIL: $example.$ext exists in one build tree and not the other"
            status=1
        fi
        continue
    fi
    ha="$(sha256 <"$a")"
    hb="$(sha256 <"$b")"
    if [ "$ha" = "$hb" ]; then
        echo "ok   $example.$ext  $ha"
    else
        echo "FAIL $example.$ext  $ha != $hb"
        status=1
    fi
done

# Negative control: prove the comparison can actually fail. If nothing above
# reads the build output, the gate is decoration. Flip one byte of the .bin in
# the second tree and require the same comparison to report a difference.
if [ "$status" -eq 0 ]; then
    a="$one/alloy/examples/$example/.alloy/build-tree/$board/out/$example.bin"
    b="$two/alloy/examples/$example/.alloy/build-tree/$board/out/$example.bin"
    if [ -f "$a" ] && [ -f "$b" ]; then
        printf 'x' | dd of="$b" bs=1 seek=0 conv=notrunc status=none
        if [ "$(sha256 <"$a")" = "$(sha256 <"$b")" ]; then
            echo "FAIL: negative control — a mutated .bin still compared equal"
            status=1
        else
            echo "ok   negative control: a one-byte change is detected"
        fi
    else
        echo "skip negative control: this target emits no .bin (multi-segment ELF)"
    fi
fi

# And the artefact must carry no absolute host path at all — a prefix map that
# silently stopped applying would still pass the two-path comparison if BOTH
# builds leaked the same prefix, which is exactly what a shared CI checkout
# looks like.
elf="$one/alloy/examples/$example/.alloy/build-tree/$board/out/$example.elf"
leaks="$(strings "$elf" | grep -c -E "^(/Users|/home|/private|/tmp|${work}|${root})" || true)"
if [ "$leaks" != "0" ]; then
    echo "FAIL: $leaks absolute host path(s) still embedded in $example.elf"
    strings "$elf" | grep -E "^(/Users|/home|/private|/tmp)" | sort -u | head -5
    status=1
else
    echo "ok   no absolute host paths in $example.elf"
fi

if [ "$status" -eq 0 ]; then
    echo "reproducible: $example@$board is byte-identical from two different paths"
fi
exit "$status"
