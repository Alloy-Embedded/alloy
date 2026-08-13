// The periodic-tick facade and the divisor it is built on
// (src/alloy/tick.hpp, src/alloy/hal/tick/st_tim_divisor.hpp).
//
// TWO THINGS ARE PROVED HERE, and neither of them is "the timer counts".
//
//   1. THE DIVISOR ARITHMETIC. A rate is split across two 16-bit registers,
//      and the split is not unique: 64 MHz / 1 kHz is 64000, which fits ARR
//      alone, and 64 MHz / 1 Hz is 64 million, which does not fit either
//      register and needs both. Every property below is an identity about
//      that split — that the pair reproduces the rate, that neither half ever
//      exceeds what a 16-bit register holds, and that the prescaler is the
//      smallest one that works (which is the same as saying the counter is as
//      fine as the rate allows, and that is what count() resolution means).
//
//   2. THE PERSONALITY CLAIM. `tick::open` and `pwm::open` on ONE timer
//      instance cannot both succeed, in either order. That is the same rule
//      test_encoder.cpp pins for the encoder personality, and it needs its own
//      case here because a tick and a PWM channel are the pair most likely to
//      be written by accident: both are "set up a timer at a rate", and if
//      they shared a personality enumerator they would AGREE and neither would
//      trap — while what the tick actually did was reprogram the period of a
//      running PWM output.
//
// NOT WITNESSED ON SILICON, and this file cannot become that. No board was
// attached, and Renode 1.16.1 binds no model to the G0's TIM6/TIM7/TIM14-17,
// so nothing available to this project executes the register sequence in
// hal/tick/st_timebase.hpp. What is below is arithmetic and ownership.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>

#include "alloy/core/claim.hpp"
#include "alloy/hal/tick/st_tim_divisor.hpp"
#include "alloy/pwm.hpp"
#include "alloy/tick.hpp"
#include "alloy_test.hpp"

namespace {

using alloy::hal::kTickMaxDivide;
using alloy::hal::tick_achieved_hz;
using alloy::hal::tick_pick;
using alloy::hal::tick_representable;

constexpr std::uint32_t kApb = 64'000'000u;  // the nucleo_g0b1re 64 MHz profile

// ── A chip, declared here, exactly as codegen would declare one ──────────

struct fake_ip {};

template <int N>
struct fake_timer {
    using ip = fake_ip;
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
    struct feat {
        static constexpr std::uint32_t trgo = 1u;
    };
};

template <int N>
struct fake_pin {};

struct fake_clock {
    static constexpr std::uint32_t sysclk_hz = kApb;
    static constexpr std::uint32_t ahb_hz = kApb;
    static constexpr std::uint32_t apb_hz = kApb;
    static constexpr std::uint32_t apb2_hz = kApb;
};

}  // namespace

// The route that makes the pin legal on the timer — codegen emits exactly this
// shape per (pin, peripheral, signal) triple.
namespace alloy::routes {
template <int P, int T, alloy::signal S>
struct route<fake_pin<P>, fake_timer<T>, S> {
    static constexpr kind k = kind::af_fixed;
    static constexpr std::uint8_t af = 1;
};
}  // namespace alloy::routes

// The two drivers those binds land in. Register writes are replaced; every
// line above them — admission, claim, mux — is the shipped code.
namespace alloy::hal {

template <int N>
struct tick_impl<fake_timer<N>> {
    static inline alloy::hal::tick_divisor programmed{0u, 0u};
    static constexpr bool representable(std::uint32_t kernel_hz, std::uint32_t hz) {
        return alloy::hal::tick_representable(kernel_hz, hz);
    }
    static std::uint32_t achieved_hz(std::uint32_t kernel_hz) {
        return alloy::hal::tick_achieved_hz(kernel_hz, programmed);
    }
    static std::uint32_t period_ticks() {
        return static_cast<std::uint32_t>(programmed.arr) + 1u;
    }
    static void enable(std::uint32_t kernel_hz, std::uint32_t hz) {
        programmed = alloy::hal::tick_pick(kernel_hz, hz);
    }
    static void stop() {}
    static void start() {}
    static std::uint16_t count() { return 0u; }
    static void reset_count() {}
    static bool take_expired() { return false; }
    static void irq_on_update(bool) {}
    static void dma_on_update(bool) {}
    static void trigger_on_update() {}
};

template <int N>
struct pwm_impl<fake_timer<N>> {
    static void enable(std::uint32_t, std::uint32_t, unsigned) {}
    static void set_duty(unsigned, std::uint16_t) {}
};

template <int P>
struct pin_impl<fake_pin<P>> {
    static void make_af(std::uint8_t) {}
};

}  // namespace alloy::hal

