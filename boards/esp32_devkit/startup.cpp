// ESP32 C-level startup — called from startup.S after stack is set up.
// BSS clear → .data copy → C++ ctors → main().

#include <cstdint>

extern std::uint32_t _sbss, _ebss;
extern std::uint32_t _sdata, _edata, _sidata;
extern void (*__init_array_start[])() noexcept;
extern void (*__init_array_end[])() noexcept;

extern int main();

extern "C" [[noreturn]] void _alloy_startup() noexcept {
    for (auto* p = &_sbss; p < &_ebss; ++p) { *p = 0u; }

    auto* src = &_sidata;
    for (auto* dst = &_sdata; dst < &_edata; ++dst, ++src) { *dst = *src; }

    for (auto** fn = __init_array_start; fn < __init_array_end; ++fn) { (*fn)(); }

    main();

    while (true) { asm volatile("waiti 0"); }
}
