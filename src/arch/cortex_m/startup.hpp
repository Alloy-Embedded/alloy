#pragma once

#include <cstdint>

namespace alloy::arch::cortex_m {

extern "C" {
extern std::uint32_t _sidata;
extern std::uint32_t _sdata;
extern std::uint32_t _edata;
extern std::uint32_t _sbss;
extern std::uint32_t _ebss;

extern void (*__init_array_start[])();
extern void (*__init_array_end[])();

int alloy_application_main() asm("main");
void SystemInit();
}

inline void initialize_data() {
    auto* source = &_sidata;
    auto* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *source++;
    }
}

inline void initialize_bss() {
    auto* dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0u;
    }
}

inline void run_static_constructors() {
    for (auto ctor = __init_array_start; ctor < __init_array_end; ++ctor) {
        if (*ctor != nullptr) {
            (*ctor)();
        }
    }
}

[[noreturn]] inline void idle_forever() {
    while (true) {
        __asm__ volatile("wfi");
    }
}

[[noreturn]] inline void default_handler() {
    idle_forever();
}

[[noreturn]] inline void reset_and_enter_main() {
    initialize_data();
    initialize_bss();
    SystemInit();
    run_static_constructors();
    alloy_application_main();
    idle_forever();
}

}  // namespace alloy::arch::cortex_m
