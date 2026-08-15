// The PWM timebase — the arithmetic that decides how many counts of duty a
// caller actually gets, which until now nothing tested at all.
//
// WHY THIS FILE EXISTS. `alloy::pwm` shipped with zero tests of its own; it
// appeared only as the other party in personality-conflict tests elsewhere.
// The driver pinned a 1 MHz tick, so a 20 kHz carrier had FIFTY counts of
// duty — under six bits — and nothing said so: set_duty() still takes a
// uint16_t and still appears to work, the bottom ten bits simply stop meaning
// anything. That is the failure this file exists to make impossible to
// reintroduce.
//
// It runs the REAL arithmetic — hal/pwm/st_tim_timebase.hpp is the driver's
// own, split out precisely so a host test can reach it (the driver proper
// needs a generated alloy/ip/st/tim_gp16.hpp that exists only inside a built
// project, and tests/ deliberately never reads generated headers).
//
// WHAT IT DOES NOT PROVE: that the silicon divides by what these numbers say.
// No board has run this.

#include <cstdint>

#include "alloy/hal/pwm/st_tim_timebase.hpp"
#include "alloy_test.hpp"

namespace dt = alloy::hal::detail;

namespace {
//: The G0's 16-bit ARR, as the driver derives it from the curated width.
constexpr std::uint32_t kSpan = 65536u;
constexpr std::uint32_t k64MHz = 64'000'000u;
}  // namespace

