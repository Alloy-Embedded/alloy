#!/usr/bin/env bash
# Contract gates — NORTH_STAR guards #1 and #3, enforced mechanically.
# Run locally before committing; CI runs it on every push.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
fail=0

# Guard #1: no silicon addresses in hand-written code.
# Candidates: hex literals of >= 8 significant digits (with or without C++
# digit separators). Allowed: all-ones masks, and architecture-defined
# constants under src/alloy/arch/ (ARM SysTick block etc.).
addr_pattern="0x[0-9A-Fa-f']{8,}"
if grep -rnE "$addr_pattern" "$root/src/alloy" --include='*.hpp' --include='*.cpp' \
    | grep -v "/src/alloy/arch/" \
    | grep -vE "0xFFFF'?FFFF"; then
    echo "FAIL: hardware address in hand-written C++ (facts must come from alloy-devices)"
    fail=1
fi
# A 32-bit mask needs a 32-bit literal. `1u` is `unsigned int`, which is 16 bits
# wherever the target says so (AVR, MSP430, RL78) — `1u << 20` is then undefined
# behaviour, and the compilers that would tell you are the ones alloy does not
# build for yet. Every mask in src/ is 32 bits wide, so spell the literal that
# way: `std::uint32_t{1} << n`. arch/ is exempt: that code already knows its ISA.
if grep -rnE "\b1u\s*<<" "$root/src/alloy" --include='*.hpp' --include='*.cpp' \
    | grep -v "/src/alloy/arch/"; then
    echo "FAIL: 1u << n builds a 32-bit mask from a 16-bit literal (use std::uint32_t{1})"
    fail=1
fi

# Explicit, auditable exceptions carry a `contract-ok: <reason>` line comment
# (file-format magics like UF2). Grep for contract-ok to review them all.
if grep -rnE "$addr_pattern" "$root/tools/alloy/alloy_cli" --include='*.py' \
    | grep -vE "0xFFFF'?FFFF" \
    | grep -v "contract-ok:"; then
    echo "FAIL: hardware address hardcoded in the code generator"
    fail=1
fi
# Guard #1 over the library ecosystem: libs/ drivers PROMISE chip-freedom
# (concept-templated, "never name a chip") — without this gate the promise
# was unenforced and libs/ was a permanent back door. Same contract-ok
# escape as the generator for auditable device facts (a sensor's own wire
# magic, not MCU silicon). */tests/ are host-only and exempt, like tests/.
if grep -rnE "$addr_pattern" "$root/libs" --include='*.hpp' --include='*.cpp' \
    | grep -v "/tests/" \
    | grep -vE "0xFFFF'?FFFF" \
    | grep -v "contract-ok:"; then
    echo "FAIL: hardware address in a library (libs/ drivers must be chip-free)"
    fail=1
fi

# Guard #3: zero preprocessor conditionals in example/user code.
# (.alloy/ trees are generated/build output, not example code.)
if grep -rnE '^[[:space:]]*#[[:space:]]*(if|ifdef|ifndef|elif)\b' "$root/examples" \
    --include='*.cpp' --include='*.hpp' --exclude-dir='.alloy' 2>/dev/null; then
    echo "FAIL: preprocessor conditional in example code (portability comes from board roles)"
    fail=1
fi

if [ "$fail" -eq 0 ]; then
    echo "contract gates green"
fi
exit "$fail"
