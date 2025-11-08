/**
 * @file adc.hpp
 * @brief Template-based ADC implementation for STM32F4 (Platform Layer)
 *
 * This file implements ADC peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Error handling: Uses Result<T, ErrorCode> for robust error handling
 * - DMA support: Can work with DMA for continuous high-speed conversion
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: stm32f4
 * Generator: generate_platform_adc.py
 * Generated: 2025-11-07 17:56:40
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "hal/types.hpp"

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/st/stm32f4/registers/adc1_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/st/stm32f4/bitfields/adc1_bitfields.hpp"


namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfield access
namespace adc = alloy::hal::st::stm32f4::adc1;

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief ADC Resolution
 */
enum class AdcResolution : uint8_t {
    Bits12 = 0,  ///< 12-bit resolution (default)
    Bits10 = 1,  ///< 10-bit resolution
    Bits8 = 2,   ///< 8-bit resolution
    Bits6 = 3,   ///< 6-bit resolution
};

/**
 * @brief ADC Channel Number
 */
enum class AdcChannel : uint8_t {
    CH0 = 0,    ///< Channel 0
    CH1 = 1,    ///< Channel 1
    CH2 = 2,    ///< Channel 2
    CH3 = 3,    ///< Channel 3
    CH4 = 4,    ///< Channel 4
    CH5 = 5,    ///< Channel 5
    CH6 = 6,    ///< Channel 6
    CH7 = 7,    ///< Channel 7
    CH8 = 8,    ///< Channel 8
    CH9 = 9,    ///< Channel 9
    CH10 = 10,  ///< Channel 10
    CH11 = 11,  ///< Channel 11
    CH12 = 12,  ///< Channel 12
    CH13 = 13,  ///< Channel 13
    CH14 = 14,  ///< Channel 14
    CH15 = 15,  ///< Channel 15
    CH16 = 16,  ///< Channel 16 (Temperature sensor)
    CH17 = 17,  ///< Channel 17 (Vrefint)
    CH18 = 18,  ///< Channel 18 (Vbat)
};


/**
 * @brief ADC configuration structure
 */
struct AdcConfig {
    AdcResolution resolution = AdcResolution::Bits12;  ///< ADC resolution
    uint8_t sample_time = 3;  ///< Sample time (0-7: 3, 15, 28, 56, 84, 112, 144, 480 cycles)
};

