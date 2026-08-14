// DMA driver for the ST v2 STREAM engine (F2/F4/F7 — CHSEL request selection,
// no DMAMUX companion).
//
// SELECTION only: the constraint below is what binds the generated
// alloy::ip::st::dma_v2 tag to the engine. The engine itself — the ISR/latch/
// poll contract, the SxCR fold rules, the EN-is-a-request teardown, every
// register sequence — lives in st_dma_v2_body.hpp, split out so the host latch
// witness (tests/test_st_dma_v2_latch.cpp) can run the REAL code against a
// hand-written IP double; that file carries the full RM notes.
//
// No mux_t requirement here, and none may ever appear: the ABSENCE of a
// DMAMUX companion is what keeps this driver and st_dma_v1 (which requires
// one) from ever competing for the same instance (design §3.2).

#pragma once

#include <concepts>

#include "alloy/hal/dma/dma_impl.hpp"
#include "alloy/hal/dma/st_dma_v2_body.hpp"
#include "alloy/ip/st/dma_v2.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::dma_v2>
struct dma_impl<Inst> : detail::st_dma_v2_engine<Inst> {};

}  // namespace alloy::hal
