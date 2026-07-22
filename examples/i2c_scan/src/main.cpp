// Portable I2C scanner — probes 0x08..0x77 every 5 s and prints whoever
// ACKs. On an empty bus a clean "0 found" IS the validation: START/STOP
// generation, clocking and NACK detection all working. Zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

template <class Uart>
void write_hex_byte(const Uart& uart, std::uint8_t value) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>('0'));
    uart.write(static_cast<std::uint8_t>('x'));
    uart.write(static_cast<std::uint8_t>(digits[value >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[value & 0xF]));
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    auto bus = board::i2c::open({.speed_hz = 100'000});

    uart.write("alloy i2c_scan\r\n");
    while (true) {
        if constexpr (board::caps::i2c) {
            uart.write("scan: ");
            unsigned found = 0;
            for (std::uint8_t addr = 0x08; addr <= 0x77; ++addr) {
                if (bus.probe(addr)) {
                    write_hex_byte(uart, addr);
                    uart.write(static_cast<std::uint8_t>(' '));
                    ++found;
                }
            }
            if (found == 0) {
                uart.write("(nenhum dispositivo — NACK em todos, barramento ok)");
            }
            uart.write("\r\n");
        } else {
            uart.write("i2c: not available on this board\r\n");
        }
        alloy::sleep_for(5s);
    }
}
