// flash_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per flash-controller IP version, constrained on
// the instance's IP tag type. Operations: erase_page, program (double-word),
// unlock/lock. Reads are memory-mapped (the flash array is addressable), so no
// read method exists here.

#pragma once

namespace alloy::hal {

template <class Inst>
struct flash_impl;

}  // namespace alloy::hal
