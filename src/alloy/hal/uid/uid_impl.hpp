// uid_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per unique-ID block, constrained on the
// instance's IP tag type (data-driven driver selection).

#pragma once

namespace alloy::hal {

template <class Inst>
struct uid_impl;

}  // namespace alloy::hal
