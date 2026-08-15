// The encoder facade — a timer's SECOND personality (src/alloy/encoder.hpp).
//
// WHAT THIS FILE PROVES, and it is the whole runtime half of the personality
// rule: two hand-written `bind<>`s naming ONE timer, one as a PWM generator
// and one as an encoder counter, cannot both open it. Both orders. Not the
// claim primitives in isolation — test_claim.cpp already pins those — but the
// REAL `pwm::bind::open()` and `encoder::bind::open()`, running their real
// admission, claim and mux sequence, with only the register pokes replaced.
//
// HOW IT CAN RUN AT ALL ON A HOST, and the seam is worth naming: alloy's
// binders are parameterised on an INSTANCE, a PIN, a ROUTE and an IMPL, all
// four of which are ordinary templates a test may specialize. So this file
// declares a chip: one fake timer, two fake pins, the two routes that make
// them legal, and the two `hal::` drivers those routes lead to. Everything
// above the register write is the shipped code.
//
// WHAT IT DOES NOT PROVE: that the register sequence in
// hal/encoder/st_tim_gp16.hpp counts quadrature edges. Nothing available to
// this project proves that — Renode's Timers.STM32_Timer rejects both of the
// writes the personality is made of ("Unhandled write to offset 0x8 ...
// Tags: SMS", "Channel 1: input capture mode is not supported"). Only
// hardware can, and it has not.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>

#include "alloy/encoder.hpp"
#include "alloy/pwm.hpp"
#include "alloy_test.hpp"
#include "alloy/core/claim.hpp"

namespace {

// ── A chip, declared here, exactly as codegen would declare one ──────────

struct fake_ip {};

// Two instances, so a test that claims one does not spend the other.
template <int N>
struct fake_timer {
    using ip = fake_ip;
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
};

template <int N>
struct fake_pin {};

struct fake_clock {
    static constexpr std::uint32_t sysclk_hz = 64'000'000u;
    static constexpr std::uint32_t ahb_hz = 64'000'000u;
    static constexpr std::uint32_t apb_hz = 64'000'000u;
    static constexpr std::uint32_t apb2_hz = 64'000'000u;
};

}  // namespace

// The routes that make the pins legal on the timers. Codegen emits exactly
// this shape per (pin, peripheral, signal) triple in the chip data.
namespace alloy::routes {
template <int P, int T, alloy::signal S>
struct route<fake_pin<P>, fake_timer<T>, S> {
    static constexpr kind k = kind::af_fixed;
    static constexpr std::uint8_t af = 1;
};
}  // namespace alloy::routes

namespace alloy::hal {

template <int P>
struct pin_impl<fake_pin<P>> {
    static void make_af(std::uint8_t) {}
};

// The two personalities' drivers, stubbed at the register boundary and
// nowhere above it. `enable()` here does what a driver does minus the poke:
// it records, so a test can also see that open() reached the hardware layer
// with the values the caller asked for.
inline std::uint32_t last_pwm_freq = 0;
inline std::uint32_t last_encoder_period = 0;
inline std::uint32_t fake_count = 0;

template <int T>
struct pwm_impl<fake_timer<T>> {
    static void enable(std::uint32_t, alloy::hal::pwm_config c, unsigned) {
        const std::uint32_t freq_hz = c.freq_hz;
        last_pwm_freq = freq_hz;
    }
    static void set_duty(unsigned, std::uint16_t) {}
};

template <int T>
struct encoder_impl<fake_timer<T>> {
    // A 16-bit ARR, like the tim_gp16 this facade was built against.
    static constexpr std::uint32_t max_period = 65'536u;
    template <encoder_opts<fake_timer<T>> = {}>
    static void enable(hal::encoder_config c) { last_encoder_period = c.period; }
    static std::uint32_t count() { return fake_count; }
    static hal::encoder_direction direction() {
        return hal::encoder_direction::up;
    }
    static void reset() { fake_count = 0; }
};

}  // namespace alloy::hal

namespace {

// A child that must NOT exit cleanly: the guard under test has to fire. Same
// idiom as test_claim.cpp, for the same reason — a personality conflict is a
// runtime trap because C++ cannot see across translation units.
template <class Fn>
bool refuses(Fn body) {
    const pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);  // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

template <int T>
using pwm_on = alloy::pwm::bind<fake_timer<T>, 1u, fake_pin<1>,
                                alloy::signal::ch1, fake_clock>;
template <int T>
using encoder_on = alloy::encoder::bind<fake_timer<T>,
                                        alloy::encoder::a<fake_pin<1>>,
                                        alloy::encoder::b<fake_pin<2>>>;

}  // namespace

// ── The personality conflict, from two hand-written binders ──────────────

