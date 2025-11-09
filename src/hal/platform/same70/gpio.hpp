/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for SAME70 (Platform Layer)
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
 * - Error handling: Uses Result<T, E> for robust error handling
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: same70
 * Generator: generate_platform_gpio.py
 * Generated: 2025-11-08 20:54:51
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
#include "hal/vendors/atmel/same70/registers/pioa_registers.hpp"

// Bitfields (family-level, if available)
// #include "hal/vendors/atmel/same70/bitfields/pioa_bitfields.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace alloy::hal::atmel::same70;

// Note: GPIO configuration uses common HAL types from hal/types.hpp:
// - PinDirection (Input, Output)
// - PinPull (None, PullUp, PullDown)
// - PinDrive (PushPull, OpenDrain)


/**
 * @brief Template-based GPIO pin for SAME70
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
 * // Define button pin (PIOA pin 11)
 * using Button0 = GpioPin<PIOA_BASE, 11>;
 * // Use GPIO pin
 * auto led = LedGreen{};
auto result = led.setDirection(PinDirection::Output);
if (result.is_ok()) {
    led.set();     // Turn on
    led.clear();   // Turn off
    led.toggle();  // Toggle state
}
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
     * @brief Set pin HIGH (output = 1)
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> set() {
        auto* port = get_port();

        port->SODR = pin_mask;  // Set Output Data Register
#ifdef ALLOY_GPIO_TEST_HOOK_SODR
        ALLOY_GPIO_TEST_HOOK_SODR();
#endif

        return Ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> clear() {
        auto* port = get_port();

        port->CODR = pin_mask;  // Clear Output Data Register
#ifdef ALLOY_GPIO_TEST_HOOK_CODR
        ALLOY_GPIO_TEST_HOOK_CODR();
#endif

        return Ok();
    }

    /**
     * @brief Toggle pin state
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> toggle() {
        auto* port = get_port();

        uint32_t current_state = port->ODSR;  // Read current output data status

        if (current_state & pin_mask) {
            port->CODR = pin_mask;  // Pin is HIGH, set it LOW
        } else {
            port->SODR = pin_mask;  // Pin is LOW, set it HIGH
        }
        return Ok();
    }

    /**
     * @brief Write pin value
     *
     * @param value true for HIGH, false for LOW
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> write(bool value) {
        auto* port = get_port();

        if (value) {}
        return Ok();
    }

    /**
     * @brief Read pin input value
     *
     * @return Result<bool, ErrorCode>     */
    Result<bool, ErrorCode> read() const {
        auto* port = get_port();

        uint32_t pin_state = port->PDSR;  // Reads the actual pin state from PDSR register

        bool value = (pin_state & pin_mask) != 0;

        return Ok(bool(value));
    }

    /**
     * @brief Set GPIO pin direction
     *
     * @param direction Pin direction (Input or Output)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDirection(PinDirection direction) {
        auto* port = get_port();


        if (direction == PinDirection::Input) {
            port->ODR = pin_mask;  // Configure as input
#ifdef ALLOY_GPIO_TEST_HOOK_ODR
            ALLOY_GPIO_TEST_HOOK_ODR();
#endif
        } else {
            port->OER = pin_mask;  // Configure as output
#ifdef ALLOY_GPIO_TEST_HOOK_OER
            ALLOY_GPIO_TEST_HOOK_OER();
#endif
        }
        return Ok();
    }

    /**
     * @brief Set GPIO pin drive mode
     *
     * @param drive Drive mode (PushPull or OpenDrain)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDrive(PinDrive drive) {
        auto* port = get_port();

        if (drive == PinDrive::OpenDrain) {
            port->MDER = pin_mask;  // Enable multi-driver (open-drain)
#ifdef ALLOY_GPIO_TEST_HOOK_MDER
            ALLOY_GPIO_TEST_HOOK_MDER();
#endif
        } else {
            port->MDDR = pin_mask;  // Disable multi-driver (push-pull)
#ifdef ALLOY_GPIO_TEST_HOOK_MDDR
            ALLOY_GPIO_TEST_HOOK_MDDR();
#endif
        }
        return Ok();
    }

    /**
     * @brief Configure pull resistor
     *
     * @param pull Pull resistor configuration
     * @return Result<void, ErrorCode> SAME70 only supports pull-up, not pull-down. PullDown will
     * return ErrorCode::NotSupported.     *
     * @note SAME70 only supports pull-up, not pull-down. PullDown will return
     * ErrorCode::NotSupported.
     */
    Result<void, ErrorCode> setPull(PinPull pull) {
        auto* port = get_port();

        switch (pull) {
            case PinPull::None:
                port->PUDR = pin_mask;  // Disable pull-up
#ifdef ALLOY_GPIO_TEST_HOOK_PUDR
                ALLOY_GPIO_TEST_HOOK_PUDR();
#endif
                break;

            case PinPull::PullUp:
                port->PUER = pin_mask;  // Enable pull-up
#ifdef ALLOY_GPIO_TEST_HOOK_PUER
                ALLOY_GPIO_TEST_HOOK_PUER();
#endif
                break;

            case PinPull::PullDown:
                return Err(ErrorCode::NotSupported);
        }
        return Ok();
    }

    /**
     * @brief Enable input glitch filter
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> enableFilter() {
        auto* port = get_port();


        return Ok();
    }

    /**
     * @brief Disable input glitch filter
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> disableFilter() {
        auto* port = get_port();


        return Ok();
    }

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result<bool, ErrorCode>     */
    Result<bool, ErrorCode> isOutput() const {
        auto* port = get_port();

        uint32_t status = port->OSR;  //

        bool is_output = (status & pin_mask) != 0;

        return Ok(bool(is_output));
    }
};

// ==============================================================================
// Port Base Address Constants
// ==============================================================================

constexpr uint32_t PIOA_BASE = 0x400E0E00;
constexpr uint32_t PIOB_BASE = 0x400E1000;
constexpr uint32_t PIOC_BASE = 0x400E1200;
constexpr uint32_t PIOD_BASE = 0x400E1400;
constexpr uint32_t PIOE_BASE = 0x400E1600;

// ==============================================================================
// Common Pin Type Aliases
// ==============================================================================

// Board-specific pin definitions should be in board.hpp
// Example:
// using LedGreen = GpioPin<PIOC_BASE, 8>;
// using Button0 = GpioPin<PIOA_BASE, 11>;
// auto led = LedGreen{};
// auto result = led.setDirection(PinDirection::Output);
// if (result.is_ok()) {
//     led.set();     // Turn on
//     led.clear();   // Turn off
//     led.toggle();  // Toggle state
// }

}  // namespace alloy::hal::same70
