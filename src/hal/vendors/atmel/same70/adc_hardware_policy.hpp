/**
 * @file adc_hardware_policy.hpp
 * @brief Hardware Policy for ADC on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for ADC using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_ADC_MOCK_HW)
 *
 * Auto-generated from: same70/adc.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-11 20:39:30
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
#include "hal/vendors/atmel/same70/registers/afec0_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/bitfields/afec0_bitfields.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace afec = atmel::same70::afec0;

/**
 * @brief Hardware Policy for ADC on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for ADC. It is designed to be used as a template
 * parameter in generic ADC implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: AFEC peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency for timing calculations
 *
 * @tparam BASE_ADDR AFEC peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency for timing calculations
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70ADCHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = AFEC0_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t ADC_TIMEOUT = 100000;  ///< ADC timeout in loop iterations
    static constexpr uint8_t MAX_CHANNELS = 12;  ///< Maximum number of ADC channels

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
        #ifdef 
            return ();  // Test hook
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
     * @brief Reset AFEC peripheral
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_RESET
     */
    static inline void reset() {
        #ifdef ALLOY_ADC_TEST_HOOK_RESET
            ALLOY_ADC_TEST_HOOK_RESET();
        #endif

        hw()->CR = afec::cr::SWRST::mask;
    }

    /**
     * @brief Enable AFEC peripheral
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ENABLE
     */
    static inline void enable() {
        #ifdef ALLOY_ADC_TEST_HOOK_ENABLE
            ALLOY_ADC_TEST_HOOK_ENABLE();
        #endif

        hw()->CR = afec::cr::START::mask;
    }

    /**
     * @brief Disable AFEC peripheral
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_DISABLE
     */
    static inline void disable() {
        #ifdef ALLOY_ADC_TEST_HOOK_DISABLE
            ALLOY_ADC_TEST_HOOK_DISABLE();
        #endif

        hw()->MR &= ~afec::mr::FREERUN::mask;
    }

    /**
     * @brief Configure ADC resolution (12-bit)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_RESOLUTION
     */
    static inline void configure_resolution() {
        #ifdef ALLOY_ADC_TEST_HOOK_RESOLUTION
            ALLOY_ADC_TEST_HOOK_RESOLUTION();
        #endif

        hw()->EMR = (hw()->EMR & ~afec::emr::RES::mask) | afec::emr::RES::write(0, 0);
    }

    /**
     * @brief Set ADC prescaler for sampling clock
     * @param prescaler Prescaler value (0-255)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_PRESCALER
     */
    static inline void set_prescaler(uint8_t prescaler) {
        #ifdef ALLOY_ADC_TEST_HOOK_PRESCALER
            ALLOY_ADC_TEST_HOOK_PRESCALER(prescaler);
        #endif

        hw()->MR = (hw()->MR & ~afec::mr::PRESCAL::mask) | afec::mr::PRESCAL::write(0, prescaler);
    }

    /**
     * @brief Enable ADC channel
     * @param channel Channel number (0-11)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ENABLE_CH
     */
    static inline void enable_channel(uint8_t channel) {
        #ifdef ALLOY_ADC_TEST_HOOK_ENABLE_CH
            ALLOY_ADC_TEST_HOOK_ENABLE_CH(channel);
        #endif

        hw()->CHER = (1u << channel);
    }

    /**
     * @brief Disable ADC channel
     * @param channel Channel number (0-11)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_DISABLE_CH
     */
    static inline void disable_channel(uint8_t channel) {
        #ifdef ALLOY_ADC_TEST_HOOK_DISABLE_CH
            ALLOY_ADC_TEST_HOOK_DISABLE_CH(channel);
        #endif

        hw()->CHDR = (1u << channel);
    }

    /**
     * @brief Select ADC channel for conversion
     * @param channel Channel number (0-11)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_SELECT_CH
     */
    static inline void select_channel(uint8_t channel) {
        #ifdef ALLOY_ADC_TEST_HOOK_SELECT_CH
            ALLOY_ADC_TEST_HOOK_SELECT_CH(channel);
        #endif

        hw()->CSELR = afec::cselr::CSEL::write(0, channel);
    }

    /**
     * @brief Start ADC conversion
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_START
     */
    static inline void start_conversion() {
        #ifdef ALLOY_ADC_TEST_HOOK_START
            ALLOY_ADC_TEST_HOOK_START();
        #endif

        hw()->CR = afec::cr::START::mask;
    }

    /**
     * @brief Check if conversion is complete
     * @param channel Channel number (0-11)
     * @return bool
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_IS_DONE
     */
    static inline bool is_conversion_done(uint8_t channel) const {
        #ifdef ALLOY_ADC_TEST_HOOK_IS_DONE
            ALLOY_ADC_TEST_HOOK_IS_DONE(channel);
        #endif

        return (hw()->ISR & (1u << channel)) != 0;
    }

    /**
     * @brief Read ADC conversion result
     * @return uint16_t
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_READ
     */
    static inline uint16_t read_value() const {
        #ifdef ALLOY_ADC_TEST_HOOK_READ
            ALLOY_ADC_TEST_HOOK_READ();
        #endif

        return static_cast<uint16_t>(hw()->CDR & 0xFFF);
    }

    /**
     * @brief Read last converted data register
     * @return uint16_t
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_READ_LAST
     */
    static inline uint16_t read_last_value() const {
        #ifdef ALLOY_ADC_TEST_HOOK_READ_LAST
            ALLOY_ADC_TEST_HOOK_READ_LAST();
        #endif

        return static_cast<uint16_t>(hw()->LCDR & 0xFFF);
    }

    /**
     * @brief Enable free-running mode
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_FREERUN
     */
    static inline void enable_freerun_mode() {
        #ifdef ALLOY_ADC_TEST_HOOK_FREERUN
            ALLOY_ADC_TEST_HOOK_FREERUN();
        #endif

        hw()->MR |= afec::mr::FREERUN::mask;
    }

    /**
     * @brief Set conversion trigger source
     * @param trigger Trigger source (0-6)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_TRIGGER
     */
    static inline void set_trigger(uint8_t trigger) {
        #ifdef ALLOY_ADC_TEST_HOOK_TRIGGER
            ALLOY_ADC_TEST_HOOK_TRIGGER(trigger);
        #endif

        hw()->MR = (hw()->MR & ~afec::mr::TRGSEL::mask) | afec::mr::TRGSEL::write(0, trigger);
    }

    /**
     * @brief Wait for conversion to complete
     * @param channel Channel number
     * @param timeout_loops Timeout in loop iterations
     * @return bool
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_WAIT
     */
    static inline bool wait_conversion(uint8_t channel, uint32_t timeout_loops) {
        #ifdef ALLOY_ADC_TEST_HOOK_WAIT
            ALLOY_ADC_TEST_HOOK_WAIT(channel, timeout_loops);
        #endif

        uint32_t timeout = timeout_loops; while (!is_conversion_done(channel) && --timeout); return timeout != 0;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Adc0
using Adc0Hardware = Same70ADCHardwarePolicy<0x4003C000, 150000000>;
/// @brief Hardware policy for Adc1
using Adc1Hardware = Same70ADCHardwarePolicy<0x40064000, 150000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic ADC API:
 *
 * @code
 * #include "hal/api/adc_simple.hpp"
 * #include "hal/platform/same70/adc.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create ADC with hardware policy
 * using Instance0 = Adc0Hardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */