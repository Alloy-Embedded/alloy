// pwm_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per PWM IP version, constrained on the
// instance's IP tag type.
//
// The Layer-1 VOCABULARY lives here too, not in alloy/pwm.hpp, for the same
// reason hal/bridge/bridge_impl.hpp holds bridge_config: a driver's enable()
// takes the config, so the config has to be reachable from a driver, and a
// driver cannot include the facade that includes it. alloy::pwm aliases these
// names so a caller never spells `hal`.

#pragma once

#include <cstdint>

namespace alloy::hal {

template <class Inst>
struct pwm_impl;

//: Edge- or centre-aligned counting. BLOCK-scoped like the frequency, and for
//: the same reason: one counter serves every channel of the timer.
//:
//: Edge is the default and stays the default — it is what an LED, a heater or
//: a fan wants, and it is what every existing caller already has. Centre
//: matters when several channels of ONE timer drive one load: their edges stop
//: coinciding. It halves the duty steps at a given carrier, because the
//: counter walks up and back down inside one period, and that trade belongs to
//: the caller.
enum class pwm_alignment : std::uint8_t { edge, center };

//: What this timer publishes on its trigger output, so a converter can be
//: started by the counter instead of by the CPU — the difference between
//: sampling at a chosen point in the period and sampling wherever software got
//: to. Refused at open() on a block whose curated data reports no trigger
//: output, rather than accepted and never fired.
enum class pwm_trigger : std::uint8_t { none, on_update, on_compare };

struct pwm_config {
    //: Switching frequency. BLOCK-scoped by nature — one prescaler and one
    //: reload behind every channel — and stated at a channel's call site
    //: because that is the only call there is.
    std::uint32_t freq_hz = 1'000;
    hal::pwm_alignment align = pwm_alignment::edge;
    hal::pwm_trigger trigger = pwm_trigger::none;
};

}  // namespace alloy::hal
