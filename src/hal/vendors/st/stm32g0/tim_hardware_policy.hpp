/**
 * @file tim_hardware_policy.hpp
 * @brief Hardware Policy for TIM on STM32G0 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for TIM using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_TIM_MOCK_HW)
 *
 * Auto-generated from: stm32g0/tim.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-14 17:36:48
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
#include "hal/vendors/st/stm32g0/registers/tim1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/bitfields/tim1_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::tim1;

/**
 * @brief Hardware Policy for TIM on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for TIM. It is designed to be used as a template
 * parameter in generic TIM implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0TIMHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = TIM1_Registers;

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
        #ifdef ALLOY_TIM_MOCK_HW
            return ALLOY_TIM_MOCK_HW();  // Test hook
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
     * @brief Enable timer counter
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CEN
     */
    static inline void enable_counter() {
        #ifdef ALLOY_TIM_TEST_HOOK_CEN
            ALLOY_TIM_TEST_HOOK_CEN();
        #endif

        hw()->TIM1_CR1 |= (1U << 0);
    }

    /**
     * @brief Disable timer counter
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CEN
     */
    static inline void disable_counter() {
        #ifdef ALLOY_TIM_TEST_HOOK_CEN
            ALLOY_TIM_TEST_HOOK_CEN();
        #endif

        hw()->TIM1_CR1 &= ~(1U << 0);
    }

    /**
     * @brief Set prescaler value (actual division = PSC + 1)
     * @param prescaler Prescaler value (0-65535)
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_PSC
     */
    static inline void set_prescaler(uint16_t prescaler) {
        #ifdef ALLOY_TIM_TEST_HOOK_PSC
            ALLOY_TIM_TEST_HOOK_PSC(prescaler);
        #endif

        hw()->TIM1_PSC = prescaler;
    }

    /**
     * @brief Set auto-reload value (period)
     * @param value Auto-reload value
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_ARR
     */
    static inline void set_auto_reload(uint32_t value) {
        #ifdef ALLOY_TIM_TEST_HOOK_ARR
            ALLOY_TIM_TEST_HOOK_ARR(value);
        #endif

        hw()->TIM1_ARR = value;
    }

    /**
     * @brief Get current counter value
     * @return uint32_t
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CNT
     */
    static inline uint32_t get_counter() {
        #ifdef ALLOY_TIM_TEST_HOOK_CNT
            ALLOY_TIM_TEST_HOOK_CNT();
        #endif

        return hw()->TIM1_CNT;
    }

    /**
     * @brief Set counter value
     * @param value Counter value
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CNT
     */
    static inline void set_counter(uint32_t value) {
        #ifdef ALLOY_TIM_TEST_HOOK_CNT
            ALLOY_TIM_TEST_HOOK_CNT(value);
        #endif

        hw()->TIM1_CNT = value;
    }

    /**
     * @brief Enable update interrupt
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_UIE
     */
    static inline void enable_update_interrupt() {
        #ifdef ALLOY_TIM_TEST_HOOK_UIE
            ALLOY_TIM_TEST_HOOK_UIE();
        #endif

        hw()->TIM1_DIER |= (1U << 0);
    }

    /**
     * @brief Disable update interrupt
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_UIE
     */
    static inline void disable_update_interrupt() {
        #ifdef ALLOY_TIM_TEST_HOOK_UIE
            ALLOY_TIM_TEST_HOOK_UIE();
        #endif

        hw()->TIM1_DIER &= ~(1U << 0);
    }

    /**
     * @brief Check if update interrupt flag is set
     * @return bool
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_UIF
     */
    static inline bool is_update_flag_set() {
        #ifdef ALLOY_TIM_TEST_HOOK_UIF
            ALLOY_TIM_TEST_HOOK_UIF();
        #endif

        return (hw()->TIM1_SR & (1U << 0)) != 0;
    }

    /**
     * @brief Clear update interrupt flag
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_UIF
     */
    static inline void clear_update_flag() {
        #ifdef ALLOY_TIM_TEST_HOOK_UIF
            ALLOY_TIM_TEST_HOOK_UIF();
        #endif

        hw()->TIM1_SR &= ~(1U << 0);
    }

    /**
     * @brief Set PWM mode 1 for channel 1
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_OC1M
     */
    static inline void set_pwm_mode_1() {
        #ifdef ALLOY_TIM_TEST_HOOK_OC1M
            ALLOY_TIM_TEST_HOOK_OC1M();
        #endif

        hw()->TIM1_CCMR1_OUTPUT = (hw()->TIM1_CCMR1_OUTPUT & ~(0x7U << 4)) | (0x6U << 4);
    }

    /**
     * @brief Set PWM mode 2 for channel 1
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_OC1M
     */
    static inline void set_pwm_mode_2() {
        #ifdef ALLOY_TIM_TEST_HOOK_OC1M
            ALLOY_TIM_TEST_HOOK_OC1M();
        #endif

        hw()->TIM1_CCMR1_OUTPUT = (hw()->TIM1_CCMR1_OUTPUT & ~(0x7U << 4)) | (0x7U << 4);
    }

    /**
     * @brief Enable channel 1 output
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CC1E
     */
    static inline void enable_channel_1_output() {
        #ifdef ALLOY_TIM_TEST_HOOK_CC1E
            ALLOY_TIM_TEST_HOOK_CC1E();
        #endif

        hw()->TIM1_CCER |= (1U << 0);
    }

    /**
     * @brief Disable channel 1 output
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CC1E
     */
    static inline void disable_channel_1_output() {
        #ifdef ALLOY_TIM_TEST_HOOK_CC1E
            ALLOY_TIM_TEST_HOOK_CC1E();
        #endif

        hw()->TIM1_CCER &= ~(1U << 0);
    }

    /**
     * @brief Set compare value for channel 1 (duty cycle)
     * @param value Compare value
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CCR1
     */
    static inline void set_compare_1(uint32_t value) {
        #ifdef ALLOY_TIM_TEST_HOOK_CCR1
            ALLOY_TIM_TEST_HOOK_CCR1(value);
        #endif

        hw()->TIM1_CCR1 = value;
    }

    /**
     * @brief Set compare value for channel 2
     * @param value Compare value
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CCR2
     */
    static inline void set_compare_2(uint32_t value) {
        #ifdef ALLOY_TIM_TEST_HOOK_CCR2
            ALLOY_TIM_TEST_HOOK_CCR2(value);
        #endif

        hw()->TIM1_CCR2 = value;
    }

    /**
     * @brief Set compare value for channel 3
     * @param value Compare value
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CCR3
     */
    static inline void set_compare_3(uint32_t value) {
        #ifdef ALLOY_TIM_TEST_HOOK_CCR3
            ALLOY_TIM_TEST_HOOK_CCR3(value);
        #endif

        hw()->TIM1_CCR3 = value;
    }

    /**
     * @brief Set compare value for channel 4
     * @param value Compare value
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_CCR4
     */
    static inline void set_compare_4(uint32_t value) {
        #ifdef ALLOY_TIM_TEST_HOOK_CCR4
            ALLOY_TIM_TEST_HOOK_CCR4(value);
        #endif

        hw()->TIM1_CCR4 = value;
    }

    /**
     * @brief Enable auto-reload preload
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_ARPE
     */
    static inline void enable_auto_reload_preload() {
        #ifdef ALLOY_TIM_TEST_HOOK_ARPE
            ALLOY_TIM_TEST_HOOK_ARPE();
        #endif

        hw()->TIM1_CR1 |= (1U << 7);
    }

    /**
     * @brief Disable auto-reload preload
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_ARPE
     */
    static inline void disable_auto_reload_preload() {
        #ifdef ALLOY_TIM_TEST_HOOK_ARPE
            ALLOY_TIM_TEST_HOOK_ARPE();
        #endif

        hw()->TIM1_CR1 &= ~(1U << 7);
    }

    /**
     * @brief Generate update event to reload registers
     *
     * @note Test hook: ALLOY_TIM_TEST_HOOK_UG
     */
    static inline void generate_update_event() {
        #ifdef ALLOY_TIM_TEST_HOOK_UG
            ALLOY_TIM_TEST_HOOK_UG();
        #endif

        hw()->TIM1_EGR = (1U << 0);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic TIM API:
 *
 * @code
 * #include "hal/api/tim_simple.hpp"
 * #include "hal/vendors/st/stm32g0/tim_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create TIM with hardware policy
 * using Uart0 = UartImpl<Stm32g0TIMHardwarePolicy<UART0_BASE, 150000000>>;
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