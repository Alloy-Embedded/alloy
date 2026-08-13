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

# SUB-RESOURCE ORDINAL vs A GENERATED COUNT. `adc.watchdog<N>()` is bounded by
# `Inst::feat::analog_watchdogs`, which comes from alloy-devices — the whole
# point of degree being a generated number rather than a hand-written constant.
# The G0B1RE's adc_v2 has three, so ordinal 3 is one too many. Both numbers
# have to reach the reader, and here they arrive as GCC's "the comparison
# reduces to" note rather than as template arguments, which is exactly why this
# case exists: nothing else in the tree compiles this line.
ADC_WATCHDOG_ORDINAL = """
#include <alloy/board.hpp>
void probe() {
    auto adc = board::adc::open();
    (void)adc.watchdog<3>({.channel = 3, .low = 1000, .high = 3000});
}
"""

# CROSS-PERIPHERAL CAPACITY, stated by the OTHER block. The number of standard
# acceptance filters an FDCAN can hold is the companion message RAM's element
# count (28), not the width of the controller field that counts them (RXGFC.LSS
# is five bits and would admit 31). Asking for 29 must fail with both numbers,
# and they are template arguments so the instantiation line carries them.
CAN_FILTERS_OVER_CAPACITY = """
#include <alloy/board.hpp>
using alloy::can::match;
void probe() {
    board::can.accept_only(
        match(0x000), match(0x001), match(0x002), match(0x003), match(0x004),
        match(0x005), match(0x006), match(0x007), match(0x008), match(0x009),
        match(0x00A), match(0x00B), match(0x00C), match(0x00D), match(0x00E),
        match(0x00F), match(0x010), match(0x011), match(0x012), match(0x013),
        match(0x014), match(0x015), match(0x016), match(0x017), match(0x018),
        match(0x019), match(0x01A), match(0x01B), match(0x01C));
}
"""


# TWO ROLES THAT LOOK LIKE ONE. The window watchdog is a SECOND role beside
# `watchdog`, not a mode of it, and the whole argument for that rests on the
# wrong pairing failing. It does not fail where roles.py claimed it did — an
# ip_class mismatch is a validator WARNING for every role — so the stop is a
# static_assert on the absent `wwdt_impl<>` specialization. An IWDG specializes
# `wdt_impl` and nothing else, which is exactly what makes it detectable here.
WWDT_WRONG_BLOCK = """
#include <alloy/board.hpp>
using wrong = alloy::wwdt::window_watchdog<alloy::dev::iwdg_t, board::clock_profile>;
void probe() { (void)sizeof(wrong); }  // instantiate the class body
"""

# LAYER 1, IMPOSSIBLE VALUE, WHERE THE BOUND IS THE BOARD'S. A WWDG counts its
# own bus clock through a fixed /4096 and seven bits, so the longest window it
# can enforce is a function of PCLK — about 524 ms at 64 MHz, three orders of
# magnitude under the IWDG's ~32 s. The failure this prevents is a user reading
# "watchdog" and writing four seconds: without the check the planner clamps and
# the program believes in a deadline nothing enforces. Rides on
# __builtin_constant_p like the baud case, so it needs a real -O compile.
WWDT_WINDOW_IMPOSSIBLE = """
#include <alloy/board.hpp>
void probe() {
    (void)board::window_watchdog.start({.deadline = std::chrono::microseconds{4'000'000},
                                        .earliest = std::chrono::microseconds{5'000}});
}
"""


# ── The bridge, where the compile-time net is the safety argument ────────
#
# WHY THESE SIX CASES EXIST AND WHY THEY ARE NOT `optimize=True`. The bridge's
# five admissions used to be `__builtin_constant_p` + [[gnu::error]], like the
# baud and window cases above, and src/alloy/bridge.hpp promised they were "a
# COMPILE ERROR at every literal call site". Measured on arm-none-eabi-g++
# 14.2.1 that was false in two directions: nothing fired at -O0, -Og or -O1,
# and even at -Os nothing fired when one function contained two calls to
# open() (neither constant gets propagated, so `__builtin_constant_p` is 0 at
# both sites). A user debugging a bridge at -O0 had no compile-time protection
# at all, and the runtime trap was the entire net.
#
# `open_checked<Config>()` puts the config in a TEMPLATE PARAMETER and the
# five checks become static_asserts, which is why every case below compiles
# with -fsyntax-only — no optimizer, no inlining, nothing to fold. If a future
# edit turns one back into something the optimizer has to find, the case here
# goes red rather than the promise going quietly false again.
#
# ONE MISTAKE, ONE DIAGNOSTIC, which is the half a reader with a smoking
# inverter needs: "I forgot", "I meant zero", "this timer cannot make one that
# long", "it does not fit in a period" and "that frequency is impossible" have
# five different fixes, so each needle below is unique to its own case.

BRIDGE_DEAD_TIME_UNSTATED = """
#include <alloy/board.hpp>
void probe() {
    (void)board::bridge::open_checked<board::bridge::config{.freq_hz = 20'000}>();
}
"""

BRIDGE_DEAD_TIME_ZERO = """
#include <alloy/board.hpp>
void probe() {
    (void)board::bridge::open_checked<board::bridge::config{
        .freq_hz = 20'000,
        .dead_time = alloy::bridge::dead_time::ns(0)}>();
}
"""

# The bound is the DEGREE fact: the curated DTG field width against this
# instance's kernel clock. On the G0B1RE's 64 MHz TIM1 it is 15750 ns, and both
# numbers have to reach the reader.
BRIDGE_DEAD_TIME_TOO_LONG = """
#include <alloy/board.hpp>
void probe() {
    (void)board::bridge::open_checked<board::bridge::config{
        .freq_hz = 1'000,
        .dead_time = alloy::bridge::dead_time::ns(15'751)}>();
}
"""

# 3000 ns of dead time at 200 kHz (a 5000 ns period) leaves nothing for a
# pulse — and 3000 is comfortably inside the generator, so the case that fires
# is this one and not "too long".
BRIDGE_DEAD_TIME_VS_PERIOD = """
#include <alloy/board.hpp>
void probe() {
    (void)board::bridge::open_checked<board::bridge::config{
        .freq_hz = 200'000,
        .dead_time = alloy::bridge::dead_time::ns(3'000)}>();
}
"""

BRIDGE_FREQ_IMPOSSIBLE = """
#include <alloy/board.hpp>
void probe() {
    (void)board::bridge::open_checked<board::bridge::config{
        .freq_hz = 0,
        .dead_time = alloy::bridge::dead_time::ns(500)}>();
}
"""

# THE POSITIVE CONTROL, and without it the five above pass on a facade that
# refuses everything. Three configurations that MUST compile:
#   * the board's own defaults, which is what the example calls;
#   * exactly max_dead_time_ns — the boundary the "too long" bound rounds to,
#     and the one a rounding bug in dtg_max_dead_time_ns already broke once;
#   * the gate-driver spelling, which is a legitimate zero at the timer and
#     must NOT take the ns(0) path.
BRIDGE_ADMITTED = """
#include <alloy/board.hpp>
void probe() {
    (void)board::bridge::open_checked<board::bridge_defaults>();
    (void)board::bridge::open_checked<board::bridge::config{
        .freq_hz = 1'000,
        .dead_time = alloy::bridge::dead_time::ns(15'750)}>();
    (void)board::bridge::open_checked<board::bridge::config{
        .freq_hz = 20'000,
        .dead_time = alloy::bridge::dead_time::inserted_by_the_gate_driver()}>();
}
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


def _expect_success(name: str, result) -> int:
    """A positive control: this one MUST compile. Without it a check that
    rejects everything looks identical to a check that works."""
    if result is None:
        return 1
    if result.returncode != 0:
        print(f"FAIL: {name} did NOT compile — the check is refusing valid "
              f"configurations:\n" + result.stderr[:1500])
        return 1
    print(f"OK: {name} — compiles, as it must")
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


def check_adc_watchdog_ordinal() -> int:
    """Sub-resource degree: an ordinal past the GENERATED count, with both
    numbers in the diagnostic."""
    board = "nucleo_g0b1re"
    return _expect_failure(
        "a sub-resource ordinal past the generated count (adc_v2 has 3 watchdogs)",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          ADC_WATCHDOG_ORDINAL, ["--board", board], board=board),
        ["this ADC does not have that many analog watchdogs", "(3 < 3)"])


def check_can_filters_over_capacity() -> int:
    """Cross-peripheral degree: the capacity is the COMPANION's element count,
    and over-asking names both numbers."""
    board = "nucleo_g0b1re"
    return _expect_failure(
        "more CAN filters than the companion message RAM holds",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          CAN_FILTERS_OVER_CAPACITY, ["--board", board], board=board),
        ["more acceptance filters than this", "filters_fit<29, 28>"])


def check_wwdt_wrong_block() -> int:
    """Two roles that look like one: the IWDG has no window-watchdog driver,
    and the diagnostic has to say which of the two dogs was meant."""
    board = "nucleo_g0b1re"
    return _expect_failure(
        "an INDEPENDENT watchdog bound to the window_watchdog facade",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          WWDT_WRONG_BLOCK, ["--board", board], board=board),
        ["no WINDOW watchdog driver", "wwdt_impl", "alloy::dev::iwdg_t"])


def check_wwdt_window_impossible() -> int:
    """Layer 1 value admission where the bound is the BOARD's clock, not the
    driver's: a four-second window on a 64 MHz APB cannot exist."""
    board = "nucleo_g0b1re"
    return _expect_failure(
        "a window longer than this board's WWDG can count",
        _compile_wrong_tu(ALLOY / "examples" / "blink", board,
                          WWDT_WINDOW_IMPOSSIBLE, ["--board", board],
                          board=board, optimize=True),
        ["alloy::core::admit::wwdt_window", "this window cannot exist"])


