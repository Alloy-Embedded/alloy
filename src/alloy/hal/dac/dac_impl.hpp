// dac_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per DAC IP version, constrained on the instance's
// IP tag type. Operations: enable(), write(code).

#pragma once

namespace alloy::hal {

template <class Inst>
struct dac_impl;

}  // namespace alloy::hal
