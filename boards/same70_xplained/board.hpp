#pragma once

/**
 * @file board.hpp
 * @brief SAME70 Xplained Ultra - Board Abstraction Layer
 *
 * This header implements the standard board interface for SAME70 Xplained Ultra.
 * Other boards implement the same interface, enabling portable examples.
 *
 * Standard Board Interface:
 * - board::init() - Initialize board hardware
 * - board::delay_ms(ms) - Millisecond delay
 * - board::millis() - Get system uptime
 * - board::led::on/off/toggle() - LED control
 *
 * Hardware:
 * - MCU: ATSAME70Q21B (ARM Cortex-M7 @ 300MHz)
 * - LED: Green LED on PC8 (active LOW)
 * - Button: SW0 on PA11 (active LOW)
 * - UART Console: UART1 on EDBG virtual COM port
 */

#include "board_config.hpp"
#include <cstdint>

namespace board {

// Import board-specific configuration
using namespace same70_xplained;

// PIOC base address and LED pin (PC8)
constexpr uint32_t PIOC_BASE = 0x400E1200;
constexpr uint32_t LED_PIN = 8;

// =============================================================================
// Timing
// =============================================================================

/// Global tick counter (incremented in SysTick_Handler)
extern volatile uint32_t system_ticks_ms;

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 * 
 * Uses SysTick counter for timing
 */
inline void delay_ms(uint32_t ms) {
    uint32_t start = system_ticks_ms;
    while ((system_ticks_ms - start) < ms) {
        __asm volatile ("wfi");  // Wait for interrupt (power save)
    }
}

/**
 * @brief Get system uptime in milliseconds
 * @return Milliseconds since board_init()
 */
inline uint32_t millis() {
    return system_ticks_ms;
}

// =============================================================================
// LED Control
// =============================================================================

namespace led {

// PIOC register offsets (from SAME70 datasheet)
struct PioC {
    volatile uint32_t PER;    // 0x00 - PIO Enable
    volatile uint32_t PDR;    // 0x04 - PIO Disable
    volatile uint32_t PSR;    // 0x08 - PIO Status
    uint32_t reserved1;       // 0x0C
    volatile uint32_t OER;    // 0x10 - Output Enable
    volatile uint32_t ODR;    // 0x14 - Output Disable
    volatile uint32_t OSR;    // 0x18 - Output Status
    uint32_t reserved2;       // 0x1C
    volatile uint32_t IFER;   // 0x20 - Glitch Input Filter Enable
    volatile uint32_t IFDR;   // 0x24 - Glitch Input Filter Disable
    volatile uint32_t IFSR;   // 0x28 - Glitch Input Filter Status
    uint32_t reserved3;       // 0x2C
    volatile uint32_t SODR;   // 0x30 - Set Output Data
    volatile uint32_t CODR;   // 0x34 - Clear Output Data
    volatile uint32_t ODSR;   // 0x38 - Output Data Status
    volatile uint32_t PDSR;   // 0x3C - Pin Data Status
};

static inline PioC* pio() {
    return reinterpret_cast<PioC*>(PIOC_BASE);
}

/**
 * @brief Initialize LED GPIO (PC8, active LOW)
 */
inline void init() {
    pio()->PER = (1u << LED_PIN);   // Enable PIO control (not peripheral)
    pio()->OER = (1u << LED_PIN);   // Enable output
    pio()->SODR = (1u << LED_PIN);  // Set high = LED OFF (active LOW)
}

/**
 * @brief Turn LED on
 */
inline void on() {
    pio()->CODR = (1u << LED_PIN);  // Clear = LED ON (active LOW)
}

/**
 * @brief Turn LED off
 */
inline void off() {
    pio()->SODR = (1u << LED_PIN);  // Set = LED OFF (active LOW)
}

/**
 * @brief Toggle LED state
 */
inline void toggle() {
    if (pio()->ODSR & (1u << LED_PIN)) {
        pio()->CODR = (1u << LED_PIN);  // Currently HIGH -> make LOW (ON)
    } else {
        pio()->SODR = (1u << LED_PIN);  // Currently LOW -> make HIGH (OFF)
    }
}

} // namespace led

// =============================================================================
// Board Initialization
// =============================================================================

/**
 * @brief Initialize board hardware
 * 
 * Configures:
 * - System clocks (300 MHz)
 * - SysTick (1ms tick)
 * - LED GPIO
 * - (Future: UART console, etc.)
 * 
 * Call this early in main() before using board peripherals
 */
void init();

} // namespace board
