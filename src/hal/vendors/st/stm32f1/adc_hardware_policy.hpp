/**
 * @file adc_hardware_policy.hpp
 * @brief Hardware Policy for ADC on STM32F1 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for ADC using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32F1 ADC Features:
 * - 12-bit resolution
 * - Up to 3 ADCs (ADC1, ADC2, ADC3 depending on device)
 * - Up to 18 channels (16 external + 2 internal)
 * - Conversion time: 1 µs at 56 MHz (1 MHz ADC clock)
 * - Single and continuous conversion modes
 * - DMA support for data transfer
 * - Dual mode operation (ADC1 and ADC2)
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
#include "hal/vendors/st/stm32f1/generated/registers/adc1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f1/generated/bitfields/adc1_bitfields.hpp"

namespace ucore::hal::stm32f1 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32f1;

// Namespace alias for bitfields
namespace adc = st::stm32f1::adc1;

/**
 * @brief Hardware Policy for ADC on STM32F1
 *
 * This policy provides all platform-specific hardware access methods
 * for ADC. It is designed to be used as a template parameter in
 * generic ADC implementations.
 *
 * The STM32F1 ADC peripheral has a simpler register layout compared to
 * STM32F4/F7 (no resolution selection - fixed at 12-bit).
 *
 * Template Parameters:
 * - BASE_ADDR: ADC peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using Adc1 = AdcImpl<Stm32f1AdcHardwarePolicy<0x40012400, 72000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f1AdcHardwarePolicy {
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
        100000;  ///< ADC timeout in loop iterations (~10ms at 72MHz)

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
     * @brief Calibrate ADC
     *
     * STM32F1 requires calibration after power-on.
     * Calibration takes about 83 ADC clock cycles.
     */
    static inline void calibrate() {
        // Start calibration
        hw()->CR2 |= adc::cr2::CAL::mask;

        // Wait for calibration to complete
        while (hw()->CR2 & adc::cr2::CAL::mask)
            ;
    }

    /**
     * @brief Reset calibration
     */
    static inline void reset_calibration() {
        hw()->CR2 |= adc::cr2::RSTCAL::mask;

        // Wait for reset to complete
        while (hw()->CR2 & adc::cr2::RSTCAL::mask)
            ;
    }

    /**
     * @brief Set 12-bit resolution (STM32F1 only supports 12-bit)
     *
     * This method is provided for API compatibility with STM32F4/F7.
     * STM32F1 only supports 12-bit resolution.
     */
    static inline void set_resolution_12bit() {
        // STM32F1 is always 12-bit, no configuration needed
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
     * @param channel Channel number (0-17)
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
     * - 0: 1.5 cycles
     * - 1: 7.5 cycles
     * - 2: 13.5 cycles
     * - 3: 28.5 cycles
     * - 4: 41.5 cycles
     * - 5: 55.5 cycles
     * - 6: 71.5 cycles
     * - 7: 239.5 cycles
     *
     * @param channel Channel number (0-17)
     * @param cycles Sampling time code (0-7)
     */
    static inline void set_sampling_time(uint8_t channel, uint8_t cycles) {
        if (channel <= 9) {
            // SMPR2: channels 0-9
            uint32_t shift = channel * 3;
            hw()->SMPR2 = (hw()->SMPR2 & ~(0x7 << shift)) | ((cycles & 0x7) << shift);
        } else if (channel <= 17) {
            // SMPR1: channels 10-17
            uint32_t shift = (channel - 10) * 3;
            hw()->SMPR1 = (hw()->SMPR1 & ~(0x7 << shift)) | ((cycles & 0x7) << shift);
        }
    }

    /**
     * @brief Read conversion result
     *
     * @return Conversion result (12-bit)
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

}  // namespace ucore::hal::stm32f1

/**
 * @example STM32F1 ADC Example
 * @code
 * #include "hal/platform/stm32f1/adc.hpp"
 *
 * using namespace ucore::hal::stm32f1;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto adc = Adc1::quick_setup<AdcPin>(AdcResolution::Bits12);
 *     adc.initialize();
 *
 *     // STM32F1 requires calibration
 *     adc.calibrate();
 *
 *     // Read analog value from channel 0
 *     uint16_t value = adc.read_channel(0);
 *
 *     // Convert to voltage (assuming 3.3V reference)
 *     float voltage = (value * 3.3f) / 4095.0f;
 * }
 * @endcode
 */
