/**
 * @file systick_hardware_policy.hpp
 * @brief Hardware Policy for SysTick on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for SysTick using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_SYSTICK_MOCK_HW)
 *
 * Auto-generated from: same70/systick.json
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
#include "hal/vendors/arm/cortex_m7/systick_registers.hpp"

// Bitfield definitions
#include "hal/vendors/arm/cortex_m7/systick_bitfields.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::arm::cortex_m7;

// Namespace alias for bitfields
namespace systick = alloy::hal::arm::cortex_m7;

/**
 * @brief Hardware Policy for SysTick on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for SysTick. It is designed to be used as a template
 * parameter in generic SysTick implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: SysTick base address (0xE000E010)
 * - CPU_CLOCK_HZ: CPU clock frequency in Hz
 *
 * @tparam BASE_ADDR SysTick base address (0xE000E010)
 * @tparam CPU_CLOCK_HZ CPU clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t CPU_CLOCK_HZ>
struct Same70SysTickHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = cortex_m7::SysTick_Type;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t cpu_clock_hz = CPU_CLOCK_HZ;
    static constexpr uint32_t SYSTICK_BASE = 0xE000E010;  ///< SysTick base address in ARM Cortex-M

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
     * @brief Enable SysTick timer
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_ENABLE
     */
    static inline void enable() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_ENABLE
            ALLOY_SYSTICK_TEST_HOOK_ENABLE();
        #endif

        hw()->CTRL |= (1u << 0);
    }

    /**
     * @brief Disable SysTick timer
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_DISABLE
     */
    static inline void disable() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_DISABLE
            ALLOY_SYSTICK_TEST_HOOK_DISABLE();
        #endif

        hw()->CTRL &= ~(1u << 0);
    }

    /**
     * @brief Enable SysTick interrupt
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_INT_EN
     */
    static inline void enable_interrupt() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_INT_EN
            ALLOY_SYSTICK_TEST_HOOK_INT_EN();
        #endif

        hw()->CTRL |= (1u << 1);
    }

    /**
     * @brief Disable SysTick interrupt
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_INT_DIS
     */
    static inline void disable_interrupt() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_INT_DIS
            ALLOY_SYSTICK_TEST_HOOK_INT_DIS();
        #endif

        hw()->CTRL &= ~(1u << 1);
    }

    /**
     * @brief Set clock source to CPU clock
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_CLK_CPU
     */
    static inline void set_clock_source_cpu() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_CLK_CPU
            ALLOY_SYSTICK_TEST_HOOK_CLK_CPU();
        #endif

        hw()->CTRL |= (1u << 2);
    }

    /**
     * @brief Set clock source to external reference
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_CLK_EXT
     */
    static inline void set_clock_source_external() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_CLK_EXT
            ALLOY_SYSTICK_TEST_HOOK_CLK_EXT();
        #endif

        hw()->CTRL &= ~(1u << 2);
    }

    /**
     * @brief Set reload value (period)
     * @param value Reload value (24-bit, max 0xFFFFFF)
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_RELOAD
     */
    static inline void set_reload_value(uint32_t value) {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_RELOAD
            ALLOY_SYSTICK_TEST_HOOK_RELOAD(value);
        #endif

        hw()->LOAD = value & 0xFFFFFF;
    }

    /**
     * @brief Get current counter value
     * @return uint32_t
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_GET_VAL
     */
    static inline uint32_t get_current_value() const {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_GET_VAL
            ALLOY_SYSTICK_TEST_HOOK_GET_VAL();
        #endif

        return hw()->VAL;
    }

    /**
     * @brief Clear current counter value
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_CLEAR
     */
    static inline void clear_current_value() {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_CLEAR
            ALLOY_SYSTICK_TEST_HOOK_CLEAR();
        #endif

        hw()->VAL = 0;
    }

    /**
     * @brief Check if counter has counted to zero (COUNTFLAG)
     * @return bool
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_FLAG
     */
    static inline bool has_counted_to_zero() const {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_FLAG
            ALLOY_SYSTICK_TEST_HOOK_FLAG();
        #endif

        return (hw()->CTRL & (1u << 16)) != 0;
    }

    /**
     * @brief Configure SysTick for millisecond interrupts
     * @param ms Millisecond period
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_CONFIG_MS
     */
    static inline void configure_ms(uint32_t ms) {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_CONFIG_MS
            ALLOY_SYSTICK_TEST_HOOK_CONFIG_MS(ms);
        #endif

        uint32_t ticks = (CPU_CLOCK_HZ / 1000) * ms; if (ticks > 0xFFFFFF) ticks = 0xFFFFFF; hw()->LOAD = ticks - 1; hw()->VAL = 0; hw()->CTRL = 0x07;
    }

    /**
     * @brief Configure SysTick for microsecond interrupts
     * @param us Microsecond period
     *
     * @note Test hook: ALLOY_SYSTICK_TEST_HOOK_CONFIG_US
     */
    static inline void configure_us(uint32_t us) {
        #ifdef ALLOY_SYSTICK_TEST_HOOK_CONFIG_US
            ALLOY_SYSTICK_TEST_HOOK_CONFIG_US(us);
        #endif

        uint32_t ticks = (CPU_CLOCK_HZ / 1000000) * us; if (ticks > 0xFFFFFF) ticks = 0xFFFFFF; hw()->LOAD = ticks - 1; hw()->VAL = 0; hw()->CTRL = 0x07;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for SysTick
using SysTickHardware = Same70SysTickHardwarePolicy<0xE000E010, 300000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic SysTick API:
 *
 * @code
 * #include "hal/api/systick_simple.hpp"
 * #include "hal/platform/same70/systick.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create SysTick with hardware policy
 * using Instance0 = SysTickHardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */