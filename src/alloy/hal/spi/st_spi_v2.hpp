// SPI master driver for the ST FIFO variant (spi2s1 v3.x: G0/F7/L4/H7-lite).
//
// SELECTION only: the constraint below is what binds the generated
// alloy::ip::st::spi_v2 tag to the engine. The engine itself — the polled
// lockstep, the RXNE-driven interrupt transfer, the DMA endpoint hooks and
// every register sequence — lives in st_spi_v2_body.hpp, split out so the
// host witness (tests/test_st_spi_v2_dma.cpp) can run the REAL code against a
// hand-written IP double; that file carries the full RM notes.

#pragma once

#include <concepts>

#include "alloy/hal/spi/spi_impl.hpp"
#include "alloy/hal/spi/st_spi_v2_body.hpp"
#include "alloy/ip/st/spi_v2.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::spi_v2>
struct spi_impl<Inst> : detail::st_spi_v2_engine<Inst> {};

}  // namespace alloy::hal
