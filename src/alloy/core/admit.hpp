// Layer 1 admits only values the port can reach — by DEFAULT, not opt-in.
//
// The layered surface (docs/reference/peripheral-surface.md) made a Layer-1
// FIELD honest: a field exists only where every driver programs it. It said
// nothing about the VALUE. `open({.baud = 0})` compiled with no diagnostic:
// the driver divided by it, GCC folded the constant division to unreachable,
// and the program fell into the nearest trap — which belonged to the
// double-open guard, so even the crash pointed at the wrong bug.
//
// `open_checked<Baud>` existed and was right, and being opt-in made the
// DEFAULT the unsafe one. This closes that, in two mechanisms with one rule:
//
//   compile time, WHEN THE OPTIMIZER PROPAGATES THE CONSTANT — a bonus, not
//   a promise. This comment used to say "which every literal call site is",
//   and that was measured false in two directions (arm-none-eabi-g++ 14.2.1,
//   the bridge/tick reviews):
//
//     one literal call site   -O0 no, -Og no, -O1 varies, -O2 yes, -Os yes
//     two in one function     -O0 no, -Og no, -O1 no,     -O2 yes, -Os NO
//
//   A debug build therefore compiles the impossible value clean and lands in
//   the trap below. `__builtin_constant_p` plus a function declared with the
//   `error` attribute: if the impossible branch survives optimization, the
//   call is a hard error naming this function and, through the inline chain,
//   the binder and the instance.
//
//   run time — THE GUARANTEE. A named trap code in a register (see
//   alloy/core/claim.hpp, including what that does and does not buy).
//
// Where the value can ride a TEMPLATE PARAMETER, that net fires at every
// optimization level including -fsyntax-only: alloy::uart::open_checked<Baud>
// and alloy::bridge::open_checked<Config> are that shape, and it is the one
// to reach for when a check must never depend on inliner heuristics.
//
// WHAT IT CHECKS, and this bound is deliberate: only values no divisor can
// REPRESENT — zero, and rates above the peripheral's own kernel clock. Both
// are true of every dividing peripheral on every vendor, so the check needs
// no per-driver knowledge and can never reject a port that works.
//
// WHAT IT DOES NOT CHECK: accuracy. Whether 3 Mbaud off a 16 MHz kernel lands
// within 2% is `open_checked<Baud>`'s job, and it stays opt-in on purpose — a
// tolerance applied silently to a RUNTIME value is a policy, and a policy that
// refuses a port a product has shipped on for years is its own kind of lie.
// The seam is named on the docs page rather than papered over.
//
// SPELL THE CHECK AT THE CALL SITE. Wrapping these declarations in a helper
// that takes the reject function as a template parameter reads better and
// does not work: GCC folds the indirect call and fires, clang does not, and a
// diagnostic that exists on one compiler is worse than one that exists on
// neither. Two lines per facade, in exchange for both.

#pragma once

