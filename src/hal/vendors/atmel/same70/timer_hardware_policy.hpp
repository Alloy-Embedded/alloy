/**
 * @file timer_hardware_policy.hpp
 * @brief Hardware Policy for Timer on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for Timer using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_TIMER_MOCK_HW)
 *
 * Auto-generated from: same70/timer.json
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
#include "hal/vendors/atmel/same70/registers/tc0_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/bitfields/tc0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace tc = alloy::hal::atmel::same70::tc0;

/**
 * @brief Hardware Policy for Timer on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for Timer. It is designed to be used as a template
 * parameter in generic Timer implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: TC peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency
 *
 * @tparam BASE_ADDR TC peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70TimerHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = tc0::TC0_Registers;

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
        #ifdef ALLOY_TIMER_MOCK_HW
            return ALLOY_TIMER_MOCK_HW();  // Test hook
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
     * @brief Enable timer clock
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_ENABLE_CLOCK
     */
    static inline void enable_clock() {
        #ifdef ALLOY_TIMER_TEST_HOOK_ENABLE_CLOCK
            ALLOY_TIMER_TEST_HOOK_ENABLE_CLOCK();
        #endif

        hw()->CCR = tc::ccr::CLKEN::mask;
    }

    /**
     * @brief Disable timer clock
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_DISABLE_CLOCK
     */
    static inline void disable_clock() {
        #ifdef ALLOY_TIMER_TEST_HOOK_DISABLE_CLOCK
            ALLOY_TIMER_TEST_HOOK_DISABLE_CLOCK();
        #endif

        hw()->CCR = tc::ccr::CLKDIS::mask;
    }

    /**
     * @brief Start timer (software trigger)
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_START
     */
    static inline void start() {
        #ifdef ALLOY_TIMER_TEST_HOOK_START
            ALLOY_TIMER_TEST_HOOK_START();
        #endif

        hw()->CCR = tc::ccr::SWTRG::mask;
    }

    /**
     * @brief Stop timer
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_STOP
     */
    static inline void stop() {
        #ifdef ALLOY_TIMER_TEST_HOOK_STOP
            ALLOY_TIMER_TEST_HOOK_STOP();
        #endif

        hw()->CCR = tc::ccr::CLKDIS::mask;
    }

    /**
     * @brief Set waveform mode
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_WAVEFORM
     */
    static inline void set_waveform_mode() {
        #ifdef ALLOY_TIMER_TEST_HOOK_WAVEFORM
            ALLOY_TIMER_TEST_HOOK_WAVEFORM();
        #endif

        hw()->CMR_WAVEFORM_MODE |= tc::cmr_waveform_mode::WAVE::mask;
    }

    /**
     * @brief Set capture mode
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_CAPTURE
     */
    static inline void set_capture_mode() {
        #ifdef ALLOY_TIMER_TEST_HOOK_CAPTURE
            ALLOY_TIMER_TEST_HOOK_CAPTURE();
        #endif

        hw()->CMR_CAPTURE_MODE &= ~tc::cmr_waveform_mode::WAVE::mask;
    }

    /**
     * @brief Set clock source
     * @param clock_source Clock source (0-7)
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_CLOCK_SRC
     */
    static inline void set_clock_source(uint8_t clock_source) {
        #ifdef ALLOY_TIMER_TEST_HOOK_CLOCK_SRC
            ALLOY_TIMER_TEST_HOOK_CLOCK_SRC(clock_source);
        #endif

        hw()->CMR_WAVEFORM_MODE = (hw()->CMR_WAVEFORM_MODE & ~tc::cmr_waveform_mode::TCCLKS::mask) | tc::cmr_waveform_mode::TCCLKS::write(0, clock_source);
    }

    /**
     * @brief Set register A value
     * @param value Register A value
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_SET_RA
     */
    static inline void set_ra(uint32_t value) {
        #ifdef ALLOY_TIMER_TEST_HOOK_SET_RA
            ALLOY_TIMER_TEST_HOOK_SET_RA(value);
        #endif

        hw()->RA = value;
    }

    /**
     * @brief Set register B value
     * @param value Register B value
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_SET_RB
     */
    static inline void set_rb(uint32_t value) {
        #ifdef ALLOY_TIMER_TEST_HOOK_SET_RB
            ALLOY_TIMER_TEST_HOOK_SET_RB(value);
        #endif

        hw()->RB = value;
    }

    /**
     * @brief Set register C value (period)
     * @param value Register C value
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_SET_RC
     */
    static inline void set_rc(uint32_t value) {
        #ifdef ALLOY_TIMER_TEST_HOOK_SET_RC
            ALLOY_TIMER_TEST_HOOK_SET_RC(value);
        #endif

        hw()->RC = value;
    }

    /**
     * @brief Get counter value
     * @return uint32_t
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_GET_COUNT
     */
    static inline uint32_t get_counter() const {
        #ifdef ALLOY_TIMER_TEST_HOOK_GET_COUNT
            ALLOY_TIMER_TEST_HOOK_GET_COUNT();
        #endif

        return hw()->CV;
    }

    /**
     * @brief Check if counter overflow occurred
     * @return bool
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_OVERFLOW
     */
    static inline bool is_overflow() const {
        #ifdef ALLOY_TIMER_TEST_HOOK_OVERFLOW
            ALLOY_TIMER_TEST_HOOK_OVERFLOW();
        #endif

        return (hw()->SR & tc::sr::COVFS::mask) != 0;
    }

    /**
     * @brief Enable RC compare interrupt
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_INT_EN
     */
    static inline void enable_rc_interrupt() {
        #ifdef ALLOY_TIMER_TEST_HOOK_INT_EN
            ALLOY_TIMER_TEST_HOOK_INT_EN();
        #endif

        hw()->IER = tc::ier::CPCS::mask;
    }

    /**
     * @brief Disable RC compare interrupt
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_INT_DIS
     */
    static inline void disable_rc_interrupt() {
        #ifdef ALLOY_TIMER_TEST_HOOK_INT_DIS
            ALLOY_TIMER_TEST_HOOK_INT_DIS();
        #endif

        hw()->IDR = tc::idr::CPCS::mask;
    }

    /**
     * @brief Check if RC compare event occurred
     * @return bool
     *
     * @note Test hook: ALLOY_TIMER_TEST_HOOK_RC_CMP
     */
    static inline bool is_rc_compare() const {
        #ifdef ALLOY_TIMER_TEST_HOOK_RC_CMP
            ALLOY_TIMER_TEST_HOOK_RC_CMP();
        #endif

        return (hw()->SR & tc::sr::CPCS::mask) != 0;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Timer0Ch0
using Timer0Ch0Hardware = Same70TimerHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TC0, 150000000>;
/// @brief Hardware policy for Timer1Ch0
using Timer1Ch0Hardware = Same70TimerHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TC1, 150000000>;
/// @brief Hardware policy for Timer2Ch0
using Timer2Ch0Hardware = Same70TimerHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TC2, 150000000>;
/// @brief Hardware policy for Timer3Ch0
using Timer3Ch0Hardware = Same70TimerHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TC3, 150000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic Timer API:
 *
 * @code
 * #include "hal/api/timer_simple.hpp"
 * #include "hal/platform/same70/timer.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create Timer with hardware policy
 * using Instance0 = Timer0Ch0Hardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */