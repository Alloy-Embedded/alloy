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
// `pc` is the instruction that died. `alloy crash --line "<paste>"` turns it
// into a file and line against the built ELF, from a device already back up.
//
// On a board with the `nvm` role the record is also persisted to flash — one
// extra line after RECOVERED. That happens HERE, not in the fault handler:
// the handler ran on a broken machine and must touch nothing that could fault
// again (no flash programming, no driver), so it only parks the record in
// .noinit RAM — which survives a warm reset but not power-off. The next boot
// has a healthy machine and all the time in the world; persisting is its job.
#include <alloy/board.hpp>
#include <alloy/fault.hpp>

#include <cstdint>

namespace {

// nvm keys for the persisted crash record (any u32 except the erased-slot
// marker 0xFFFF'FFFF). A field unit's flash then carries the LAST crash and a
// lifetime count, readable long after the RAM record and the UART line are gone.
constexpr std::uint32_t kCrashPc = 0xC0;
constexpr std::uint32_t kCrashStatus = 0xC1;
constexpr std::uint32_t kCrashTotal = 0xC2;

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
        // take() -> nvm: the doctrine above, as code. `if constexpr` keeps the
        // same source honest on boards without the role — the branch is not
        // compiled, not merely skipped. (This board, nucleo_g071rb, declares
        // no nvm role; nucleo_g0b1re compiles the true branch. UNWITNESSED in
        // emulation either way: no Renode leg exercises nvm flash writes.)
        if constexpr (board::caps::nvm) {
            const bool saved =
                board::nvm.set(kCrashPc, crash.pc) &&
                board::nvm.set(kCrashStatus, crash.status) &&
                board::nvm.set(kCrashTotal, board::nvm.get(kCrashTotal, 0u) + 1u);
            uart.write(saved ? "crash record persisted to nvm\r\n"
                             : "nvm persist FAILED\r\n");
        }
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
