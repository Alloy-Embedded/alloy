/**
 * @file adc_hardware_policy.hpp
 * @brief Hardware Policy for ADC on STM32F4 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for ADC using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32F4 ADC Features:
 * - 12-bit, 10-bit, 8-bit, or 6-bit resolution
 * - Up to 3 ADCs (ADC1, ADC2, ADC3)
 * - Up to 16 external channels per ADC
 * - Conversion rates up to 2.4 MSPS
 * - Single and continuous conversion modes
 * - DMA support for data transfer
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef MICROCORE_ADC_MOCK_HW)
 *
 * @note Part of Phase 3.5: ADC Implementation
 * @see docs/API_TIERS.md
 */

#pragma once

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// Register definitions
#include "hal/vendors/st/stm32f4/generated/registers/adc1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f4/generated/bitfields/adc1_bitfields.hpp"

namespace ucore::hal::stm32f4 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32f4;

// Namespace alias for bitfields
namespace adc = st::stm32f4::adc1;

/**
 * @brief Hardware Policy for ADC on STM32F4
 *
 * This policy provides all platform-specific hardware access methods
 * for ADC. It is designed to be used as a template parameter in
 * generic ADC implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: ADC peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using Adc1 = AdcImpl<Stm32f4AdcHardwarePolicy<0x40012000, 84000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f4AdcHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = ADC1_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t ADC_TIMEOUT =
        100000;  ///< ADC timeout in loop iterations (~10ms at 168MHz)

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
#ifdef MICROCORE_ADC_MOCK_HW
        return MICROCORE_ADC_MOCK_HW();  // Test hook
#else
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
#endif
    }

    // ========================================================================
    // Hardware Policy Methods
    // ========================================================================

    /**
     * @brief Enable ADC
     */
    static inline void enable_adc() {
        hw()->CR2 |= adc::cr2::ADON::mask;
    }

    /**
     * @brief Disable ADC
     */
    static inline void disable_adc() {
        hw()->CR2 &= ~adc::cr2::ADON::mask;
    }

    /**
     * @brief Start ADC conversion
     */
    static inline void start_conversion() {
        hw()->CR2 |= adc::cr2::SWSTART::mask;
    }

    /**
     * @brief Set 12-bit resolution
     */
    static inline void set_resolution_12bit() {
        hw()->CR1 &= ~adc::cr1::RES::mask;  // 00: 12-bit
    }

    /**
     * @brief Set 10-bit resolution
     */
    static inline void set_resolution_10bit() {
        hw()->CR1 = (hw()->CR1 & ~adc::cr1::RES::mask) | (0x1 << 24);  // 01: 10-bit
    }

    /**
     * @brief Set 8-bit resolution
     */
    static inline void set_resolution_8bit() {
        hw()->CR1 = (hw()->CR1 & ~adc::cr1::RES::mask) | (0x2 << 24);  // 10: 8-bit
    }

    /**
     * @brief Set 6-bit resolution
     */
    static inline void set_resolution_6bit() {
        hw()->CR1 = (hw()->CR1 & ~adc::cr1::RES::mask) | (0x3 << 24);  // 11: 6-bit
    }

    /**
     * @brief Enable continuous conversion mode
     */
    static inline void set_continuous_mode() {
        hw()->CR2 |= adc::cr2::CONT::mask;
    }

    /**
     * @brief Enable single conversion mode
     */
    static inline void set_single_mode() {
        hw()->CR2 &= ~adc::cr2::CONT::mask;
    }

    /**
     * @brief Select ADC channel for regular conversion
     *
     * @param channel Channel number (0-18)
     * @param sequence Sequence position (1-16)
     */
    static inline void select_channel(uint8_t channel, uint8_t sequence = 1) {
        if (sequence <= 6) {
            // SQR3: sequences 1-6
            uint32_t shift = (sequence - 1) * 5;
            hw()->SQR3 = (hw()->SQR3 & ~(0x1F << shift)) | ((channel & 0x1F) << shift);
        } else if (sequence <= 12) {
            // SQR2: sequences 7-12
            uint32_t shift = (sequence - 7) * 5;
            hw()->SQR2 = (hw()->SQR2 & ~(0x1F << shift)) | ((channel & 0x1F) << shift);
        } else if (sequence <= 16) {
            // SQR1: sequences 13-16
            uint32_t shift = (sequence - 13) * 5;
            hw()->SQR1 = (hw()->SQR1 & ~(0x1F << shift)) | ((channel & 0x1F) << shift);
        }
    }

    /**
     * @brief Set number of conversions in regular sequence
     *
     * @param num_conversions Number of conversions (1-16)
     */
    static inline void set_sequence_length(uint8_t num_conversions) {
        if (num_conversions > 0 && num_conversions <= 16) {
            hw()->SQR1 = (hw()->SQR1 & ~(0xF << 20)) | (((num_conversions - 1) & 0xF) << 20);
        }
    }

    /**
     * @brief Set sampling time for a channel
     *
     * Sampling time cycles:
     * - 0: 3 cycles
     * - 1: 15 cycles
     * - 2: 28 cycles
     * - 3: 56 cycles
     * - 4: 84 cycles
     * - 5: 112 cycles
     * - 6: 144 cycles
     * - 7: 480 cycles
     *
     * @param channel Channel number (0-18)
     * @param cycles Sampling time code (0-7)
     */
    static inline void set_sampling_time(uint8_t channel, uint8_t cycles) {
        if (channel <= 9) {
            // SMPR2: channels 0-9
            uint32_t shift = channel * 3;
            hw()->SMPR2 = (hw()->SMPR2 & ~(0x7 << shift)) | ((cycles & 0x7) << shift);
        } else if (channel <= 18) {
            // SMPR1: channels 10-18
            uint32_t shift = (channel - 10) * 3;
            hw()->SMPR1 = (hw()->SMPR1 & ~(0x7 << shift)) | ((cycles & 0x7) << shift);
        }
    }

    /**
     * @brief Read conversion result
     *
     * @return Conversion result (12-bit max)
     */
    static inline uint16_t read_data() {
        return static_cast<uint16_t>(hw()->DR & 0xFFFF);
    }

    /**
     * @brief Check if conversion is complete (EOC flag)
     *
     * @return true if conversion is complete
     */
    static inline bool is_conversion_complete() {
        return (hw()->SR & adc::sr::EOC::mask) != 0;
    }

    /**
     * @brief Clear end of conversion flag
     */
    static inline void clear_eoc_flag() {
        hw()->SR &= ~adc::sr::EOC::mask;
    }

    /**
     * @brief Enable DMA mode
     */
    static inline void enable_dma() {
        hw()->CR2 |= adc::cr2::DMA::mask;
    }

    /**
     * @brief Disable DMA mode
     */
    static inline void disable_dma() {
        hw()->CR2 &= ~adc::cr2::DMA::mask;
    }

    /**
     * @brief Enable DMA continuous requests
     */
    static inline void enable_dma_continuous() {
        hw()->CR2 |= adc::cr2::DDS::mask;
    }

    /**
     * @brief Disable DMA continuous requests
     */
    static inline void disable_dma_continuous() {
        hw()->CR2 &= ~adc::cr2::DDS::mask;
    }

    /**
     * @brief Enable end of conversion interrupt
     */
    static inline void enable_eoc_interrupt() {
        hw()->CR1 |= adc::cr1::EOCIE::mask;
    }

    /**
     * @brief Disable end of conversion interrupt
     */
    static inline void disable_eoc_interrupt() {
        hw()->CR1 &= ~adc::cr1::EOCIE::mask;
    }

    /**
     * @brief Wait for conversion to complete with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return true if conversion completed, false if timeout
     */
    static inline bool wait_for_conversion(uint32_t timeout_loops = ADC_TIMEOUT) {
        uint32_t timeout = timeout_loops;
        while (!is_conversion_complete() && --timeout)
            ;
        return timeout != 0;
    }
};

}  // namespace ucore::hal::stm32f4

/**
 * @example STM32F4 ADC Example
 * @code
 * #include "hal/platform/stm32f4/adc.hpp"
 *
 * using namespace ucore::hal::stm32f4;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto adc = Adc1::quick_setup<AdcPin>(AdcResolution::Bits12);
 *     adc.initialize();
 *
 *     // Read analog value from channel 0
 *     uint16_t value = adc.read_channel(0);
 *
 *     // Convert to voltage (assuming 3.3V reference)
 *     float voltage = (value * 3.3f) / 4095.0f;
 * }
 * @endcode
 */
