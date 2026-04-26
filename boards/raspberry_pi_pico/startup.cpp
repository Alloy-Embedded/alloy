// RP2040 bare-metal startup for Alloy.
//
// Provides:
//   - Cortex-M0+ vector table (.vectors section)
//   - Reset_Handler: copies .data, zeroes .bss, runs ctors, calls alloy_main
//   - Default_Handler: infinite loop for unhandled faults/IRQs

#include <cstdint>

extern "C" {
extern std::uint32_t _sidata;       // flash LMA of .data
extern std::uint32_t _sdata;        // RAM VMA start of .data
extern std::uint32_t _edata;        // RAM VMA end   of .data
extern std::uint32_t _sbss;
extern std::uint32_t _ebss;
extern std::uint32_t _stack_top;    // initial SP (top of SRAM)

extern void (*__init_array_start[])();
extern void (*__init_array_end[])();

[[noreturn]] void alloy_main();
}

[[noreturn]] static void Default_Handler() noexcept { for (;;) {} }
extern "C" [[noreturn]] void Reset_Handler() noexcept;

// Cortex-M0+ vector table. The RP2040 ROM reads words [0] (SP) and [1] (PC)
// from the vector table base and branches to the reset handler.
// Place in .vectors so the linker script's KEEP(*(.vectors)) picks it up first.
__attribute__((section(".vectors"), used))
const std::uintptr_t kVectorTable[] = {
    reinterpret_cast<std::uintptr_t>(&_stack_top),         //  0: initial SP
    reinterpret_cast<std::uintptr_t>(&Reset_Handler),      //  1: Reset
    reinterpret_cast<std::uintptr_t>(&Default_Handler),    //  2: NMI
    reinterpret_cast<std::uintptr_t>(&Default_Handler),    //  3: HardFault
    0u, 0u, 0u, 0u, 0u, 0u, 0u,                           //  4-10: reserved
    reinterpret_cast<std::uintptr_t>(&Default_Handler),    // 11: SVCall
    0u, 0u,                                                //  12-13: reserved
    reinterpret_cast<std::uintptr_t>(&Default_Handler),    // 14: PendSV
    reinterpret_cast<std::uintptr_t>(&Default_Handler),    // 15: SysTick
    // IRQ 0-25 (RP2040 has 26 external interrupts)
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
    reinterpret_cast<std::uintptr_t>(&Default_Handler),
};

extern "C" [[noreturn]] void Reset_Handler() noexcept {
    // Copy initialised data from flash to RAM
    auto* src = &_sidata;
    for (auto* dst = &_sdata; dst < &_edata; *dst++ = *src++) {}

    // Zero-initialise BSS
    for (auto* p = &_sbss; p < &_ebss; *p++ = 0u) {}

    // Run C++ static constructors
    for (auto f = __init_array_start; f < __init_array_end; (*f++)()) {}

    alloy_main();
    for (;;) {}
}
