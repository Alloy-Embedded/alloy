// Portable Modbus RTU master. The SAME file compiles for every alloy board
// with a debug_uart role; boards without an RS-485 transceiver keep the
// default no-op direction pin. Zero preprocessor conditionals.
//
// The client polls unit 17 for two holding registers once a second and
// reports each transaction on the same uart — which doubles as the Modbus
// wire under emulation, where the robot plays the server and asserts the
// request bytes are exact (11 03 00 00 00 02 C6 9B). On a bench, wire the
// bus to a second uart role instead.
#include <alloy/board.hpp>
#include <alloy/time.hpp>
#include <modbus.hpp>

#include <array>
#include <cstdint>

namespace mb = alloy::lib::modbus;
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
    auto uart = board::debug_uart::open({.baud = 19'200});
    uart.write("alloy modbus_rtu_client ready\r\n");

    // MaxRegisters fixes the buffers AT COMPILE TIME: an 8-register budget
    // costs a 21-byte response framer — the whole client fits in ~40 bytes.
    mb::rtu_client<decltype(uart), /*MaxRegisters=*/8> bus{
        uart, {.baud = 19'200, .response_timeout = 500ms}};

    std::array<std::uint16_t, 2> regs{};
    for (;;) {
        if (const auto r = bus.read_holding(/*unit=*/17, /*address=*/0, regs)) {
            uart.write("modbus: ok ");
            write_u32(uart, regs[0]);
            uart.write(" ");
            write_u32(uart, regs[1]);
            uart.write("\r\n");
        } else if (r.error() == mb::modbus_error::timeout) {
            uart.write("modbus: timeout\r\n");
        } else if (mb::is_exception(r.error())) {
            uart.write("modbus: exception ");  // r.error() IS the wire code
            write_u32(uart, mb::exception_code(r.error()));
            uart.write("\r\n");
        } else {
            uart.write("modbus: error\r\n");
        }
        alloy::sleep_for(1s);
    }
}
