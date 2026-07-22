// adc_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per ADC IP version, constrained on the
// instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct adc_impl;

}  // namespace alloy::hal
