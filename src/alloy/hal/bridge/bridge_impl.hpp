// bridge_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per advanced-timer IP version, constrained on
// the instance's IP tag type, exactly like pwm_impl<Inst>.
//
// WHAT A BRIDGE IS, AND WHY IT IS NOT A MODE OF alloy::pwm. A PWM channel is
// one output pin whose duty you set. A bridge PHASE is a *pair* of pins that
// must never be on at the same time, plus a hardware generator that guarantees
// they are not, plus a fault input that switches both of them off without
// asking software. Nothing in that sentence is a knob on a PWM channel:
//
//   * The unit of ownership changes. `pwm::bind` is per channel and four of
//     them share a timer; a bridge owns the whole block, because dead time,
//     the break input, the off-state selection and MOE are one register for
//     all of them. That is the block-scope/channel-scope mismatch
//     docs/reference/peripheral-surface.md has had open since revision 2 —
//     two `pwm::bind`s on one timer each carrying `opts{.dead_time_ns = …}`
//     is hole (A) in a new dress. This facade closes it by construction: a
//     bridge is opened ONCE, from one call, and hands out phases. There is no
//     second call site for a second value to be stated at.
//   * The operations change. A bridge has `enable_outputs()`, `faulted()` and
//     `acknowledge_fault()`; a PWM channel has none of them and never will.
//   * The failure changes. A wrong PWM duty is a dim LED. A wrong dead time
//     is a short circuit across the DC link through two transistors, and it
//     happens on the first switching edge.
//
// So it is a PERSONALITY (claim::personality::bridge), the third one this
// timer block has, and the exclusion from `pwm` and `encoder` is enforced by
// alloy::claim rather than documented.

#pragma once

#include <cstdint>

namespace alloy::hal {

// ── Layer 1 vocabulary ──────────────────────────────────────────────────

//: Where the switching edges of the three phases sit relative to each other.
//: Center-aligned counts up and back down, so each phase's pulse is centred
//: in the period and the three half-bridges do not all commutate on the same
//: instant. It is the default because it is what a motor bridge wants; the
//: three ST "center-aligned mode" variants differ only in when the compare
//: interrupt fires, not in the waveform, so they are Layer 2.
enum class bridge_alignment : std::uint8_t { center, edge };

//: What the pins do while the outputs are OFF — which is the state a break
//: leaves them in, and therefore the most safety-relevant value in the whole
//: config.
//:
//: `drive_idle_level` keeps the timer driving each pin to its idle level (all
//: six low, see the driver: it refuses to program any other idle pattern).
//: A gate driver with an active-high input then sees a hard low and holds the
//: transistor off, through a fault, with no dependence on a pull-down that
//: may or may not be fitted.
//:
//: `high_impedance` releases the pins to the GPIO. That is the right answer
//: only when the gate driver's own pull resistors define the safe state —
//: and the wrong answer when they do not, because a floating gate on a
//: half-bridge is exactly how a fault becomes a shoot-through. It is offered
//: because some designs genuinely need it; it is not the default.
enum class bridge_off_state : std::uint8_t { drive_idle_level, high_impedance };

//: Which level on the break pin means "fault". A board fact — how the
//: desaturation or over-current comparator is wired — so it travels with the
//: PIN, as a parameter of the `brk<>` binder tag, and not as a config field
//: that would exist on boards with no break input at all.
//:
//: `active_low` is the default because a fault line that idles high and is
//: pulled down by the comparator ALSO trips when the cable falls off. An
//: active-high fault line reads as "no fault" when it is disconnected.
enum class bridge_break_polarity : std::uint8_t { active_low, active_high };

// THE TYPE THAT MAKES A MISSING DEAD TIME A COMPILE ERROR.
//
// The obvious spelling is `std::uint32_t dead_time_ns = 0;`, and it is the
// dangerous one: the safest-looking call in the world —
// `open({.freq_hz = 20'000})` — would then quietly mean "no dead time", which
// is the one value that destroys the hardware. A default of 500 ns is no
// better; it invents a number for a quantity that belongs to the gate driver
// and the transistor, and it would be silently wrong on the next board.
//
// So the field's type has THREE states, not one number, and only two of them
// are admitted:
//
//   unstated       what `config{}` gives you. A hard compile error at open(),
//                  naming this member. There is no way to reach it by
//                  accident because there is no way to write it on purpose.
//   nanoseconds    a real dead time. `ns(0)` is refused separately, with its
//                  own message, so "I wrote zero" and "I wrote nothing" are
//                  never the same diagnostic.
//   external       zero at the timer, on purpose, because the gate driver
//                  inserts it. Spelled `inserted_by_the_gate_driver()` — long
//                  and greppable, so a reviewer sees the claim being made and
//                  a hardware engineer can be asked whether it is true.
//
// This is the same shape as `uart::frame`'s `baud` member: the type's name is
// the message.
class bridge_dead_time {
public:
    enum class kind : std::uint8_t { unstated, nanoseconds, external };

