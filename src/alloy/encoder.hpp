// User-facing quadrature encoder: two routed input pins, a hardware counter,
// and a position.
//
//   using Knob = alloy::encoder::bind<dev::tim3_t,
//                                     alloy::encoder::a<dev::pa6_t>,
//                                     alloy::encoder::b<dev::pa7_t>>;
//   auto knob = Knob::open({.period = 1'440});
//   knob.count();      // 0 .. 1439, wraps in hardware
//   knob.delta();      // signed movement since the previous call
//
// THIS IS A SECOND PERSONALITY OF THE TIMER, not a mode of alloy::pwm. The
// two facades are separate on purpose and the separation is enforced, not
// documented: `pwm::bind::open()` claims the instance as
// `personality::pwm`, this one claims it as `personality::encoder`, and the
// second of the two to run traps with `personality_conflict`. A board that
// states the same conflict in board.json is refused earlier still, by the
// generator, which can see it before anything is built.
//
// Why a whole binder rather than a flag: nothing about the PWM binder
// survives the change. It takes ONE pin and a channel ordinal; this takes
// TWO pins and no ordinal, because the channels are not a choice — a
// quadrature counter is wired to channels 1 and 2 or it is not a quadrature
// counter. Its block-scoped value is a frequency; this one's is a modulo.
// A `mode` field on the PWM binder would have had to make `channel`,
// `freq_hz` and `set_duty()` conditionally meaningless, which is a longer
// way of saying they belong to a different type.

#pragma once

#include <cstdint>
#include <limits>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/encoder/encoder_impl.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"

namespace alloy::encoder {

// LAYER 1 — see hal::encoder_config for the admission test and for what is
// deliberately NOT a field.
using config = hal::encoder_config;
using direction = hal::encoder_direction;

// LAYER 2 — the vendor knobs, keyed by INSTANCE. Surfaced on the binder as
// `Role::opts`; naming the type at the call site is what buys the good
// diagnostic.
//
//     auto knob = board::encoder::open<board::encoder::opts{
//                     .edges = board::encoder::opts::count_on::ti1_edges}>(
//                     {.period = 1'440});
template <class Inst>
using opts = hal::encoder_opts<Inst>;

// The two quadrature inputs, as binder tags. Named A and B rather than ch1
// and ch2 because that is what the connector is labelled, and because the
// mapping to channels is not the caller's choice — see the static_asserts.
template <class Pin>
struct a {
    using pin = Pin;
};
template <class Pin>
struct b {
    using pin = Pin;
};

namespace detail {

// Layer 1's VALUE admission — see alloy/core/admit.hpp. Compile error when
// the period is a constant (which every literal call site is), named trap
// when it is not.
//
// THREE WAYS A MODULO IS IMPOSSIBLE, and only the third is about silicon:
//   0 and 1  the driver programs ARR = period - 1, so a period of 0 wraps
//            that subtraction to the widest value the register holds — a
//            counter asked to wrap at nothing would instead never wrap.
//   > max    more counts than this instance's ARR can hold. `max_period`
//            is derived from the curated width of ARR, so the bound is the
//            silicon's and not a constant written here.
//   > INT32  `delta()` returns a signed count; a modulo it cannot represent
//            would make the return value ambiguous rather than merely large.
inline void admit_period(std::uint32_t period, std::uint32_t max_period) {
    constexpr auto kSignedMax =
        static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
    const bool ok = period >= 2u && period <= max_period && period <= kSignedMax;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::encoder_period();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

// Signed movement across a modulo-`period` counter, taking the SHORTER of
// the two arcs. Portable, board-free and side-effect-free, which is what
// makes it the part of this facade a host test can pin exactly.
//
// THE AMBIGUITY IS REAL AND IS NOT PAPERED OVER: a counter that moved more
// than half a period between two reads is indistinguishable from one that
// moved the other way by the remainder. There is no information in the
// counter that resolves it, so the rule is stated instead of hidden — poll
// faster than half a revolution.
[[nodiscard]] constexpr std::int32_t wrapped_delta(std::uint32_t previous,
                                                   std::uint32_t current,
                                                   std::uint32_t period) {
    const std::uint32_t forward =
        current >= previous ? current - previous : period - previous + current;
    return forward * 2u > period
               ? static_cast<std::int32_t>(forward) - static_cast<std::int32_t>(period)
               : static_cast<std::int32_t>(forward);
}

}  // namespace detail

template <class Inst>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    //: Raw position, 0 .. period()-1. Wraps in hardware, never in software.
    [[nodiscard]] std::uint32_t count() const {
        return hal::encoder_impl<Inst>::count();
    }

