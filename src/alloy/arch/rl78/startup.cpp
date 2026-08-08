// RL78 reset path.
//
// The order below is not stylistic. On this architecture the stack pointer is
// undefined at reset — there is no vector slot carrying it, the way slot 0 does
// on Cortex-M — so SP must be loaded before anything that could push: before
// the first call, before the first interrupt is ever accepted.
//
// Everything after that is the usual: give .data its initial values, zero .bss,
// run the static constructors, enter main.

#include <cstdint>

#include "alloy/arch/rl78/systick.hpp"

extern "C" {

// Filled in by the generated linker script.
extern std::uint8_t _sidata[];
extern std::uint8_t _sdata[];
extern std::uint8_t _edata[];
extern std::uint8_t _sbss[];
extern std::uint8_t _ebss[];
extern std::uint8_t _estack[];

using init_fn = void (*)();
extern init_fn __preinit_array_start[];
extern init_fn __preinit_array_end[];
extern init_fn __init_array_start[];
extern init_fn __init_array_end[];

int main();

[[noreturn]] void _start();
[[noreturn]] void alloy_start_c();

}  // extern "C"

namespace {

// Deliberately not memcpy/memset: those live in newlib, and calling into a
// library before .data exists is exactly the kind of ordering assumption that
// works until the day the implementation changes.
inline void copy(std::uint8_t* dst, const std::uint8_t* src, std::uint8_t* end) {
    while (dst != end) {
        *dst++ = *src++;
    }
}

inline void zero(std::uint8_t* dst, std::uint8_t* end) {
    while (dst != end) {
        *dst++ = 0;
    }
}

}  // namespace

// The reset entry MUST be naked.
//
// A normal function gets a prologue, and on this target GCC's prologue starts
// with `push ax` — a stack write before the stack pointer exists. Reading the
// disassembly is the only way that shows up: the C++ compiles either way, and
// [[noreturn]] does not suppress it. So the entry is asm-only: load SP, then
// branch into ordinary C++ which may now have all the prologue it likes.
extern "C" [[noreturn]] __attribute__((naked)) void _start() {
    __asm volatile(
        "movw ax, #__estack\n\t"
        "movw sp, ax\n\t"
        // Interrupts stay masked through initialisation: a handler running now
        // would touch .bss before it is zeroed.
        "di\n\t"
        "br !!_alloy_start_c");
}

extern "C" [[noreturn]] void alloy_start_c() {
    copy(_sdata, _sidata, _edata);
    zero(_sbss, _ebss);

    for (init_fn* f = __preinit_array_start; f != __preinit_array_end; ++f) {
        (*f)();
    }
    for (init_fn* f = __init_array_start; f != __init_array_end; ++f) {
        (*f)();
    }

    main();

    // main() returning on a microcontroller is a bug, not an exit. Stop
    // somewhere a debugger can find rather than falling into whatever follows.
    for (;;) {
    }
}
