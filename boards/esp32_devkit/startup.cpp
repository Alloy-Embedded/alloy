/// ESP32 Startup Code
///
/// Minimal startup code for ESP32 (Xtensa LX6)
/// Handles reset, exception vectors, and initialization

#include <cstdint>

// Linker symbols
extern "C" {
    extern uint32_t _bss_start;
    extern uint32_t _bss_end;
    extern uint32_t _data_start;
    extern uint32_t _data_end;
    extern uint32_t _data_load;
    extern uint32_t _init_start;
    extern uint32_t _iram_start;
    extern uint32_t _stack_end;

    // C++ constructor support
    typedef void (*init_func_t)();
    extern init_func_t __init_array_start;
    extern init_func_t __init_array_end;

    // Main application entry point
    int main();
}

/// System initialization (before main)
extern "C" void SystemInit() {
    // ESP32 system initialization would include:
    // - Cache configuration
    // - CPU frequency setup
    // - Peripheral initialization
    // For now, minimal implementation
}

/// Default exception handler
extern "C" [[noreturn]] void Default_Handler() {
    while (true) {
        __asm__ volatile("waiti 0");  // Wait for interrupt
    }
}

/// Reset handler - entry point after reset
extern "C" [[noreturn]] void call_start_cpu0() {
    // 1. Initialize .bss section (zero-initialize)
    uint32_t* bss = &_bss_start;
    uint32_t* bss_end = &_bss_end;
    while (bss < bss_end) {
        *bss++ = 0;
    }

    // 2. Copy .data section from Flash to RAM
    uint32_t* data = &_data_start;
    uint32_t* data_end = &_data_end;
    uint32_t* data_load = &_data_load;
    while (data < data_end) {
        *data++ = *data_load++;
    }

    // 3. Call system initialization
    SystemInit();

    // 4. Call C++ constructors
    for (init_func_t* func = &__init_array_start; func < &__init_array_end; func++) {
        (*func)();
    }

    // 5. Call main
    main();

    // 6. If main returns, hang
    while (true) {
        __asm__ volatile("waiti 0");
    }
}

/// CPU1 entry point (not used in single-core mode)
extern "C" [[noreturn]] void call_start_cpu1() {
    // CPU1 startup - not implemented for minimal example
    while (true) {
        __asm__ volatile("waiti 0");
    }
}

// Exception vectors (Xtensa architecture)
// These are placed at specific addresses by the linker script

extern "C" [[noreturn]] void WindowOverflow4Handler() {
    Default_Handler();
}

extern "C" [[noreturn]] void WindowUnderflow4Handler() {
    Default_Handler();
}

extern "C" [[noreturn]] void WindowOverflow8Handler() {
    Default_Handler();
}

extern "C" [[noreturn]] void WindowUnderflow8Handler() {
    Default_Handler();
}

extern "C" [[noreturn]] void WindowOverflow12Handler() {
    Default_Handler();
}

extern "C" [[noreturn]] void WindowUnderflow12Handler() {
    Default_Handler();
}

extern "C" [[noreturn]] void Level2InterruptHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void Level3InterruptHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void Level4InterruptHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void Level5InterruptHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void DebugExceptionHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void NMIExceptionHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void KernelExceptionHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void UserExceptionHandler() {
    Default_Handler();
}

extern "C" [[noreturn]] void DoubleExceptionHandler() {
    Default_Handler();
}

// Window exception vectors (placed at specific addresses)
__attribute__((section(".WindowVectors.text")))
[[noreturn]] void _WindowOverflow4() {
    WindowOverflow4Handler();
}

__attribute__((section(".WindowVectors.text")))
[[noreturn]] void _WindowUnderflow4() {
    WindowUnderflow4Handler();
}

__attribute__((section(".WindowVectors.text")))
[[noreturn]] void _WindowOverflow8() {
    WindowOverflow8Handler();
}

__attribute__((section(".WindowVectors.text")))
[[noreturn]] void _WindowUnderflow8() {
    WindowUnderflow8Handler();
}

__attribute__((section(".WindowVectors.text")))
[[noreturn]] void _WindowOverflow12() {
    WindowOverflow12Handler();
}

__attribute__((section(".WindowVectors.text")))
[[noreturn]] void _WindowUnderflow12() {
    WindowUnderflow12Handler();
}

// Interrupt level vectors
__attribute__((section(".Level2InterruptVector.text")))
[[noreturn]] void _Level2Vector() {
    Level2InterruptHandler();
}

__attribute__((section(".Level3InterruptVector.text")))
[[noreturn]] void _Level3Vector() {
    Level3InterruptHandler();
}

__attribute__((section(".Level4InterruptVector.text")))
[[noreturn]] void _Level4Vector() {
    Level4InterruptHandler();
}

__attribute__((section(".Level5InterruptVector.text")))
[[noreturn]] void _Level5Vector() {
    Level5InterruptHandler();
}

__attribute__((section(".DebugExceptionVector.text")))
[[noreturn]] void _DebugExceptionVector() {
    DebugExceptionHandler();
}

__attribute__((section(".NMIExceptionVector.text")))
[[noreturn]] void _NMIExceptionVector() {
    NMIExceptionHandler();
}

__attribute__((section(".KernelExceptionVector.text")))
[[noreturn]] void _KernelExceptionVector() {
    KernelExceptionHandler();
}

__attribute__((section(".UserExceptionVector.text")))
[[noreturn]] void _UserExceptionVector() {
    UserExceptionHandler();
}

__attribute__((section(".DoubleExceptionVector.text")))
[[noreturn]] void _DoubleExceptionVector() {
    DoubleExceptionHandler();
}
