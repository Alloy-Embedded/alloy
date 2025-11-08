// Alloy Framework - Common Startup Code
//
// Provides common initialization routines used across all MCU architectures:
// - .data section copy from flash to RAM
// - .bss section zero initialization
// - C++ global constructors initialization
// - Default exception handlers
//
// This reduces code duplication across board-specific startup files.

#pragma once

#include <stdint.h>

namespace alloy::startup {

/// Linker script symbols that must be defined in each board's linker script
extern "C" {
// Standard ARM linker symbols
extern uint32_t _sidata;  // Start of .data in flash
extern uint32_t _sdata;   // Start of .data in RAM
extern uint32_t _edata;   // End of .data in RAM
extern uint32_t _sbss;    // Start of .bss
extern uint32_t _ebss;    // End of .bss

// ESP32/Xtensa alternative linker symbols
extern uint32_t _data_start;
extern uint32_t _data_end;
extern uint32_t _data_load;
extern uint32_t _bss_start;
extern uint32_t _bss_end;

// C++ constructor/destructor support
extern void (*__init_array_start[])();
extern void (*__init_array_end[])();
extern void (*__fini_array_start[])();
extern void (*__fini_array_end[])();
}

/// Copy .data section from Flash to RAM (ARM/Standard naming)
/// This must be called before accessing any initialized global variables
inline void copy_data_section() {
#if defined(__XTENSA__)
    // ESP32/Xtensa uses different symbol names
    uint32_t* src = &_data_load;
    uint32_t* dest = &_data_start;
    while (dest < &_data_end) {
        *dest++ = *src++;
    }
#else
    // ARM/Standard naming
    uint32_t* src = &_sidata;
    uint32_t* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }
#endif
}

/// Zero-initialize .bss section
/// This must be called before accessing any uninitialized global variables
inline void zero_bss_section() {
#if defined(__XTENSA__)
    // ESP32/Xtensa uses different symbol names
    uint32_t* dest = &_bss_start;
    while (dest < &_bss_end) {
        *dest++ = 0;
    }
#else
    // ARM/Standard naming
    uint32_t* dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }
#endif
}

/// Call C++ global constructors
/// This must be called before main() to properly initialize global C++ objects
inline void call_init_array() {
    for (auto ctor = __init_array_start; ctor < __init_array_end; ++ctor) {
        (*ctor)();
    }
}

/// Call C++ global destructors (rarely used in embedded systems)
/// This is typically only called if main() returns
inline void call_fini_array() {
    for (auto dtor = __fini_array_start; dtor < __fini_array_end; ++dtor) {
        (*dtor)();
    }
}

/// Standard Reset_Handler implementation
/// Performs all necessary initialization before calling main()
///
/// Board-specific startup files should call this from their Reset_Handler
/// after performing any board-specific hardware initialization
///
/// Example usage in board-specific startup.cpp:
/// ```cpp
/// extern "C" [[noreturn]] void Reset_Handler() {
///     // Optional: Early hardware init (e.g., FPU enable)
///     alloy::startup::initialize_runtime();
///     main();
///     alloy::startup::infinite_loop();
/// }
/// ```
inline void initialize_runtime() {
    // 1. Copy initialized data from Flash to RAM
    copy_data_section();

    // 2. Zero-initialize uninitialized data
    zero_bss_section();

    // 3. Call C++ global constructors
    call_init_array();
}

/// Cleanup function (rarely used in embedded)
/// Calls C++ global destructors
inline void cleanup_runtime() {
    call_fini_array();
}

/// Infinite loop with wait-for-interrupt
/// Called after main() returns to prevent undefined behavior
[[noreturn]] inline void infinite_loop() {
    while (true) {
#if defined(__ARM_ARCH)
        __asm__ volatile("wfi");  // ARM: Wait for interrupt
#elif defined(__XTENSA__)
        __asm__ volatile("waiti 0");  // Xtensa: Wait for interrupt
#else
        // Generic fallback
        __asm__ volatile("nop");
#endif
    }
}

}  // namespace alloy::startup

// Default exception handler for unhandled interrupts
// Boards can override this by providing their own Default_Handler
extern "C" [[noreturn]] void Default_Handler() {
    while (true) {
        // Trap in infinite loop
        // In debug builds, a debugger can break here to inspect the failure
    }
}

// SystemInit declaration - boards should provide this
// This function performs board-specific initialization
// (e.g., clock configuration, FPU enable, peripheral setup, etc.)
extern "C" void SystemInit();
