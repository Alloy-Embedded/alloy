/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for STM32F4 (Platform Layer)
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
 * Auto-generated from: stm32f4
 * Generator: generate_platform_gpio.py
 * Generated: 2025-11-07 17:18:08
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
#include "hal/vendors/st/stm32f4/registers/gpioa_registers.hpp"

// Bitfields (family-level, if available)
// #include "hal/vendors/st/stm32f4/bitfields/gpioa_bitfields.hpp"

namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace alloy::hal::st::stm32f4;

// Note: GPIO configuration uses common HAL types from hal/types.hpp:
// - PinDirection (Input, Output)
// - PinPull (None, PullUp, PullDown)
// - PinDrive (PushPull, OpenDrain)

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief GPIO output speed (STM32-specific)
 */
enum class GpioSpeed : uint8_t {
    Low = 0,       ///< Low speed (2 MHz)
    Medium = 1,    ///< Medium speed (25 MHz)
    High = 2,      ///< High speed (50 MHz)
    VeryHigh = 3,  ///< Very high speed (100 MHz)
};


/**
 * @brief Template-based GPIO pin for STM32F4
 *
 * This class provides a template-based GPIO implementation with ZERO runtime
 * overhead. All pin masks and operations are resolved at compile-time.
 *
 * Template Parameters:
 * - PORT_BASE: GPIO port base address (compile-time constant)
 * - PIN_NUM: Pin number within port (0-31)
 *
 * Example usage:
 * @code
 * // Define LED pin (GPIOA pin 5)
 * using LedGreen = GpioPin<GPIOA_BASE, 5>;
 * // Define button pin (GPIOC pin 13)
 * using Button0 = GpioPin<GPIOC_BASE, 13>;
 * // Use GPIO pin with speed control
 * auto led = LedGreen{};
led.setDirection(PinDirection::Output);
led.setSpeed(GpioSpeed::Medium);
led.set();
 * @endcode
 *
 * @tparam PORT_BASE GPIO port base address
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
     * @brief Set pin HIGH (output = 1)
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> set() {
        auto* port = get_port();


        return Ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> clear() {
        auto* port = get_port();


        return Ok();
    }

    /**
     * @brief Toggle pin state
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> toggle() {
        auto* port = get_port();

        uint32_t current_state = port->ODR;  // Read current output data

        if (current_state & pin_mask) {
            port->BSRR = (pin_mask << 16);  // Pin is HIGH, reset it
        } else {
            port->BSRR = pin_mask;  // Pin is LOW, set it
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

        uint32_t pin_state = port->IDR;  // Read from IDR register

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

        // Set MODER bits: 00=input, 01=output
        uint32_t temp = port->MODER;
        temp &= ~(0x3 << (PIN_NUM * 2));
        temp |= (direction == PinDirection::Input ? 0x0 : 0x1) << (PIN_NUM * 2);
        port->MODER = temp;
#ifdef ALLOY_GPIO_TEST_HOOK_MODER
        ALLOY_GPIO_TEST_HOOK_MODER();
#endif

        return Ok();
    }

    /**
     * @brief Set GPIO pin drive mode
     *
     * @param drive Drive mode (PushPull or OpenDrain)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDrive(PinDrive drive) {
        auto* port = get_port();

        // Set OTYPER bit: 0=push-pull, 1=open-drain
        uint32_t temp = port->OTYPER;
        temp &= ~pin_mask;
        temp |= drive == PinDrive::OpenDrain ? pin_mask : 0;
        port->OTYPER = temp;
#ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
        ALLOY_GPIO_TEST_HOOK_OTYPER();
#endif

        return Ok();
    }

    /**
     * @brief Configure pull resistor
     *
     * @param pull Pull resistor configuration
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setPull(PinPull pull) {
        auto* port = get_port();

        // Set PUPDR bits: 00=none, 01=pull-up, 10=pull-down
        uint32_t temp = port->PUPDR;
        temp &= ~(0x3 << (PIN_NUM * 2));
        uint32_t value = 0;
        switch (pull) {
            case PinPull::None:
                value = 0x0 << (PIN_NUM * 2);
                break;
            case PinPull::PullUp:
                value = 0x1 << (PIN_NUM * 2);
                break;
            case PinPull::PullDown:
                value = 0x2 << (PIN_NUM * 2);
                break;
        }
        temp |= value;
        port->PUPDR = temp;
#ifdef ALLOY_GPIO_TEST_HOOK_PUPDR
        ALLOY_GPIO_TEST_HOOK_PUPDR();
#endif

        return Ok();
    }

    /**
     * @brief Set GPIO output speed
     *
     * @param speed Output speed (Low, Medium, High, VeryHigh)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setSpeed(GpioSpeed speed) {
        auto* port = get_port();

        // Set OSPEEDR bits
        uint32_t temp = port->OSPEEDR;
        temp &= ~(0x3 << (PIN_NUM * 2));
        temp |= (static_cast<uint32_t>(speed)) << (PIN_NUM * 2);
        port->OSPEEDR = temp;
#ifdef ALLOY_GPIO_TEST_HOOK_OSPEEDR
        ALLOY_GPIO_TEST_HOOK_OSPEEDR();
#endif

        return Ok();
    }

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result<bool, ErrorCode>     */
    Result<bool, ErrorCode> isOutput() const {
        auto* port = get_port();

        uint32_t moder = port->MODER;  //

        bool is_output = ((moder >> (PIN_NUM * 2)) & 0x3) == 0x1;

        return Ok(bool(is_output));
    }
};

// ==============================================================================
// Port Base Address Constants
// ==============================================================================

constexpr uint32_t GPIOA_BASE = 0x40020000;
constexpr uint32_t GPIOB_BASE = 0x40020400;
constexpr uint32_t GPIOC_BASE = 0x40020800;
constexpr uint32_t GPIOD_BASE = 0x40020C00;
constexpr uint32_t GPIOE_BASE = 0x40021000;
constexpr uint32_t GPIOF_BASE = 0x40021400;
constexpr uint32_t GPIOG_BASE = 0x40021800;
constexpr uint32_t GPIOH_BASE = 0x40021C00;
constexpr uint32_t GPIOI_BASE = 0x40022000;

// ==============================================================================
// Common Pin Type Aliases
// ==============================================================================

// Board-specific pin definitions should be in board.hpp
// Example:
// using LedGreen = GpioPin<GPIOA_BASE, 5>;
// using Button0 = GpioPin<GPIOC_BASE, 13>;
// auto led = LedGreen{};
// led.setDirection(PinDirection::Output);
// led.setSpeed(GpioSpeed::Medium);
// led.set();

}  // namespace alloy::hal::stm32f4
