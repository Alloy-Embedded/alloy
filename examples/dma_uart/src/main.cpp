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

namespace {
// Set from the DMA completion INTERRUPT. Seeing it true after the transfer is
// proof the NVIC line fired and the handler ran — not merely that the transfer
// finished, which wait() already tells us.
volatile bool g_dma_irq_fired = false;

template <class Dma>
concept HasDma = requires { alloy::hal::dma_impl<Dma>::enable_controller(); };
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy dma_uart\r\n");

    // Templated lambda (like dma_probe) so `Dma` is dependent: the requires below
    // then SFINAE-guards uart.write_dma, which is constrained on Inst::dmareq_tx —
    // absent on e.g. the STM32F7 USART — instead of hard-erroring in a concrete
    // context. Boards without that DMA-TX request just print "not available".
    [&uart]<class Dma>(Dma*) {
        if constexpr (HasDma<Dma> && requires(alloy::dma::channel<Dma, 1>& c) {
                          uart.write_dma(c, std::span<const std::uint8_t>{});
                          c.on_complete(nullptr, nullptr);  // absent on the SAM E70 XDMAC
                      }) {
            auto chan = alloy::dma::channel<Dma, 1>::claim();
            // Registered BEFORE the transfer on purpose: the channel config
            // register may only be written while the channel is disabled, so
            // the interrupt enables are folded in at setup.
            chan.on_complete(+[](void* flag) {
                *static_cast<volatile bool*>(flag) = true;
            }, const_cast<bool*>(&g_dma_irq_fired));
            std::uint8_t msg[] = "dma via DMA\r\n";  // in RAM (stack) for the m2p read
            if (!uart.write_dma(chan, {msg, sizeof(msg) - 1})) {
                uart.write("dma: FAIL\r\n");
            }
            uart.write(g_dma_irq_fired ? "dma irq: fired\r\n"
                                       : "dma irq: NOT fired\r\n");
        } else {
            uart.write("dma: not available on this board\r\n");
        }
    }(static_cast<board::dma_t*>(nullptr));

    for (;;) {
        alloy::sleep_for(1s);
    }
}
