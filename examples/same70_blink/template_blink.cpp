/**
 * @file template_blink.cpp
 * @brief SAME70 Blink using GPIO template (zero overhead)
 *
 * This version uses the template-based GPIO from the Alloy HAL.
 * Should generate IDENTICAL binary size to standalone_blink.cpp
 * but with type-safe, portable code.
 *
 * Board: SAME70 Xplained Ultra
 * LED: PC8 (active LOW)
 */

#include <stdint.h>
#include "hal/platform/same70/gpio.hpp"

using namespace alloy::hal::same70;
using namespace alloy::core;

// Simple delay function (same as standalone version)
void delay_ms(uint32_t ms) {
    // Rough approximation at 300MHz
    volatile uint32_t count = ms * 100000;
    while (count--) {
        __asm__ volatile("nop");
    }
}

// Enable PIOC peripheral clock
void enable_pioc_clock() {
    // PMC PCER0 register to enable peripheral clocks
    volatile uint32_t* pmc_pcer0 = reinterpret_cast<volatile uint32_t*>(0x400E0610);
    *pmc_pcer0 = (1U << 12);  // ID_PIOC = 12

    // Small delay for clock stabilization
    for (volatile uint32_t i = 0; i < 100; i++) {
        __asm__ volatile("nop");
    }
}

// Reset handler - entry point
extern "C" void Reset_Handler() {
    // 1. Enable PIOC peripheral clock
    enable_pioc_clock();

    // 2. Create LED GPIO instance using type alias
    //    This is compile-time resolved - zero overhead!
    auto led = Led0{};  // Led0 = GpioPin<PIOC_BASE, 8>

    // 3. Configure as output
    led.setMode(GpioMode::Output);

    // 4. Blink forever
    //    Note: LED is active LOW on this board
    //    - set() = LED OFF (pin HIGH)
    //    - clear() = LED ON (pin LOW)
    while (1) {
        // Turn LED ON (active LOW)
        led.clear();
        delay_ms(500);

        // Turn LED OFF (active LOW)
        led.set();
        delay_ms(500);
    }
}

// Minimal vector table
extern uint32_t _estack;  // Defined in linker script

__attribute__((section(".isr_vector")))
__attribute__((used))
const void* vector_table[] = {
    &_estack,                                    // Initial stack pointer
    reinterpret_cast<void*>(Reset_Handler),     // Reset handler
};

// Default handler for all exceptions/interrupts
extern "C" void Default_Handler() {
    while(1);
}
