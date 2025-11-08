/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for STM32F1 (Platform Layer)
 *
 * This file implements GPIO peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead. It automatically includes all
 * necessary vendor-specific headers.
 *
 * Design Principles:
 * - Template-based: Port address and pin number resolved at compile-time
 * - Zero overhead: Fully inlined, single instruction for read/write
 * - Compile-time masks: Pin masks computed at compile-time
 * - Type-safe: Strong typing prevents pin conflicts
 * - Error handling: Uses Result<T> for robust error handling
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: stm32f103re
 * Generator: generate_platform_gpio.py
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "hal/types.hpp"

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/st/stm32f1/registers/gpioa_registers.hpp"

// Bitfields (family-level, if available)
// #include "hal/vendors/st/stm32f1/bitfields/gpioa_bitfields.hpp"

// Hardware definitions (MCU-specific - if available)
// Note: Board files should include hardware.hpp from specific MCU if needed
// #include "hal/vendors/st/stm32f1/{mcu}/hardware.hpp"

// Pin definitions (MCU-specific - if available)
// Note: These should be included by board files as they're MCU-specific
// Example: #include "hal/vendors/st/stm32f1/stm32f407vg/pins.hpp"

namespace alloy::hal::stm32f1 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace alloy::hal::st::stm32f1;

/**
 * @brief GPIO pin modes
 */
enum class GpioMode {
    Input,            ///< Input mode
    Output,           ///< Output mode (push-pull)
    OutputOpenDrain,  ///< Output mode with open-drain
    Alternate,        ///< Alternate function mode
    Analog            ///< Analog mode
};

/**
 * @brief GPIO pull resistor configuration
 */
enum class GpioPull {
    None,  ///< No pull resistor (floating)
    Up,    ///< Pull-up resistor enabled
    Down   ///< Pull-down resistor enabled
};

/**
 * @brief GPIO output speed
 */
enum class GpioSpeed {
    Low,      ///< Low speed
    Medium,   ///< Medium speed
    High,     ///< High speed (fast)
    VeryHigh  ///< Very high speed
};

