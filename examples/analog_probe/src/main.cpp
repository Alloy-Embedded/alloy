// Portable analog probe — prints raw readings of the ADC's INTERNAL
// channels (bandgap vref + temperature sensor) once a second: numeric
// hardware proof with zero wiring. Boards without an ADC role print an
// honest note instead. Zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

template <class Uart>
void write_u32(const Uart& uart, std::uint32_t value) {
    char buf[10];
    unsigned n = 0;
    do {
        buf[n++] = static_cast<char>('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (n > 0) {
        uart.write(static_cast<std::uint8_t>(buf[--n]));
    }
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    auto adc = board::adc::open();

    uart.write("alloy analog_probe\r\n");
    while (true) {
        if constexpr (board::caps::adc) {
            uart.write("vref_raw=");
            write_u32(uart, adc.read(board::adc_vref_channel));
            uart.write(" temp_raw=");
            write_u32(uart, adc.read(board::adc_temp_channel));
            uart.write("\r\n");
        } else {
            uart.write("adc: not available on this board\r\n");
        }
        alloy::sleep_for(1s);
    }
}
