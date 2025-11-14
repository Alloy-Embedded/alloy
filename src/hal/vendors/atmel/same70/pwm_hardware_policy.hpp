/**
 * @file pwm_hardware_policy.hpp
 * @brief Hardware Policy for PWM on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for PWM using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_PWM_MOCK_HW)
 *
 * Auto-generated from: same70/pwm.json
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
#include "hal/vendors/atmel/same70/registers/pwm0_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/bitfields/pwm0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace pwm = alloy::hal::atmel::same70::pwm0;

/**
 * @brief Hardware Policy for PWM on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for PWM. It is designed to be used as a template
 * parameter in generic PWM implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: PWM peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency
 *
 * @tparam BASE_ADDR PWM peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70PWMHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = pwm0::PWM0_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint8_t NUM_CHANNELS = 4;  ///< Number of PWM channels

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
        #ifdef ALLOY_PWM_MOCK_HW
            return ALLOY_PWM_MOCK_HW();  // Test hook
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
     * @brief Enable PWM channel
     * @param channel Channel number (0-3)
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_ENABLE
     */
    static inline void enable_channel(uint8_t channel) {
        #ifdef ALLOY_PWM_TEST_HOOK_ENABLE
            ALLOY_PWM_TEST_HOOK_ENABLE(channel);
        #endif

        hw()->ENA = (1u << channel);
    }

    /**
     * @brief Disable PWM channel
     * @param channel Channel number (0-3)
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_DISABLE
     */
    static inline void disable_channel(uint8_t channel) {
        #ifdef ALLOY_PWM_TEST_HOOK_DISABLE
            ALLOY_PWM_TEST_HOOK_DISABLE(channel);
        #endif

        hw()->DIS = (1u << channel);
    }

    /**
     * @brief Set PWM period
     * @param channel Channel number (0-3)
     * @param period Period value
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_PERIOD
     */
    static inline void set_period(uint8_t channel, uint32_t period) {
        #ifdef ALLOY_PWM_TEST_HOOK_PERIOD
            ALLOY_PWM_TEST_HOOK_PERIOD(channel, period);
        #endif

        if (channel < 4) { volatile uint32_t* cprd = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x200 + (channel * 0x20) + 0x0C); *cprd = period; }
    }

    /**
     * @brief Set PWM duty cycle
     * @param channel Channel number (0-3)
     * @param duty Duty cycle value
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_DUTY
     */
    static inline void set_duty_cycle(uint8_t channel, uint32_t duty) {
        #ifdef ALLOY_PWM_TEST_HOOK_DUTY
            ALLOY_PWM_TEST_HOOK_DUTY(channel, duty);
        #endif

        if (channel < 4) { volatile uint32_t* cdty = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x200 + (channel * 0x20) + 0x04); *cdty = duty; }
    }

    /**
     * @brief Set PWM clock prescaler
     * @param prescaler Prescaler value (0-10)
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_PRESCALER
     */
    static inline void set_prescaler(uint8_t prescaler) {
        #ifdef ALLOY_PWM_TEST_HOOK_PRESCALER
            ALLOY_PWM_TEST_HOOK_PRESCALER(prescaler);
        #endif

        hw()->CLK = (hw()->CLK & ~0x0FF) | (prescaler & 0xFF);
    }

    /**
     * @brief Set PWM output polarity
     * @param channel Channel number (0-3)
     * @param inverted true for inverted polarity
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_POLARITY
     */
    static inline void set_polarity(uint8_t channel, bool inverted) {
        #ifdef ALLOY_PWM_TEST_HOOK_POLARITY
            ALLOY_PWM_TEST_HOOK_POLARITY(channel, inverted);
        #endif

        if (channel < 4) { volatile uint32_t* cmr = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x200 + (channel * 0x20)); if (inverted) { *cmr |= (1u << 9); } else { *cmr &= ~(1u << 9); } }
    }

    /**
     * @brief Check if PWM channel is enabled
     * @param channel Channel number (0-3)
     * @return bool
     *
     * @note Test hook: ALLOY_PWM_TEST_HOOK_IS_ENABLED
     */
    static inline bool is_channel_enabled(uint8_t channel) const {
        #ifdef ALLOY_PWM_TEST_HOOK_IS_ENABLED
            ALLOY_PWM_TEST_HOOK_IS_ENABLED(channel);
        #endif

        return (hw()->SR & (1u << channel)) != 0;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Pwm0
using Pwm0Hardware = Same70PWMHardwarePolicy<alloy::generated::atsame70q21b::peripherals::PWM0, 150000000>;
/// @brief Hardware policy for Pwm1
using Pwm1Hardware = Same70PWMHardwarePolicy<alloy::generated::atsame70q21b::peripherals::PWM1, 150000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic PWM API:
 *
 * @code
 * #include "hal/api/pwm_simple.hpp"
 * #include "hal/platform/same70/pwm.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create PWM with hardware policy
 * using Instance0 = Pwm0Hardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */