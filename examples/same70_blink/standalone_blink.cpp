/// ATSAME70 Standalone Blink - Minimal bare metal example
/// Completely standalone - no framework dependencies
/// Blinks LED on PC8 of ATSAME70 Xplained board

#include <stdint.h>

// PMC (Power Management Controller) - needed to enable PIOC clock
#define PMC_BASE     0x400E0600UL
#define PMC_PCER0    (*(volatile uint32_t*)(PMC_BASE + 0x0010))  // Peripheral Clock Enable Register 0
#define PMC_PCSR0    (*(volatile uint32_t*)(PMC_BASE + 0x0018))  // Peripheral Clock Status Register 0

// Peripheral IDs (for PMC)
#define ID_PIOC      12  // PIOC Peripheral ID

// PIOC Base Address (Port C - where LED is connected)
#define PIOC_BASE    0x400E1200UL

// PIOC Register Offsets
#define PIOC_PER     (*(volatile uint32_t*)(PIOC_BASE + 0x0000))  // PIO Enable
#define PIOC_OER     (*(volatile uint32_t*)(PIOC_BASE + 0x0010))  // Output Enable
#define PIOC_SODR    (*(volatile uint32_t*)(PIOC_BASE + 0x0030))  // Set Output Data
#define PIOC_CODR    (*(volatile uint32_t*)(PIOC_BASE + 0x0034))  // Clear Output Data
#define PIOC_ODSR    (*(volatile uint32_t*)(PIOC_BASE + 0x0038))  // Output Data Status

// LED on PC8
#define LED_PIN      (1U << 8)

// Simple delay function
void delay_ms(uint32_t ms) {
    // Rough approximation at 300MHz
    volatile uint32_t count = ms * 100000;
    while (count--) {
        __asm__ volatile("nop");
    }
}

// Minimal startup - Reset handler
extern "C" void Reset_Handler() {
    // 1. Enable PIOC peripheral clock in PMC
    //    This is CRITICAL - without clock, PIOC registers won't work!
    PMC_PCER0 = (1U << ID_PIOC);

    // Small delay to let clock stabilize
    for (volatile uint32_t i = 0; i < 100; i++) {
        __asm__ volatile("nop");
    }

    // 2. Disable pull-up on PC8 (optional, but good practice)
    // PIOC_PUDR = LED_PIN;  // We'll skip this for minimal example

    // 3. Enable PIO control on PC8 (disable peripheral control)
    PIOC_PER = LED_PIN;

    // 4. Configure PC8 as output
    PIOC_OER = LED_PIN;

    // 5. Disable multi-drive (open-drain) - use push-pull
    // PIOC_MDDR = LED_PIN;  // We'll skip this, push-pull is default

    // Blink forever
    while (1) {
        // Turn LED ON (active LOW, so clear output)
        PIOC_CODR = LED_PIN;
        delay_ms(500);

        // Turn LED OFF (active LOW, so set output)
        PIOC_SODR = LED_PIN;
        delay_ms(500);
    }
}

// Minimal vector table
extern uint32_t _estack;  // Defined in linker script

__attribute__((section(".isr_vector")))
__attribute__((used))
const void* vector_table[] = {
    &_estack,                           // Initial stack pointer
    reinterpret_cast<void*>(Reset_Handler),  // Reset handler
};

// Default handler for all exceptions/interrupts
extern "C" void Default_Handler() {
    while(1);
}
