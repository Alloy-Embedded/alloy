/**
 * @file pio_hardware_policy.hpp
 * @brief Hardware Policy for PIO on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for PIO using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_PIO_MOCK_HW)
 *
 * Auto-generated from: same70/pio.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-14 09:58:28
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
#include "hal/vendors/atmel/same70/registers/pioa_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/bitfields/pioa_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace  = pioa;

/**
 * @brief Hardware Policy for PIO on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for PIO. It is designed to be used as a template
 * parameter in generic PIO implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: PIO peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency
 *
 * @tparam BASE_ADDR PIO peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70PIOHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = PIOA_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;

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
     * @brief Get pointer to hardware registers
     * @return volatile RegisterType*
     */
    static inline volatile RegisterType* hw_accessor() {
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    }

    /**
     * @brief Enable PIO control for pin mask
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PER
     */
    static inline void enable_pio(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PER
            ALLOY_GPIO_TEST_HOOK_PER(pin_mask);
        #endif

        hw()->PER = pin_mask;
    }

    /**
     * @brief Disable PIO control (enable peripheral function)
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PDR
     */
    static inline void disable_pio(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PDR
            ALLOY_GPIO_TEST_HOOK_PDR(pin_mask);
        #endif

        hw()->PDR = pin_mask;
    }

    /**
     * @brief Configure pin as output
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_OER
     */
    static inline void enable_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_OER
            ALLOY_GPIO_TEST_HOOK_OER(pin_mask);
        #endif

        hw()->OER = pin_mask;
    }

    /**
     * @brief Configure pin as input
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_ODR
     */
    static inline void disable_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_ODR
            ALLOY_GPIO_TEST_HOOK_ODR(pin_mask);
        #endif

        hw()->ODR = pin_mask;
    }

    /**
     * @brief Set output pin high
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_SODR
     */
    static inline void set_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_SODR
            ALLOY_GPIO_TEST_HOOK_SODR(pin_mask);
        #endif

        hw()->SODR = pin_mask;
    }

    /**
     * @brief Set output pin low
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_CODR
     */
    static inline void clear_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_CODR
            ALLOY_GPIO_TEST_HOOK_CODR(pin_mask);
        #endif

        hw()->CODR = pin_mask;
    }

    /**
     * @brief Toggle output pin
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_TOGGLE
     */
    static inline void toggle_output(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_TOGGLE
            ALLOY_GPIO_TEST_HOOK_TOGGLE(pin_mask);
        #endif

        if (hw()->ODSR & pin_mask) {
    hw()->CODR = pin_mask;
} else {
    hw()->SODR = pin_mask;
}
    }

    /**
     * @brief Read pin input state
     * @param pin_mask Pin mask
     * @return bool
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PDSR
     */
    static inline bool read_pin(uint32_t pin_mask) const {
        #ifdef ALLOY_GPIO_TEST_HOOK_PDSR
            ALLOY_GPIO_TEST_HOOK_PDSR(pin_mask);
        #endif

        return (hw()->PDSR & pin_mask) != 0;
    }

    /**
     * @brief Check if pin is configured as output
     * @param pin_mask Pin mask
     * @return bool
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_OSR
     */
    static inline bool is_output(uint32_t pin_mask) const {
        #ifdef ALLOY_GPIO_TEST_HOOK_OSR
            ALLOY_GPIO_TEST_HOOK_OSR(pin_mask);
        #endif

        return (hw()->OSR & pin_mask) != 0;
    }

    /**
     * @brief Enable pull-up resistor
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PUER
     */
    static inline void enable_pull_up(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PUER
            ALLOY_GPIO_TEST_HOOK_PUER(pin_mask);
        #endif

        hw()->PUER = pin_mask;
    }

    /**
     * @brief Disable pull-up resistor
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_PUDR
     */
    static inline void disable_pull_up(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_PUDR
            ALLOY_GPIO_TEST_HOOK_PUDR(pin_mask);
        #endif

        hw()->PUDR = pin_mask;
    }

    /**
     * @brief Enable multi-driver (open-drain)
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_MDER
     */
    static inline void enable_multi_driver(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_MDER
            ALLOY_GPIO_TEST_HOOK_MDER(pin_mask);
        #endif

        hw()->MDER = pin_mask;
    }

    /**
     * @brief Disable multi-driver (push-pull)
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_MDDR
     */
    static inline void disable_multi_driver(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_MDDR
            ALLOY_GPIO_TEST_HOOK_MDDR(pin_mask);
        #endif

        hw()->MDDR = pin_mask;
    }

    /**
     * @brief Enable input glitch filter
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_IFER
     */
    static inline void enable_input_filter(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_IFER
            ALLOY_GPIO_TEST_HOOK_IFER(pin_mask);
        #endif

        hw()->IFER = pin_mask;
    }

    /**
     * @brief Disable input glitch filter
     * @param pin_mask Pin mask
     *
     * @note Test hook: ALLOY_GPIO_TEST_HOOK_IFDR
     */
    static inline void disable_input_filter(uint32_t pin_mask) {
        #ifdef ALLOY_GPIO_TEST_HOOK_IFDR
            ALLOY_GPIO_TEST_HOOK_IFDR(pin_mask);
        #endif

        hw()->IFDR = pin_mask;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for PioA
using PioAHardware = Same70PIOHardwarePolicy<0x400E0E00, 150000000>;
/// @brief Hardware policy for PioB
using PioBHardware = Same70PIOHardwarePolicy<0x400E1000, 150000000>;
/// @brief Hardware policy for PioC
using PioCHardware = Same70PIOHardwarePolicy<0x400E1200, 150000000>;
/// @brief Hardware policy for PioD
using PioDHardware = Same70PIOHardwarePolicy<0x400E1400, 150000000>;
/// @brief Hardware policy for PioE
using PioEHardware = Same70PIOHardwarePolicy<0x400E1600, 150000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic PIO API:
 *
 * @code
 * #include "hal/api/pio_simple.hpp"
 * #include "hal/platform/same70/pio.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create PIO with hardware policy
 * using Instance0 = PioAHardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */