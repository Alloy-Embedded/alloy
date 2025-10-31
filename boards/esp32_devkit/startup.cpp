/// ESP32 Startup Code
/// Uses Alloy common startup framework

#include "../../src/startup/startup_common.hpp"
#include <cstdint>

// Linker symbols specific to ESP32
extern "C" {
    extern uint32_t _init_start;
    extern uint32_t _iram_start;
    extern uint32_t _stack_end;

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

/// Reset handler - entry point after reset (ESP32 calls it call_start_cpu0)
extern "C" [[noreturn]] void call_start_cpu0() {
    // Perform runtime initialization (data/bss/constructors)
    alloy::startup::initialize_runtime();

    // Call system initialization (clock, peripherals, etc.)
    SystemInit();

    // Call main
    main();

    // If main returns, loop forever
    alloy::startup::infinite_loop();
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
