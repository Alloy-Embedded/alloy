// DMA driver for the ST v1 channel engine + DMAMUX request router
// (G0-era pairing; the mux is the instance's `mux` companion).
//
// SELECTION only: the constraint below is what binds the generated
// alloy::ip::st::dma_v1 tag to the engine. The engine itself — the ISR/latch/
// poll contract, the CCR fold rules, every register sequence — lives in
// st_dma_v1_body.hpp, split out so the host latch witness
// (tests/test_st_dma_v1_latch.cpp) can run the REAL code against a
// hand-written IP double; the file carries the full RM0444 notes.

#pragma once

#include <concepts>

#include "alloy/hal/dma/dma_impl.hpp"
#include "alloy/hal/dma/st_dma_v1_body.hpp"
#include "alloy/ip/st/dma_v1.hpp"

namespace alloy::hal {

// Constrained on the IP tag AND the mux companion: this driver models the
// DMAMUX pairing specifically (the dma_v1 layout also exists on families
// with fixed or CSELR routing — those get their own driver when curated).
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::dma_v1> &&
             requires { typename Inst::mux_t; }
struct dma_impl<Inst> : detail::st_dma_v1_engine<Inst> {};

}  // namespace alloy::hal