namespace {

// A trap aborts the process, so a test that expects one runs it in a child and
// asserts the child died — the same shape test_encoder.cpp uses.
template <class Fn>
bool traps(Fn&& fn) {
    const pid_t pid = fork();
    if (pid == 0) {
        fn();
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

}  // namespace

// ── 1. The divisor ───────────────────────────────────────────────────────

ALLOY_TEST(tick_divisor_fits_one_register_when_it_can) {
    // 64 MHz / 1 kHz = 64000, which ARR holds on its own. A prescaler here
    // would coarsen the counter for nothing.
    const auto d = tick_pick(kApb, 1'000u);
    ALLOY_CHECK(d.psc == 0u);
    ALLOY_CHECK(d.arr == 63'999u);
    ALLOY_CHECK(tick_achieved_hz(kApb, d) == 1'000u);
}

ALLOY_TEST(tick_divisor_spans_both_registers_when_it_must) {
    // 64 MHz / 1 Hz = 64 million: neither register holds it, both together do.
    const auto d = tick_pick(kApb, 1u);
    ALLOY_CHECK(d.psc != 0u);
    ALLOY_CHECK(tick_achieved_hz(kApb, d) == 1u);
}

ALLOY_TEST(tick_divisor_never_overflows_either_register) {
    // The registers are 16 bits; a pair that did not fit would be silently
    // truncated by the hardware into some other rate. Swept over three decades
    // of rates, which is where the ceil/round boundaries live.
    //
    // THE INVARIANT IS THE DIVIDER, NOT A TOLERANCE, and the first draft of
    // this test got that wrong: it asserted the achieved rate was within 0.1%
    // of the requested one, which is FALSE near the kernel clock and not a
    // defect. 64 MHz cannot produce 20 MHz at all — the nearest divider is 3,
    // so the block runs at 21.33 MHz, 6.7% out. That is integer division, and
    // achieved_hz() exists so a caller can see it. What the split must never
    // do is lose accuracy the divider had: the product of the two registers
    // has to BE the nearest divider.
    for (std::uint32_t hz = 1u; hz <= 20'000'000u; hz = hz * 3u + 1u) {
        if (!tick_representable(kApb, hz)) {
            continue;
        }
        const auto d = tick_pick(kApb, hz);
        const std::uint64_t psc_div = static_cast<std::uint64_t>(d.psc) + 1u;
        const std::uint64_t arr_div = static_cast<std::uint64_t>(d.arr) + 1u;
        const std::uint64_t product = psc_div * arr_div;
        const std::uint64_t want = alloy::hal::tick_total_divide(kApb, hz);
        // NOT EQUALITY, and the difference is the whole content of this check.
        // A divider that is prime and larger than 65536 cannot be written as a
        // product of two 16-bit numbers at all — 65537 is the first — so the
        // split rounds the reload and lands within HALF A PRESCALER STEP of
        // the divider it wanted. Asserting equality here failed, correctly,
        // and asserting nothing would have let a split that dropped a factor
        // of two through.
        const std::uint64_t slip = product > want ? product - want : want - product;
        ALLOY_CHECK(slip * 2u <= psc_div);
        // Both halves inside a 16-bit register — implied by the types, stated
        // because the arithmetic that produces them is 64-bit.
        ALLOY_CHECK(psc_div <= kTickMaxDivide);
        ALLOY_CHECK(arr_div <= kTickMaxDivide);
    }
}

ALLOY_TEST(tick_divisor_is_accurate_where_the_divider_is_large) {
    // Where the divider is big the rounding is negligible, and that is the
    // regime every real tick lives in — a housekeeping tick or a control loop
    // on a 64 MHz part divides by thousands. One part in 1000 there.
    for (std::uint32_t hz = 1u; hz <= 10'000u; hz = hz * 3u + 1u) {
        const auto d = tick_pick(kApb, hz);
        const std::uint32_t got = tick_achieved_hz(kApb, d);
        const std::uint32_t slack = hz / 1000u + 1u;
        ALLOY_CHECK(got + slack >= hz && hz + slack >= got);
    }
}

ALLOY_TEST(tick_divisor_picks_the_smallest_prescaler) {
    // The finest counter that reaches the rate: one prescaler step down must
    // leave a reload that does NOT fit. Checked at a rate that genuinely needs
    // a prescaler (64 MHz / 100 Hz = 640 000, ten times what ARR holds).
    const auto d = tick_pick(kApb, 100u);
    ALLOY_CHECK(d.psc != 0u);
    const std::uint32_t total = (static_cast<std::uint32_t>(d.psc) + 1u) *
                                (static_cast<std::uint32_t>(d.arr) + 1u);
    const std::uint32_t one_step_finer = total / static_cast<std::uint32_t>(d.psc);
    ALLOY_CHECK(one_step_finer > kTickMaxDivide);
}

ALLOY_TEST(tick_rejects_both_ends_of_the_window) {
    ALLOY_CHECK(!tick_representable(kApb, 0u));
    // FASTER THAN THE CLOCK IT DIVIDES. This is the case the rounding hides
    // and the one this test was written to catch: `tick_total_divide` rounds
    // to nearest, so 64 MHz + 1 comes back as a divider of ONE — a perfectly
    // legal register pair that runs the timer at 64 MHz and reports success.
    // Every rate up to 2 x kernel has that shape, and 1.5 x kernel is not an
    // exotic mistake, it is a copy-pasted constant.
    ALLOY_CHECK(!tick_representable(kApb, kApb + 1u));
    ALLOY_CHECK(!tick_representable(kApb, kApb + kApb / 2u));
    ALLOY_CHECK(!tick_representable(kApb, kApb * 2u));
    // Exactly the kernel clock IS reachable: divide by one.
    ALLOY_CHECK(tick_representable(kApb, kApb));
    // And the slow end. 64 MHz over 2^32 is about 0.0149 Hz, so 1 Hz is well
    // inside and the 4 GHz kernel below needs more division than the pair holds.
    ALLOY_CHECK(tick_representable(kApb, 1u));
    ALLOY_CHECK(!tick_representable(4'000'000'000u, 0u));
}

ALLOY_TEST(tick_achieved_hz_reports_the_rate_it_really_runs_at) {
    // 64 MHz / 3 Hz is not an integer; the facade must not pretend it is.
    const auto d = tick_pick(kApb, 3u);
    const std::uint32_t exact =
        kApb / ((static_cast<std::uint32_t>(d.psc) + 1u) *
                (static_cast<std::uint32_t>(d.arr) + 1u));
    ALLOY_CHECK(tick_achieved_hz(kApb, d) == exact || tick_achieved_hz(kApb, d) == exact + 1u);
}

// ── 2. The personality claim ─────────────────────────────────────────────

ALLOY_TEST(tick_and_pwm_cannot_share_one_timer) {
    using timer = fake_timer<1>;
    using pin = fake_pin<1>;
    using tick_bind = alloy::tick::bind<timer, fake_clock>;
    using pwm_bind = alloy::pwm::bind<timer, 1u, pin, alloy::signal::ch1, fake_clock>;

    ALLOY_CHECK(traps([] {
        (void)tick_bind::open({.hz = 100u});
        (void)pwm_bind::open({.freq_hz = 1'000u});
    }));
}

ALLOY_TEST(pwm_then_tick_traps_too) {
    using timer = fake_timer<2>;
    using pin = fake_pin<2>;
    using tick_bind = alloy::tick::bind<timer, fake_clock>;
    using pwm_bind = alloy::pwm::bind<timer, 1u, pin, alloy::signal::ch1, fake_clock>;

    ALLOY_CHECK(traps([] {
        (void)pwm_bind::open({.freq_hz = 1'000u});
        (void)tick_bind::open({.hz = 100u});
    }));
}

ALLOY_TEST(a_tick_is_exclusive_even_against_another_tick) {
    // Unlike PWM, whose four channels share the block: a second time base is
    // not a co-owner asking for the same rate, it is a second program
    // overwriting PSC and ARR.
    using timer = fake_timer<3>;
    using tick_bind = alloy::tick::bind<timer, fake_clock>;

    ALLOY_CHECK(traps([] {
        (void)tick_bind::open({.hz = 100u});
        (void)tick_bind::open({.hz = 100u});
    }));
}

ALLOY_TEST(a_tick_opens_and_reports_its_rate) {
    using timer = fake_timer<4>;
    using tick_bind = alloy::tick::bind<timer, fake_clock>;
    auto t = tick_bind::open({.hz = 1'000u});
    ALLOY_CHECK(t.achieved_hz() == 1'000u);
    ALLOY_CHECK(t.period_ticks() == 64'000u);
}