    constexpr bridge_dead_time() = default;

    //: The dead time the gate driver and the transistors need: turn-off delay
    //: plus fall time plus the propagation mismatch between the two channels,
    //: plus margin. It is a hardware measurement, not a preference.
    [[nodiscard]] static constexpr bridge_dead_time ns(std::uint32_t nanoseconds) {
        return bridge_dead_time{kind::nanoseconds, nanoseconds};
    }

    //: The timer inserts none because something between this pin and the gate
    //: already does. Say it out loud or do not say it.
    [[nodiscard]] static constexpr bridge_dead_time inserted_by_the_gate_driver() {
        return bridge_dead_time{kind::external, 0u};
    }

    [[nodiscard]] constexpr bridge_dead_time::kind stated() const { return kind_; }
    [[nodiscard]] constexpr std::uint32_t requested_ns() const { return ns_; }

    friend constexpr bool operator==(bridge_dead_time, bridge_dead_time) = default;

private:
    constexpr bridge_dead_time(bridge_dead_time::kind k, std::uint32_t n)
        : kind_(k), ns_(n) {}
    bridge_dead_time::kind kind_ = kind::unstated;
    std::uint32_t ns_ = 0;
};

// The portable bridge shape.
//
// ADMISSION TEST, with the same honest caveat the encoder's config carries:
// alloy ships exactly ONE bridge_impl today, so "every driver programs it"
// cannot yet distinguish a portable knob from an ST one. The test used
// instead is structural — is this a property of *driving a complementary
// bridge*, or of *this silicon*:
//
//   freq_hz    yes. Every bridge switches at some frequency.
//   dead_time  yes, in NANOSECONDS. Every complementary-output peripheral on
//              every vendor has a dead-time generator and every one of them
//              counts it in ticks of some clock with some encoding. The
//              encoding is exactly what a portable unit hides: DTG's four
//              piecewise ranges are not expressible as a number of ticks, and
//              a user who had to write the code would be writing a different
//              number on every clock profile.
//   align      yes. Center vs edge alignment is a property of the modulation,
//              not of ST.
//   when_off   yes, and it is the field this config exists for. "What do the
//              pins do when the outputs are off" has an answer on every
//              bridge peripheral, and it is the answer a fault depends on.
//
// NOT here, and the reasons are the interesting half:
//   * the break input's POLARITY and its PIN. A pin is a binder tag (the same
//     rule that makes RS-485 DE `uart::de<pin>`), and the polarity belongs to
//     the pin, so it rides on the tag. A config bool would exist, and be
//     ignored, on every board with no fault line.
//   * the break FILTER and the center-aligned VARIANT. Both are units and
//     value-sets of the silicon (`BKF` counts samples of a clock derived from
//     t_DTS; CMS 1/2/3 differ in interrupt timing). Layer 2: `Role::opts`.
//   * the OUTPUT IDLE LEVELS (CR2's OISx/OISxN). Not a layering decision —
//     the driver refuses to program anything but all-zero, and argues it
//     where it does so. There is no combination worth offering: the one that
//     is not "both transistors off" is "both transistors on".
//   * the write-once configuration LOCK. Layer 2, because which fields each
//     of the three lock levels covers is per-IP.
struct bridge_config {
    //: Switching frequency of the bridge, in hertz. BLOCK-scoped by nature —
    //: there is one prescaler and one auto-reload behind all three phases —
    //: and stated once, because this facade has exactly one call site.
    std::uint32_t freq_hz = 20'000;
    //: How long both transistors of a pair are held off around every edge.
    //: NO DEFAULT VALUE, deliberately: see bridge_dead_time above.
    hal::bridge_dead_time dead_time = {};
    //: Center-aligned unless you say otherwise.
    hal::bridge_alignment align = bridge_alignment::center;
    //: What the pins do while the outputs are off, including after a break.
    hal::bridge_off_state when_off = bridge_off_state::drive_idle_level;
};

// ── Layer 2 ─────────────────────────────────────────────────────────────
//
// Empty primary, so `bridge_opts<Inst>{}` compiles for an IP nobody has
// taught any vendor knobs. Same contract as hal::uart_opts<Inst>, including
// the unenforced half: a feature two IPs share must get the same member name
// in both, and nothing checks it.
template <class Inst>
struct bridge_opts {};

template <class Inst>
struct bridge_impl;

}  // namespace alloy::hal
