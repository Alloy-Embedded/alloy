// User-facing complementary bridge: N half-bridges driven from one advanced
// timer, with hardware dead time and a break input.
//
//   using inverter = alloy::bridge::bind<
//       alloy::dev::tim1_t, board::clock_profile,
//       alloy::bridge::brk<alloy::dev::pb12_t>,
//       alloy::bridge::phase<1, alloy::dev::pa8_t,  alloy::dev::pb13_t>,
//       alloy::bridge::phase<2, alloy::dev::pa9_t,  alloy::dev::pb14_t>,
//       alloy::bridge::phase<3, alloy::dev::pa10_t, alloy::dev::pb15_t>>;
//
//   auto inv = inverter::open_checked<alloy::bridge::config{
//       .freq_hz   = 20'000,
//       .dead_time = alloy::bridge::dead_time::ns(500),
//   }>();
//   inv.set_duty<1>(0x8000);        // 50 %
//   inv.enable_outputs();           // …and only NOW does a pin move
//
// ── THE THREE THINGS THIS FACADE EXISTS TO MAKE IMPOSSIBLE ──────────────
//
// ONE: A COMPLEMENTARY PAIR WITH NO DEAD TIME. `config::dead_time` is not a
// number, it is a three-state type (`hal::bridge_dead_time`) whose default
// state is "unstated" and whose only zero is spelled
// `dead_time::inserted_by_the_gate_driver()`. Under `open_checked<>` leaving
// it out is a COMPILE ERROR and `dead_time::ns(0)` is a DIFFERENT compile
// error, so "I forgot" and "I meant zero" arrive as two distinct
// static_assert messages.
//
// THAT SEPARATION IS A COMPILE-TIME PROPERTY AND ONLY THAT, and the sentence
// used to say it without the qualifier. The runtime trap that both entry
// points keep is ONE `trap_code::impossible_config` for all five admissions —
// measured, at -Os, as a single merged range check branching to a single
// `trap<4>` — because a trap code is a register value and not a paragraph. A
// program that reaches the trap knows it configured the bridge impossibly and
// does not know which way. Which is the third reason the checked call is the
// one to write.
//
// WHICH open() YOU CALL DECIDES WHICH NET YOU GET, and the difference was
// measured rather than assumed:
//
//   open_checked<Config>()  the config is a TEMPLATE PARAMETER, so the five
//                           checks are static_asserts. They fire under
//                           -fsyntax-only, at -O0 and at -Os alike, whether or
//                           not anything is inlined. This is the call to
//                           write; the five acceptance cases in
//                           scripts/check_compile_errors.py are on it.
//
//   open(config)            for a config that is genuinely computed at run
//                           time. Its guarantee is the RUNTIME trap, which is
//                           unconditional. It also carries the older
//                           `__builtin_constant_p` + [[gnu::error]] admissions
//                           (messages in alloy/core/admit.hpp), and those are
//                           a bonus, not a promise: on arm-none-eabi-g++ 14.2.1
//                           they fire at -O2/-O3/-Os for a single literal call
//                           site, and NOT at -O0, -Og or -O1, and not even at
//                           -Os when one function contains two calls to open()
//                           and the optimizer therefore propagates neither
//                           constant. That page of measurements is why
//                           open_checked() exists.
//
// Both paths keep the trap: a value computed at run time is not something a
// template can check, and belt and braces is the correct answer on a bridge.
//
// TWO: TWO CALL SITES DISAGREEING ABOUT A BLOCK-SCOPED VALUE. This is the
// defect docs/reference/peripheral-surface.md has had open since revision 2:
// dead time is BLOCK-scoped and `pwm::bind` is CHANNEL-scoped, so two PWM
// channels on one timer, each carrying `opts{.dead_time_ns = …}`, is hole (A)
// in a new dress and `claim::shared`'s witness does not catch it. The page
// listed two candidate repairs and declined to choose. This is the second one
// — a BLOCK handle that phases are opened FROM — taken without the breaking
// change to `alloy::pwm` the page feared, because a bridge is a different
// personality and therefore a different facade. There is exactly one
// `open()`, it states the frequency, the dead time and the off-state once,
// and `set_duty<N>()` cannot restate any of them because it does not take
// them.
//
// THREE: OUTPUTS THAT COME UP LIVE. `open()` programs everything with MOE
// clear and returns a handle whose pins are off. `enable_outputs()` is a
// separate call the application makes after it has decided what the duties
// are. The break input, if the board has one, is armed before that happens.
//
// ── WHAT IS NOT PROVEN ──────────────────────────────────────────────────
//
// No silicon has run this. The dead-time arithmetic and this facade's
// admission, claim and MOE ordering are pinned by tests/test_bridge.cpp.
//
// The register WRITES are pinned by less than that, and exactly this much:
// alloy's generated platform does not instantiate TIM1 for Renode, so nothing
// MODELS a dead time, a break or an output pin, and there is no emulation leg
// — but Renode's sysbus logs the accesses, so their ORDER and VALUES are
// witnessed (see the driver header for the trace). What the silicon then DOES
// with them is witnessed by nothing.
//
// That sentence used to be here with no test file behind it. When one was
// finally written it found a live defect in the very bound the sentence was
// bragging about — see `dtg_max_dead_time_ns` in
// alloy/hal/bridge/st_tim_adv_dtg.hpp.

