// dma_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per DMA IP version, constrained on the
// instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct dma_impl;

}  // namespace alloy::hal
