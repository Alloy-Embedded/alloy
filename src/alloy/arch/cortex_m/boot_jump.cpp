// Cortex-M implementations of the boot-control seam (arch/cpu.hpp): hand off to
// another image's vector table, and request a system reset. Everything here is
// ARCHITECTURAL — the same SCS registers on every Cortex-M — never chip data.
#include <cstdint>

#include "alloy/arch/cpu.hpp"

namespace alloy::arch {

namespace {
volatile std::uint32_t& reg32(std::uintptr_t addr) {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}
constexpr std::uintptr_t systick_ctrl = 0xE000E010u;  // contract-ok: architectural Cortex-M SysTick CSR
constexpr std::uintptr_t scb_icsr = 0xE000ED04u;      // contract-ok: architectural Cortex-M SCB ICSR
constexpr std::uintptr_t scb_vtor = 0xE000ED08u;      // contract-ok: architectural Cortex-M SCB VTOR
constexpr std::uintptr_t scb_aircr = 0xE000ED0Cu;     // contract-ok: architectural Cortex-M SCB AIRCR
}  // namespace

[[noreturn]] void boot_image(std::uintptr_t vector_base) {
    // Quiet the core: stop the bootloader's SysTick and drop any pending tick so
    // the app never takes an exception through the WRONG vector table. The
    // bootloader polls its UART (no NVIC interrupts to disable by design).
    reg32(systick_ctrl) = 0u;
    reg32(scb_icsr) = 1u << 25;  // PENDSTCLR
    asm volatile("dsb\nisb" ::: "memory");

    const auto* vt = reinterpret_cast<const std::uint32_t*>(vector_base);
    reg32(scb_vtor) = static_cast<std::uint32_t>(vector_base);
    asm volatile(
        "msr msp, %0\n"
        "bx %1\n"
        :
        : "r"(vt[0]), "r"(vt[1] | 1u)  // thumb bit, defensively
        : "memory");
    __builtin_unreachable();
}

[[noreturn]] void system_reset() {
    asm volatile("dsb" ::: "memory");
    reg32(scb_aircr) = (0x05FAu << 16) | (1u << 2);  // contract-ok: VECTKEY | SYSRESETREQ, architectural
    for (;;) {
        asm volatile("" ::: "memory");  // wait for the reset to take
    }
}

}  // namespace alloy::arch
