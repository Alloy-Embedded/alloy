// Prescaler/reload arithmetic for an ST timer, with no register in sight so a
// host test can run it — the same split, and for the same reason, as
// hal/bridge/st_tim_adv_dtg.hpp and hal/window_watchdog/st_wwdg_detail.hpp:
// the driver above needs a generated `alloy/ip/st/tim_gp16.hpp` that only
// exists inside a built project, and tests/ deliberately never reads generated
// headers. The counter's WIDTH arrives as a parameter, so the numbers a test
// pins are the numbers the driver uses.
//
// ── WHY THE PRESCALER IS NOT A CONSTANT ─────────────────────────────────
//
// The obvious implementation picks a round tick — a megahertz, say — and
// divides. It is wrong in a way that only shows up on the bench: at a 1 MHz
// tick a 20 kHz carrier has FIFTY counts of duty resolution, under six bits.
// That is fine for an LED and useless for anything that moves current, and it
// is invisible from the API because set_duty() still takes a 16-bit number and
// still appears to work — the bottom ten bits simply stop meaning anything.
//
// So the prescaler is DERIVED, and it is the smallest one that makes the
// period fit the counter. PSC = 0 whenever the frequency allows it, which
// hands the caller every count the silicon has: at 64 MHz and 20 kHz that is
// 3200 counts instead of 50, or 11.6 bits instead of 5.6. The identical
// argument, and the identical loop, is in st_tim_adv_dtg.hpp for the advanced
// timer — the two are NOT yet one function, deliberately: unifying them means
// editing the bridge's timebase, and that is a safety-critical path that
// should not move in the same change that alters PWM resolution. Doing it is
// a separate step with the bridge's own tests as the gate.

#pragma once

#include <cstdint>

namespace alloy::hal::detail {

struct tim_timebase {
    std::uint32_t psc;  //: what goes in PSC — the DIVIDER MINUS ONE
    std::uint32_t arr;  //: what goes in ARR — the period in ticks, minus one
};

//: The smallest prescaler whose period still fits `max_period_ticks`.
//:
//: `max_period_ticks` is the counter's span (65536 for a 16-bit ARR), taken as
//: a parameter rather than assumed, so a 32-bit counter or a narrower one
//: needs no edit here.
//:
//: Returns {0, 0} for a frequency this timer cannot express rather than
//: dividing by zero. The facade's admission is what rejects that case with a
//: message; this stays total so a test can ask about the edges.
[[nodiscard]] constexpr tim_timebase tim_timebase_for(std::uint32_t kernel_hz,
                                                      std::uint32_t freq_hz,
                                                      std::uint32_t max_period_ticks) {
    if (freq_hz == 0u || max_period_ticks == 0u) {
        return {0u, 0u};
    }
    const std::uint32_t total = kernel_hz / freq_hz;
    std::uint32_t psc = 0u;
    while ((total / (psc + 1u)) > max_period_ticks) {
        ++psc;
    }
    const std::uint32_t arr = total / (psc + 1u);
    return {psc, arr == 0u ? 0u : arr - 1u};
}

//: How many counts of duty a caller actually gets at this setting — the number
//: that decides whether a control loop can hold a set-point, and the one the
//: old fixed-tick implementation silently threw away.
[[nodiscard]] constexpr std::uint32_t tim_duty_steps(std::uint32_t kernel_hz,
                                                     std::uint32_t freq_hz,
                                                     std::uint32_t max_period_ticks) {
    return tim_timebase_for(kernel_hz, freq_hz, max_period_ticks).arr + 1u;
}

}  // namespace alloy::hal::detail
