// Portable Modbus RTU server (slave). The SAME file compiles for every alloy
// board with a debug_uart role; zero preprocessor conditionals. Unit 17,
// 9600 baud — chosen deliberately: t3.5 at 9600 is 4011 µs, several SysTick
// periods, so under emulation the frame timing does not lean on the
// emulator's sub-millisecond fidelity (the robot leg states the same).
//
// The register bank is plain user memory behind the DataModel concept — the
// application owns it and can update it between polls; here registers 0 and
// 1 carry known constants the emulation robot (playing the master with an
// independent CRC implementation) reads back and asserts.
#include <alloy/board.hpp>
#include <alloy/time.hpp>
#include <modbus.hpp>

namespace mb = alloy::lib::modbus;
using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = 9'600});

    mb::holding_bank<64> bank;
    bank.regs[0] = 0x1234;
    bank.regs[1] = 0xABCD;

    mb::rtu_server<decltype(uart), mb::holding_bank<64>, 8> server{
        uart, bank, {.unit = 17, .baud = 9'600}};

    uart.write("alloy modbus_rtu_server ready\r\n");
    for (;;) {
        // Bounded work per call: drain what the uart has, dispatch at most
        // one request, answer at most once. Never blocks, never sleeps.
        (void)server.poll();
    }
}
