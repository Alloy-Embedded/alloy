// DMA driver-conformance check for emulation (completes the bus/peripheral set
// after i2c_read/spi_read/adc_read). The firmware sends a distinct line over the
// debug UART *by DMA* (memory->TDR, the m2p path), so the terminal tester sees
// "dma via DMA" only if the driver's channel config (CPAR/CMAR/CNDTR/CCR) and the
// transfer + TCIF-complete handshake actually moved the bytes — no hardware.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

#include <cstdint>
#include <span>

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy dma_uart\r\n");

    bool used_dma = false;
    if constexpr (board::caps::dma) {
        using chan_t = alloy::dma::channel<board::dma_t, 1>;
        // write_dma is only available where the debug UART exposes a DMA TX request
        // (dmareq_tx); guard on the actual call so this compiles on every board
        // (e.g. the STM32F7 USART has no such request).
        if constexpr (requires(chan_t& c) {
                          uart.write_dma(c, std::span<const std::uint8_t>{});
                      }) {
            auto chan = chan_t::claim();
            std::uint8_t msg[] = "dma via DMA\r\n";  // in RAM (stack) for the m2p read
            if (!uart.write_dma(chan, {msg, sizeof(msg) - 1})) {
                uart.write("dma: FAIL\r\n");
            }
            used_dma = true;
        }
    }
    if (!used_dma) {
        uart.write("dma: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
