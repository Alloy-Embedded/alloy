// can_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per CAN IP version, constrained on the instance's
// IP tag type. Operations: enable(), send(frame), receive(frame).

#pragma once

namespace alloy::hal {

template <class Inst>
struct can_impl;

}  // namespace alloy::hal