#pragma once

#include <cstdint>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/bridge/bridge_impl.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"

namespace alloy::bridge {

// LAYER 1 — see hal::bridge_config for the admission test and for what is
// deliberately NOT a field.
using config = hal::bridge_config;
using dead_time = hal::bridge_dead_time;
using alignment = hal::bridge_alignment;
using off_state = hal::bridge_off_state;
using break_polarity = hal::bridge_break_polarity;
//: Added with the field, one commit late: `config::trigger` shipped and its
//: TYPE did not, so the only way to name a value was `alloy::hal::
//: bridge_trigger::on_update` — reaching into `hal` from a call site, which is
//: exactly what every other alias on this list exists to prevent. Writing the
//: user guide is what surfaced it.
using trigger = hal::bridge_trigger;

// LAYER 2 — the vendor knobs, keyed by INSTANCE. Surfaced on the binder as
// `Role::opts`; naming the type at the call site is what buys the good
// diagnostic.
template <class Inst>
using opts = hal::bridge_opts<Inst>;

// ── Binder tags ─────────────────────────────────────────────────────────

// One half-bridge: the high-side pin, the low-side pin, and which of the
// timer's channel pairs drives them.
//
// BOTH PINS ARE MANDATORY, and that is the type system carrying the safety
// rule. A `phase` with only a high-side pin would be a single-ended PWM
// output, which is a thing this timer can do and a thing alloy::pwm is for.
// Here it would be a half-bridge whose low side is driven by something this
// program does not know about, with a dead time that therefore protects
// nothing. If you want one pin, you do not want a bridge.
template <unsigned Index, class HighPin, class LowPin>
struct phase {
    static constexpr unsigned index = Index;
    using high = HighPin;
    using low = LowPin;
};

// The fault input. `Active` is the level that MEANS fault, and it lives on
// the tag rather than in `config` because it is a property of how the
// comparator is wired to this pin — a board fact, fixed at design time, and
// meaningless on a board with no fault line at all.
//
// active_low is the default because a fault line that idles high and is
// pulled down by the comparator ALSO trips when the connector falls off. An
// active-high line reads "healthy" when it is disconnected.
template <class Pin, break_polarity Active = break_polarity::active_low>
struct brk {
    using pin = Pin;
    static constexpr bool present = true;
    static constexpr break_polarity active = Active;
};

// A bridge with NO hardware fault input. Spelled out, in the bind, where a
// reviewer reads it — because a three-phase inverter whose only protection is
// software is a decision somebody made, and it should look like one.
struct unprotected {
    static constexpr bool present = false;
    static constexpr break_polarity active = break_polarity::active_low;
};

namespace detail {

// A phase ordinal maps to two route signals. The mapping is the silicon's,
// not a choice: channel 1's complementary output is CH1N or the chip has
// none.
[[nodiscard]] constexpr alloy::signal high_signal(unsigned index) {
    switch (index) {
        case 1: return alloy::signal::ch1;
        case 2: return alloy::signal::ch2;
        case 3: return alloy::signal::ch3;
        default: return alloy::signal::ch4;
    }
}
[[nodiscard]] constexpr alloy::signal low_signal(unsigned index) {
    switch (index) {
        case 1: return alloy::signal::ch1n;
        case 2: return alloy::signal::ch2n;
        default: return alloy::signal::ch3n;
    }
}

// One phase's routes, checked in a type of its own so the failing
// static_assert names the phase that failed rather than reporting one
// combined "some phase is unroutable".
template <class Inst, class P>
struct check_phase {
    static_assert(P::index >= 1u,
                  "a bridge phase is numbered from 1 — phase<0, …> names no "
                  "channel of any timer");
    static_assert(
        routes::routable<typename P::high, Inst, high_signal(P::index)>,
        "this bridge phase's HIGH-side pin has no route to that channel of "
        "this timer on the selected chip (check the chip's route table in "
        "alloy-devices; phase<N, high, low> needs `high` on CHN)");
    static_assert(
        routes::routable<typename P::low, Inst, low_signal(P::index)>,
        "this bridge phase's LOW-side pin has no route to that channel's "
        "COMPLEMENTARY output on this timer (check the chip's route table in "
        "alloy-devices; phase<N, high, low> needs `low` on CHNN — an ordinary "
        "PWM pin is not a complementary output and driving one as if it were "
        "gives a half-bridge with no dead time)");
    static constexpr bool ok = true;
};

// The phases must be 1..N with nothing skipped: the driver enables channel
// pairs 1..count, so `phase<1, …>, phase<3, …>` would enable CH2's pair —
// whose pins nobody muxed and whose duty nobody sets — alongside them.
template <class... Ps>
[[nodiscard]] constexpr bool numbered_from_one() {
    constexpr unsigned n = sizeof...(Ps);
    const unsigned idx[] = {Ps::index...};
    for (unsigned i = 0; i < n; ++i) {
        if (idx[i] != i + 1u) { return false; }
    }
    return true;
}

// Layer 1's VALUE admission for the open(config) path — see
// alloy/core/admit.hpp.
//
// THE UNCONDITIONAL HALF IS THE TRAP, and the [[gnu::error]] above it is a
// bonus that arrives only when the optimizer has already folded the value.
// The top of this file has the measurements; the short version, on
// arm-none-eabi-g++ 14.2.1 with examples/bridge's own flags, is that a single
// literal call site fires at -O1/-O2/-O3/-Os and does NOT fire at -O0 or -Og,
// and that two open() calls in one function defeat it at -Os as well, because
// neither constant is propagated and __builtin_constant_p is 0 at both sites.
//
// This comment used to say "compile error when the value is a constant (which
// every literal call site is)". A literal call site is not a constant here
// until an optimizer makes it one, which is the entire reason
// open_checked<Config>() exists: its five static_asserts are on a template
// parameter and fire under -fsyntax-only at every -O level. Write that one.
inline void admit_freq(std::uint32_t freq_hz, std::uint32_t kernel) {
    const bool ok = freq_hz != 0u && kernel != 0u && freq_hz <= kernel;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::bridge_freq();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

// FOUR CHECKS, FOUR MESSAGES, and the separation is the point: "you said
// nothing", "you said zero", "this timer cannot make one that long" and
// "it does not fit in a period" are four different mistakes with four
// different fixes, and a single "bad dead time" diagnostic would send a
// reader looking in the wrong place with an inverter on the bench.
inline void admit_dead_time(bridge::dead_time dt, std::uint32_t max_ns,
                            std::uint32_t period_ns) {
    const bool unstated = dt.stated() == dead_time::kind::unstated;
    if (__builtin_constant_p(unstated) && unstated) {
        alloy::core::admit::bridge_dead_time_unstated();
    }
    if (unstated) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }

    const bool zero =
        dt.stated() == dead_time::kind::nanoseconds && dt.requested_ns() == 0u;
    if (__builtin_constant_p(zero) && zero) {
        alloy::core::admit::bridge_dead_time_zero();
    }
    if (zero) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }

    const bool too_long = dt.requested_ns() > max_ns;
    if (__builtin_constant_p(too_long) && too_long) {
        alloy::core::admit::bridge_dead_time_too_long();
    }
    if (too_long) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }

    // Both edges of every pair carry the dead time, so two of them have to
    // fit inside one switching period with room left for the pulse.
    const bool swallows_period = dt.requested_ns() * 2u >= period_ns;
    if (__builtin_constant_p(swallows_period) && swallows_period) {
        alloy::core::admit::bridge_dead_time_vs_period();
    }
    if (swallows_period) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

[[nodiscard]] constexpr std::uint32_t period_ns_of(std::uint32_t freq_hz) {
    return freq_hz == 0u ? 0u : static_cast<std::uint32_t>(1'000'000'000ull / freq_hz);
}

}  // namespace detail

// THE BLOCK HANDLE. Phases are reached THROUGH it and cannot be opened
// separately, which is what makes the block-scoped values untypeable at a
// second call site.
template <class Inst, unsigned Phases>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    //: How many half-bridges this handle drives.
    static constexpr unsigned phases = Phases;

    //: Duty of one phase's HIGH side: 0 = the low side is on for the whole
    //: period, 0xFFFF = the high side is. The complementary output is the
    //: inverse, minus the dead time at both edges, in hardware.
    //:
    //: The phase is a TEMPLATE argument, so asking for a phase this bridge
    //: does not have is a compile error rather than a silently ignored write
    //: to a channel nobody muxed.
    template <unsigned N>
    void set_duty(std::uint16_t duty) const {
        static_assert(N >= 1u && N <= Phases,
                      "this bridge does not have that phase — the phases are "
                      "numbered 1..N for the N phase<> tags the bind declares");
        hal::bridge_impl<Inst>::set_duty(N, duty);
    }

    //: Let the outputs reach the pins. Everything is programmed before this;
    //: nothing moves until it. Calling it again after a fault is how a bridge
    //: is re-armed, and it is deliberately a separate call from
    //: acknowledge_fault().
    void enable_outputs() const { hal::bridge_impl<Inst>::outputs_on(); }

    //: Outputs off, immediately, in hardware. What the pins then do is what
    //: `config::when_off` selected.
    void disable_outputs() const { hal::bridge_impl<Inst>::outputs_off(); }

    [[nodiscard]] bool outputs_enabled() const {
        return hal::bridge_impl<Inst>::outputs_enabled();
    }

    //: Has the break fired since this was last acknowledged? The flag is set
    //: by the HARDWARE — this is reading the fault, not reading back a
    //: setting — and the outputs are already off by the time a program can
    //: observe it.
    [[nodiscard]] bool faulted() const { return hal::bridge_impl<Inst>::faulted(); }

    //: Clear the fault flag. Does NOT re-enable the outputs, and could not:
    //: while the fault line is still asserted the hardware holds the outputs
    //: off and the flag comes straight back. Re-arming is
    //: acknowledge_fault() then enable_outputs(), in that order, when the
    //: application has decided the fault is over.
    void acknowledge_fault() const { hal::bridge_impl<Inst>::acknowledge_fault(); }

    //: Raise a break in software — the same hardware path as the pin, so the
    //: outputs go off without depending on this code running again.
    void emergency_stop() const { hal::bridge_impl<Inst>::force_break(); }

    //: Counter ticks in one period. The resolution `set_duty` actually has.
    //: Write every phase's duty and say whether they will land TOGETHER.
    //:
    //: The three compare registers are preloaded, so each write takes effect
    //: at the next update event — which means three writes that straddle one
    //: produce a TORN FRAME: two phases from the new set, one from the old.
    //: On a motor that is a vector nobody asked for, once, and it is invisible
    //: afterwards because the next frame is correct.
    //:
    //: This does NOT make the write atomic. Nothing on this timer can: there
    //: is no commit register for compare VALUES (CCPC preloads the enable and
    //: mode bits, not CCRx). What it does is WITNESS the straddle — clear the
    //: update flag, write, read it back — so a caller who cares can retry, and
    //: one who does not is no worse off than before. Returning true is not a
    //: promise the hardware made; it is the flag saying no reload happened in
    //: between.
    //:
    //: Cost: two register accesses around the writes. A loop that already runs
    //: from the update interrupt is inside the window by construction and can
    //: keep using set_duty<N>() and ignore this entirely.
    template <class... Duties>
        requires (sizeof...(Duties) == Phases)
    [[nodiscard]] bool set_duties_coherent(Duties... duties) const
        requires requires { hal::bridge_impl<Inst>::update_elapsed(); }
    {
        hal::bridge_impl<Inst>::clear_update();
        unsigned n = 0u;
        (hal::bridge_impl<Inst>::set_duty(++n, static_cast<std::uint16_t>(duties)), ...);
        return !hal::bridge_impl<Inst>::update_elapsed();
    }

    //: Move the trigger point inside the switching period. Only exists when
    //: this timer HAS a trigger output — `feat::trgo` is a generated number
    //: from the IP's curated data, so a block without one is a compile error
    //: naming the instance rather than a write that does nothing.
    //:
    //: It does NOT check that `config::trigger` was `on_compare`: that is a
    //: runtime value and this is a compile-time gate. Calling it on a bridge
    //: configured `none` moves CCR4 and publishes nothing, which is inert.
    void set_sample_point(std::uint16_t point) const
        requires (Inst::feat::trgo != 0u)
    {
        hal::bridge_impl<Inst>::set_sample_point(point);
    }

    [[nodiscard]] std::uint32_t period_ticks() const {
        return hal::bridge_impl<Inst>::period_ticks;
    }

    //: The dead time the hardware is ACTUALLY inserting, in nanoseconds —
    //: the requested figure rounded UP to what the generator's encoding can
    //: represent. Reported rather than assumed because above 127 ticks the
    //: encoding's steps are 2, 8 and 16 ticks wide and an exact request
    //: usually cannot be met.
    [[nodiscard]] std::uint32_t dead_time_ns() const { return dead_time_ns_; }

private:
    template <class, class, class, class...>
    friend struct bind;
    explicit handle(std::uint32_t dead_ns) : dead_time_ns_(dead_ns) {}
    std::uint32_t dead_time_ns_ = 0;
};

// `Break` is `brk<pin[, polarity]>` or `unprotected`; `Phases...` are one to
// N `phase<index, high, low>` tags, numbered from 1.
template <class Inst, class Clock, class Break, class... Phases>
struct bind {
    static_assert(sizeof...(Phases) >= 1u,
                  "a bridge with no phases drives nothing — declare at least "
                  "one phase<index, high_pin, low_pin>");
    static_assert(detail::numbered_from_one<Phases...>(),
                  "bridge phases must be numbered 1..N in order, with none "
                  "skipped: the driver enables channel pairs 1..N, so a gap "
                  "would enable a pair whose pins nobody routed");
    // DEGREE, and the number is generated. `complementary_channels` is a
    // `feat` of the IP in the chip database; 0 means this timer has no
    // complementary outputs at all, and then this static_assert is the whole
    // diagnostic a user gets for pointing a bridge at a general-purpose
    // timer.
    static_assert(sizeof...(Phases) <= hal::bridge_impl<Inst>::phases,
                  "this timer has fewer complementary channel pairs than the "
                  "bind declares phases (the count comes from the IP's `feat` "
                  "in alloy-devices, not from this header) — a general-purpose "
                  "timer has none at all and cannot drive a bridge");
    static_assert((detail::check_phase<Inst, Phases>::ok && ...));

    using opts = hal::bridge_opts<Inst>;
    // Nameable at the call site, which is what `open_checked<>` needs: a
    // non-type template argument of class type has to be spelled with its
    // type, exactly like `Role::opts{…}` on every other facade.
    using config = hal::bridge_config;

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    // THE CHECKED CALL, AND THE ONE TO WRITE. The block-scoped values ride in
    // a template parameter, so every one of the five admissions below is a
    // static_assert on a constant the compiler already has. That is the whole
    // difference from open(): a static_assert does not depend on the inliner,
    // on the optimization level, or on how many call sites share a function.
    //
    //     auto inv = board::bridge::open_checked<board::bridge_defaults>();
    //     auto inv = board::bridge::open_checked<board::bridge::config{
    //         .freq_hz = 20'000,
    //         .dead_time = alloy::bridge::dead_time::ns(500)}>();
    //
    // FIVE CHECKS, FIVE MESSAGES, and the separation is the point: "you said
    // nothing", "you said zero", "this timer cannot make one that long", "it
    // does not fit in a period" and "that switching frequency is impossible"
    // are five different mistakes with five different fixes. A single "bad
    // dead time" would send a reader looking in the wrong place with an
    // inverter on the bench. Each condition is written so that exactly ONE of
    // them fires for each mistake — a refused frequency does not also drag the
    // period check down with it.
    //
    // The messages are twins of the [[gnu::error]] strings in
    // alloy/core/admit.hpp. They are spelled out here rather than shared
    // because static_assert takes a string LITERAL until C++26; if you edit
    // one, edit the other.
    template <config C, opts Opts = {}>
    static handle<Inst, sizeof...(Phases)> open_checked() {
        constexpr std::uint32_t kernel = kernel_hz();
        constexpr std::uint32_t period_ns = detail::period_ns_of(C.freq_hz);
        constexpr std::uint32_t longest_ns =
            hal::bridge_impl<Inst>::max_dead_time_ns(kernel);
        constexpr std::uint32_t asked_ns = C.dead_time.requested_ns();

        static_assert(C.freq_hz != 0u && kernel != 0u && C.freq_hz <= kernel,
                      "alloy::bridge::open_checked: this switching frequency is "
                      "impossible on this timer — it is zero, or above the "
                      "timer's own kernel clock");

        static_assert(
            C.dead_time.stated() != dead_time::kind::unstated,
            "alloy::bridge::open_checked: this configuration states NO DEAD "
            "TIME. A complementary pair with no dead time turns both "
            "transistors of a half-bridge on at once during every switching "
            "transition, which is a short across the DC link through the "
            "bridge — the failure is immediate and it is not recoverable. Say "
            "what you mean: `.dead_time = alloy::bridge::dead_time::ns(500)` "
            "for the gate driver's turn-off time plus margin, or "
            "`alloy::bridge::dead_time::inserted_by_the_gate_driver()` if the "
            "hardware between this pin and the gate already does it");

        static_assert(
            !(C.dead_time.stated() == dead_time::kind::nanoseconds &&
              asked_ns == 0u),
            "alloy::bridge::open_checked: `dead_time::ns(0)` is a "
            "shoot-through spelled as a number. Zero is not a dead time; if "
            "the gate driver inserts it, say so with "
            "`alloy::bridge::dead_time::inserted_by_the_gate_driver()`, which "
            "is greppable and reviewable and does not read as an oversight");

        static_assert(
            asked_ns <= longest_ns,
            "alloy::bridge::open_checked: this dead time cannot be programmed "
            "on this timer. The dead-time generator counts in ticks of the "
            "timer's own clock and its encoding has a longest value; asking "
            "for more than that would program the widest code and silently "
            "give you LESS dead time than you asked for. The bound comes from "
            "the curated DTG field width and the instance's kernel clock, not "
            "from a constant written here");

        // `period_ns == 0` only when the frequency was already refused above;
        // letting it through here keeps one mistake to one diagnostic.
        static_assert(
            period_ns == 0u || asked_ns * 2u < period_ns,
            "alloy::bridge::open_checked: the dead time does not fit inside "
            "the switching period. Both edges of every pair carry it, so 2 x "
            "dead_time must be less than one period — at this frequency it is "
            "not, and the bridge would spend the whole period with both "
            "transistors off (or, on a timer that wraps the compare, with "
            "neither of them where the duty says)");

        // …and then the ordinary path, which still runs the runtime trap. A
        // second, independent mechanism on the same five facts is not
        // redundancy here, it is the belt beside the braces.
        return open<Opts>(C);
    }

