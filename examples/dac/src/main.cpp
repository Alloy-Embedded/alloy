// DAC demo with self-check: drive the DAC output through a few 12-bit codes and
// read each one back through the ADC. On this board DAC_OUT1 and ADC_IN4 are the
// SAME pin (PA4), so the ADC reading tracks the DAC code with no external wiring
// — numeric proof the DAC really drives the pin. Zero #ifdefs: board::dac and
// board::adc are no-op/absent stubs where the board lacks them, guarded by
// `if constexpr`.
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
    uart.write("\r\nalloy dac demo\r\n");

    if constexpr (board::caps::dac && board::caps::adc) {
        board::dac.enable();
        auto adc = board::adc::open();
        constexpr std::uint16_t codes[] = {0u, 1024u, 2048u, 3072u, 4095u};
        uart.write("DAC_OUT1=PA4=ADC_IN4: dac code -> adc readback\r\n");
        for (;;) {
            for (std::uint16_t code : codes) {
                board::dac.write(code);
                alloy::sleep_for(20ms);        // let the output buffer settle
                const std::uint16_t back = adc.read(4);  // ADC_IN4 = PA4
                uart.write("dac=");
                write_u32(uart, code);
                uart.write(" adc=");
                write_u32(uart, back);
                uart.write("\r\n");
                alloy::sleep_for(1s);
            }
        }
    } else {
        uart.write("this board declares no dac+adc roles\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    }
}