    //: Which way the counter last moved, as the HARDWARE recorded it — the
    //: driver never writes this bit, the encoder does.
    [[nodiscard]] encoder::direction direction() const {
        return hal::encoder_impl<Inst>::direction();
    }

    //: Signed movement since the previous call to delta() (or since open()).
    //: Mutating, and the state lives HERE rather than in a static beside the
    //: driver: "how far since I last looked" is a property of the looker.
    [[nodiscard]] std::int32_t delta() {
        const std::uint32_t now = count();
        const std::int32_t moved = detail::wrapped_delta(last_, now, period_);
        last_ = now;
        return moved;
    }

    //: Back to zero — homing. Also resets what delta() measures from, so a
    //: reset can never be reported as a movement.
    void reset() {
        hal::encoder_impl<Inst>::reset();
        last_ = 0u;
    }

    [[nodiscard]] std::uint32_t period() const { return period_; }

private:
    template <class, class, class>
    friend struct bind;
    explicit handle(std::uint32_t period) : period_(period) {}

    std::uint32_t period_ = 0;
    std::uint32_t last_ = 0;
};

// NO CLOCK PARAMETER, AND THE ABSENCE IS THE POINT. Every other binder in
// this tree takes the board's clock profile because its peripheral divides a
// kernel clock to reach the rate the user asked for — baud, bus speed, PWM
// frequency. An encoder divides nothing: PSC is forced to zero and the
// counter is clocked by the shaft. The bus gate still has to be opened, but
// that is `Inst::gate` and comes from the instance, not from a profile.
//
// Taking a `Clock` here to look like the others would be a lie about what
// this peripheral depends on, and the kind of lie that survives for years
// because it never breaks anything. The one thing that would bring it back
// is the input filter, which samples at a rate derived from the kernel
// clock — and that field is not curated (registers/st/tim_gp16.yaml), so
// this binder cannot want it yet.
template <class Inst, class A, class B>
struct bind {
    // THE CHANNELS ARE NOT A PARAMETER. A quadrature counter reads the two
    // inputs the timer's slave-mode controller is wired to, which are
    // channels 1 and 2 on every IP that has this mode. Offering an ordinal
    // would offer combinations the silicon does not have.
    static_assert(routes::routable<typename A::pin, Inst, alloy::signal::ch1>,
                  "encoder input A has no route to channel 1 of this timer on "
                  "the selected chip (check the chip's route table in "
                  "alloy-devices; A must be the timer's CH1 pin)");
    static_assert(routes::routable<typename B::pin, Inst, alloy::signal::ch2>,
                  "encoder input B has no route to channel 2 of this timer on "
                  "the selected chip (check the chip's route table in "
                  "alloy-devices; B must be the timer's CH2 pin)");

    using opts = hal::encoder_opts<Inst>;

    template <opts Opts = {}>
    static handle<Inst> open(config c = {}) {
        detail::admit_period(c.period, hal::encoder_impl<Inst>::max_period);
        // EXCLUSIVE, not shared, and the contrast with pwm::bind is the
        // whole of the personality rule in two lines. PWM claims the block
        // as `shared` because four channels are four legitimate co-owners
        // that merely have to agree about one frequency. An encoder has no
        // co-owner: it consumes the counter itself, so there is nothing a
        // second claimant could be doing that is not a contradiction —
        // including a PWM channel on channels 3-4 of the same timer, whose
        // period would be whatever the shaft happened to be doing.
        alloy::claim::exclusive<Inst, alloy::claim::personality::encoder>();
        using route_a = routes::route<typename A::pin, Inst, alloy::signal::ch1>;
        using route_b = routes::route<typename B::pin, Inst, alloy::signal::ch2>;
        hal::pin_impl<typename A::pin>::make_af(routes::mux_value<route_a>());
        hal::pin_impl<typename B::pin>::make_af(routes::mux_value<route_b>());
        hal::encoder_impl<Inst>::template enable<Opts>(c);
        return handle<Inst>{c.period};
    }
};

}  // namespace alloy::encoder
