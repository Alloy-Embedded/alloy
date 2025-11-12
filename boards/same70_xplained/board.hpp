#pragma once

/**
 * @file board.hpp
 * @brief SAME70 Xplained Ultra Board API
 * 
 * High-level board abstraction providing:
 * - Pre-configured peripherals (LED, UART, etc.)
 * - Board initialization
 * - Utility functions (delay, etc.)
 */

#include "board_config.hpp"
#include "hal/vendors/atmel/same70/pio_hardware_policy.hpp"
#include "hal/vendors/atmel/same70/systick_hardware_policy.hpp"

namespace board {

using namespace same70_xplained;

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

using LedGpio = alloy::hal::same70::PioCHardware;

/**
 * @brief Initialize LED GPIO
 */
inline void init() {
    LedGpio::enable_pio(1u << LedConfig::led_green_pin);
    LedGpio::enable_output(1u << LedConfig::led_green_pin);
    
    // Start with LED off
    if (LedConfig::led_green_active_high) {
        LedGpio::clear_output(1u << LedConfig::led_green_pin);
    } else {
        LedGpio::set_output(1u << LedConfig::led_green_pin);
    }
}

/**
 * @brief Turn LED on
 */
inline void on() {
    if (LedConfig::led_green_active_high) {
        LedGpio::set_output(1u << LedConfig::led_green_pin);
    } else {
        LedGpio::clear_output(1u << LedConfig::led_green_pin);
    }
}

/**
 * @brief Turn LED off
 */
inline void off() {
    if (LedConfig::led_green_active_high) {
        LedGpio::clear_output(1u << LedConfig::led_green_pin);
    } else {
        LedGpio::set_output(1u << LedConfig::led_green_pin);
    }
}

/**
 * @brief Toggle LED state
 */
inline void toggle() {
    LedGpio::toggle_output(1u << LedConfig::led_green_pin);
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
