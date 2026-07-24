#!/usr/bin/env python3
"""Guard #7 acceptance test — the one NORTH_STAR calls "an acceptance test, not
a nice-to-have": a wrong pin route must FAIL to compile with a diagnostic that
NAMES the offending pin, not silently compile and not regress into an unreadable
template dump. Needs a cross toolchain, so it runs in CI (and locally with one).

It binds USART2 to PA5 (which has no TX route to USART2 on the STM32G071) and
asserts the compiler rejects it and its output mentions both the missing route
and the pin.
"""

from __future__ import annotations

import json
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ALLOY = Path(__file__).resolve().parent.parent
EXAMPLE = ALLOY / "examples" / "blink"
BOARD = "nucleo_g071rb"

WRONG_BIND = """
#include <alloy/board.hpp>
// PA5 has no TX route to USART2 on the STM32G071: the bind's static_assert must fire.
using wrong = alloy::uart::bind<alloy::dev::usart2_t,
                                alloy::uart::tx<alloy::dev::pa5_t>,
                                alloy::uart::rx<alloy::dev::pa3_t>,
                                board::clock_profile>;
void probe() { (void)sizeof(wrong); }  // force instantiation of the bind body
"""


def _compile_flags(cc_entry: dict) -> list[str]:
    """The example's own compile command, minus the source/output/dep flags."""
    toks = shlex.split(cc_entry["command"])
    flags: list[str] = []
    i = 0
    while i < len(toks):
        t = toks[i]
        if t in ("-o", "-MF", "-MT"):
            i += 2
            continue
        if t in ("-c", "-MD"):
            i += 1
            continue
        if t.endswith((".cpp", ".obj", ".o")):
            i += 1
            continue
        flags.append(t)
        i += 1
    return flags


def main() -> int:
    # Generate + build the board so the headers and compile_commands.json exist.
    subprocess.run(
        ["uv", "run", "--project", str(ALLOY / "tools/alloy"), "alloy", "build", "--board", BOARD],
        cwd=EXAMPLE, check=True, capture_output=True, text=True,
    )
    cc = next(EXAMPLE.glob(f".alloy/build-tree/{BOARD}/**/compile_commands.json"), None)
    if cc is None:
        print("FAIL: no compile_commands.json after build")
        return 1
    entry = json.loads(cc.read_text())[0]

    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "wrong_route.cpp"
        src.write_text(WRONG_BIND)
        result = subprocess.run(
            [*_compile_flags(entry), "-fsyntax-only", str(src)],
            cwd=entry["directory"], capture_output=True, text=True,
        )

    if result.returncode == 0:
        print("FAIL: a wrong pin route COMPILED — the guard #7 route check is not firing")
        return 1
    err = result.stderr.lower()
    if "route" not in err:
        print("FAIL: compile failed but the diagnostic never mentions the route:\n"
              + result.stderr[:1000])
        return 1
    if "pa5" not in err:
        print("FAIL: the diagnostic does not name the offending pin (pa5):\n"
              + result.stderr[:1000])
        return 1
    print("OK: a wrong pin route fails to compile and the diagnostic names pa5 + the missing route")
    return 0


if __name__ == "__main__":
    sys.exit(main())