namespace alloy::core::admit {

[[gnu::error(
    "alloy::uart::open: this baud rate is impossible on this port — it is zero, "
    "or above the UART's own kernel clock. Layer 1 admits only rates a divisor "
    "can represent; for the accuracy question use open_checked<Baud>()")]]
void uart_baud();

// The bound above is "no divisor can represent it", which is enough for every
// UART whose divisor is a plain divide. An LPUART's is not: it divides
// 256 x f_ck, into a 20-bit register with a forbidden floor, so it has a
// WINDOW — rates too SLOW for the kernel clock are as impossible as rates too
// fast, and no other UART in the tree has that shape.
[[gnu::error(
    "alloy::uart::open: this baud rate is outside the LPUART's divisor window. "
    "The LPUART divides 256 x kernel, and its BRR must land between the "
    "curated floor (feat::brr_min, 0x300 on ST) and its curated width — so the "
    "kernel clock must be between 3x and 4096x the baud rate. On a 64 MHz PCLK "
    "that makes 15 626 baud the SLOWEST reachable rate: for anything slower "
    "the LPUART must be clocked from LSE or HSI16, which alloy's clock_node "
    "cannot select yet")]]
void lpuart_baud_window();

[[gnu::error(
    "alloy::i2c::open: this bus speed is impossible on this port — it is zero, "
    "or above the I2C's own kernel clock")]]
void i2c_speed();

[[gnu::error(
    "alloy::spi::open: this clock rate is impossible on this port — it is zero, "
    "or above the SPI's own kernel clock (every SPI prescaler divides)")]]
void spi_clock();

[[gnu::error(
    "alloy::pwm::open: this frequency is impossible on this timer — it is zero, "
    "or above the timer's own kernel clock")]]
void pwm_freq();

[[gnu::error(
    "alloy::pwm::open: a trigger output was asked for on a timer that has "
    "none. `feat::trgo` for this instance is 0 in the curated chip data — the "
    "block either has no CR2 or its bits 6:4 are the output-idle state, so "
    "there is nothing to program and nothing would ever fire. Use a timer "
    "whose data says it has a trigger output, or drop `.trigger`")]]
void pwm_no_trigger();

[[gnu::error(
    "alloy::tick::open: this rate is impossible on this timer. Both ends of the "
    "window are real: the rate is zero or above the timer's own kernel clock, or "
    "it is so slow that dividing down to it needs more than the 2^32 that a "
    "16-bit prescaler and a 16-bit reload hold together. On a 64 MHz kernel that "
    "floor is about 0.015 Hz")]]
void tick_hz();

[[gnu::error(
    "alloy::encoder::open: this period is impossible on this timer — it is "
    "below 2, or wider than the instance's own counter (the bound is derived "
    "from the curated width of ARR, not written by hand), or wider than the "
    "signed count delta() returns")]]
void encoder_period();

[[gnu::error(
    "alloy::adc::handle::watchdog: this window cannot exist — its low threshold "
    "is above its high threshold, so no conversion can ever fall inside it. An "
    "analog watchdog with an empty window is not a tighter watchdog, it is one "
    "that trips on every sample")]]
void adc_watchdog_window();

[[gnu::error(
    "alloy::adc::handle::watchdog: a threshold is wider than this ADC's watchdog "
    "threshold field — the value would be truncated and the watchdog would guard "
    "a window nobody asked for. The field width comes from the chip database, so "
    "the bound is the silicon's, not this driver's")]]
void adc_watchdog_threshold();

[[gnu::error(
    "alloy::can::accept_only: this acceptance filter cannot exist — a classic-CAN "
    "standard identifier and its match mask are eleven bits "
    "(alloy::can::standard_id_mask), and this one is wider. The bits above the "
    "eleventh are not compared by any controller, so such a filter does not match "
    "NOTHING, it matches a DIFFERENT identifier: match(0x800) is match(0x000) "
    "under another name, and a port that receives the wrong traffic is worse "
    "than one that receives none")]]
void can_filter_id();

// ── The bridge, where an admitted value that is wrong destroys hardware ──
//
// Four separate functions rather than one, because these are the four ways a
// three-phase bridge is misconfigured and telling them apart is the entire
// value of the diagnostic. The message has to be readable by someone holding
// a smoking inverter.

[[gnu::error(
    "alloy::bridge::open: this configuration states NO DEAD TIME. A "
    "complementary pair with no dead time turns both transistors of a "
    "half-bridge on at once during every switching transition, which is a "
    "short across the DC link through the bridge — the failure is immediate "
    "and it is not recoverable. Say what you mean: "
    "`.dead_time = alloy::bridge::dead_time::ns(500)` for the gate driver's "
    "turn-off time plus margin, or "
    "`alloy::bridge::dead_time::inserted_by_the_gate_driver()` if the "
    "hardware between this pin and the gate already does it")]]
void bridge_dead_time_unstated();

[[gnu::error(
    "alloy::bridge::open: `dead_time::ns(0)` is a shoot-through spelled as a "
    "number. Zero is not a dead time; if the gate driver inserts it, say so "
    "with `alloy::bridge::dead_time::inserted_by_the_gate_driver()`, which is "
    "greppable and reviewable and does not read as an oversight")]]
void bridge_dead_time_zero();

[[gnu::error(
    "alloy::bridge::open: this dead time cannot be programmed on this timer. "
    "The dead-time generator counts in ticks of the timer's own clock and its "
    "encoding has a longest value; asking for more than that would program "
    "the widest code and silently give you LESS dead time than you asked for. "
    "The bound comes from the curated DTG field width and the instance's "
    "kernel clock, not from a constant written here")]]
void bridge_dead_time_too_long();

[[gnu::error(
    "alloy::bridge::open: the dead time does not fit inside the switching "
    "period. Both edges of every pair carry it, so 2 x dead_time must be less "
    "than one period — at this frequency it is not, and the bridge would "
    "spend the whole period with both transistors off (or, on a timer that "
    "wraps the compare, with neither of them where the duty says)")]]
void bridge_dead_time_vs_period();

[[gnu::error(
    "alloy::bridge::open: this switching frequency is impossible on this "
    "timer — it is zero, or above the timer's own kernel clock")]]
void bridge_freq();

[[gnu::error(
    "alloy::wwdt::window_watchdog::start: this window cannot exist on this "
    "board. Either the deadline is zero, or the early bound is not BELOW the "
    "deadline (a window that opens after it closes admits no feed at all and "
    "resets the part on the first one), or the deadline is longer than this "
    "WWDG can count. That last bound is the board's, not this driver's: a "
    "window watchdog counts its own bus clock through a fixed /4096 and a "
    "seven-bit counter, so on a 64 MHz APB it tops out near half a SECOND — "
    "three orders of magnitude short of the independent watchdog's ~32 s. "
    "Read `board::window_watchdog.longest` and, if you wanted a long "
    "backstop, the peripheral you wanted is alloy::wdt (the IWDG)")]]
void wwdt_window();

// The only one of these that admits a value the GENERATOR produced rather
// than one the user typed. A board whose chip data carries no map for an
// internal source still emits `board::adc_<source>_channel`, because the
// portable idiom guards the READ with `if constexpr (board::adc_has_<source>)`
// in main() — and outside a template, a discarded if-constexpr branch is
// still type-checked, so an absent symbol would break the code that is doing
// the right thing. The constant therefore carries an impossible channel
// (alloy::adc::channel_none, 0xFF) and this fires when someone reaches for it
// without the guard.
[[gnu::error(
    "alloy::adc::read: this board has no such ADC channel. The channel number "
    "is 0xFF, the value a board emits for an internal source (VREFINT, "
    "temperature, VBAT) that its CHIP DATA DOES NOT MAP — not a channel the "
    "silicon lacks, necessarily, but one nobody has curated a number for. "
    "Guard the read with `if constexpr (board::adc_has_vref)` (or _temp, "
    "_vbat), which is what the generated has-flag exists for; converting this "
    "value would sample whatever pin happens to sit at 255 on a future part")]]
void adc_channel();

}  // namespace alloy::core::admit
