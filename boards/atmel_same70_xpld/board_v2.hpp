/**
 * @file board_v2.hpp
 * @brief ATSAME70 Xplained Board Configuration (Template-Based HAL)
 *
 * Modern C++20 board support using template-based HAL with zero overhead.
 * This file provides board-specific peripheral mappings using the new
 * platform abstraction layer.
 *
 * Board: ATSAME70-XPLD
 * MCU:   ATSAME70Q21B (Cortex-M7 @ 300MHz)
 *
 * Features:
 *   - 2MB Flash, 384KB SRAM
 *   - FPU (double precision)
 *   - I-Cache and D-Cache
 *   - DSP instructions
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @see openspec/changes/platform-abstraction/
 */

#ifndef ALLOY_BOARD_ATSAME70_XPLD_V2_HPP
#define ALLOY_BOARD_ATSAME70_XPLD_V2_HPP

#include <stdint.h>
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/uart.hpp"
#include "core/result.hpp"

namespace board {

// Import platform HAL
using namespace alloy::hal::same70;
using namespace alloy::core;

// ==============================================================================
// Board Information
// ==============================================================================

inline constexpr const char* name = "ATSAME70 Xplained Ultra";
inline constexpr const char* mcu = "ATSAME70Q21B";
inline constexpr const char* vendor = "Microchip (Atmel)";

// ==============================================================================
// Clock Configuration
// ==============================================================================

inline constexpr uint32_t xtal_frequency_hz = 12'000'000;    // 12 MHz crystal
inline constexpr uint32_t system_clock_hz = 300'000'000;     // 300 MHz (max)
inline constexpr uint32_t peripheral_clock_hz = 150'000'000; // 150 MHz

// ==============================================================================
// GPIO Pin Mappings (Board-Specific)
// ==============================================================================

// LEDs (active LOW on SAME70 Xplained)
using led_green = Led0;  // PC8 - User LED (green)
using led_blue = Led1;   // PC9 - Optional second LED

// Buttons
using button_user = Button0;  // PA11 - SW0 button (active LOW)

// Debug UART (connected to EDBG)
// UART0: RX=PA9, TX=PA10

// Extension headers provide access to other pins
// EXT1, EXT2 connectors with SPI, I2C, UART, GPIO

// ==============================================================================
// Peripheral Mappings (Type Aliases)
// ==============================================================================

// UARTs - map to board-specific instances
using uart_debug = Uart0;   // UART0 - Connected to EDBG (USB debug)
using uart_ext1 = Uart1;    // UART1 - Available on EXT1 connector
using uart_ext2 = Uart2;    // UART2 - Available on EXT2 connector

// Future peripherals (to be implemented)
// using spi_ext1 = Spi0;
// using i2c_ext1 = I2c0;
// using can_bus = Can0;

// ==============================================================================
// Board Initialization
// ==============================================================================

/**
 * @brief Initialize board peripherals and clocks
 *
 * This function must be called before using any board peripherals.
 * It enables clocks for all GPIO ports and configures essential settings.
 *
 * @return Result<void> Ok() if successful
 */
inline Result<void> initialize() {
    // PMC (Power Management Controller) - enable peripheral clocks
    volatile uint32_t* pmc_pcer0 = reinterpret_cast<volatile uint32_t*>(0x400E0610);

    // Enable clocks for all GPIO ports (PIOA-PIOE)
    constexpr uint32_t ID_PIOA = 10;
    constexpr uint32_t ID_PIOB = 11;
    constexpr uint32_t ID_PIOC = 12;
    constexpr uint32_t ID_PIOD = 16;
    constexpr uint32_t ID_PIOE = 17;

    *pmc_pcer0 = (1U << ID_PIOA) | (1U << ID_PIOB) | (1U << ID_PIOC) |
                 (1U << ID_PIOD) | (1U << ID_PIOE);

    // Small delay to let clocks stabilize
    for (volatile int i = 0; i < 10000; i++) {
        __asm__ volatile("nop");
    }

    // TODO: Configure system clock to 300MHz using PLL
    // TODO: Configure flash wait states for 300MHz operation
    // TODO: Enable I-Cache and D-Cache

    return Result<void>::ok();
}

// ==============================================================================
// LED Control Functions
// ==============================================================================

namespace led {
    /**
     * @brief Initialize LED GPIO
     *
     * Configures LED pins as outputs and turns them off.
     *
     * @return Result<void> Ok() if successful
     */
    inline Result<void> init() {
        auto green = led_green{};
        auto result = green.setMode(GpioMode::Output);
        if (result.is_error()) {
            return result;
        }

        // Turn off LED (active LOW, so set HIGH)
        return green.set();
    }

