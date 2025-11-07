/**
 * @file gpio_template_test.cpp
 * @brief Assembly comparison for GPIO template vs manual register access
 *
 * Compares assembly output between:
 * 1. Manual register access (baseline)
 * 2. Template-based GPIO (should be identical)
 *
 * Compile:
 *   arm-none-eabi-g++ -mcpu=cortex-m7 -mthumb -O2 -S \
 *       -I../../src \
 *       -o gpio_test.s gpio_template_test.cpp
 *
 * Expected result: IDENTICAL assembly (zero overhead)
 */

#include <cstdint>

// ==============================================================================
// Manual Register Access (Baseline)
// ==============================================================================

struct PIO_Regs {
    volatile uint32_t PER;    // 0x00
    volatile uint32_t PDR;    // 0x04
    volatile uint32_t PSR;    // 0x08
    uint32_t RESERVED0;       // 0x0C
    volatile uint32_t OER;    // 0x10
    volatile uint32_t ODR;    // 0x14
    volatile uint32_t OSR;    // 0x18
    uint32_t RESERVED1;       // 0x1C
    volatile uint32_t IFER;   // 0x20
    volatile uint32_t IFDR;   // 0x24
    volatile uint32_t IFSR;   // 0x28
    uint32_t RESERVED2;       // 0x2C
    volatile uint32_t SODR;   // 0x30
    volatile uint32_t CODR;   // 0x34
    volatile uint32_t ODSR;   // 0x38
    volatile uint32_t PDSR;   // 0x3C
};

constexpr uint32_t PIOC_BASE = 0x400E1200;
constexpr uint8_t PIN_8 = 8;
constexpr uint32_t PIN_8_MASK = (1u << PIN_8);

__attribute__((noinline))
void test_manual_gpio() {
    volatile PIO_Regs* pio = reinterpret_cast<volatile PIO_Regs*>(PIOC_BASE);

    // Configure as output
    pio->PER = PIN_8_MASK;   // Enable PIO control
    pio->OER = PIN_8_MASK;   // Enable output

    // Set HIGH
    pio->SODR = PIN_8_MASK;

    // Set LOW
    pio->CODR = PIN_8_MASK;

    // Toggle
    if (pio->ODSR & PIN_8_MASK) {
        pio->CODR = PIN_8_MASK;
    } else {
        pio->SODR = PIN_8_MASK;
    }

    // Read pin
    volatile uint32_t state = pio->PDSR & PIN_8_MASK;
    (void)state;
}

// ==============================================================================
// Template-Based GPIO (Should Generate Identical Assembly)
// ==============================================================================

#include "hal/platform/same70/gpio.hpp"

using namespace alloy::hal::same70;

__attribute__((noinline))
void test_template_gpio() {
    // Using type alias - compile-time resolution
    auto led = Led0{};

    // Configure as output
    led.setMode(GpioMode::Output);

    // Set HIGH
    led.set();

    // Set LOW
    led.clear();

    // Toggle
    led.toggle();

    // Read pin
    auto state = led.read();
    (void)state;
}

// ==============================================================================
// Inline Version (Fully Optimized)
// ==============================================================================

__attribute__((noinline))
void test_template_inline() {
    // All operations should inline to single instructions
    volatile auto* pio = reinterpret_cast<volatile PIO_Regs*>(0x400E1200);
    constexpr uint32_t MASK = (1u << 8);

    // Configure
    pio->PER = MASK;
    pio->OER = MASK;

    // Set/clear/toggle
    pio->SODR = MASK;
    pio->CODR = MASK;

    if (pio->ODSR & MASK) {
        pio->CODR = MASK;
    } else {
        pio->SODR = MASK;
    }

    // Read
    volatile uint32_t state = pio->PDSR & MASK;
    (void)state;
}

// ==============================================================================
// Entry point (prevents dead code elimination)
// ==============================================================================

int main() {
    test_manual_gpio();
    test_template_gpio();
    test_template_inline();
    return 0;
}
