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

/// Global tick counter in microseconds (incremented in SysTick_Handler)
extern volatile uint64_t system_tick_us;

/**
 * @brief Get system uptime in microseconds with sub-millisecond precision
 * @return Microseconds since board_init()
 *
 * Reads the SysTick VAL register to get microsecond precision between interrupts.
 * SysTick counts DOWN from LOAD to 0.
 */
inline uint64_t micros() {
    // SysTick registers
    constexpr uint32_t SYSTICK_VAL = 0xE000E008;
    constexpr uint32_t SYSTICK_LOAD = 0xE000E004;

    // Read current values (do multiple reads for consistency)
    volatile uint32_t* val_reg = reinterpret_cast<volatile uint32_t*>(SYSTICK_VAL);
    volatile uint32_t* load_reg = reinterpret_cast<volatile uint32_t*>(SYSTICK_LOAD);

    uint32_t load = *load_reg;
    uint32_t val1 = *val_reg;
    uint64_t tick1 = system_tick_us;
    uint32_t val2 = *val_reg;

    // If VAL decreased (normal case) or wrapped around, use first reading
    // If VAL increased, interrupt occurred between reads, use second reading
    uint64_t base_us;
    uint32_t counter;

    if (val2 <= val1) {
        // Normal case: counter decreased or stayed same
        base_us = tick1;
        counter = val1;
    } else {
        // Counter wrapped (interrupt occurred)
        base_us = system_tick_us;
        counter = val2;
    }

    // SysTick counts DOWN from LOAD to 0
    // Convert counter ticks to microseconds
    // At 150MHz: each tick = 1/150MHz = 6.67ns
    // LOAD = 150000 for 1ms, so elapsed_ticks * 1000us / LOAD
    uint32_t elapsed_ticks = load - counter;
    uint32_t sub_ms_us = (elapsed_ticks * 1000) / (load + 1);

    return base_us + sub_ms_us;
}

/**
 * @brief Get system uptime in milliseconds
 * @return Milliseconds since board_init()
 */
inline uint32_t millis() {
    // Fast path: just convert microseconds to milliseconds without reading VAL
    return static_cast<uint32_t>(system_tick_us / 1000);
}

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 *
 * Uses SysTick counter for timing
 */
inline void delay_ms(uint32_t ms) {
    uint64_t start = system_tick_us;
    uint64_t delay_us = ms * 1000ULL;
    while ((system_tick_us - start) < delay_us) {
        // Busy wait - don't use WFI for now to debug
    }
}

/**
 * @brief Delay for specified microseconds
 * @param us Microseconds to delay
 *
 * Uses SysTick counter for timing
 */
inline void delay_us(uint32_t us) {
    uint64_t start = system_tick_us;
    while ((system_tick_us - start) < us) {
        // Busy wait
    }
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
