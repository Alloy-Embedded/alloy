/**
 * @file nvic_hardware_policy.hpp
 * @brief Hardware Policy for NVIC on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for NVIC using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_NVIC_MOCK_HW)
 *
 * Auto-generated from: same70/nvic.json
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
#include "hal/vendors/arm/cortex_m7/nvic_registers.hpp"

// Bitfield definitions
#include "hal/vendors/arm/cortex_m7/nvic_bitfields.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::arm::cortex_m7;

// Namespace alias for bitfields
namespace nvic = alloy::hal::arm::cortex_m7;

/**
 * @brief Hardware Policy for NVIC on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for NVIC. It is designed to be used as a template
 * parameter in generic NVIC implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: NVIC base address (0xE000E100)
 * - CPU_CLOCK_HZ: CPU clock frequency (unused, for compatibility)
 *
 * @tparam BASE_ADDR NVIC base address (0xE000E100)
 * @tparam CPU_CLOCK_HZ CPU clock frequency (unused, for compatibility)
 */
template <uint32_t BASE_ADDR, uint32_t CPU_CLOCK_HZ>
struct Same70NVICHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = cortex_m7::NVIC_Type;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t cpu_clock_hz = CPU_CLOCK_HZ;
    static constexpr uint32_t NVIC_BASE = 0xE000E100;  ///< NVIC base address in ARM Cortex-M

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
     * @brief Enable interrupt
     * @param irq_num IRQ number (0-239)
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_ENABLE
     */
    static inline void enable_irq(uint8_t irq_num) {
        #ifdef ALLOY_NVIC_TEST_HOOK_ENABLE
            ALLOY_NVIC_TEST_HOOK_ENABLE(irq_num);
        #endif

        if (irq_num < 240) { hw()->ISER[irq_num / 32] = (1u << (irq_num % 32)); }
    }

    /**
     * @brief Disable interrupt
     * @param irq_num IRQ number (0-239)
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_DISABLE
     */
    static inline void disable_irq(uint8_t irq_num) {
        #ifdef ALLOY_NVIC_TEST_HOOK_DISABLE
            ALLOY_NVIC_TEST_HOOK_DISABLE(irq_num);
        #endif

        if (irq_num < 240) { hw()->ICER[irq_num / 32] = (1u << (irq_num % 32)); }
    }

    /**
     * @brief Set interrupt pending
     * @param irq_num IRQ number (0-239)
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_SET_PEND
     */
    static inline void set_pending(uint8_t irq_num) {
        #ifdef ALLOY_NVIC_TEST_HOOK_SET_PEND
            ALLOY_NVIC_TEST_HOOK_SET_PEND(irq_num);
        #endif

        if (irq_num < 240) { hw()->ISPR[irq_num / 32] = (1u << (irq_num % 32)); }
    }

    /**
     * @brief Clear interrupt pending
     * @param irq_num IRQ number (0-239)
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_CLR_PEND
     */
    static inline void clear_pending(uint8_t irq_num) {
        #ifdef ALLOY_NVIC_TEST_HOOK_CLR_PEND
            ALLOY_NVIC_TEST_HOOK_CLR_PEND(irq_num);
        #endif

        if (irq_num < 240) { hw()->ICPR[irq_num / 32] = (1u << (irq_num % 32)); }
    }

    /**
     * @brief Check if interrupt is pending
     * @param irq_num IRQ number (0-239)
     * @return bool
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_IS_PEND
     */
    static inline bool is_pending(uint8_t irq_num) const {
        #ifdef ALLOY_NVIC_TEST_HOOK_IS_PEND
            ALLOY_NVIC_TEST_HOOK_IS_PEND(irq_num);
        #endif

        if (irq_num < 240) { return (hw()->ISPR[irq_num / 32] & (1u << (irq_num % 32))) != 0; } return false;
    }

    /**
     * @brief Check if interrupt is active
     * @param irq_num IRQ number (0-239)
     * @return bool
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_IS_ACTIVE
     */
    static inline bool is_active(uint8_t irq_num) const {
        #ifdef ALLOY_NVIC_TEST_HOOK_IS_ACTIVE
            ALLOY_NVIC_TEST_HOOK_IS_ACTIVE(irq_num);
        #endif

        if (irq_num < 240) { return (hw()->IABR[irq_num / 32] & (1u << (irq_num % 32))) != 0; } return false;
    }

    /**
     * @brief Set interrupt priority
     * @param irq_num IRQ number (0-239)
     * @param priority Priority (0-15, lower is higher priority)
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_SET_PRIO
     */
    static inline void set_priority(uint8_t irq_num, uint8_t priority) {
        #ifdef ALLOY_NVIC_TEST_HOOK_SET_PRIO
            ALLOY_NVIC_TEST_HOOK_SET_PRIO(irq_num, priority);
        #endif

        if (irq_num < 240) { hw()->IP[irq_num] = (priority << 4) & 0xF0; }
    }

    /**
     * @brief Get interrupt priority
     * @param irq_num IRQ number (0-239)
     * @return uint8_t
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_GET_PRIO
     */
    static inline uint8_t get_priority(uint8_t irq_num) const {
        #ifdef ALLOY_NVIC_TEST_HOOK_GET_PRIO
            ALLOY_NVIC_TEST_HOOK_GET_PRIO(irq_num);
        #endif

        if (irq_num < 240) { return (hw()->IP[irq_num] >> 4) & 0x0F; } return 0;
    }

    /**
     * @brief Trigger system reset
     *
     * @note Test hook: ALLOY_NVIC_TEST_HOOK_RESET
     */
    static inline void system_reset() {
        #ifdef ALLOY_NVIC_TEST_HOOK_RESET
            ALLOY_NVIC_TEST_HOOK_RESET();
        #endif

        volatile uint32_t* AIRCR = reinterpret_cast<volatile uint32_t*>(0xE000ED0C); *AIRCR = (0x05FA << 16) | (1u << 2);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Nvic
using NvicHardware = Same70NVICHardwarePolicy<0xE000E100, 300000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic NVIC API:
 *
 * @code
 * #include "hal/api/nvic_simple.hpp"
 * #include "hal/platform/same70/nvic.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create NVIC with hardware policy
 * using Instance0 = NvicHardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */