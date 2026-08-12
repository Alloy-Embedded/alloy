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


# ── The peripheral surface (docs/reference/peripheral-surface.md) ────────
#
# Three cases, one per failure mode the design promises. All three must be a
# COMPILE ERROR that names what you asked for — never a silent no-op, never a
# runtime error code.

# LAYER 2, ABSENT MEMBER. `de_assert_16ths` is a member of uart_opts on the
# usart_v3 driver and is NOT a member on usart_v4, because alloy's curated v4
# register data carries no DEAT/DEDT. On a G0 (usart_v4) the knob is not
# typeable — this is the "you cannot type the lie" claim, mechanised.
OPTS_ABSENT_FIELD = """
#include <alloy/board.hpp>
using Dbg = board::debug_uart;
void probe() {
    (void)Dbg::open<Dbg::opts{.de_assert_16ths = 12}>({.baud = 19'200});
}
"""

# DEGREE, OVER-ASK. On a usart_v3 board the knob EXISTS, and the bound comes
# from the generated register data (DEAT is five bits wide, so raw_mask is 31).
# Asking for 40 must fail with both numbers in the message.
OPTS_OVER_ASK = """
#include <alloy/board.hpp>
using Dbg = board::debug_uart;
void probe() {
    (void)Dbg::open<Dbg::opts{.de_assert_16ths = 40}>({.baud = 19'200});
}
"""

# PIN-BEARING KNOB ON AN IP THAT HAS NONE. Hardware driver-enable is a binder
# TAG, not a config field; a driver whose IP has no DEM bit must reject the tag
# rather than mux a pin and program nothing.
DE_TAG_UNSUPPORTED = """
#include <alloy/board.hpp>
using Bus = alloy::uart::bind<alloy::dev::usart2_t,
                              alloy::uart::tx<alloy::dev::pa2_t>,
                              alloy::uart::rx<alloy::dev::pa3_t>,
                              board::clock_profile,
                              alloy::uart::de<alloy::dev::pa1_t>>;
void probe() { (void)Bus::open({.baud = 19'200}); }
"""

# LAYER 1, IMPOSSIBLE VALUE. The layers made a FIELD honest and said nothing
# about the VALUE: `open({.baud = 0})` used to compile with no diagnostic, fold
# the constant division to unreachable, and fall into the double-open trap — so
# even the crash named the wrong guard. Needs a real -O compile, not
# -fsyntax-only: the check rides on __builtin_constant_p, which is what makes
# it free when the value is a literal.
BAUD_ZERO = """
#include <alloy/board.hpp>
void probe() { (void)board::debug_uart::open({.baud = 0}); }
"""

# TWO SPELLINGS OF ONE FACT. `open_checked<115'200_baud>({.baud = 9'600})`
# compiled clean and ran at 115 200, with the loser never mentioned. The rate
# is stated by the template argument, so open_checked() takes a `frame` whose
# `baud` member has a type nothing converts to — the type's NAME is the error.
OPEN_CHECKED_CONFLICT = """
#include <alloy/board.hpp>
using namespace alloy::literals;
void probe() {
    (void)board::debug_uart::open_checked<115'200_baud>({.baud = 9'600});
}
"""

