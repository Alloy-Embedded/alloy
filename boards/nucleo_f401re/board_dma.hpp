#pragma once

#include "board_uart.hpp"

#include "hal/dma.hpp"

namespace board {

using DebugUartTxDma =
    alloy::hal::dma::channel_handle<alloy::hal::dma::PeripheralId::USART2,
                                    alloy::hal::dma::SignalId::signal_TX>;
using DebugUartRxDma =
    alloy::hal::dma::channel_handle<alloy::hal::dma::PeripheralId::USART2,
                                    alloy::hal::dma::SignalId::signal_RX>;

[[nodiscard]] inline auto make_debug_uart_tx_dma(alloy::hal::dma::Config config = {})
    -> DebugUartTxDma {
    return alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART2,
                                 alloy::hal::dma::SignalId::signal_TX>(config);
}

[[nodiscard]] inline auto make_debug_uart_rx_dma(alloy::hal::dma::Config config = {})
    -> DebugUartRxDma {
    return alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART2,
                                 alloy::hal::dma::SignalId::signal_RX>(config);
}

}  // namespace board
