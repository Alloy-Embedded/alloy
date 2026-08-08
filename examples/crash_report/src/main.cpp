// A crash in the field is a support call with no evidence — unless the device
// survives it and says where it died. This is that whole loop, on any board:
// boot, report the PREVIOUS boot's fault, then deliberately cause one.
//
// Run it and the UART shows a device crashing and explaining itself, until it
// notices it is in a crash loop and refuses to continue:
//
//     alloy crash_report ready
//     no crash on record — first clean boot
//     about to fault deliberately...
//     alloy crash_report ready
//     RECOVERED FROM A FAULT  pc=0x0800... lr=0x0800... status=0x00000000 (1 in a row)
//     about to fault deliberately...
//     ...
//     RECOVERED FROM A FAULT  pc=0x0800... lr=0x0800... status=0x00000000 (3 in a row)
//     three faults in a row — staying in safe mode
//
// `pc` is the instruction that died. Feed it to addr2line against the .elf and
// you get the file and line, from a device that is already back up.
#include <alloy/board.hpp>
#include <alloy/fault.hpp>

#include <cstdint>

namespace {

void put_hex(auto& uart, std::uint32_t v) {
    uart.write("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        const auto nibble = static_cast<std::uint8_t>((v >> shift) & 0xFu);
        uart.write(static_cast<std::uint8_t>(nibble < 10 ? '0' + nibble
                                                         : 'a' + nibble - 10));
    }
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy crash_report ready\r\n");

    if (alloy::fault::record crash; alloy::fault::take(crash)) {
        uart.write("RECOVERED FROM A FAULT  pc=");
        put_hex(uart, crash.pc);
        uart.write(" lr=");
        put_hex(uart, crash.lr);
        uart.write(" status=");
        put_hex(uart, crash.status);
        uart.write(" (");
        uart.write(static_cast<std::uint8_t>('0' + crash.consecutive % 10));
        uart.write(" in a row)\r\n");
        // A device that keeps dying should stop doing the thing that kills it.
        // Here that means parking instead of faulting again.
        if (crash.consecutive >= 3) {
            uart.write("three faults in a row — staying in safe mode\r\n");
            for (;;) {
            }
        }
    } else {
        uart.write("no crash on record — first clean boot\r\n");
    }

    alloy::sleep_for(std::chrono::milliseconds{200});
    uart.write("about to fault deliberately...\r\n");
    uart.flush();

    // Real firmware dies by wild jumps and corrupted function pointers, and on
    // silicon those vector into the very handler this example exercises. But an
    // emulator is weaker than silicon here — Renode never vectors an organic
    // fault (a wild jump just kills the core, vector table unread) — so the
    // deliberate crash goes through fault::trigger() instead: a software-pended
    // NMI, an architectural Cortex-M feature that takes the same
    // capture-record-reset path everywhere. One trigger, honest on the bench
    // AND provable in CI.
    alloy::fault::trigger();
}
