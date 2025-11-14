/**
 * @file adc_hardware_policy.hpp
 * @brief Hardware Policy for ADC on STM32G0 (Policy-Based Design)
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
 * Auto-generated from: stm32g0/adc.json
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
#include "hal/vendors/st/stm32g0/registers/adc_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/bitfields/adc_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::adc;

/**
 * @brief Hardware Policy for ADC on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for ADC. It is designed to be used as a template
 * parameter in generic ADC implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0ADCHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = ADC_Registers;

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
        #ifdef ALLOY_ADC_MOCK_HW
            return ALLOY_ADC_MOCK_HW();  // Test hook
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
     * @brief Enable ADC
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADEN
     */
    static inline void enable_adc() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADEN
            ALLOY_ADC_TEST_HOOK_ADEN();
        #endif

        hw()->ADC_CR |= (1U << 0);
    }

    /**
     * @brief Disable ADC
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADDIS
     */
    static inline void disable_adc() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADDIS
            ALLOY_ADC_TEST_HOOK_ADDIS();
        #endif

        hw()->ADC_CR |= (1U << 1);
    }

    /**
     * @brief Start ADC conversion
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADSTART
     */
    static inline void start_conversion() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADSTART
            ALLOY_ADC_TEST_HOOK_ADSTART();
        #endif

        hw()->ADC_CR |= (1U << 2);
    }

    /**
     * @brief Stop ADC conversion
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADSTP
     */
    static inline void stop_conversion() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADSTP
            ALLOY_ADC_TEST_HOOK_ADSTP();
        #endif

        hw()->ADC_CR |= (1U << 4);
    }

    /**
     * @brief Start ADC calibration
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADCAL
     */
    static inline void calibrate() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADCAL
            ALLOY_ADC_TEST_HOOK_ADCAL();
        #endif

        hw()->ADC_CR |= (1U << 31);
    }

    /**
     * @brief Set 12-bit resolution
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_RES
     */
    static inline void set_resolution_12bit() {
        #ifdef ALLOY_ADC_TEST_HOOK_RES
            ALLOY_ADC_TEST_HOOK_RES();
        #endif

        hw()->ADC_CFGR1 &= ~(0x3U << 3);
    }

    /**
     * @brief Set 10-bit resolution
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_RES
     */
    static inline void set_resolution_10bit() {
        #ifdef ALLOY_ADC_TEST_HOOK_RES
            ALLOY_ADC_TEST_HOOK_RES();
        #endif

        hw()->ADC_CFGR1 = (hw()->ADC_CFGR1 & ~(0x3U << 3)) | (0x1U << 3);
    }

    /**
     * @brief Set 8-bit resolution
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_RES
     */
    static inline void set_resolution_8bit() {
        #ifdef ALLOY_ADC_TEST_HOOK_RES
            ALLOY_ADC_TEST_HOOK_RES();
        #endif

        hw()->ADC_CFGR1 = (hw()->ADC_CFGR1 & ~(0x3U << 3)) | (0x2U << 3);
    }

    /**
     * @brief Enable continuous conversion mode
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_CONT
     */
    static inline void set_continuous_mode() {
        #ifdef ALLOY_ADC_TEST_HOOK_CONT
            ALLOY_ADC_TEST_HOOK_CONT();
        #endif

        hw()->ADC_CFGR1 |= (1U << 13);
    }

    /**
     * @brief Enable single conversion mode
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_CONT
     */
    static inline void set_single_mode() {
        #ifdef ALLOY_ADC_TEST_HOOK_CONT
            ALLOY_ADC_TEST_HOOK_CONT();
        #endif

        hw()->ADC_CFGR1 &= ~(1U << 13);
    }

    /**
     * @brief Select ADC channel
     * @param channel Channel number (0-18)
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_CHSEL
     */
    static inline void select_channel(uint8_t channel) {
        #ifdef ALLOY_ADC_TEST_HOOK_CHSEL
            ALLOY_ADC_TEST_HOOK_CHSEL(channel);
        #endif

        hw()->ADC_CHSELR = (1U << channel);
    }

    /**
     * @brief Read conversion result
     * @return uint16_t
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_DR
     */
    static inline uint16_t read_data() {
        #ifdef ALLOY_ADC_TEST_HOOK_DR
            ALLOY_ADC_TEST_HOOK_DR();
        #endif

        return static_cast<uint16_t>(hw()->ADC_DR & 0xFFFF);
    }

    /**
     * @brief Check if conversion is complete
     * @return bool
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_EOC
     */
    static inline bool is_conversion_complete() {
        #ifdef ALLOY_ADC_TEST_HOOK_EOC
            ALLOY_ADC_TEST_HOOK_EOC();
        #endif

        return (hw()->ADC_ISR & (1U << 2)) != 0;
    }

    /**
     * @brief Check if ADC is ready
     * @return bool
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADRDY
     */
    static inline bool is_ready() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADRDY
            ALLOY_ADC_TEST_HOOK_ADRDY();
        #endif

        return (hw()->ADC_ISR & (1U << 0)) != 0;
    }

    /**
     * @brief Clear ADC ready flag
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_ADRDY
     */
    static inline void clear_ready_flag() {
        #ifdef ALLOY_ADC_TEST_HOOK_ADRDY
            ALLOY_ADC_TEST_HOOK_ADRDY();
        #endif

        hw()->ADC_ISR = (1U << 0);
    }

    /**
     * @brief Enable DMA mode
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_DMAEN
     */
    static inline void enable_dma() {
        #ifdef ALLOY_ADC_TEST_HOOK_DMAEN
            ALLOY_ADC_TEST_HOOK_DMAEN();
        #endif

        hw()->ADC_CFGR1 |= (1U << 0);
    }

    /**
     * @brief Disable DMA mode
     *
     * @note Test hook: ALLOY_ADC_TEST_HOOK_DMAEN
     */
    static inline void disable_dma() {
        #ifdef ALLOY_ADC_TEST_HOOK_DMAEN
            ALLOY_ADC_TEST_HOOK_DMAEN();
        #endif

        hw()->ADC_CFGR1 &= ~(1U << 0);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic ADC API:
 *
 * @code
 * #include "hal/api/adc_simple.hpp"
 * #include "hal/vendors/st/stm32g0/adc_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create ADC with hardware policy
 * using Uart0 = UartImpl<Stm32g0ADCHardwarePolicy<UART0_BASE, 150000000>>;
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