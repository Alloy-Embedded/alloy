/**
 * @file board_interface.hpp
 * @brief Standard Board Interface for Alloy Framework
 *
 * This file defines the standard interface that all board support packages
 * must implement. This ensures portability and consistency across different
 * hardware platforms.
 *
 * ## Interface Requirements
 *
 * All boards MUST implement:
 * - `board::init()` - Initialize clock, peripherals, GPIO
 * - `board::delay_ms()` / `board::delay_us()` - Delay functions
 * - `board::name()`, `board::mcu()`, `board::clock_frequency_hz()` - Board info
 *
 * All boards SHOULD implement (if hardware supports):
 * - `board::led::on/off/toggle()` - LED control
 * - `board::button::read()` - Button reading
 * - `board::uart::write()` - UART output
 *
 * ## Usage in Examples
 *
 * Examples should include the board header via CMake macro:
 *
 * ```cpp
 * #include BOARD_HEADER
 *
 * int main() {
 *     board::init();  // Initialize board
 *
 *     while (true) {
 *         board::led::toggle();
 *         board::delay_ms(500);
 *     }
 * }
 * ```
 *
 * ## Adding a New Board
 *
 * To add a new board:
 * 1. Create `boards/<board_name>/board.hpp`
 * 2. Create `boards/<board_name>/board_config.hpp`
 * 3. Include this file in your `board.hpp`
 * 4. Implement all required functions in `board_config.hpp`
 * 5. Add CMake configuration for the board
 * 6. Create README.md documenting the board
 *
 * @see docs/BOARD_ABSTRACTION_DESIGN.md for complete design
 *
 * @note Part of Alloy Framework Board Support
 * @author Alloy Framework Team
 */

#pragma once

#include "core/types.hpp"
#include <cstdint>

/**
 * @namespace board
 * @brief Standard board interface namespace
 *
 * All board implementations provide functions under this namespace.
 * This allows portable code that works across different hardware.
 *
 * Note: This is documentation only. Actual namespace is defined by each board.
 */
// namespace board {  // Defined by each board implementation

// ============================================================================
// REQUIRED - All boards must implement these
// ============================================================================

/**
 * @brief Initialize the board
 *
 * This function must:
 * - Configure system clock
 * - Initialize SysTick or other timers for delays
 * - Enable peripheral clocks as needed
 * - Configure GPIO for LEDs, buttons, etc
 *
 * Must be called before using any other board functions.
 *
 * @note Implementation may accept optional configuration parameters
 */
// void init();  // Implemented by board_config.hpp

/**
 * @brief Delay for specified milliseconds
 *
 * Blocking delay using SysTick or other timer.
 * Accurate to +/- 1ms depending on implementation.
 *
 * @param ms Milliseconds to delay
 */
// void delay_ms(uint32_t ms);  // Implemented by board_config.hpp

/**
 * @brief Delay for specified microseconds
 *
 * Blocking delay using SysTick or other timer.
 * Accuracy depends on timer resolution (typically +/- 1us).
 *
 * @param us Microseconds to delay
 */
// void delay_us(uint32_t us);  // Implemented by board_config.hpp

/**
 * @brief Get board name
 * @return Human-readable board name (e.g., "SAME70 Xplained Ultra")
 */
// constexpr const char* name();  // Implemented by board_config.hpp

/**
 * @brief Get MCU name
 * @return MCU part number (e.g., "ATSAME70Q21B")
 */
// constexpr const char* mcu();  // Implemented by board_config.hpp

/**
 * @brief Get system clock frequency
 * @return Clock frequency in Hz
 */
// constexpr uint32_t clock_frequency_hz();  // Implemented by board_config.hpp

// ============================================================================
// OPTIONAL - Implement if hardware supports
// ============================================================================

/**
 * @namespace led
 * @brief LED control functions
 *
 * If the board has LEDs, implement these functions.
 * For boards with multiple LEDs, provide led1, led2, etc. namespaces.
 */
namespace led {
    /**
     * @brief Turn LED on
     * @note Handles active-high or active-low automatically
     */
    // void on();  // Implemented by board_config.hpp if available

    /**
     * @brief Turn LED off
     * @note Handles active-high or active-low automatically
     */
    // void off();  // Implemented by board_config.hpp if available

    /**
     * @brief Toggle LED state
     */
    // void toggle();  // Implemented by board_config.hpp if available

    /**
     * @brief Set LED to specific state
     * @param state true = ON, false = OFF
     */
    // void set(bool state);  // Optional
}

/**
 * @namespace button
 * @brief Button reading functions
 *
 * If the board has buttons, implement these functions.
 * For boards with multiple buttons, provide button1, button2, etc. namespaces.
 */
namespace button {
    /**
     * @brief Read button state
     * @return true if button is pressed, false otherwise
     * @note Handles active-high or active-low, pull-ups, etc. automatically
     */
    // bool read();  // Implemented by board_config.hpp if available

    /**
     * @brief Wait for button press
     * @note Blocking function
     */
    // void wait_press();  // Optional

    /**
     * @brief Wait for button release
     * @note Blocking function
     */
    // void wait_release();  // Optional
}

/**
 * @namespace uart
 * @brief UART communication functions
 *
 * If the board has a default UART (e.g., console, debug), implement these.
 * For boards with multiple UARTs, provide uart1, uart2, etc. namespaces.
 */
namespace uart {
    /**
     * @brief Write string to UART
     * @param data Null-terminated string
     * @note Blocking function
     */
    // void write(const char* data);  // Implemented by board_config.hpp if available

    /**
     * @brief Write single character
     * @param c Character to write
     */
    // void write_char(char c);  // Optional

    /**
     * @brief Read single character
     * @return Character read from UART
     * @note Blocking function
     */
    // char read_char();  // Optional
}

// ============================================================================
// ADVANCED - Optional features
// ============================================================================

/**
 * @namespace adc
 * @brief ADC reading functions
 */
namespace adc {
    /**
     * @brief Read ADC channel
     * @param channel Channel number (0-based)
     * @return Raw ADC value
     */
    // uint16_t read(uint8_t channel);  // Optional
}

/**
 * @namespace pwm
 * @brief PWM output functions
 */
namespace pwm {
    /**
     * @brief Set PWM duty cycle
     * @param channel Channel number
     * @param duty_cycle Duty cycle (0-100%)
     */
    // void set_duty_cycle(uint8_t channel, uint8_t duty_cycle);  // Optional
}

// } // namespace board  // Commented out - defined by each board

/**
 * @def BOARD_HAS_LED
 * @brief Define if board has LED support
 *
 * Board implementations should define this if they implement led functions.
 */
// #define BOARD_HAS_LED

/**
 * @def BOARD_HAS_BUTTON
 * @brief Define if board has button support
 */
// #define BOARD_HAS_BUTTON

/**
 * @def BOARD_HAS_UART
 * @brief Define if board has UART support
 */
// #define BOARD_HAS_UART
