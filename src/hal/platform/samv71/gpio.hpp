/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for SAMV71 (Platform Layer)
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
 * Auto-generated from: atsamv71n19b
 * Generator: generate_platform_gpio.py
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/atmel/samv71/registers/pioa_registers.hpp"

// Bitfields (family-level, if available)
// #include "hal/vendors/atmel/samv71/bitfields/pioa_bitfields.hpp"

// Hardware definitions (MCU-specific - port bases, etc)
// Note: Board files should include hardware.hpp from specific MCU if needed
// #include "hal/vendors/atmel/samv71/atsamv71n19b/hardware.hpp"

// Pin definitions and functions (MCU-specific)
// Note: These should be included by board files as they're MCU-specific
// Example: #include "hal/vendors/atmel/samv71/stm32f407vg/pins.hpp"

namespace alloy::hal::samv71 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace alloy::hal::atmel::samv71;

/**
 * @brief GPIO pin modes
 */
enum class GpioMode {
    Input,           ///< Input mode
    Output,          ///< Output mode (push-pull)
    OutputOpenDrain  ///< Output mode with open-drain
};

/**
 * @brief GPIO pull resistor configuration
 */
enum class GpioPull {
    None,      ///< No pull resistor
    Up,        ///< Pull-up resistor enabled
    Down       ///< Pull-down resistor enabled (if supported)
};

/**
 * @brief Template-based GPIO pin for SAMV71
 *
 * This class provides a template-based GPIO implementation with ZERO runtime
 * overhead. All pin masks and operations are resolved at compile-time.
 *
 * Template Parameters:
 * - PORT_BASE: PIO port base address (compile-time constant)
 * - PIN_NUM: Pin number within port (0-31)
 *
 * Example usage:
 * @code
 * // Define LED pin (PIOC pin 8)
 * using LedGreen = GpioPin<PIOC_BASE, 8>;
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
 * @tparam PORT_BASE PIO port base address
 * @tparam PIN_NUM Pin number (0-31)
 */
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Validate pin number at compile-time
    static_assert(PIN_NUM < 32, "Pin number must be 0-31");

    /**
     * @brief Get PIO port registers
     *
     * Returns pointer to PIO registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile pioa::PIOA_Registers* get_port() {
#ifdef ALLOY_GPIO_MOCK_PORT
        // In tests, use the mock port pointer
        return ALLOY_GPIO_MOCK_PORT();
#else
        return reinterpret_cast<volatile pioa::PIOA_Registers*>(PORT_BASE);
#endif
    }

    /**
     * @brief Set GPIO pin mode
     *
     * Configures pin as input or output with optional open-drain.
     *
     * @param mode Desired pin mode
     * @return Result<void> Ok() if successful
     */
    Result<void> setMode(GpioMode mode) {
        auto* port = get_port();

        // Enable PIO control (disable peripheral function)
        port->PER = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_PER
        ALLOY_GPIO_TEST_HOOK_PER();
#endif

        switch (mode) {
            case GpioMode::Input:
                // Configure as input
                port->ODR = pin_mask;  // Disable output
#ifdef ALLOY_GPIO_TEST_HOOK_ODR
                ALLOY_GPIO_TEST_HOOK_ODR();
#endif
                break;

            case GpioMode::Output:
                // Configure as output (push-pull)
                port->OER = pin_mask;   // Enable output
#ifdef ALLOY_GPIO_TEST_HOOK_OER
                ALLOY_GPIO_TEST_HOOK_OER();
#endif
                port->MDDR = pin_mask;  // Disable multi-driver (open-drain)
#ifdef ALLOY_GPIO_TEST_HOOK_MDDR
                ALLOY_GPIO_TEST_HOOK_MDDR();
#endif
                break;

            case GpioMode::OutputOpenDrain:
                // Configure as output with open-drain
                port->OER = pin_mask;   // Enable output
#ifdef ALLOY_GPIO_TEST_HOOK_OER
                ALLOY_GPIO_TEST_HOOK_OER();
#endif
                port->MDER = pin_mask;  // Enable multi-driver (open-drain)
#ifdef ALLOY_GPIO_TEST_HOOK_MDER
                ALLOY_GPIO_TEST_HOOK_MDER();
#endif
                break;
        }

        return Result<void>::ok();
    }

    /**
     * @brief Set pin HIGH (output = 1)
     *
     * Single instruction: writes pin mask to SODR register.
     * Only affects this specific pin.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> set() {
        auto* port = get_port();
        port->SODR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_SODR
        ALLOY_GPIO_TEST_HOOK_SODR();
#endif
        return Result<void>::ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     *
     * Single instruction: writes pin mask to CODR register.
     * Only affects this specific pin.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> clear() {
        auto* port = get_port();
        port->CODR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_CODR
        ALLOY_GPIO_TEST_HOOK_CODR();
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

        // Read current output data status
        if (port->ODSR & pin_mask) {
            // Pin is HIGH, set it LOW
            port->CODR = pin_mask;
        } else {
            // Pin is LOW, set it HIGH
            port->SODR = pin_mask;
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
     * Reads the actual pin state from PDSR register.
     *
     * @return Result<bool> Pin state (true = HIGH, false = LOW)
     */
    Result<bool> read() const {
        auto* port = get_port();
        bool value = (port->PDSR & pin_mask) != 0;
        return Result<bool>::ok(value);
    }

    /**
     * @brief Configure pull resistor
     *
     * Note: SAMV71 PIO has built-in pull-up resistors.
     * Pull-down support depends on hardware.
     *
     * @param pull Pull resistor configuration
     * @return Result<void> Ok() if successful, Err() if not supported
     */
    Result<void> setPull(GpioPull pull) {
        auto* port = get_port();

        switch (pull) {
            case GpioPull::None:
                // Disable pull-up
                port->PUDR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_PUDR
                ALLOY_GPIO_TEST_HOOK_PUDR();
#endif
                break;

            case GpioPull::Up:
                // Enable pull-up
                port->PUER = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_PUER
                ALLOY_GPIO_TEST_HOOK_PUER();
#endif
                break;

            case GpioPull::Down:
                // Pull-down not supported in SAMV71 hardware
                return Result<void>::error(ErrorCode::NotSupported);
        }

        return Result<void>::ok();
    }

    /**
     * @brief Enable input glitch filter
     *
     * @return Result<void> Always Ok()
     */
    Result<void> enableFilter() {
        auto* port = get_port();
        port->IFER = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_IFER
        ALLOY_GPIO_TEST_HOOK_IFER();
#endif
        return Result<void>::ok();
    }

    /**
     * @brief Disable input glitch filter
     *
     * @return Result<void> Always Ok()
     */
    Result<void> disableFilter() {
        auto* port = get_port();
        port->IFDR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_IFDR
        ALLOY_GPIO_TEST_HOOK_IFDR();
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
        bool is_output = (port->OSR & pin_mask) != 0;
        return Result<bool>::ok(is_output);
    }
};

// ==============================================================================
// Port Base Address Constants
// ==============================================================================

constexpr uint32_t PIOA_BASE = 0x400E0E00;

// ==============================================================================
// Common Pin Type Aliases
// ==============================================================================

// Board-specific pin definitions should be in board.hpp
// Example:
// using LedGreen = GpioPin<PIOC_BASE, 8>;
// using Button0 = GpioPin<PIOA_BASE, 11>;

} // namespace alloy::hal::samv71
