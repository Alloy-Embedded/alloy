// ADC driver-conformance check for emulation (sibling of i2c_read/spi_read),
// against Renode's OWN Analog.STM32G0_ADC. The emulation leg feeds DISTINCT
// voltages to DISTINCT channels (1650 mV on ch3, 3300 mV on ch4 at a 3.3 V
// reference) and this firmware converts both, so the printed counts must be
// round(mV/3300*4095) — "adc ch3: 2048" and "adc ch4: 4095". A wrong channel
// converts a different voltage and prints different counts (an unfed channel
// prints 0 — witnessed), and wrong conversion math prints wrong counts, so
// neither can pass by coincidence. Two handshakes are vacuous under emulation
// (CCRDY is shimmed in, ADCAL never sticks — see emit/renode.py); the rest of
// enable / channel-select / start / poll-EOC / read-DR runs against a model
// this project did not write. No hardware.
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

template <class Uart, class Adc>
void convert_and_print(const Uart& uart, Adc& adc, std::uint8_t channel) {
    const std::uint16_t raw = adc.read(channel);
    uart.write("adc ch");
    write_u32(uart, channel);
    uart.write(": ");
    write_u32(uart, raw);
    uart.write("\r\n");
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy adc_read\r\n");
    if constexpr (board::caps::adc) {
        auto adc = board::adc::open();
        convert_and_print(uart, adc, 3);
        convert_and_print(uart, adc, 4);
    } else {
        uart.write("adc: not available on this board\r\n");
    }
    for (;;) { alloy::sleep_for(1s); }
}
