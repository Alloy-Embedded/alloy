#include <cstdint>

// Linker-provided symbols
extern std::uint32_t _sbss, _ebss;
extern std::uint32_t _sdata, _edata, _sidata;
extern void (*__init_array_start[])() noexcept;
extern void (*__init_array_end[])() noexcept;

extern int main();

extern "C" {

[[noreturn]] void _alloy_startup() noexcept;

// ROM jumps to flash[4] = 0x42000004 after seeing direct-boot magic at flash[0].
// This function MUST be the first thing in .reset_vector (immediately after .boot_magic).
__attribute__((section(".reset_vector"), used, naked))
void _start() {
    asm volatile(
        ".option push          \n"
        ".option norelax       \n"
        "la   sp, _stack_top   \n"
        ".option pop           \n"
        "j    _alloy_startup   \n"
    );
}

[[noreturn]] void _alloy_startup() noexcept {
    // Zero BSS
    for (auto* p = &_sbss; p < &_ebss; ++p) {
        *p = 0u;
    }

    // Copy .data initializers from flash to SRAM
    auto* src = &_sidata;
    for (auto* dst = &_sdata; dst < &_edata; ++dst, ++src) {
        *dst = *src;
    }

    // C++ global constructors
    for (auto** fn = __init_array_start; fn < __init_array_end; ++fn) {
        (*fn)();
    }

    main();

    // main() should not return; hang if it does
    while (true) {
        asm volatile("wfi");
    }
}

}  // extern "C"
