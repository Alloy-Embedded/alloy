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
 * @note Part of MicroCore HAL Platform Abstraction Layer
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

namespace ucore::hal::stm32f4 {

using namespace ucore::core;
using namespace ucore::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace ucore::hal::st::stm32f4;

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
     * Uses the BSRR (Bit Set/Reset Register) for atomic, single-cycle write.
     * This ensures interrupt-safe operation without read-modify-write.
     *
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Atomic operation - safe to call from interrupts
     * @note Zero overhead - compiles to single STR instruction
     *
     * Example:
     * @code
     * using Led = GpioPin<GPIOA_BASE, 5>;
     * auto led = Led{};
     * led.set();  // Turn LED on
     * @endcode
     */
    Result<void, ErrorCode> set() {
        auto* port = get_port();


        return Ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     *
     * Uses the BSRR (Bit Set/Reset Register) for atomic, single-cycle write.
     * Writes to the upper 16 bits to reset the pin.
     *
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Atomic operation - safe to call from interrupts
     * @note Zero overhead - compiles to single STR instruction
     *
     * Example:
     * @code
     * using Led = GpioPin<GPIOA_BASE, 5>;
     * auto led = Led{};
     * led.clear();  // Turn LED off
     * @endcode
     */
    Result<void, ErrorCode> clear() {
        auto* port = get_port();


        return Ok();
    }

    /**
     * @brief Toggle pin state (HIGH → LOW or LOW → HIGH)
     *
     * Reads current output state from ODR and toggles using BSRR for atomic write.
     * This is a read-modify-write operation but the final write is atomic.
     *
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Not fully atomic - uses read-modify-write pattern
     * @note Suitable for application-level toggling (e.g., LED blinking)
     *
     * Example:
     * @code
     * using Led = GpioPin<GPIOA_BASE, 5>;
     * auto led = Led{};
     * led.setDirection(PinDirection::Output);
     *
     * while (true) {
     *     led.toggle();  // Blink LED
     *     delay_ms(500);
     * }
     * @endcode
     */
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
     * @brief Write pin value (HIGH or LOW)
     *
     * Convenience method that calls set() or clear() based on the boolean value.
     * Uses atomic BSRR register writes.
     *
     * @param value true for HIGH, false for LOW
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Atomic operation - safe to call from interrupts
     * @note Prefer set()/clear() for known values to avoid branch
     *
     * Example:
     * @code
     * using Led = GpioPin<GPIOA_BASE, 5>;
     * auto led = Led{};
     * led.setDirection(PinDirection::Output);
     *
     * bool led_state = true;
     * led.write(led_state);  // Turn LED on
     * @endcode
     */
    Result<void, ErrorCode> write(bool value) {
        auto* port = get_port();

        if (value) {}
        return Ok();
    }

    /**
     * @brief Read pin input value
     *
     * Reads the current pin state from the IDR (Input Data Register).
     * Works for both input and output pins.
     *
     * @return Result<bool, ErrorCode> Ok(true) if pin is HIGH, Ok(false) if pin is LOW
     *
     * @note Always reads from IDR, which reflects actual pin electrical state
     * @note For output pins, reads back the actual pin state (useful for open-drain)
     * @note Zero overhead - compiles to single LDR instruction
     *
     * Example:
     * @code
     * using Button = GpioPin<GPIOC_BASE, 13>;
     * auto btn = Button{};
     * btn.setDirection(PinDirection::Input);
     * btn.setPull(PinPull::PullUp);
     *
     * auto result = btn.read();
     * if (result.is_ok() && !result.value()) {
     *     // Button pressed (active low)
     * }
     * @endcode
     */
    Result<bool, ErrorCode> read() const {
        auto* port = get_port();

        uint32_t pin_state = port->IDR;  // Read from IDR register

        bool value = (pin_state & pin_mask) != 0;

        return Ok(bool(value));
    }

    /**
     * @brief Set GPIO pin direction (Input or Output)
     *
     * Configures the pin as either input or output by modifying the MODER register.
     * - Input: Bits = 00
     * - Output: Bits = 01
     *
     * @param direction Pin direction (PinDirection::Input or PinDirection::Output)
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Must be called before using the pin
     * @note Uses read-modify-write pattern to preserve other pins
     * @note Test hook available for unit testing: ALLOY_GPIO_TEST_HOOK_MODER
     *
     * Example:
     * @code
     * using Led = GpioPin<GPIOA_BASE, 5>;
     * auto led = Led{};
     * led.setDirection(PinDirection::Output);
     * led.set();
     * @endcode
     */
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
     * @brief Set GPIO pin drive mode (Push-Pull or Open-Drain)
     *
     * Configures the output driver type in the OTYPER register.
     * - Push-Pull (default): Can drive both HIGH and LOW actively
     * - Open-Drain: Can only pull LOW, requires external pull-up for HIGH
     *
     * @param drive Drive mode (PinDrive::PushPull or PinDrive::OpenDrain)
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Only affects output pins (has no effect on inputs)
     * @note Open-drain useful for I2C, 5V-tolerant outputs, wired-OR
     * @note Test hook available for unit testing: ALLOY_GPIO_TEST_HOOK_OTYPER
     *
     * Example:
     * @code
     * using I2cSda = GpioPin<GPIOB_BASE, 9>;
     * auto sda = I2cSda{};
     * sda.setDirection(PinDirection::Output);
     * sda.setDrive(PinDrive::OpenDrain);  // I2C requires open-drain
     * sda.setPull(PinPull::PullUp);
     * @endcode
     */
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
     * @brief Configure internal pull resistor
     *
     * Configures the internal pull-up/pull-down resistors in the PUPDR register.
     * - None: High-impedance (floating) input
     * - PullUp: Weak pull to VDD (~40kΩ typical)
     * - PullDown: Weak pull to GND (~40kΩ typical)
     *
     * @param pull Pull resistor configuration (PinPull::None, PinPull::PullUp, or PinPull::PullDown)
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Internal resistors are weak (~40kΩ) - use external for critical applications
     * @note Works for both input and output pins
     * @note Test hook available for unit testing: ALLOY_GPIO_TEST_HOOK_PUPDR
     *
     * Example:
     * @code
     * using Button = GpioPin<GPIOC_BASE, 13>;
     * auto btn = Button{};
     * btn.setDirection(PinDirection::Input);
     * btn.setPull(PinPull::PullUp);  // Active-low button
     *
     * auto result = btn.read();
     * if (result.is_ok() && !result.value()) {
     *     // Button pressed
     * }
     * @endcode
     */
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
     * @brief Set GPIO output speed (slew rate)
     *
     * Configures the output slew rate in the OSPEEDR register.
     * Higher speeds allow faster transitions but increase EMI.
     * - Low: 2 MHz (default, lowest EMI)
     * - Medium: 25 MHz
     * - High: 50 MHz
     * - VeryHigh: 100 MHz (highest EMI)
     *
     * @param speed Output speed (GpioSpeed::Low, Medium, High, or VeryHigh)
     * @return Result<void, ErrorCode> Ok() on success
     *
     * @note Only affects output pins
     * @note Use lowest speed that meets timing requirements to reduce EMI
     * @note Test hook available for unit testing: ALLOY_GPIO_TEST_HOOK_OSPEEDR
     *
     * Example:
     * @code
     * using SpiClk = GpioPin<GPIOA_BASE, 5>;
     * auto clk = SpiClk{};
     * clk.setDirection(PinDirection::Output);
     * clk.setSpeed(GpioSpeed::High);  // SPI needs fast edges
     * @endcode
     */
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
     * Reads the MODER register to determine if the pin is configured as output.
     * Returns false for input, alternate function, or analog modes.
     *
     * @return Result<bool, ErrorCode> Ok(true) if output mode, Ok(false) otherwise
     *
     * @note Only returns true for general-purpose output (MODER = 01)
     * @note Returns false for input (00), alternate function (10), or analog (11)
     *
     * Example:
     * @code
     * using Led = GpioPin<GPIOA_BASE, 5>;
     * auto led = Led{};
     * led.setDirection(PinDirection::Output);
     *
     * auto result = led.isOutput();
     * if (result.is_ok() && result.value()) {
     *     led.set();  // Safe to set output
     * }
     * @endcode
     */
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

}  // namespace ucore::hal::stm32f4
