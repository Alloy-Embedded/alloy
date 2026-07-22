// i2c_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per I2C IP version, constrained on the
// instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct i2c_impl;

}  // namespace alloy::hal
