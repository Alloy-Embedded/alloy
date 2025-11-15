/**
 * @file uart_hardware_policy.hpp
 * @brief Hardware Policy for UART on STM32F7 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for UART using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_UART_MOCK_HW)
 *
 * Auto-generated from: stm32f7/gpio.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-14 20:18:55
 *
 * @note Part of Alloy HAL Vendor Layer
 * @note See ARCHITECTURE.md for Policy-Based Design rationale
 */

#pragma once

#include "core/types.hpp"
#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"

// Register definitions
#include "hal/vendors/st/stm32f7/registers/gpioa_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f7/bitfields/gpioa_bitfields.hpp"

namespace alloy::hal::st::stm32f7 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32f7;
using namespace alloy::hal::st::stm32f7::gpioa;


/**
 * @brief Hardware Policy for GPIO on STM32F7
 *
 * This policy provides all platform-specific hardware access methods
 * for GPIO. It is designed to be used as a template
 * parameter in generic GPIO implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: GPIO peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * Usage:
 * @code
 * // In generic API
 * template <typename HardwarePolicy>
 * class UartImpl {
 *     void initialize() {
 *         HardwarePolicy::reset();
 *         HardwarePolicy::configure_8n1();
 *         HardwarePolicy::set_baudrate(115200);
 *         HardwarePolicy::enable_tx();
 *         HardwarePolicy::enable_rx();
 *     }
 * };
 *
 * // Platform-specific alias
 * using Uart0 = UartImpl<Stm32f7UartHardwarePolicy<UART0_BASE, 150000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for baud rate calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f7GPIOHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = GPIOA_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;

    // ========================================================================
    // Hardware Accessor (with Mock Hook)
    // ========================================================================

    /**
     * @brief Get pointer to hardware registers
     *
     * This method provides access to the actual hardware registers.
     * It includes a mock hook for testing purposes.
     *
     * @return Pointer to hardware registers
     */
    static inline volatile RegisterType* hw() {
        #ifdef ALLOY_GPIO_MOCK_HW
            return ALLOY_GPIO_MOCK_HW();  // Test hook
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    // ========================================================================
    // Hardware Policy Methods
    // ========================================================================

    /**
     * @brief Configure pin as output
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_MODER
     */
    static inline void set_mode_output(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_MODER
            ALLOY_GPIO_TEST_HOOK_MODER(pin_number);
        #endif

        hw()->MODER = (hw()->MODER & ~(0x3U << (pin_number * 2))) | (0x1U << (pin_number * 2));
    }

    /**
     * @brief Configure pin as input
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_MODER
     */
    static inline void set_mode_input(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_MODER
            ALLOY_GPIO_TEST_HOOK_MODER(pin_number);
        #endif

        hw()->MODER = (hw()->MODER & ~(0x3U << (pin_number * 2)));
    }

    /**
     * @brief Configure pin as alternate function
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_MODER
     */
    static inline void set_mode_alternate(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_MODER
            ALLOY_GPIO_TEST_HOOK_MODER(pin_number);
        #endif

        hw()->MODER = (hw()->MODER & ~(0x3U << (pin_number * 2))) | (0x2U << (pin_number * 2));
    }

    /**
     * @brief Configure pin as analog
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_MODER
     */
    static inline void set_mode_analog(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_MODER
            ALLOY_GPIO_TEST_HOOK_MODER(pin_number);
        #endif

        hw()->MODER = (hw()->MODER & ~(0x3U << (pin_number * 2))) | (0x3U << (pin_number * 2));
    }

    /**
     * @brief Configure output as push-pull
     *
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_OTYPER
     */
    static inline void set_output_type_pushpull(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
            ALLOY_GPIO_TEST_HOOK_OTYPER(pin_mask);
        #endif

        hw()->OTYPER &= ~pin_mask;
    }

    /**
     * @brief Configure output as open-drain
     *
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_OTYPER
     */
    static inline void set_output_type_opendrain(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
            ALLOY_GPIO_TEST_HOOK_OTYPER(pin_mask);
        #endif

        hw()->OTYPER |= pin_mask;
    }

    /**
     * @brief Disable pull-up/pull-down resistors
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PUPDR
     */
    static inline void set_pull_none(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PUPDR
            ALLOY_GPIO_TEST_HOOK_PUPDR(pin_number);
        #endif

        hw()->PUPDR = (hw()->PUPDR & ~(0x3U << (pin_number * 2)));
    }

    /**
     * @brief Enable pull-up resistor
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PUPDR
     */
    static inline void set_pull_up(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PUPDR
            ALLOY_GPIO_TEST_HOOK_PUPDR(pin_number);
        #endif

        hw()->PUPDR = (hw()->PUPDR & ~(0x3U << (pin_number * 2))) | (0x1U << (pin_number * 2));
    }

    /**
     * @brief Enable pull-down resistor
     *
     * @param pin_number Pin number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PUPDR
     */
    static inline void set_pull_down(uint8_t pin_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PUPDR
            ALLOY_GPIO_TEST_HOOK_PUPDR(pin_number);
        #endif

        hw()->PUPDR = (hw()->PUPDR & ~(0x3U << (pin_number * 2))) | (0x2U << (pin_number * 2));
    }

    /**
     * @brief Set output pin high
     *
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_BSRR
     */
    static inline void set_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_BSRR
            ALLOY_GPIO_TEST_HOOK_BSRR(pin_mask);
        #endif

        hw()->BSRR = pin_mask;
    }

    /**
     * @brief Set output pin low
     *
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_BSRR
     */
    static inline void clear_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_BSRR
            ALLOY_GPIO_TEST_HOOK_BSRR(pin_mask);
        #endif

        hw()->BSRR = (pin_mask << 16);
    }

    /**
     * @brief Toggle output pin
     *
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_ODR
     */
    static inline void toggle_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_ODR
            ALLOY_GPIO_TEST_HOOK_ODR(pin_mask);
        #endif

        hw()->ODR ^= pin_mask;
    }

    /**
     * @brief Read input pin state
     *
     * @param pin_mask Pin mask
     * @return bool
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_IDR
     */
    static inline bool read_input(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_IDR
            ALLOY_GPIO_TEST_HOOK_IDR(pin_mask);
        #endif

        return (hw()->IDR & pin_mask) != 0;
    }

    /**
     * @brief Read output register state
     *
     * @param pin_mask Pin mask
     * @return bool
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_ODR
     */
    static inline bool read_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_ODR
            ALLOY_GPIO_TEST_HOOK_ODR(pin_mask);
        #endif

        return (hw()->ODR & pin_mask) != 0;
    }

    /**
     * @brief Write entire port output data
     *
     * @param value Port value
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_ODR
     */
    static inline void write_port(uint32_t value) {
        #ifdef ALLOY_GPIO_TEST_HOOK_ODR
            ALLOY_GPIO_TEST_HOOK_ODR(value);
        #endif

        hw()->ODR = value;
    }

    /**
     * @brief Read entire port input data
     * @return uint32_t
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_IDR
     */
    static inline uint32_t read_port() {
        #ifdef ALLOY_GPIO_TEST_HOOK_IDR
            ALLOY_GPIO_TEST_HOOK_IDR();
        #endif

        return hw()->IDR;
    }

    /**
     * @brief Configure alternate function for a pin
     *
     * @param pin_number Pin number (0-15)
     * @param af_number Alternate function number (0-15)
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_AFR
     */
    static inline void set_alternate_function(uint8_t pin_number, uint8_t af_number) {
        #ifdef ALLOY_GPIO_TEST_HOOK_AFR
            ALLOY_GPIO_TEST_HOOK_AFR(pin_number, af_number);
        #endif

        if (pin_number < 8) {
            hw()->AFRL = (hw()->AFRL & ~(0xFU << (pin_number * 4))) | (af_number << (pin_number * 4));
        } else {
            hw()->AFRH = (hw()->AFRH & ~(0xFU << ((pin_number - 8) * 4))) | (af_number << ((pin_number - 8) * 4));
        }
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32f7

/**
 * @example
 * Using the hardware policy with generic UART API:
 *
 * @code
 * #include "hal/api/uart_simple.hpp"
 * #include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create UART with hardware policy
 * using Uart0 = UartImpl<PeripheralId::USART0, Uart0Hardware>;
 *
 * int main() {
 *     auto config = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
 *     config.initialize();
 *
 *     const char* msg = "Hello World\n";
 *     config.write(reinterpret_cast<const uint8_t*>(msg), 12);
 * }
 * @endcode
 */