ALLOY_TEST(pwm_timebase_uses_no_prescaler_when_the_period_fits) {
    // THE POINT OF THE WHOLE CHANGE. 64 MHz / 20 kHz = 3200 ticks, which fits
    // a 16-bit counter, so the prescaler must stay out of the way and hand the
    // caller all 3200 counts.
    const dt::tim_timebase tb = dt::tim_timebase_for(k64MHz, 20'000u, kSpan);
    ALLOY_CHECK_EQ(tb.psc, 0u);
    ALLOY_CHECK_EQ(tb.arr, 3199u);
    ALLOY_CHECK_EQ(dt::tim_duty_steps(k64MHz, 20'000u, kSpan), 3200u);
}

ALLOY_TEST(pwm_timebase_beats_the_fixed_megahertz_tick_it_replaced) {
    // The regression this file was written for, stated as a comparison rather
    // than as a number to trust: the old implementation was PSC = kernel/1MHz
    // and period = 1MHz/freq, so 20 kHz gave 50. Anything at or below that at
    // an audible switching frequency is the defect coming back.
    constexpr std::uint32_t old_steps = 1'000'000u / 20'000u;  // = 50
    ALLOY_CHECK_EQ(old_steps, 50u);
    ALLOY_CHECK(dt::tim_duty_steps(k64MHz, 20'000u, kSpan) > old_steps * 60u);
}

ALLOY_TEST(pwm_timebase_reaches_for_the_prescaler_only_when_it_must) {
    // 10 Hz at 64 MHz is 6.4 million ticks and cannot fit 16 bits, so a
    // prescaler is unavoidable — but it must be the SMALLEST that works, or
    // resolution is thrown away for nothing.
    const dt::tim_timebase tb = dt::tim_timebase_for(k64MHz, 10u, kSpan);
    ALLOY_CHECK(tb.psc > 0u);
    ALLOY_CHECK(tb.arr + 1u <= kSpan);
    // One prescaler lower would have overflowed the counter — that is what
    // "smallest that works" means, and it is the half a naive loop gets wrong.
    const std::uint32_t total = k64MHz / 10u;
    ALLOY_CHECK((total / tb.psc) > kSpan);
}

ALLOY_TEST(pwm_timebase_is_monotone_in_frequency) {
    // Sweeping the audio-to-ultrasonic band a converter actually uses: a
    // higher carrier must never buy MORE counts than a lower one. A rounding
    // slip in the prescaler search shows up here as a non-monotone step.
    bool monotone = true;
    std::uint32_t prev = 0xFFFF'FFFFu;
    for (std::uint32_t f = 1'000u; f <= 100'000u; f += 997u) {
        const std::uint32_t steps = dt::tim_duty_steps(k64MHz, f, kSpan);
        if (steps > prev) { monotone = false; }
        prev = steps;
    }
    ALLOY_CHECK(monotone);
}

ALLOY_TEST(pwm_timebase_never_overflows_the_counter_it_was_given) {
    // The safety property: ARR is a 16-bit register on this IP, and a value
    // past its width does not fail — it truncates, and the carrier silently
    // comes out at the wrong frequency. Swept across the whole usable band and
    // both counter widths the tree has.
    bool fits = true;
    for (std::uint32_t f = 1u; f <= 1'000'000u; f = f * 3u + 1u) {
        for (std::uint32_t span : {256u, 65536u}) {
            const dt::tim_timebase tb = dt::tim_timebase_for(k64MHz, f, span);
            if (tb.arr + 1u > span) { fits = false; }
        }
    }
    ALLOY_CHECK(fits);
}

ALLOY_TEST(pwm_timebase_takes_the_counter_span_as_a_fact_not_an_assumption) {
    // A narrower counter must give a coarser answer with no edit to the
    // driver — the same "degree, from the data" rule the DTG mask follows.
    const std::uint32_t wide = dt::tim_duty_steps(k64MHz, 20'000u, 65536u);
    const std::uint32_t narrow = dt::tim_duty_steps(k64MHz, 20'000u, 256u);
    ALLOY_CHECK(narrow < wide);
    ALLOY_CHECK(narrow <= 256u);
}

ALLOY_TEST(pwm_timebase_does_not_divide_by_zero) {
    // Total, so a test can ask about the edges; the facade's admission is
    // what rejects them with a message.
    const dt::tim_timebase z = dt::tim_timebase_for(k64MHz, 0u, kSpan);
    ALLOY_CHECK_EQ(z.psc, 0u);
    ALLOY_CHECK_EQ(z.arr, 0u);
    const dt::tim_timebase n = dt::tim_timebase_for(k64MHz, 20'000u, 0u);
    ALLOY_CHECK_EQ(n.arr, 0u);
    // A frequency above the kernel cannot make a period at all.
    ALLOY_CHECK_EQ(dt::tim_timebase_for(k64MHz, k64MHz * 2u, kSpan).arr, 0u);
}

// ── Centre-aligned, and the trigger gate ─────────────────────────────────

ALLOY_TEST(pwm_centre_aligned_halves_the_steps_it_gets) {
    // A centre-aligned counter walks up and back down inside one period, so
    // its reload is half the edge-aligned one for the same carrier — and so
    // is the duty resolution. That is the trade the caller makes by asking
    // for it, and it must be visible rather than surprising.
    const std::uint32_t edge = dt::tim_duty_steps(k64MHz, 20'000u, kSpan);
    const std::uint32_t centre = dt::tim_duty_steps(k64MHz, 40'000u, kSpan);
    ALLOY_CHECK_EQ(edge, 3200u);
    ALLOY_CHECK_EQ(centre, 1600u);  // the driver asks for 2x the carrier
    ALLOY_CHECK_EQ(edge, centre * 2u);
}

ALLOY_TEST(pwm_centre_aligned_still_never_overflows_the_counter) {
    // The doubling happens BEFORE the prescaler search, so a carrier that
    // barely fits edge-aligned must not overflow when centred — it must reach
    // for the prescaler instead.
    bool fits = true;
    for (std::uint32_t f = 1u; f <= 200'000u; f = f * 3u + 7u) {
        const dt::tim_timebase tb = dt::tim_timebase_for(k64MHz, f * 2u, kSpan);
        if (tb.arr + 1u > kSpan) { fits = false; }
    }
    ALLOY_CHECK(fits);
}
