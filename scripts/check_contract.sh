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
if grep -rnE "$addr_pattern" "$root/tools/alloy/alloy_cli" --include='*.py' \
    | grep -vE "0xFFFF'?FFFF"; then
    echo "FAIL: hardware address hardcoded in the code generator"
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
