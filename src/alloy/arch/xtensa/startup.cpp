// Xtensa C startup — data/bss/ctors/main, reached from call_start_cpu0.

#include <cstdint>

extern "C" {

extern std::uint32_t _sidata;
extern std::uint32_t _sdata;
extern std::uint32_t _edata;
extern std::uint32_t _sbss;
extern std::uint32_t _ebss;

using init_fn = void (*)();
extern init_fn __preinit_array_start[];
extern init_fn __preinit_array_end[];
extern init_fn __init_array_start[];
extern init_fn __init_array_end[];

int main();

[[noreturn]] void _alloy_startup() {
    std::uint32_t* src = &_sidata;
    for (std::uint32_t* dst = &_sdata; dst < &_edata; ++dst, ++src) {
        *dst = *src;
    }
    for (std::uint32_t* dst = &_sbss; dst < &_ebss; ++dst) {
        *dst = 0u;
    }
    for (init_fn* fn = __preinit_array_start; fn != __preinit_array_end; ++fn) {
        (*fn)();
    }
    for (init_fn* fn = __init_array_start; fn != __init_array_end; ++fn) {
        (*fn)();
    }
    (void)main();
    for (;;) {
        __asm volatile("waiti 0");
    }
}

}  // extern "C"
