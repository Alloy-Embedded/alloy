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
extern std::uint32_t _sifastcode;  // .fastcode load address (flash)
extern std::uint32_t _sfastcode;   // .fastcode start (ram)
extern std::uint32_t _efastcode;   // .fastcode end (ram)
extern std::uint32_t _sifastdata;  // .fastdata load address (flash)
extern std::uint32_t _sfastdata;   // .fastdata start (coupled ram)
extern std::uint32_t _efastdata;   // .fastdata end
extern std::uint32_t _sfastbss;    // .fastbss start (coupled ram)
extern std::uint32_t _efastbss;    // .fastbss end

// Static-constructor arrays (walked directly — no newlib crti/_init needed).
using init_fn = void (*)();
extern init_fn __preinit_array_start[];
extern init_fn __preinit_array_end[];
extern init_fn __init_array_start[];
extern init_fn __init_array_end[];

int main();

[[noreturn]] void Reset_Handler() {
    // Point VTOR at our vector table. Architectural SCB register (0xE000ED08);
    // required on chips whose boot flow enters us with VTOR elsewhere (RP2040
    // boot2, bootloaders), harmless where the table is already at the boot
    // address. Every supported core implements VTOR (M0+ w/ VTOR, M4, M7).
    extern const std::uint32_t g_vector_table[];
    *reinterpret_cast<volatile std::uint32_t*>(0xE000ED08u) =
        reinterpret_cast<std::uint32_t>(&g_vector_table[0]);

    // Copy initialized data from flash to RAM.
    std::uint32_t* src = &_sidata;
    for (std::uint32_t* dst = &_sdata; dst < &_edata; ++dst, ++src) {
        *dst = *src;
    }
    // Copy ALLOY_FASTCODE from flash to fast RAM (same scheme as .data). Empty
    // and free unless a function opted in.
    src = &_sifastcode;
    for (std::uint32_t* dst = &_sfastcode; dst < &_efastcode; ++dst, ++src) {
        *dst = *src;
    }
    // ALLOY_FASTDATA / ALLOY_FASTBSS, in the chip's coupled RAM (DTCM on an
    // M7, CCM on an F3/F4/G4). Same scheme again; both empty unless something
    // opted in, and both absent entirely on a chip with no coupled memory —
    // the linker defines the symbols equal, so the loops run zero times.
    src = &_sifastdata;
    for (std::uint32_t* dst = &_sfastdata; dst < &_efastdata; ++dst, ++src) {
        *dst = *src;
    }
    for (std::uint32_t* dst = &_sfastbss; dst < &_efastbss; ++dst) {
        *dst = 0u;
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
