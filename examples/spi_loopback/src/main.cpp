// Portable SPI loopback probe — clocks a test pattern out MOSI every 2 s
// and prints what MISO returned. With a jumper between MOSI and MISO the
// pattern echoes back byte-perfect (full electrical path proved); without
// one the reads float (0xFF/0x00) but TX still proves SCK/MOSI drive and
// the flag sequence. Zero #ifdefs.
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
    auto bus = board::spi::open({.clock_hz = 1'000'000, .mode = 0});

    uart.write("alloy spi_loopback (jumper MOSI->MISO para eco)\r\n");
    while (true) {
        if constexpr (board::caps::spi) {
            constexpr std::uint8_t pattern[] = {0xA5, 0x3C, 0x0F, 0x96};
            std::uint8_t rx[sizeof(pattern)];
            bool echo = true;
            for (unsigned i = 0; i < sizeof(pattern); ++i) {
                rx[i] = bus.xfer(pattern[i]);
                echo = echo && rx[i] == pattern[i];
            }
            uart.write("tx: ");
            for (auto b : pattern) {
                write_hex_byte(uart, b);
                uart.write(static_cast<std::uint8_t>(' '));
            }
            uart.write("rx: ");
            for (auto b : rx) {
                write_hex_byte(uart, b);
                uart.write(static_cast<std::uint8_t>(' '));
            }
            uart.write(echo ? "-> LOOPBACK OK\r\n" : "-> sem eco (sem jumper?)\r\n");
        } else {
            uart.write("spi: not available on this board\r\n");
        }
        alloy::sleep_for(2s);
    }
}
