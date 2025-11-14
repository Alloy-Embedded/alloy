/**
 * @file dac_hardware_policy.hpp
 * @brief Hardware Policy for DAC on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for DAC using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_DAC_MOCK_HW)
 *
 * Auto-generated from: same70/dac.json
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
#include "hal/vendors/atmel/same70/registers/dacc_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/bitfields/dacc_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace dacc = atmel::same70::dacc;

/**
 * @brief Hardware Policy for DAC on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for DAC. It is designed to be used as a template
 * parameter in generic DAC implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: DACC peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency
 *
 * @tparam BASE_ADDR DACC peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70DACHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = DACC_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint8_t NUM_CHANNELS = 2;  ///< Number of DAC channels

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
        #ifdef ALLOY_DAC_MOCK_HW
            return ALLOY_DAC_MOCK_HW();  // Test hook
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
     * @brief Software reset of DACC
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_RESET
     */
    static inline void reset() {
        #ifdef ALLOY_DAC_TEST_HOOK_RESET
            ALLOY_DAC_TEST_HOOK_RESET();
        #endif

        hw()->CR = dacc::cr::SWRST::mask;
    }

    /**
     * @brief Enable DAC channel
     * @param channel Channel number (0-1)
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_ENABLE
     */
    static inline void enable_channel(uint8_t channel) {
        #ifdef ALLOY_DAC_TEST_HOOK_ENABLE
            ALLOY_DAC_TEST_HOOK_ENABLE(channel);
        #endif

        hw()->CHER = (1u << channel);
    }

    /**
     * @brief Disable DAC channel
     * @param channel Channel number (0-1)
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DISABLE
     */
    static inline void disable_channel(uint8_t channel) {
        #ifdef ALLOY_DAC_TEST_HOOK_DISABLE
            ALLOY_DAC_TEST_HOOK_DISABLE(channel);
        #endif

        hw()->CHDR = (1u << channel);
    }

    /**
     * @brief Write value to DAC channel
     * @param channel Channel number (0-1)
     * @param value 12-bit value (0-4095)
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_WRITE
     */
    static inline void write_value(uint8_t channel, uint16_t value) {
        #ifdef ALLOY_DAC_TEST_HOOK_WRITE
            ALLOY_DAC_TEST_HOOK_WRITE(channel, value);
        #endif

        if (channel == 0) { hw()->CDR0 = value & 0xFFF; } else { hw()->CDR1 = value & 0xFFF; }
    }

    /**
     * @brief Configure DAC mode register
     * @param mode Mode register value
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_MODE
     */
    static inline void configure_mode(uint32_t mode) {
        #ifdef ALLOY_DAC_TEST_HOOK_MODE
            ALLOY_DAC_TEST_HOOK_MODE(mode);
        #endif

        hw()->MR = mode;
    }

    /**
     * @brief Enable external trigger
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_TRIG_EN
     */
    static inline void enable_trigger() {
        #ifdef ALLOY_DAC_TEST_HOOK_TRIG_EN
            ALLOY_DAC_TEST_HOOK_TRIG_EN();
        #endif

        hw()->MR |= dacc::mr::TRGEN::mask;
    }

    /**
     * @brief Disable external trigger
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_TRIG_DIS
     */
    static inline void disable_trigger() {
        #ifdef ALLOY_DAC_TEST_HOOK_TRIG_DIS
            ALLOY_DAC_TEST_HOOK_TRIG_DIS();
        #endif

        hw()->MR &= ~dacc::mr::TRGEN::mask;
    }

    /**
     * @brief Check if DAC is ready
     * @param channel Channel number (0-1)
     * @return bool
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_READY
     */
    static inline bool is_ready(uint8_t channel) const {
        #ifdef ALLOY_DAC_TEST_HOOK_READY
            ALLOY_DAC_TEST_HOOK_READY(channel);
        #endif

        return (hw()->ISR & (dacc::isr::TXRDY0::mask << channel)) != 0;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Dac
using DacHardware = Same70DACHardwarePolicy<0x40040000, 150000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic DAC API:
 *
 * @code
 * #include "hal/api/dac_simple.hpp"
 * #include "hal/platform/same70/dac.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create DAC with hardware policy
 * using Instance0 = DacHardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */