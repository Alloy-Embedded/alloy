// DMA driver-conformance check for emulation (completes the bus/peripheral set
// after i2c_read/spi_read/adc_read). The firmware sends a distinct line over the
// debug UART *by DMA* (memory->TDR, the m2p path), so the terminal tester sees
// "dma via DMA" only if the driver's channel config (CPAR/CMAR/CNDTR/CCR) and the
// transfer + TCIF-complete handshake actually moved the bytes — no hardware.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

#include <cstdint>

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy dma_uart\r\n");

    if constexpr (board::caps::dma) {
        auto chan = alloy::dma::channel<board::dma_t, 1>::claim();
        std::uint8_t msg[] = "dma via DMA\r\n";  // in RAM (stack) for the m2p read
        const bool ok = uart.write_dma(chan, {msg, sizeof(msg) - 1});
        if (!ok) {
            uart.write("dma: FAIL\r\n");
        }
    } else {
        uart.write("dma: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
