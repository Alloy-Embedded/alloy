/// ARM SysTick Integration with RTOS
///
/// Integrates SysTick timer with RTOS scheduler.
/// The SysTick_Handler calls both the SysTick counter update
/// and the RTOS scheduler tick.

#include "rtos/platform/arm_context.hpp"
#include "rtos/rtos.hpp"

// SysTick Handler - called every 1ms
//
// This handler does two things:
// 1. Updates SysTick microsecond counter (for time tracking)
// 2. Calls RTOS scheduler tick (for task scheduling)
extern "C" void SysTick_Handler() {
// Update SysTick counter (call platform-specific handler)
// This is defined in systick.hpp for each platform
#if defined(STM32F1)
    extern void stm32f1_systick_handler();
    stm32f1_systick_handler();
#elif defined(STM32F4)
    extern void stm32f4_systick_handler();
    stm32f4_systick_handler();
#elif defined(SAMD21)
    extern void samd21_systick_handler();
    samd21_systick_handler();
#elif defined(RP2040)
    // RP2040 doesn't need systick handler (hardware timer)
#endif

    // Call RTOS scheduler tick
    alloy::rtos::RTOS::tick();

    // Trigger context switch if needed
    if (alloy::rtos::RTOS::need_context_switch()) {
        alloy::rtos::trigger_context_switch();
    }
}
