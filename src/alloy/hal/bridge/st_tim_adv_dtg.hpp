// The advanced timer's dead-time arithmetic, with no register in sight so a
// host test can run it. Same split, and for the same reason, as
// alloy/hal/window_watchdog/st_wwdg_detail.hpp: the driver above needs a
// generated `alloy/ip/st/tim_adv.hpp` that only exists inside a built project,
// and tests/ deliberately never reads generated headers. Everything here takes
// the curated field WIDTH as a parameter instead, so the numbers a test pins
// are the numbers the driver uses.
//
// ── DTG IS NOT A NUMBER OF TICKS ────────────────────────────────────────
//
// It is four ranges selected by its own top bits (RM0444 section 21, the BDTR
// description), each with a different step:
//
//   DTG[7:5] = 0xx    DT = DTG[7:0]        x  1 x t_DTS   (0 .. 127)
//   DTG[7:5] = 10x    DT = (64 + DTG[5:0]) x  2 x t_DTS   (128 .. 254)
//   DTG[7:5] = 110    DT = (32 + DTG[4:0]) x  8 x t_DTS   (256 .. 504)
//   DTG[7:5] = 111    DT = (32 + DTG[4:0]) x 16 x t_DTS   (512 .. 1008)
//
// Two consequences, and the whole facade above depends on both:
//
//   NOT EVERY TICK COUNT IS REPRESENTABLE. Above 127 the steps are 2, then 8,
//   then 16, and the ranges do not even abut — 255 and 505..511 are ticks this
//   generator cannot produce at all. So a request usually cannot be met
//   exactly, which is why the handle REPORTS the achieved dead time instead of
//   parroting the request.
//
//   THE ROUNDING DIRECTION IS A SAFETY DECISION, AND IT IS UP. Rounding to
//   nearest would, for a request of 129 ticks, program 128 — 15 ns less dead
//   time than the gate driver was measured to need, on every edge, forever.
//   Too much dead time distorts the output voltage; too little destroys the
//   bridge. Those are not comparable errors.
//
// ── THE THREE ROUNDINGS, WHICH ARE NOT ALL THE SAME DIRECTION ───────────
//
// It took a bug to separate them (see `dtg_max_dead_time_ns`), so they are
// stated as three rules rather than one "round up":
//
//   a REQUEST rounds UP     — ns -> ticks, so the hardware never inserts less
//                             dead time than was asked for.
//   the BOUND rounds DOWN   — the largest request that is still programmable.
//                             Rounding this one up admits a request that the
//                             encoder cannot express, which is the bug.
//   the REPORT rounds DOWN  — ticks -> ns, so a program is never told it has
//                             MORE protection than the silicon inserts. It is
//                             still never below the request: the request was
//                             rounded up to get here.
//
// Nothing in this file has been seen on silicon; no board is on hand. It is
// arithmetic read from the reference data and swept by tests/test_bridge.cpp.

#pragma once

#include <cstdint>