    /**
     * @brief Turn green LED on
     *
     * @return Result<void> Ok() if successful
     */
    inline Result<void> on() {
        auto green = led_green{};
        return green.clear();  // Active LOW
    }

    /**
     * @brief Turn green LED off
     *
     * @return Result<void> Ok() if successful
     */
    inline Result<void> off() {
        auto green = led_green{};
        return green.set();  // Active LOW
    }

    /**
     * @brief Toggle green LED
     *
     * @return Result<void> Ok() if successful
     */
    inline Result<void> toggle() {
        auto green = led_green{};
        return green.toggle();
    }
}

// ==============================================================================
// Button Control Functions
// ==============================================================================

namespace button {
    /**
     * @brief Initialize button GPIO
     *
     * Configures button pin as input with pull-up and glitch filter.
     *
     * @return Result<void> Ok() if successful
     */
    inline Result<void> init() {
        auto btn = button_user{};

        auto result = btn.setMode(GpioMode::Input);
        if (result.is_error()) {
            return result;
        }

        result = btn.setPull(GpioPull::Up);
        if (result.is_error()) {
            return result;
        }

        return btn.enableFilter();
    }

    /**
     * @brief Check if user button is pressed
     *
     * @return Result<bool> true if pressed, false if released
     */
    inline Result<bool> is_pressed() {
        auto btn = button_user{};
        auto result = btn.read();

        if (result.is_error()) {
            return result;
        }

        // Button is active LOW
        return Result<bool>::ok(!result.value());
    }
}

// ==============================================================================
// Delay Functions (Busy-Wait)
// ==============================================================================

/**
 * @brief Simple delay (busy wait)
 *
 * @note Not accurate - for basic use only
 * @param ms Approximate milliseconds to delay
 */
inline void delay_ms(uint32_t ms) {
    // MCU starts with 12MHz RC oscillator (not 300MHz PLL!)
    // At 12MHz, approximately 12,000 cycles per millisecond
    // Rough approximation: 3 cycles per loop iteration
    // So: 12,000 / 3 = 4000 iterations per ms
    volatile uint32_t count = ms * 4000;
    while (count > 0) {
        count--;
        __asm__ volatile("nop");
    }
}

/**
 * @brief Microsecond delay (busy wait)
 *
 * @note Not accurate - for basic use only
 * @param us Approximate microseconds to delay
 */
inline void delay_us(uint32_t us) {
    // At 12MHz, approximately 12 cycles per microsecond
    // Rough approximation: 3 cycles per loop iteration
    // So: 12 / 3 = 4 iterations per us
    volatile uint32_t count = us * 4;
    while (count > 0) {
        count--;
        __asm__ volatile("nop");
    }
}

// ==============================================================================
// Board Information Functions
// ==============================================================================

namespace info {
    inline constexpr const char* get_name() { return name; }
    inline constexpr const char* get_mcu() { return mcu; }
    inline constexpr const char* get_vendor() { return vendor; }
    inline constexpr uint32_t get_clock_hz() { return system_clock_hz; }
    inline constexpr uint32_t get_xtal_hz() { return xtal_frequency_hz; }
}

} // namespace board

#endif // ALLOY_BOARD_ATSAME70_XPLD_V2_HPP
