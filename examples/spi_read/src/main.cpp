// Minimal SPI driver-conformance check for emulation, the sibling of i2c_read.
// SPI is full-duplex with no addressing or ACK, so the proof is a byte exchange:
// clock one byte out (MOSI) and read the byte the device shifts back (MISO). A
// test attaches a slave primed to answer with a known value; a green run proves
// the driver's clock / MOSI / MISO path actually talks to a device — no hardware.
#include <alloy/board.hpp>

#include <cstdint>
#include <span>

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
    } else {
        uart.write("spi: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