    template <opts Opts = {}>
    static handle<Inst, sizeof...(Phases)> open(config c = {}) {
        detail::admit_freq(c.freq_hz, kernel_hz());
        detail::admit_dead_time(
            c.dead_time,
            hal::bridge_impl<Inst>::max_dead_time_ns(kernel_hz()),
            detail::period_ns_of(c.freq_hz));

        // EXCLUSIVE, and the contrast with `pwm::bind` is the personality
        // rule in one line. PWM claims the block as `shared` because four
        // channels are four legitimate co-owners that merely have to agree
        // about a frequency. A bridge has no co-owner: dead time, the break
        // input, the off-state selection and MOE are one register for the
        // whole block, and a second claimant — a PWM channel on CH4 of the
        // same TIM1, say — would be a pin whose safety envelope is the
        // bridge's and whose configuration is not.
        alloy::claim::exclusive<Inst, alloy::claim::personality::bridge>();

        // ── THE ORDER OF THE NEXT THREE STEPS IS A SAFETY ARGUMENT ──────
        //
        // BREAK PIN FIRST, PHASE PINS LAST, and the driver's whole programming
        // sequence in between. It used to be all seven pins first.
        //
        // What a pin does between "muxed to the timer" and "BDTR written":
        // TIM1 comes out of reset with BDTR = 0, so OSSI = 0 and MOE = 0, and
        // OSSI = 0 means "outputs released to the GPIO" (alloy-devices
        // registers/st/tim_adv.yaml, BDTR; RM0444 section 21). The GPIO is in
        // ALTERNATE FUNCTION mode by then, so its own output driver is not
        // driving either — the pad is held by nothing. Muxing first therefore
        // leaves six gate-driver inputs FLOATING for the length of enable():
        // an RCC gate write and about twenty register writes.
        //
        // Two things that can do, and the first is the one that made this
        // change. An application that was holding the gate inputs at a hard
        // level as ordinary GPIO outputs — the usual "keep the gates down
        // until the timer is ready" bring-up — has them RELEASED by make_af(),
        // turning a driven low into a float at the one moment when no dead
        // time is programmed yet. A floating gate-driver input with no fitted
        // pull-down is undefined, and two of them on one half-bridge can both
        // read high; that is a shoot-through with DTG still at zero. Where
        // nothing had driven the pins, the old order does not CREATE the float
        // (the G0's reset state is analog, which is equally undriven) — it
        // just declines the first chance to end it.
        //
        // Muxing after enable() means the pin's first instant as an alternate
        // function is already governed: MOE = 0, OSSI = 1, CCxE/CCxNE = 1 and
        // CR2's OISx/OISxN forced to 0 by the driver, so the timer drives it
        // LOW — the same state `when_off = drive_idle_level` promises after a
        // break.
        //
        // THIS BUYS NOTHING WHEN `when_off` IS `high_impedance`, and it cannot:
        // that setting is OSSI = 0 by request, so the pin is undriven under AF
        // in either order. It is a statement that the gate driver's own pull
        // resistors define the safe state, and this facade takes it at its
        // word.
        //
        // The BREAK pin keeps its old place, ahead of the driver, because it
        // is an INPUT and the argument runs the other way: with the pin
        // already muxed when BDTR sets BKE, a fault line that is asserted at
        // that instant is seen by the hardware immediately. The outputs are
        // dark either way — MOE is 0 throughout — so this costs nothing and
        // removes a window in which the break is armed against a pin the
        // peripheral is not connected to yet.
        //
        // Read from the register descriptions in alloy-devices and RM0444's
        // off-state semantics. No board is on hand; nothing below has been
        // seen on silicon. What IS pinned is the ORDER — see
        // `the_phase_pins_are_muxed_after_the_off_state_is_in_force` in
        // tests/test_bridge.cpp.
        if constexpr (Break::present) {
            static_assert(
                routes::routable<typename Break::pin, Inst, alloy::signal::bk>,
                "this pin has no route to this timer's BREAK input on the "
                "selected chip (check the chip's route table in "
                "alloy-devices). A break input is not an ordinary GPIO "
                "interrupt: it switches the outputs off in hardware, with no "
                "software in the path, which is the entire reason to use it");
            using brk_route =
                routes::route<typename Break::pin, Inst, alloy::signal::bk>;
            hal::pin_impl<typename Break::pin>::make_af(
                routes::mux_value<brk_route>());
        }

        hal::bridge_impl<Inst>::template enable<Opts>(
            kernel_hz(), c, sizeof...(Phases), Break::present, Break::active);

        // …and only now do the six phase pins become the timer's. BDTR has
        // been written, so whatever they are handed to is already forcing the
        // off state.
        (mux_phase<Phases>(), ...);

        return handle<Inst, sizeof...(Phases)>{
            hal::bridge_impl<Inst>::achieved_dead_time_ns(
                c.dead_time.requested_ns(), kernel_hz())};
    }

private:
    template <class P>
    static void mux_phase() {
        using high_route =
            routes::route<typename P::high, Inst, detail::high_signal(P::index)>;
        using low_route =
            routes::route<typename P::low, Inst, detail::low_signal(P::index)>;
        hal::pin_impl<typename P::high>::make_af(routes::mux_value<high_route>());
        hal::pin_impl<typename P::low>::make_af(routes::mux_value<low_route>());
    }
};

}  // namespace alloy::bridge