def _bridge_case(name: str, source: str, needles: list[str]) -> int:
    """A bridge admission, compiled with -fsyntax-only. The absence of an
    optimizer here is the claim under test."""
    board = "nucleo_g0b1re"
    return _expect_failure(
        name,
        _compile_wrong_tu(ALLOY / "examples" / "bridge", board, source, [],
                          board=board),
        needles)


def check_bridge_dead_time_unstated() -> int:
    """The one that matters: `config{}` has an `unstated` dead time, and a
    complementary pair with no dead time is a short across the DC link."""
    return _bridge_case(
        "a bridge opened with NO stated dead time",
        BRIDGE_DEAD_TIME_UNSTATED,
        ["states NO DEAD TIME", "inserted_by_the_gate_driver",
         "open_checked"])


def check_bridge_dead_time_zero() -> int:
    """…and "I meant zero" is a DIFFERENT diagnostic from "I forgot". Two
    mistakes, two fixes; a shared message would send a reader to the wrong
    one."""
    return _bridge_case(
        "a bridge opened with dead_time::ns(0)",
        BRIDGE_DEAD_TIME_ZERO,
        ["shoot-through spelled as a number"])


def check_bridge_dead_time_too_long() -> int:
    """Degree, from the curated DTG width and this instance's kernel clock —
    15750 ns on a 64 MHz TIM1 — with both numbers in the diagnostic."""
    return _bridge_case(
        "a dead time longer than this timer's generator can encode",
        BRIDGE_DEAD_TIME_TOO_LONG,
        ["cannot be programmed on this timer", "(15751 <= 15750)"])


def check_bridge_dead_time_vs_period() -> int:
    """Both edges of every pair carry the dead time, so 2 x dead_time has to
    fit inside one switching period with room left for a pulse."""
    return _bridge_case(
        "a dead time that swallows the switching period",
        BRIDGE_DEAD_TIME_VS_PERIOD,
        ["does not fit inside the switching period"])


def check_bridge_freq_impossible() -> int:
    return _bridge_case(
        "a bridge switching frequency of zero",
        BRIDGE_FREQ_IMPOSSIBLE,
        ["switching frequency is impossible"])


def check_bridge_admitted() -> int:
    """The positive control for all five above."""
    board = "nucleo_g0b1re"
    return _expect_success(
        "the board defaults, the longest programmable dead time, and the "
        "gate-driver spelling",
        _compile_wrong_tu(ALLOY / "examples" / "bridge", board,
                          BRIDGE_ADMITTED, [], board=board))


def main() -> int:
    return (check_wrong_pin_route() | check_strategy_lacking_concept() |
            check_opts_absent_field() | check_opts_over_ask() |
            check_de_tag_unsupported() | check_baud_zero() |
            check_open_checked_conflict() | check_pwm_channel_disagrees() |
            check_adc_watchdog_ordinal() | check_can_filters_over_capacity() |
            check_wwdt_wrong_block() | check_wwdt_window_impossible() |
            check_bridge_dead_time_unstated() | check_bridge_dead_time_zero() |
            check_bridge_dead_time_too_long() |
            check_bridge_dead_time_vs_period() |
            check_bridge_freq_impossible() | check_bridge_admitted())


if __name__ == "__main__":
    sys.exit(main())
