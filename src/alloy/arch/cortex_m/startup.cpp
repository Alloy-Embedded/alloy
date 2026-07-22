// Cortex-M reset handler — hand-written BEHAVIOR.
//
// The vector table itself (order, count, handler names) is a generated FACT
// (vectors.cpp in the project's .alloy/generated tree); it points its Reset
// entry here. Linker symbols come from the generated linker script.

#include <cstdint>

extern "C" {

extern std::uint32_t _sidata;  // .data load address (flash)
extern std::uint32_t _sdata;   // .data start (ram)
extern std::uint32_t _edata;   // .data end (ram)
extern std::uint32_t _sbss;    // .bss start
extern std::uint32_t _ebss;    // .bss end

// Static-constructor arrays (walked directly — no newlib crti/_init needed).
using init_fn = void (*)();
extern init_fn __preinit_array_start[];
extern init_fn __preinit_array_end[];
extern init_fn __init_array_start[];
extern init_fn __init_array_end[];

int main();

[[noreturn]] void Reset_Handler() {
    // Copy initialized data from flash to RAM.
    std::uint32_t* src = &_sidata;
    for (std::uint32_t* dst = &_sdata; dst < &_edata; ++dst, ++src) {
        *dst = *src;
    }
    // Zero the BSS.
    for (std::uint32_t* dst = &_sbss; dst < &_ebss; ++dst) {
        *dst = 0u;
    }
    // Run static constructors, then the application.
    for (init_fn* fn = __preinit_array_start; fn != __preinit_array_end; ++fn) {
        (*fn)();
    }
    for (init_fn* fn = __init_array_start; fn != __init_array_end; ++fn) {
        (*fn)();
    }
    (void)main();
    for (;;) {
    }
}

}  // extern "C"