namespace alloy::hal::detail {

//: Ticks of t_DTS that a given DTG code actually produces. The inverse of
//: dtg_code(), and the function the facade uses to report reality.
[[nodiscard]] constexpr std::uint32_t dtg_ticks(std::uint32_t code,
                                                std::uint32_t mask) {
    code &= mask;
    if ((code & 0x80u) == 0u) { return code; }
    if ((code & 0xC0u) == 0x80u) { return (64u + (code & 0x3Fu)) * 2u; }
    if ((code & 0xE0u) == 0xC0u) { return (32u + (code & 0x1Fu)) * 8u; }
    return (32u + (code & 0x1Fu)) * 16u;
}

//: The longest dead time this generator can produce, in ticks — the widest
//: code, decoded. DERIVED from the curated width of DTG rather than written as
//: 1008, so an IP with a narrower or wider field needs no edit here.
[[nodiscard]] constexpr std::uint32_t dtg_max_ticks(std::uint32_t mask) {
    return dtg_ticks(mask, mask);
}

//: Smallest DTG code whose dead time is >= `ticks`.
//:
//: WHEN `ticks` IS BEYOND THE ENCODING this returns the WIDEST code, and that
//: is a deliberate change from returning something above the mask. The caller
//: ORs this into BDTR, where the bits just above DTG are LOCK — so an
//: out-of-range answer of mask+1 did not fail loudly, it programmed DTG = 0
//: (no dead time at all) AND a configuration lock nobody asked for, which is
//: the exact catastrophe this whole facade exists to prevent. Saturating gives
//: the longest dead time the silicon has instead. It is still less than was
//: asked for, so the facade above refuses the case before ever calling here;
//: this is the behaviour when that refusal is bypassed, and of the two
//: available wrong answers it is the survivable one.
[[nodiscard]] constexpr std::uint32_t dtg_code(std::uint32_t ticks,
                                               std::uint32_t mask) {
    const auto ceil_div = [](std::uint32_t a, std::uint32_t b) {
        return (a + b - 1u) / b;
    };
    std::uint32_t code = mask;
    if (ticks <= 127u) {
        code = ticks;
    } else if (ticks <= 254u) {
        code = 0x80u | (ceil_div(ticks, 2u) - 64u);
    } else if (ticks <= 504u) {
        code = 0xC0u | (ceil_div(ticks, 8u) - 32u);
    } else if (ticks <= 1008u) {
        code = 0xE0u | (ceil_div(ticks, 16u) - 32u);
    }
    return code > mask ? mask : code;
}

//: Nanoseconds -> ticks of t_DTS, rounded UP: a REQUEST, so the hardware never
//: inserts less than was asked for.
[[nodiscard]] constexpr std::uint32_t dtg_ns_to_ticks(std::uint32_t ns,
                                                      std::uint32_t dts_hz) {
    const std::uint64_t num =
        static_cast<std::uint64_t>(ns) * static_cast<std::uint64_t>(dts_hz);
    return static_cast<std::uint32_t>((num + 999'999'999u) / 1'000'000'000u);
}

//: Ticks -> nanoseconds, rounded DOWN: a REPORT, so a program is never told it
//: has more dead time than the silicon inserts.
[[nodiscard]] constexpr std::uint32_t dtg_ticks_to_ns(std::uint32_t ticks,
                                                      std::uint32_t dts_hz) {
    if (dts_hz == 0u) { return 0u; }
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(ticks) * 1'000'000'000u) / dts_hz);
}

//: The largest dead time that can be REQUESTED on this instance, in whole
//: nanoseconds — the bound the facade's admission test compares against.
//:
//: IT ROUNDS DOWN, AND THAT IS A BUG FIX, NOT A PREFERENCE. It used to reuse
//: the report's conversion, which rounds up, and the two are not
//: interchangeable here. The generator tops out at exactly
//: max_ticks / dts_hz seconds; rounding that UP to whole nanoseconds names a
//: figure the encoding CANNOT reach, and the figure it names is the one the
//: API itself hands a user who asks for as much protection as the hardware
//: has. That request then passed the "too long" admission, converted back to
//: max_ticks + 1 ticks, and fell off the end of the encoding. On 144 of the
//: 200 integer-megahertz kernel clocks that bound was unreachable by one
//: nanosecond; the lead board's 64 MHz divides exactly, which is why nothing
//: caught it. Flooring makes every admitted request programmable by
//: construction: ns <= floor(max_ticks * 1e9 / hz) implies
//: ceil(ns * hz / 1e9) <= max_ticks.
[[nodiscard]] constexpr std::uint32_t dtg_max_dead_time_ns(std::uint32_t mask,
                                                           std::uint32_t dts_hz) {
    return dtg_ticks_to_ns(dtg_max_ticks(mask), dts_hz);
}

//: What the hardware will actually insert for a requested nanosecond figure.
//: Reported by the handle; never assumed equal to the request.
[[nodiscard]] constexpr std::uint32_t dtg_achieved_ns(std::uint32_t requested_ns,
                                                      std::uint32_t mask,
                                                      std::uint32_t dts_hz) {
    return dtg_ticks_to_ns(
        dtg_ticks(dtg_code(dtg_ns_to_ticks(requested_ns, dts_hz), mask), mask),
        dts_hz);
}

// ── The timebase ────────────────────────────────────────────────────────
//
// PSC = 0 whenever the period fits, which is the opposite of what
// alloy/hal/pwm/st_tim_gp16.hpp does (a fixed 1 MHz tick). A 1 MHz tick at
// 20 kHz leaves 50 counts of duty resolution — under 6 bits — which is fine
// for an LED and not fine for a motor: 2 % duty granularity is 2 % of the DC
// link on every phase. At the full kernel clock the same 20 kHz has 3200
// counts, and the prescaler only appears when the period would otherwise not
// fit in ARR.
//
// CENTER-ALIGNED COUNTS BOTH WAYS, so its period is 2 x ARR ticks and its ARR
// is half the edge-aligned one for the same frequency.

struct tim_adv_timebase {
    std::uint32_t psc;
    std::uint32_t arr;
};

[[nodiscard]] constexpr tim_adv_timebase tim_adv_timebase_for(
    std::uint32_t kernel_hz, std::uint32_t freq_hz, bool center,
    std::uint32_t max_period_ticks) {
    if (freq_hz == 0u || max_period_ticks == 0u) { return {0u, 0u}; }
    const std::uint32_t total = (kernel_hz / freq_hz) / (center ? 2u : 1u);
    std::uint32_t psc = 0u;
    while ((total / (psc + 1u)) > max_period_ticks) { ++psc; }
    const std::uint32_t arr = total / (psc + 1u);
    return {psc, arr == 0u ? 0u : arr - 1u};
}

}  // namespace alloy::hal::detail