/**
 * @brief Template-based ADC peripheral for STM32F4
 *
 * This class provides a template-based ADC implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: ADC peripheral base address
 * - IRQ_ID: ADC IRQ ID
 *
 * Example usage:
 * @code
 * // Basic ADC usage
 * using MyAdc = Adc<ADC1_BASE, ADC1_IRQ>;
 * auto adc = MyAdc{};
 * AdcConfig config;
 * adc.open();
 * adc.configure(config);
 * auto result = adc.readSingle(AdcChannel::CH0);
 * if (result.is_ok()) {
 *     uint16_t value = result.ok();
 *     uint32_t voltage_mv = MyAdc::toVoltage(value);
 * }
 * @endcode
 *
 * @tparam BASE_ADDR ADC peripheral base address
 * @tparam IRQ_ID ADC IRQ ID
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Adc {
   public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;


    /**
     * @brief Get ADC peripheral registers
     *
     * Returns pointer to ADC registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::st::stm32f4::adc1::ADC1_Registers* get_hw() {
#ifdef ALLOY_ADC_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_ADC_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::st::stm32f4::adc1::ADC1_Registers*>(BASE_ADDR);
#endif
    }

    constexpr Adc() = default;

    /**
     * @brief Initialize ADC peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable ADC clock (RCC)
        // TODO: Enable peripheral clock via RCC

        // Enable ADC
        hw->CR2 = adc::cr2::ADON::mask;

        m_opened = true;

        return Ok();
    }

    /**
     * @brief Close ADC peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable ADC
        hw->CR2 &= ~adc::cr2::ADON::mask;

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure ADC
     *
     * @param config ADC configuration
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configure(const AdcConfig& config) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Configure resolution in CR1
        uint32_t cr1 = hw->CR1 & ~adc::cr1::RES::mask;
        cr1 = adc::cr1::RES::write(cr1, static_cast<uint32_t>(config.resolution));
        hw->CR1 = cr1;

        //
        m_config = config;

        return Ok();
    }

    /**
     * @brief Enable ADC channel
     *
     * @param channel Channel to enable
     * @return Result<void, ErrorCode>     *
     * @note Configures the channel for single conversion
     */
    Result<void, ErrorCode> enableChannel(AdcChannel channel) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Set sequence length to 1 and channel in SQR3
        uint8_t ch = static_cast<uint8_t>(channel);

        // Set sequence length to 1 conversion
        uint32_t sqr1 = hw->SQR1 & ~adc::sqr1::L::mask;
        sqr1 = adc::sqr1::L::write(sqr1, 0);  // 1 conversion (L=0)
        hw->SQR1 = sqr1;

        // Set first conversion channel in SQR3
        uint32_t sqr3 = hw->SQR3 & ~adc::sqr3::SQ1::mask;
        sqr3 = adc::sqr3::SQ1::write(sqr3, ch);
        hw->SQR3 = sqr3;

        // Set sample time for this channel
        if (ch < 10) {
            // CH0-CH9: SMPR2
            uint32_t shift = ch * 3;
            uint32_t smpr2 = hw->SMPR2 & ~(0x7 << shift);
            smpr2 |= (m_config.sample_time << shift);
            hw->SMPR2 = smpr2;
        } else {
            // CH10-CH18: SMPR1
            uint32_t shift = (ch - 10) * 3;
            uint32_t smpr1 = hw->SMPR1 & ~(0x7 << shift);
            smpr1 |= (m_config.sample_time << shift);
            hw->SMPR1 = smpr1;
        }

        return Ok();
    }

    /**
     * @brief Start ADC conversion (software trigger)
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> startConversion() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Start regular conversion
        hw->CR2 |= adc::cr2::SWSTART::mask;

        return Ok();
    }

    /**
     * @brief Read converted value
     *
     * @return Result<uint16_t, ErrorCode> Read converted value     *
     * @note Waits for conversion to complete and reads the data register
     */
    Result<uint16_t, ErrorCode> read() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        // Wait for end of conversion (EOC)
        uint32_t timeout = 100000;
        while ((hw->SR & adc::sr::EOC::mask) == 0 && timeout > 0) {
            --timeout;
        }

        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        // Read data register (clears EOC flag)
        uint16_t value = static_cast<uint16_t>(hw->DR & 0xFFFF);

        return Ok(uint16_t(value));
    }

    /**
     * @brief Read single channel (enable, start, read)
     *
     * @param channel Channel to read
     * @return Result<uint16_t, ErrorCode>     *
     * @note Convenience method for one-shot readings
     */
    Result<uint16_t, ErrorCode> readSingle(AdcChannel channel) {
        auto* hw = get_hw();

        //
        auto enable_result = enableChannel(channel);
        if (!enable_result.is_ok()) {
            return Err(enable_result.err());
        }

        auto start_result = startConversion();
        if (!start_result.is_ok()) {
            return Err(start_result.err());
        }

        return read();
    }

    /**
     * @brief Check if ADC is open
     *
     * @return bool Check if ADC is open     */
    bool isOpen() const { return m_opened; }

    // ========================================================================
    // Static Utility Methods
    // ========================================================================

    /**
     * @brief Convert ADC value to voltage
     *
     * @param adc_value ADC reading
     * @param vref_mv Reference voltage in millivolts
     * @param resolution ADC resolution
     * @return uint32_t
     */
    static constexpr uint32_t toVoltage(uint16_t adc_value, uint32_t vref_mv = 3300,
                                        AdcResolution resolution = AdcResolution::Bits12) {
        uint32_t max_value = (1u << (12 - (static_cast<uint32_t>(resolution) * 2))) - 1;
        return (static_cast<uint32_t>(adc_value) * vref_mv) / max_value;
    }


   private:
    bool m_opened = false;    ///< Tracks if peripheral is initialized
    AdcConfig m_config = {};  ///< Current configuration
};

// ==============================================================================
// Predefined ADC Instances
// ==============================================================================

constexpr uint32_t ADC1_BASE = 0x40012000;
constexpr uint32_t ADC1_IRQ = 18;

constexpr uint32_t ADC2_BASE = 0x40012100;
constexpr uint32_t ADC2_IRQ = 18;

constexpr uint32_t ADC3_BASE = 0x40012200;
constexpr uint32_t ADC3_IRQ = 18;

using Adc1 = Adc<ADC1_BASE, ADC1_IRQ>;  ///< ADC1 - 19 channels (CH0-CH18)
using Adc2 = Adc<ADC2_BASE, ADC2_IRQ>;  ///< ADC2 - 19 channels (CH0-CH18)
using Adc3 = Adc<ADC3_BASE, ADC3_IRQ>;  ///< ADC3 - 19 channels (CH0-CH18)

}  // namespace alloy::hal::stm32f4
