// Minimal SPI driver-conformance check for emulation, the sibling of i2c_read.
// SPI is full-duplex with no addressing or ACK, so the proof is a byte exchange:
// clock one byte out (MOSI) and read the byte the device shifts back (MISO). A
// test attaches a slave primed to answer with a known value; a green run proves
// the driver's clock / MOSI / MISO path actually talks to a device — no hardware.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void write_hex_byte(const Uart& uart, std::uint8_t value) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>(digits[value >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[value & 0xF]));
}
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
    } else {
        uart.write("spi: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
