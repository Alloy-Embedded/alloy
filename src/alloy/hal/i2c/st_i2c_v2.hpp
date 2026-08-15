// I2C controller driver for the ST i2c_v2 IP (TIMINGR/ISR era).
//
// SELECTION only: the constraint below is what binds the generated
// alloy::ip::st::i2c_v2 tag to the engine. The engine itself — the timing
// math, the polled transfers, the interrupt-driven path and the one-shot DMA
// hooks, every register sequence — lives in st_i2c_v2_body.hpp, split out so
// the host register witness (tests/test_i2c_dma.cpp) can run the REAL code
// against a hand-written IP double; that file carries the full RM0444 notes.
//
// Same seam as st_dma_v1.hpp / st_usart_v4.hpp: the logic lives once, in the
// body; this specialization inherits it unchanged.

#pragma once

#include <concepts>

#include "alloy/hal/i2c/i2c_impl.hpp"
#include "alloy/hal/i2c/st_i2c_v2_body.hpp"
#include "alloy/ip/st/i2c_v2.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::i2c_v2>
struct i2c_impl<Inst> : detail::st_i2c_v2_engine<Inst> {};

}  // namespace alloy::hal
