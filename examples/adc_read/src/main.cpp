// ADC driver-conformance check for emulation (sibling of i2c_read/spi_read),
// against a purpose-written Renode STM32G0 ADC model. Runs one blocking
// conversion of channel 3 and prints the raw counts; the model returns a
// channel-encoded value (1000 + channel), so "adc: 1003" proves the driver's
// enable / calibrate / channel-select / start / poll-EOC / read-DR sequence
// produced the converted value of the RIGHT channel — no hardware.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void write_u32(const Uart& uart, std::uint32_t value) {
    char buf[10];
    unsigned n = 0;
    do { buf[n++] = static_cast<char>('0' + value % 10u); value /= 10u; } while (value != 0u);
    while (n != 0u) { uart.write(static_cast<std::uint8_t>(buf[--n])); }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy adc_read\r\n");
    if constexpr (board::caps::adc) {
        auto adc = board::adc::open();
        const std::uint16_t raw = adc.read(3);
        uart.write("adc: ");
        write_u32(uart, raw);
        uart.write("\r\n");
    } else {
        uart.write("adc: not available on this board\r\n");
    }
    for (;;) { alloy::sleep_for(1s); }
}
