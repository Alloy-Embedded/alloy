// A crash in the field is a support call with no evidence — unless the device
// survives it and says where it died. This is that whole loop, on any board:
// boot, report the PREVIOUS boot's fault, then deliberately cause one.
//
// Run it and the UART shows a device crashing and explaining itself forever:
//
//     alloy crash_report ready
//     no crash on record — first clean boot
//     about to fault deliberately...
//     alloy crash_report ready
//     RECOVERED FROM A FAULT  pc=0x08000abc lr=0x08000a11 status=0x00000000 (1 in a row)
//     about to fault deliberately...
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
        uart.write("\r\n");
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

    // A wild jump — the classic way real firmware dies (a corrupted function
    // pointer, a smashed return address). Bit 0 clear means "not Thumb", which
    // the core cannot execute: ARMv7-M raises a UsageFault that escalates to
    // HardFault, and ARMv6-M goes straight to HardFault. Chosen over a bad data
    // address on purpose: an unmapped LOAD is a bus fault on silicon, but Renode
    // answers unmapped reads with zero and a log line, so a data fault would
    // prove nothing here.
    auto* const wild = reinterpret_cast<void (*)()>(std::uintptr_t{2});
    wild();

    uart.write("unreachable: the fault did not fire\r\n");
    for (;;) {
    }
}
