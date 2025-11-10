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
#include "hal/signals.hpp"

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

// Import signal routing types (Phase 3: GPIO Signal Routing)
using alloy::hal::signals::AlternateFunction;
using alloy::hal::signals::PinId;

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

    // ========================================================================
    // Signal Routing Support (Phase 3: GPIO Signal Routing)
    // ========================================================================

    /**
     * @brief Set alternate function for peripheral routing
     *
     * Configures the pin's alternate function (peripheral A, B, C, or D).
     * This is required when using the pin for peripherals like USART, SPI, etc.
     *
     * SAME70 Peripheral Selection:
     * - Peripheral A: ABCDSR[0]=0, ABCDSR[1]=0
     * - Peripheral B: ABCDSR[0]=1, ABCDSR[1]=0
     * - Peripheral C: ABCDSR[0]=0, ABCDSR[1]=1
     * - Peripheral D: ABCDSR[0]=1, ABCDSR[1]=1
     *
     * @param af Alternate function to set (PERIPH_A, PERIPH_B, PERIPH_C, PERIPH_D)
     * @return Result<void, ErrorCode>
     *
     * @note Part of modernize-peripheral-architecture Phase 3
     */
    Result<void, ErrorCode> setAlternateFunction(AlternateFunction af) {
        auto* port = get_port();

        // First, disable PIO control (assign to peripheral)
        port->PDR = pin_mask;  // PIO Disable Register

        // Configure peripheral selection based on alternate function
        // Note: ABCDSR is a 2D array [2][2], we use [0][0] and [1][0]
        switch (af) {
            case AlternateFunction::PERIPH_A:
                // A: ABCDSR[0][0]=0, ABCDSR[1][0]=0
                port->ABCDSR[0][0] &= ~pin_mask;
                port->ABCDSR[1][0] &= ~pin_mask;
                break;

            case AlternateFunction::PERIPH_B:
                // B: ABCDSR[0][0]=1, ABCDSR[1][0]=0
                port->ABCDSR[0][0] |= pin_mask;
                port->ABCDSR[1][0] &= ~pin_mask;
                break;

            case AlternateFunction::PERIPH_C:
                // C: ABCDSR[0][0]=0, ABCDSR[1][0]=1
                port->ABCDSR[0][0] &= ~pin_mask;
                port->ABCDSR[1][0] |= pin_mask;
                break;

            case AlternateFunction::PERIPH_D:
                // D: ABCDSR[0][0]=1, ABCDSR[1][0]=1
                port->ABCDSR[0][0] |= pin_mask;
                port->ABCDSR[1][0] |= pin_mask;
                break;

            default:
                return Err(ErrorCode::InvalidParameter);
        }

        return Ok();
    }

    /**
     * @brief Check if pin supports a specific peripheral signal at compile-time
     *
     * Uses signal routing tables generated in Phase 2 to validate compatibility.
     *
     * @tparam Signal Signal type (e.g., Usart0RxSignal, Spi0MosiSignal)
     * @return true if pin supports signal, false otherwise
     *
     * Example:
     * @code
     * using PinD4 = GpioPin<PIOD_BASE, 4>;
     * static_assert(PinD4::supports<Usart0RxSignal>());
     * @endcode
     *
     * @note Part of modernize-peripheral-architecture Phase 3
     */
    template <typename Signal>
    static constexpr bool supports() {
        // Get PinId for this pin
        constexpr PinId pin_id = get_pin_id();

        // Check if this pin is in the signal's compatible_pins array
        if constexpr (requires { Signal::compatible_pins; }) {
            for (const auto& pin_def : Signal::compatible_pins) {
                if (pin_def.pin == pin_id) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Get alternate function for a specific signal
     *
     * Returns the alternate function needed to route this signal to this pin.
     *
     * @tparam Signal Signal type
     * @return AlternateFunction if compatible, otherwise AF0
     *
     * @note Part of modernize-peripheral-architecture Phase 3
     */
    template <typename Signal>
    static constexpr AlternateFunction get_af_for_signal() {
        constexpr PinId pin_id = get_pin_id();

        if constexpr (requires { Signal::compatible_pins; }) {
            for (const auto& pin_def : Signal::compatible_pins) {
                if (pin_def.pin == pin_id) {
                    return pin_def.af;
                }
            }
        }
        return AlternateFunction::AF0;  // Invalid
    }

    /**
     * @brief Configure pin for a specific peripheral signal
     *
     * Convenience method that sets the alternate function and configures
     * the pin for the given signal.
     *
     * @tparam Signal Signal type
     * @return Result<void, ErrorCode>
     *
     * Example:
     * @code
     * using PinD4 = GpioPin<PIOD_BASE, 4>;
     * auto pin = PinD4{};
     * auto result = pin.configure_for_signal<Usart0RxSignal>();
     * @endcode
     *
     * @note Part of modernize-peripheral-architecture Phase 3
     */
    template <typename Signal>
    Result<void, ErrorCode> configure_for_signal() {
        // Validate at compile-time
        static_assert(supports<Signal>(),
                     "Pin does not support this signal. Check signal routing tables.");

        // Get alternate function for this signal
        constexpr auto af = get_af_for_signal<Signal>();

        // Set alternate function
        return setAlternateFunction(af);
    }

    /**
     * @brief Get PinId enum value for this pin (Public for connection API)
     *
     * Converts PORT_BASE and PIN_NUM to PinId enum.
     * Used by signal connection API in Phase 3.2.
     *
     * @return PinId corresponding to this pin
     */
    static constexpr PinId get_pin_id() {
        // Calculate PinId: Port A = 0-31, Port B = 100-131, etc.
        // Compare with hard-coded addresses since constants are defined after class
        if constexpr (PORT_BASE == 0x400E0E00) {  // PIOA_BASE
            return static_cast<PinId>(PIN_NUM);
        } else if constexpr (PORT_BASE == 0x400E1000) {  // PIOB_BASE
            return static_cast<PinId>(100 + PIN_NUM);
        } else if constexpr (PORT_BASE == 0x400E1200) {  // PIOC_BASE
            return static_cast<PinId>(200 + PIN_NUM);
        } else if constexpr (PORT_BASE == 0x400E1400) {  // PIOD_BASE
            return static_cast<PinId>(300 + PIN_NUM);
        } else if constexpr (PORT_BASE == 0x400E1600) {  // PIOE_BASE
            return static_cast<PinId>(400 + PIN_NUM);
        }
        return static_cast<PinId>(0);  // Invalid
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
