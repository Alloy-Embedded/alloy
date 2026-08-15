// Minimal SPI driver-conformance check for emulation, the sibling of i2c_read.
// SPI is full-duplex with no addressing or ACK, so the proof is a byte exchange:
// clock one byte out (MOSI) and read the byte the device shifts back (MISO). A
// test attaches a slave primed to answer with a known value; a green run proves
// the driver's clock / MOSI / MISO path actually talks to a device — no hardware.
//
// A third leg is anchor 2.4 of docs/design/dma-streams.md: the SAME exchange
// with the CPU out of the data phase entirely. One call claims BOTH channels
// the board assigned to `spi.rx`/`spi.tx`, arms RX before TX, runs both, waits
// both and releases both — half a duplex is not a thing. It folds open only on
// a board that states the pair, and the marker it prints ("spi dma: ...") is
// one no polled or interrupt path in this file can produce, so a silent
// fallback cannot pass the leg that asserts it.
#include <alloy/board.hpp>

#include <cstdint>
#include <span>
#include <type_traits>

using namespace alloy::literals;

namespace {
template <class Uart>
void write_hex_byte(const Uart& uart, std::uint8_t value) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>(digits[value >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[value & 0xF]));
}

// Set from the SPI completion INTERRUPT. Seeing it true after the transfer is
// proof the NVIC line fired and the driver's handler ran — not merely that the
// bytes moved, which reading the buffer already tells us.
volatile bool g_spi_irq_fired = false;
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy spi_read\r\n");

    if constexpr (board::caps::spi) {
        auto bus = board::spi::open({.clock_hz = 1'000'000, .mode = 0});
        // Exchange one byte. What comes back is whatever the device shifts out on
        // MISO while it receives 0xA5 on MOSI — a test primes it with a known
        // response and asserts it, proving both directions clocked correctly.
        const std::uint8_t got = bus.xfer(0xA5);
        uart.write("spi: 0x");
        write_hex_byte(uart, got);
        uart.write("\r\n");

        // Second leg: the SAME exchange, interrupt-driven. Only the first byte
        // is written here; the ISR clocks the rest and sets the flag when the
        // last one lands. The templated lambda makes `S` dependent, so the
        // `requires` below removes the leg on a board whose SPI driver has no
        // interrupt hook (SAM E70) instead of hard-erroring in a concrete main.
        [&uart]<class S>(S& b) {
            if constexpr (requires {
                              b.transfer_async(std::span<const std::uint8_t>{},
                                               std::span<std::uint8_t>{});
                          }) {
                static const std::uint8_t out[] = {0x11, 0x22, 0x33};
                std::uint8_t in[3] = {};
                b.transfer_async(out, in, +[](void* flag) {
                    *static_cast<volatile bool*>(flag) = true;
                }, const_cast<bool*>(&g_spi_irq_fired));
                if (!b.wait_transfer()) {
                    uart.write("spi async: timeout\r\n");
                    b.detach_transfer();
                }
                uart.write("spi async: 0x");
                for (std::uint8_t byte : in) {
                    write_hex_byte(uart, byte);
                }
                uart.write("\r\n");
                uart.write(g_spi_irq_fired ? "spi irq: fired\r\n" : "spi irq: NOT fired\r\n");
            } else {
                uart.write("spi irq: not available on this board\r\n");
            }
        }(bus);

        // Third leg — ANCHOR 2.4. The same kind of exchange with the CPU out
        // of the data phase: `transfer_dma` claims the board's pair (RX first,
        // TX second, one masked section), arms the receive channel and raises
        // RXDMAEN before the transmit channel exists, and the write of TXDMAEN
        // is what starts the traffic. Nothing here names a channel, a DMAMUX
        // id or a CHSEL value — the routes ride the binder.
        //
        // Gated THREE times, every gate on a dependent name so a board that
        // cannot do this drops the leg instead of failing the build:
        //   1. no `rx_route` alias at all — a board with no SPI role opens a
        //      null handle, and asking a missing member is a hard error, not
        //      a substitution failure;
        //   2. either half void — the board stated no pair (or only one side);
        //   3. no `transfer_dma` — the port's SPI driver has no DMA hooks
        //      (st_spi_v1, microchip_spi_v1 today).
        // The probe goes through the HANDLE's aliases, never through
        // `board::dma::spi_rx`: a namespace-scope constant does not SFINAE-fold
        // in a requires-clause (the phase-2 lesson, paid for once).
        //
        // The buffers are ordinary automatic objects, i.e. RAM: §4 of the
        // design — no engine in the framework may be pointed at flash, and the
        // SAME70's XDMAC provably cannot read it.
        [&uart]<class S>(S& b) {
            if constexpr (!requires { typename S::rx_route; }) {
                uart.write("spi dma: not available on this board\r\n");
            } else if constexpr (std::is_void_v<typename S::rx_route> ||
                                 std::is_void_v<typename S::tx_route>) {
                uart.write("spi dma: not assigned\r\n");
            } else if constexpr (requires(S& s) {
                                     s.transfer_dma(std::span<const std::uint8_t>{},
                                                    std::span<std::uint8_t>{});
                                 }) {
                const std::uint8_t out[] = {0xC0, 0xFF, 0xEE, 0x77};
                std::uint8_t in[4] = {};
                if (!b.transfer_dma(out, in)) {
                    uart.write("spi dma: no transfer\r\n");
                } else {
                    uart.write("spi dma: 0x");
                    for (std::uint8_t byte : in) {
                        write_hex_byte(uart, byte);
                    }
                    uart.write("\r\n");
                }
                // The DUPLEX half, read back through the peer instead of
                // asserted about it: one more polled exchange, whose answer is
                // the NEXT value the device had queued. It can only be the
                // one primed after the four above if the transmit channel
                // really clocked four cycles into the device — a stub that
                // filled `in` without driving MOSI leaves the peer's cursor
                // four short and prints a different byte. (What it does NOT
                // witness: the four MOSI byte VALUES. This mock discards what
                // it receives, so their arrival is a count, not a compare.)
                uart.write("spi dma peer: 0x");
                write_hex_byte(uart, b.xfer(0x00));
                uart.write("\r\n");
            } else {
                uart.write("spi dma: driver has no hooks\r\n");
            }
        }(bus);
    } else {
        uart.write("spi: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