ALLOY_TEST(encoder_refuses_a_timer_already_generating_pwm) {
    // THE CASE THE WHOLE PERSONALITY RULE EXISTS FOR. Both binders are legal
    // C++, both name routed pins, both compile — and the second one is a
    // program asking one counter to be a time base and a position at once.
    ALLOY_CHECK(refuses([] {
        (void)pwm_on<1>::open({.freq_hz = 1'000});
        (void)encoder_on<1>::open({.period = 1'440});
    }));
}

ALLOY_TEST(pwm_refuses_a_timer_already_counting_an_encoder) {
    // AND THE OTHER ORDER, which is not the same test: pwm::bind claims a
    // SUB-resource (its channel) before it claims the block, so the two
    // guards fire in a different sequence here. Both must still refuse.
    ALLOY_CHECK(refuses([] {
        (void)encoder_on<2>::open({.period = 1'440});
        (void)pwm_on<2>::open({.freq_hz = 1'000});
    }));
}

ALLOY_TEST(encoder_refuses_a_second_binder_on_one_timer) {
    // Two encoders on one timer is the ordinary double-open, and `exclusive`
    // is what makes it impossible — unlike PWM, an encoder has no legitimate
    // co-owner to be `shared` with.
    ALLOY_CHECK(refuses([] {
        (void)encoder_on<3>::open({.period = 1'440});
        (void)encoder_on<3>::open({.period = 1'440});
    }));
}

ALLOY_TEST(encoder_opens_a_timer_nobody_else_wants) {
    // The positive control. Without it every test above passes on a facade
    // that refuses everything.
    auto knob = encoder_on<4>::open({.period = 1'440});
    ALLOY_CHECK_EQ(knob.period(), 1'440u);
    ALLOY_CHECK_EQ(alloy::hal::last_encoder_period, 1'440u);
    ALLOY_CHECK((alloy::claim::held<fake_timer<4>,
                                    alloy::claim::personality::encoder>()));
}

// ── Layer 1's value admission ────────────────────────────────────────────

ALLOY_TEST(encoder_refuses_a_period_no_counter_can_wrap_at) {
    // ARR is programmed as period-1, so a period of zero is not "no modulo",
    // it is the widest one the register holds — the counter would never wrap
    // and every position would be wrong after the first revolution.
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t zero = 0;  // defeat the compile-time arm
        (void)encoder_on<5>::open({.period = zero});
    }));
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t one = 1;
        (void)encoder_on<6>::open({.period = one});
    }));
}

ALLOY_TEST(encoder_refuses_a_period_wider_than_the_counter) {
    // The bound is the DEGREE fact — max_period, derived from the curated
    // width of ARR — and not a constant written into the facade. 65'537 on a
    // 16-bit counter is a position that silently aliases.
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t too_wide = 65'537u;
        (void)encoder_on<7>::open({.period = too_wide});
    }));
}

ALLOY_TEST(encoder_admits_the_widest_period_the_counter_does_have) {
    // The boundary from the other side — an off-by-one in the admission
    // would make this refuse a period the silicon can hold.
    auto knob = encoder_on<8>::open({.period = 65'536u});
    ALLOY_CHECK_EQ(knob.period(), 65'536u);
}

// ── The wrap-aware delta, which is the portable part of the facade ───────

using alloy::encoder::detail::wrapped_delta;

// Constant-evaluated, so these are checked by the compiler as well as by the
// runner: the function is constexpr precisely so it can be.
static_assert(wrapped_delta(10u, 14u, 1'440u) == 4);
static_assert(wrapped_delta(14u, 10u, 1'440u) == -4);
static_assert(wrapped_delta(0u, 0u, 1'440u) == 0);
// THE WRAP, in both directions. A counter that steps 1439 -> 2 moved FORWARD
// by three, not backward by 1437, and the naive subtraction gets it wrong by
// a whole revolution.
static_assert(wrapped_delta(1'439u, 2u, 1'440u) == 3);
static_assert(wrapped_delta(2u, 1'439u, 1'440u) == -3);

ALLOY_TEST(encoder_delta_takes_the_shorter_arc) {
    ALLOY_CHECK_EQ(wrapped_delta(10u, 14u, 1'440u), 4);
    ALLOY_CHECK_EQ(wrapped_delta(1'439u, 2u, 1'440u), 3);
    ALLOY_CHECK_EQ(wrapped_delta(2u, 1'439u, 1'440u), -3);
}

ALLOY_TEST(encoder_delta_is_symmetric_about_the_half_turn) {
    // EXACTLY half a period is the ambiguous point and something has to be
    // chosen; the rule is "> half is backwards", so half itself reads
    // forward. Pinned because it is the boundary a reader will assume about.
    ALLOY_CHECK_EQ(wrapped_delta(0u, 720u, 1'440u), 720);
    ALLOY_CHECK_EQ(wrapped_delta(0u, 721u, 1'440u), -719);
    ALLOY_CHECK_EQ(wrapped_delta(0u, 719u, 1'440u), 719);
}

ALLOY_TEST(encoder_delta_holds_at_the_widest_admitted_period) {
    // 65'536 is where a signed-difference implementation overflows if it
    // narrows too early. Both arcs, at the extremes.
    ALLOY_CHECK_EQ(wrapped_delta(0u, 1u, 65'536u), 1);
    ALLOY_CHECK_EQ(wrapped_delta(0u, 65'535u, 65'536u), -1);
    ALLOY_CHECK_EQ(wrapped_delta(65'535u, 0u, 65'536u), 1);
    ALLOY_CHECK_EQ(wrapped_delta(0u, 32'768u, 65'536u), 32'768);
    ALLOY_CHECK_EQ(wrapped_delta(0u, 32'769u, 65'536u), -32'767);
}

ALLOY_TEST(encoder_delta_measures_from_the_previous_call) {
    // The handle's state, which is where "how far since I last looked"
    // belongs — reading it twice with no movement must report no movement,
    // and a reset must not be reported as a turn.
    auto knob = encoder_on<9>::open({.period = 1'440u});
    alloy::hal::fake_count = 100;
    ALLOY_CHECK_EQ(knob.delta(), 100);
    ALLOY_CHECK_EQ(knob.delta(), 0);
    alloy::hal::fake_count = 90;
    ALLOY_CHECK_EQ(knob.delta(), -10);
    alloy::hal::fake_count = 500;
    knob.reset();  // clears the counter AND the mark it measures from
    ALLOY_CHECK_EQ(knob.count(), 0u);
    ALLOY_CHECK_EQ(knob.delta(), 0);
}
