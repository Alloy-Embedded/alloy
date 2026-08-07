#!/usr/bin/env python3
"""Compile-time-error UX acceptance tests — wrong code must FAIL to compile
with a diagnostic a human can act on, not silently compile and not regress
into an unreadable template dump. Needs a cross toolchain, so this runs in CI
(and locally with one). Two cases:

Guard #7 (NORTH_STAR — the one it calls "an acceptance test, not a
nice-to-have"): a wrong pin route. Binds USART2 to PA5 (which has no TX route
to USART2 on the STM32G071) and asserts the compiler rejects it and the
output names both the missing route and the pin.

The product dimension's twin: a strategy type that EXISTS but does not
satisfy the app's concept. products/family.toml names the option,
product.hpp aliases it, and the app's
`static_assert(app::control_loop<product::control>)` must fail naming the
concept, the concrete type, and each missing requirement. Witnessed on
arm-none-eabi-g++ 14.2.1: "static assertion failed: the product's control
strategy must satisfy app::control_loop", then "in requirements ... [with S =
product_strategy::vf_scalar]", "the required expression 'S::banner' is
invalid" and "the required expression 's.step(rpm_target)' is invalid".
"""

from __future__ import annotations

import json
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ALLOY = Path(__file__).resolve().parent.parent
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

# The strategy types exist — so product.hpp's `using control = ...;` alias
# binds — but neither satisfies the concept: no banner, no step(). The
# static_assert (the same shape examples/product_demo/src/main.cpp carries)
# must fail and say what is missing.
LACKING_CONCEPT = """
#include <concepts>
#include <cstdint>

namespace app {
template <class S>
concept control_loop = requires(S s, std::int32_t rpm_target) {
    { S::banner } -> std::convertible_to<const char*>;
    { s.step(rpm_target) } -> std::same_as<std::int32_t>;
};
}

namespace product_strategy {
struct vf_scalar {};        // deliberately NOT a control loop
struct sensorless_foc {};
}

#include <alloy/product.hpp>

static_assert(app::control_loop<product::control>,
              "the product's control strategy must satisfy app::control_loop");
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


def _compile_wrong_tu(example: Path, tree: str, source: str,
                      extra_build_args: list[str]) -> subprocess.CompletedProcess | None:
    """Build the example (so headers + compile_commands.json exist), then
    compile `source` with the example's own flags. None = setup failure."""
    subprocess.run(
        ["uv", "run", "--project", str(ALLOY / "tools/alloy"), "alloy",
         "build", "--board", BOARD, *extra_build_args],
        cwd=example, check=True, capture_output=True, text=True,
    )
    cc = next(example.glob(f".alloy/build-tree/{tree}/**/compile_commands.json"), None)
    if cc is None:
        print(f"FAIL: no compile_commands.json after building {example.name}")
        return None
    entry = json.loads(cc.read_text())[0]
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "wrong.cpp"
        src.write_text(source)
        return subprocess.run(
            [*_compile_flags(entry), "-fsyntax-only", str(src)],
            cwd=entry["directory"], capture_output=True, text=True,
        )


def check_wrong_pin_route() -> int:
    result = _compile_wrong_tu(ALLOY / "examples" / "blink", BOARD, WRONG_BIND, [])
    if result is None:
        return 1
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


def check_strategy_lacking_concept() -> int:
    result = _compile_wrong_tu(ALLOY / "examples" / "product_demo",
                               f"{BOARD}+fan_eco", LACKING_CONCEPT,
                               ["--product", "fan_eco"])
    if result is None:
        return 1
    if result.returncode == 0:
        print("FAIL: a strategy lacking the concept COMPILED — the product "
              "strategy contract is not being checked")
        return 1
    err = result.stderr
    if "control_loop" not in err:
        print("FAIL: compile failed but the diagnostic never names the concept "
              "(control_loop):\n" + err[:1500])
        return 1
    if "product_strategy::vf_scalar" not in err:
        print("FAIL: the diagnostic does not name the concrete strategy type "
              "(product_strategy::vf_scalar):\n" + err[:1500])
        return 1
    if "step" not in err:
        print("FAIL: the diagnostic does not point at the missing requirement "
              "(step):\n" + err[:1500])
        return 1
    print("OK: a strategy lacking the concept fails to compile; the diagnostic "
          "names app::control_loop, product_strategy::vf_scalar and the missing "
          "step() requirement")
    return 0


def main() -> int:
    return check_wrong_pin_route() | check_strategy_lacking_concept()


if __name__ == "__main__":
    sys.exit(main())
