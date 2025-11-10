/**
 * @file board_config.hpp
 * @brief SAME70 Xplained Ultra Board Configuration
 *
 * This file contains the board-specific implementation of the standard
 * board interface for the SAME70 Xplained Ultra evaluation board.
 *
 * Hardware Specifications:
 * - MCU: ATSAME70Q21B (ARM Cortex-M7 @ 300MHz max)
 * - Flash: 2MB
 * - RAM: 384KB
 * - LEDs: 2x user LEDs (LED0: PC8, LED1: PC9) - active-low
 * - Buttons: 2x user buttons (SW0: PA11, SW1: PC2) - active-low
 * - USB: Full-speed USB device + host
 * - Ethernet: 10/100 Mbps
 * - Debug: EDBG with virtual COM port
 *
 * @note Part of Alloy Framework Board Support
 */

#pragma once

#include "../common/board_interface.hpp"

// Platform-specific HAL implementations
#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/systick.hpp"
#include "hal/platform/same70/systick_delay.hpp"

// Vendor-specific register definitions
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::boards::same70_xplained {

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::same70;

// ============================================================================
// Board Identification
// ============================================================================

namespace board {

inline constexpr const char* name() {
    return "SAME70 Xplained Ultra";
}

inline constexpr const char* mcu() {
    return "ATSAME70Q21B";
}

inline constexpr uint32_t clock_frequency_hz() {
    return 300'000'000;  // 300 MHz when using default Clock300MHz preset
}

// ============================================================================
// Clock Configuration Presets
// ============================================================================

/**
 * @brief Clock configuration presets for SAME70
 *
 * Choose based on your application needs:
 * - Clock300MHz: Maximum performance (300 MHz core, 150 MHz bus)
 * - Clock150MHz: Balanced performance/power (150 MHz core, 75 MHz bus)
 * - Clock120MHz: USB-compatible (120 MHz core, 60 MHz bus)
 * - Clock48MHz:  Low power operation (48 MHz core, 24 MHz bus)
 * - Clock12MHz:  Ultra-low power (12 MHz RC oscillator)
 */
enum class ClockPreset {
    Clock300MHz,  ///< 300 MHz - Maximum performance
    Clock150MHz,  ///< 150 MHz - Balanced
    Clock120MHz,  ///< 120 MHz - USB compatible
    Clock48MHz,   ///< 48 MHz  - Low power
    Clock12MHz    ///< 12 MHz  - Ultra-low power (RC oscillator)
};

// ============================================================================
// Internal Definitions
// ============================================================================

namespace detail {
    using namespace alloy::generated::atsame70q21b;

    // LED pins (active-low: clear()=ON, set()=OFF)
    using Led0Pin = GpioPin<PIOC_BASE, 8>;
    using Led1Pin = GpioPin<PIOC_BASE, 9>;

    // Button pins (active-low with internal pull-up)
    using Button0Pin = GpioPin<PIOA_BASE, 11>;
    using Button1Pin = GpioPin<PIOC_BASE, 2>;

    // Global instances (initialized in init())
    inline Led0Pin led0_instance;
    inline Led1Pin led1_instance;
    inline Button0Pin button0_instance;
    inline Button1Pin button1_instance;

    // Initialization flag (defined in board_config.cpp)
    extern bool initialized;
}

// ============================================================================
// Board Initialization
// ============================================================================

/**
 * @brief Initialize the SAME70 Xplained Ultra board
 *
 * This function:
 * 1. Configures system clock based on preset
 * 2. Initializes SysTick timer for delays
 * 3. Enables peripheral clocks for GPIO ports
 * 4. Configures LEDs as outputs (OFF by default)
 * 5. Configures buttons as inputs with pull-ups
 *
 * @param preset Clock configuration preset (default: 300MHz)
 *
 * After calling this function, you can use:
 * - board::delay_ms() / board::delay_us() - Precise delays
 * - board::micros() - Microsecond timestamp
 * - board::led::on/off/toggle() - LED control
 * - board::button::read() - Button input
 *
 * Example:
 * @code
 * board::init();                           // Use default 300MHz
 * board::init(ClockPreset::Clock12MHz);    // Use 12MHz RC oscillator
 *
 * while (true) {
 *     board::led::toggle();
 *     board::delay_ms(500);  // Precise 500ms delay
 * }
 * @endcode
 *
 * @note Implementation is in board_config.cpp
 */
void init(ClockPreset preset = ClockPreset::Clock300MHz);

// ============================================================================
// Delay Functions
// ============================================================================

inline void delay_ms(uint32_t ms) {
    alloy::hal::same70::delay_ms(ms);
}

inline void delay_us(uint32_t us) {
    alloy::hal::same70::delay_us(us);
}

/**
 * @brief Get microsecond timestamp since boot
 * @return Microseconds elapsed since system initialization
 */
inline uint32_t micros() {
    return alloy::hal::same70::SystemTick::micros();
}

// ============================================================================
// LED Interface
// ============================================================================

namespace led {
    /**
     * @brief Turn LED0 on
     * @note LED0 is active-low, so we clear the pin
     */
    inline void on() {
        detail::led0_instance.clear();
    }

    /**
     * @brief Turn LED0 off
     * @note LED0 is active-low, so we set the pin
     */
    inline void off() {
        detail::led0_instance.set();
    }

    /**
     * @brief Toggle LED0 state
     */
    inline void toggle() {
        detail::led0_instance.toggle();
    }

    /**
     * @brief Set LED0 to specific state
     * @param state true = ON, false = OFF
     */
    inline void set(bool state) {
        if (state) {
            on();
        } else {
            off();
        }
    }

    /**
     * @namespace led1
     * @brief Second LED control (LED1 / PC9)
     */
    namespace led1 {
        inline void on() { detail::led1_instance.clear(); }
        inline void off() { detail::led1_instance.set(); }
        inline void toggle() { detail::led1_instance.toggle(); }
        inline void set(bool state) {
            if (state) {
                on();
            } else {
                off();
            }
        }
    }
}

// ============================================================================
// Button Interface
// ============================================================================

namespace button {
    /**
     * @brief Read button SW0 state
     * @return true if button is pressed, false otherwise
     * @note Button is active-low, so we invert the reading
     */
    inline bool read() {
        return !detail::button0_instance.read();  // active-low
    }

    /**
     * @brief Wait for button SW0 press
     * @note Blocking function with debouncing
     */
    inline void wait_press() {
        while (!read()) {}  // Wait for press
        delay_ms(20);       // Debounce
    }

    /**
     * @brief Wait for button SW0 release
     * @note Blocking function with debouncing
     */
    inline void wait_release() {
        while (read()) {}   // Wait for release
        delay_ms(20);       // Debounce
    }

    /**
     * @namespace button1
     * @brief Second button control (SW1 / PC2)
     */
    namespace button1 {
        inline bool read() {
            return !detail::button1_instance.read();  // active-low
        }

        inline void wait_press() {
            while (!read()) {}
            delay_ms(20);
        }

        inline void wait_release() {
            while (read()) {}
            delay_ms(20);
        }
    }
}

} // namespace board

// Define feature flags
#define BOARD_HAS_LED
#define BOARD_HAS_BUTTON

} // namespace alloy::boards::same70_xplained

// Note: SysTick_Handler is implemented in board_config.cpp
