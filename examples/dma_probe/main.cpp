#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "dma_probe requires BOARD_UART_HEADER for the selected board"
#endif

#ifndef BOARD_DMA_HEADER
    #error "dma_probe requires BOARD_DMA_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_DMA_HEADER

#include <cstdint>

#include "hal/systick.hpp"
#include "logger/logger.hpp"
#include "logger/sinks/uart_sink.hpp"

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

template <typename DmaChannel>
void log_dma_binding(const char* label) {
    constexpr auto descriptor = DmaChannel::descriptor();
    LOG_INFO(
        "%s binding=%u controller=%u request=%u route=%u conflict=%u channel=%d",
        label, static_cast<unsigned>(descriptor.binding_id),
        static_cast<unsigned>(descriptor.controller_id),
        static_cast<unsigned>(descriptor.request_line_id),
        static_cast<unsigned>(descriptor.route_id),
        static_cast<unsigned>(descriptor.conflict_group_id), descriptor.channel_index);
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (const auto result = uart.configure(); result.is_err()) {
        blink_error(100);
    }

    auto uart_sink = alloy::logger::make_uart_sink(uart);
    alloy::logger::Logger::remove_all_sinks();
    alloy::logger::Logger::configure({
        .default_level = alloy::logger::Level::Info,
        .enable_timestamps = false,
        .enable_colors = false,
        .enable_source_location = true,
        .timestamp_precision = alloy::logger::TimestampPrecision::Milliseconds,
    });

    if (!alloy::logger::Logger::add_sink(&uart_sink)) {
        blink_error(150);
    }

    [[maybe_unused]] auto tx_dma = board::make_debug_uart_tx_dma();
    [[maybe_unused]] auto rx_dma = board::make_debug_uart_rx_dma({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .mode = alloy::hal::dma::Mode::normal,
        .priority = alloy::hal::dma::Priority::medium,
        .data_width = alloy::hal::dma::DataWidth::bits8,
    });

    if (const auto result = tx_dma.configure(); result.is_err()) {
        blink_error(200);
    }
    if (const auto result = rx_dma.configure(); result.is_err()) {
        blink_error(250);
    }

    LOG_INFO("dma probe ready");
    log_dma_binding<decltype(tx_dma)>("debug-uart-tx");
    log_dma_binding<decltype(rx_dma)>("debug-uart-rx");

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    }
}