# SUB-RESOURCE SCOPE, the half of instance ownership that a compiler CAN see.
# `pwm::bind` states the channel twice: as the ordinal the driver programs and
# owns, and as the route signal that picks the pin mux. Nothing tied them
# together, so a bind could mux PA5 onto CH1 while programming CH2 and claiming
# CH2 — which defeats the sub-resource claim, because two binds genuinely
# contesting CH1 would then hold two different keys. PA5 has a real CH1 route
# to TIM2 on the G0B1RE, so the route check passes and the failure left is the
# one under test.
PWM_CHANNEL_DISAGREES = """
#include <alloy/board.hpp>
using wrong = alloy::pwm::bind<alloy::dev::tim2_t, 2u,
                               alloy::dev::pa5_t,
                               alloy::signal::ch1, board::clock_profile>;
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


def _compile_wrong_tu(example: Path, tree: str, source: str,
                      extra_build_args: list[str],
                      board: str = BOARD,
                      optimize: bool = False) -> subprocess.CompletedProcess | None:
    """Build the example (so headers + compile_commands.json exist), then
    compile `source` with the example's own flags. None = setup failure.

    `optimize` swaps -fsyntax-only for a real object compile: a diagnostic that
    rides on __builtin_constant_p (the Layer-1 value admission) only exists
    once the optimizer has folded the constant, which is also precisely why it
    costs nothing when the value is a literal.
    """
    subprocess.run(
        ["uv", "run", "--project", str(ALLOY / "tools/alloy"), "alloy",
         "build", "--board", board, *extra_build_args],
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
        mode = ["-c", "-o", str(Path(tmp) / "wrong.o")] if optimize else ["-fsyntax-only"]
        return subprocess.run(
            [*_compile_flags(entry), *mode, str(src)],
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


def _expect_failure(name: str, result, needles: list[str]) -> int:
    """Shared shape for the surface cases: must not compile, and the
    diagnostic must contain every needle."""
    if result is None:
        return 1
    if result.returncode == 0:
        print(f"FAIL: {name} COMPILED — the check is not firing")
        return 1
    err = result.stderr
    for needle in needles:
        if needle not in err:
            print(f"FAIL: {name} failed to compile but the diagnostic never "
                  f"says {needle!r}:\n" + err[:1500])
            return 1
    lines = len([ln for ln in err.splitlines() if ln.strip()])
    print(f"OK: {name} — compile error, {lines} non-blank lines, "
          f"names {', '.join(repr(n) for n in needles)}")
    return 0


def check_opts_absent_field() -> int:
    """Layer 2: a knob this IP's driver does not have is not a member."""
    return _expect_failure(
        "a Layer-2 knob absent on this IP (usart_v4 has no DEAT/DEDT)",
        _compile_wrong_tu(ALLOY / "examples" / "blink", BOARD,
                          OPTS_ABSENT_FIELD, []),
        ["de_assert_16ths", "uart_opts<alloy::dev::usart2_t>"])


def check_opts_over_ask() -> int:
    """Degree: the knob exists, the value does not fit the generated field."""
    board = "nucleo_f767zi"
    return _expect_failure(
        "a Layer-2 value wider than the generated register field (DEAT is 5 bits)",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          OPTS_OVER_ASK, ["--board", board], board=board),
        ["DE assertion time exceeds", "(40 <= 31)"])


def check_de_tag_unsupported() -> int:
    """Pin-bearing knob: the tag is rejected by drivers with no DEM bit.

    Deliberately on the G0B1RE and not the G071RB: PA1 HAS an RTS route to
    USART2 there, so the bind's route check passes and the failure that is
    left is the one under test — the driver refusing a feature its register
    data does not carry. On a chip with no RTS route at all the route
    static_assert fires first and this case would prove nothing.
    """
    board = "nucleo_g0b1re"
    return _expect_failure(
        "a uart::de<pin> tag on an IP with no hardware driver-enable",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          DE_TAG_UNSUPPORTED, ["--board", board], board=board),
        ["no hardware driver-enable", "usart_v4"])


def check_baud_zero() -> int:
    """Layer 1: a value no divisor can reach is a compile error, by default."""
    return _expect_failure(
        "a Layer-1 baud of zero (impossible value, not an impossible field)",
        _compile_wrong_tu(ALLOY / "examples" / "blink", BOARD, BAUD_ZERO, [],
                          optimize=True),
        ["alloy::core::admit::uart_baud", "this baud rate is impossible",
         "Inst = alloy::dev::usart2_t"])


def check_open_checked_conflict() -> int:
    """Two disagreeing spellings of the baud cannot both be typed."""
    return _expect_failure(
        "open_checked<Baud>() carrying a second, disagreeing baud",
        _compile_wrong_tu(ALLOY / "examples" / "blink", BOARD,
                          OPEN_CHECKED_CONFLICT, []),
        ["alloy::uart::frame", "9600"])


def check_pwm_channel_disagrees() -> int:
    """Sub-resource scope: the ordinal that owns the channel and the signal
    that routes the pin must be the same channel."""
    board = "nucleo_g0b1re"
    return _expect_failure(
        "a pwm::bind whose channel ordinal and channel signal disagree",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          PWM_CHANNEL_DISAGREES, ["--board", board], board=board),
        ["names one channel ordinal and a different channel signal"])


def main() -> int:
    return (check_wrong_pin_route() | check_strategy_lacking_concept() |
            check_opts_absent_field() | check_opts_over_ask() |
            check_de_tag_unsupported() | check_baud_zero() |
            check_open_checked_conflict() | check_pwm_channel_disagrees())


if __name__ == "__main__":
    sys.exit(main())
