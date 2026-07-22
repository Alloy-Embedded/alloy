// pwm_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per PWM IP version, constrained on the
// instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct pwm_impl;

}  // namespace alloy::hal