/**
 * @brief Template-based GPIO pin for STM32F1
 *
 * This class provides a template-based GPIO implementation with ZERO runtime
 * overhead. All pin masks and operations are resolved at compile-time.
 *
 * Template Parameters:
 * - PORT_BASE: GPIO port base address (compile-time constant)
 * - PIN_NUM: Pin number within port (0-15)
 *
 * Example usage:
 * @code
 * // Define LED pin (GPIOC pin 13)
 * using LedGreen = GpioPin<GPIOC_BASE, 13>;
 *
 * // Use it
 * auto led = LedGreen{};
 * auto result = led.setMode(GpioMode::Output);
 * if (result.isOk()) {
 *     led.set();     // Turn on
 *     led.clear();   // Turn off
 *     led.toggle();  // Toggle state
 * }
 * @endcode
 *
 * @tparam PORT_BASE GPIO port base address
 * @tparam PIN_NUM Pin number (0-15)
 */
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
   public:
    // Compile-time constants
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);
    static constexpr uint8_t pin_pos = PIN_NUM;

    // Validate pin number at compile-time
    static_assert(PIN_NUM < 16, "STM32 GPIO pin number must be 0-15");

    /**
     * @brief Get GPIO port registers
     *
     * Returns pointer to GPIO registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile gpioa::GPIOA_Registers* get_port() {
#ifdef ALLOY_GPIO_MOCK_PORT
        // In tests, use the mock port pointer
        return ALLOY_GPIO_MOCK_PORT();
#else
        return reinterpret_cast<volatile gpioa::GPIOA_Registers*>(PORT_BASE);
#endif
    }

    /**
     * @brief Set GPIO pin mode
     *
     * Configures pin as input, output, alternate function, or analog.
     *
     * @param mode Desired pin mode
     * @return Result<void> Ok() if successful
     */
    Result<void> setMode(GpioMode mode) {
        auto* port = get_port();

        // Clear mode bits for this pin (2 bits per pin)
        uint32_t moder = port->MODER;
        moder &= ~(0x3u << (pin_pos * 2));

        // Set new mode
        switch (mode) {
            case GpioMode::Input:
                // 00: Input mode
                break;

            case GpioMode::Output:
                // 01: General purpose output mode
                moder |= (0x1u << (pin_pos * 2));
                // Set to push-pull by default
                port->OTYPER &= ~pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
                ALLOY_GPIO_TEST_HOOK_OTYPER();
#endif
                break;

            case GpioMode::OutputOpenDrain:
                // 01: General purpose output mode
                moder |= (0x1u << (pin_pos * 2));
                // Set to open-drain
                port->OTYPER |= pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
                ALLOY_GPIO_TEST_HOOK_OTYPER();
#endif
                break;

            case GpioMode::Alternate:
                // 10: Alternate function mode
                moder |= (0x2u << (pin_pos * 2));
                break;

            case GpioMode::Analog:
                // 11: Analog mode
                moder |= (0x3u << (pin_pos * 2));
                break;
        }

        port->MODER = moder;
#ifdef ALLOY_GPIO_TEST_HOOK_MODER
        ALLOY_GPIO_TEST_HOOK_MODER();
#endif

        return Result<void>::ok();
    }

    /**
     * @brief Set pin HIGH (output = 1)
     *
     * Single instruction: writes pin mask to BSRR register.
     * Only affects this specific pin atomically.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> set() {
        auto* port = get_port();
        port->BSRR = pin_mask;  // Set bit (lower 16 bits)
#ifdef ALLOY_GPIO_TEST_HOOK_BSRR
        ALLOY_GPIO_TEST_HOOK_BSRR();
#endif
        return Result<void>::ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     *
     * Single instruction: writes pin mask to BSRR register (upper 16 bits).
     * Only affects this specific pin atomically.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> clear() {
        auto* port = get_port();
        port->BSRR = (pin_mask << 16);  // Reset bit (upper 16 bits)
#ifdef ALLOY_GPIO_TEST_HOOK_BSRR
        ALLOY_GPIO_TEST_HOOK_BSRR();
#endif
        return Result<void>::ok();
    }

    /**
     * @brief Toggle pin state
     *
     * Reads current output state and inverts it.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> toggle() {
        auto* port = get_port();

        // Read current output data register
        if (port->ODR & pin_mask) {
            // Pin is HIGH, set it LOW
            port->BSRR = (pin_mask << 16);
        } else {
            // Pin is LOW, set it HIGH
            port->BSRR = pin_mask;
        }

        return Result<void>::ok();
    }

    /**
     * @brief Write pin value
     *
     * @param value true for HIGH, false for LOW
     * @return Result<void> Always Ok()
     */
    Result<void> write(bool value) {
        if (value) {
            return set();
        } else {
            return clear();
        }
    }

    /**
     * @brief Read pin input value
     *
     * Reads the actual pin state from IDR register.
     *
     * @return Result<bool> Pin state (true = HIGH, false = LOW)
     */
    Result<bool> read() const {
        auto* port = get_port();
        bool value = (port->IDR & pin_mask) != 0;
        return Result<bool>::ok(value);
    }

    /**
     * @brief Configure pull resistor
     *
     * STM32 GPIO supports both pull-up and pull-down.
     *
     * @param pull Pull resistor configuration
     * @return Result<void> Ok() if successful
     */
    Result<void> setPull(GpioPull pull) {
        auto* port = get_port();

        // Clear pull bits for this pin (2 bits per pin)
        uint32_t pupdr = port->PUPDR;
        pupdr &= ~(0x3u << (pin_pos * 2));

        switch (pull) {
            case GpioPull::None:
                // 00: No pull-up, pull-down
                break;

            case GpioPull::Up:
                // 01: Pull-up
                pupdr |= (0x1u << (pin_pos * 2));
                break;

            case GpioPull::Down:
                // 10: Pull-down
                pupdr |= (0x2u << (pin_pos * 2));
                break;
        }

        port->PUPDR = pupdr;
#ifdef ALLOY_GPIO_TEST_HOOK_PUPDR
        ALLOY_GPIO_TEST_HOOK_PUPDR();
#endif

        return Result<void>::ok();
    }

    /**
     * @brief Set output speed
     *
     * @param speed Desired output speed
     * @return Result<void> Always Ok()
     */
    Result<void> setSpeed(GpioSpeed speed) {
        auto* port = get_port();

        // Clear speed bits for this pin (2 bits per pin)
        uint32_t ospeedr = port->OSPEEDR;
        ospeedr &= ~(0x3u << (pin_pos * 2));

        uint8_t speed_val = static_cast<uint8_t>(speed);
        ospeedr |= (speed_val << (pin_pos * 2));

        port->OSPEEDR = ospeedr;
#ifdef ALLOY_GPIO_TEST_HOOK_OSPEEDR
        ALLOY_GPIO_TEST_HOOK_OSPEEDR();
#endif

        return Result<void>::ok();
    }

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result<bool> true if output, false if input
     */
    Result<bool> isOutput() const {
        auto* port = get_port();
        uint32_t mode = (port->MODER >> (pin_pos * 2)) & 0x3;
        // Mode 01 = output
        bool is_output = (mode == 0x1);
        return Result<bool>::ok(is_output);
    }
};

// ==============================================================================
// Port Base Address Constants
// ==============================================================================

constexpr uint32_t GPIOA_BASE = 0x40010800;

// ==============================================================================
// Common Pin Type Aliases
// ==============================================================================

// Board-specific pin definitions should be in board.hpp
// Example:
// using LedGreen = GpioPin<GPIOC_BASE, 13>;
// using Button0 = GpioPin<GPIOA_BASE, 0>;

}  // namespace alloy::hal::stm32f1
