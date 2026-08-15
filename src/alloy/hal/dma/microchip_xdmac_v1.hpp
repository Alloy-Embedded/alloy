// DMA driver for the SAM E70 XDMAC — one NVIC line, 24 interchangeable
// channels, per-peripheral PERID request ids.
//
// SELECTION only: the constraint below is what binds the generated
// alloy::ip::microchip::xdmac_v1 tag to the engine. The engine itself — the
// ISR/latch/poll contract, the CC fold rules, every register sequence — lives
// in microchip_xdmac_v1_body.hpp, split out so the host latch witness
// (tests/test_xdmac_v1_latch.cpp) can run the REAL code against a hand-written
// IP double; the file carries the full DS60001527 notes.

#pragma once

#include <concepts>

#include "alloy/hal/dma/dma_impl.hpp"
#include "alloy/hal/dma/microchip_xdmac_v1_body.hpp"
#include "alloy/ip/microchip/xdmac_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::xdmac_v1>
struct dma_impl<Inst> : detail::microchip_xdmac_v1_engine<Inst> {};

}  // namespace alloy::hal
