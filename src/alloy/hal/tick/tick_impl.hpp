// tick_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per timer IP version, constrained on the
// instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct tick_impl;

}  // namespace alloy::hal
