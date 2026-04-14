#pragma once

#include "board_uart.hpp"

#include "hal/dma.hpp"

namespace board {

namespace detail {

[[nodiscard]] constexpr auto with_same70_dma_channel(alloy::hal::dma::Config config, int channel)
    -> alloy::hal::dma::Config {
    if (config.channel_index < 0) {
        config.channel_index = channel;
    }
    return config;
}

}  // namespace detail

using DebugUartTxDma =
    alloy::hal::dma::channel_handle<alloy::hal::dma::PeripheralId::USART0,
                                    alloy::hal::dma::SignalId::signal_TX>;
using DebugUartRxDma =
    alloy::hal::dma::channel_handle<alloy::hal::dma::PeripheralId::USART0,
                                    alloy::hal::dma::SignalId::signal_RX>;

[[nodiscard]] inline auto make_debug_uart_tx_dma(alloy::hal::dma::Config config = {})
    -> DebugUartTxDma {
    return alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART0,
                                 alloy::hal::dma::SignalId::signal_TX>(
        detail::with_same70_dma_channel(config, 0));
}

[[nodiscard]] inline auto make_debug_uart_rx_dma(alloy::hal::dma::Config config = {})
    -> DebugUartRxDma {
    return alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART0,
                                 alloy::hal::dma::SignalId::signal_RX>(
        detail::with_same70_dma_channel(config, 1));
}

}  // namespace board
