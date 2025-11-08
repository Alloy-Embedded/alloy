/**
 * @file adc.hpp
 * @brief Template-based ADC implementation for SAME70 (Platform Layer)
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
 * Auto-generated from: same70
 * Generator: generate_platform_adc.py
 * Generated: 2025-11-07 17:55:19
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/atmel/same70/registers/afec0_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/bitfields/afec0_bitfields.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace afec = alloy::hal::atmel::same70::afec0;

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief ADC Resolution
 */
enum class AdcResolution : uint8_t {
    Bits12 = 0,  ///< 12-bit resolution (default)
    Bits13 = 1,  ///< 13-bit with oversampling
};

/**
 * @brief ADC Trigger Source
 */
enum class AdcTrigger : uint8_t {
    Software = 0,  ///< Software trigger (manual)
    External = 1,  ///< External trigger pin
    Timer = 2,  ///< Timer/Counter trigger
    PwmEvent = 3,  ///< PWM event trigger
    Continuous = 4,  ///< Free-running mode
};

/**
 * @brief ADC Channel Number
 */
enum class AdcChannel : uint8_t {
    CH0 = 0,  ///< Channel 0
    CH1 = 1,  ///< Channel 1
    CH2 = 2,  ///< Channel 2
    CH3 = 3,  ///< Channel 3
    CH4 = 4,  ///< Channel 4
    CH5 = 5,  ///< Channel 5
    CH6 = 6,  ///< Channel 6
    CH7 = 7,  ///< Channel 7
    CH8 = 8,  ///< Channel 8
    CH9 = 9,  ///< Channel 9
    CH10 = 10,  ///< Channel 10
    CH11 = 11,  ///< Channel 11
};


/**
 * @brief ADC configuration structure
 */
struct AdcConfig {
    AdcResolution resolution = AdcResolution::Bits12;  ///< ADC resolution
    AdcTrigger trigger = AdcTrigger::Software;  ///< Trigger source
    uint32_t sample_rate = 1000000;  ///< Target sample rate in Hz (up to 2 MHz)
    bool use_dma = false;  ///< Enable DMA transfers
    uint8_t channels = 1;  ///< Number of channels to scan
};

/**
 * @brief Template-based ADC peripheral for SAME70
 *
 * This class provides a template-based ADC implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: AFEC peripheral base address
 * - IRQ_ID: AFEC IRQ ID
 *
 * Example usage:
 * @code
 * // Basic ADC usage
 * using MyAdc = Adc<AFEC0_BASE, AFEC0_IRQ>;
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
 * @tparam BASE_ADDR AFEC peripheral base address
 * @tparam IRQ_ID AFEC IRQ ID
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Adc {
public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;


    /**
     * @brief Get AFEC peripheral registers
     *
     * Returns pointer to AFEC registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::atmel::same70::afec0::AFEC0_Registers* get_hw() {
#ifdef ALLOY_ADC_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_ADC_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::atmel::same70::afec0::AFEC0_Registers*>(BASE_ADDR);
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

        // Enable AFEC clock
        // TODO: Enable peripheral clock via PMC

        // Reset AFEC
        hw->CR = afec::cr::SWRST::mask;

        // Configure mode register
        // ADC_CLK = MCK / ((PRESCAL+1) * 2)
        // Target 1 MHz ADC clock (assuming 150 MHz MCK)
        uint32_t prescal = 74;  // (150MHz / (2 * 1MHz)) - 1 = 74
        
        uint32_t mr = 0;
        mr = afec::mr::PRESCAL::write(mr, prescal);
        mr = afec::mr::STARTUP::write(mr, afec::mr::startup::SUT0);  // Fastest startup
        mr = afec::mr::TRACKTIM::write(mr, 0);  // Fastest tracking
        mr = afec::mr::TRANSFER::write(mr, 0);
        mr = afec::mr::USEQ::write(mr, afec::mr::useq::NUM_ORDER);  // Channel order CH0-CH11
        mr = afec::mr::ONE::set(mr);  // Must be set to 1 (ONE bit)
        
        hw->MR = mr;

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

        // Disable all channels
        hw->CHDR = 0xFFFFFFFF;

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

        // Configure extended mode register (resolution)
        uint32_t emr = 0;
        uint32_t res_value = afec::emr::res::NO_AVERAGE;  // 12-bit default
        if (config.resolution == AdcResolution::Bits13) {
            res_value = afec::emr::res::OSR4;  // 13-bit with oversampling
        }
        emr = afec::emr::RES::write(emr, res_value);
        emr = afec::emr::TAG::write(emr, 0);  // No tag
        emr = afec::emr::CMPMODE::write(emr, afec::emr::cmpmode::LOW);  // No comparison
        hw->EMR = emr;

        // Configure trigger mode
        uint32_t mr = hw->MR;
        if (config.trigger == AdcTrigger::Software) {
            mr = afec::mr::TRGEN::write(mr, afec::mr::trgen::DIS);  // Software trigger
        } else {
            mr = afec::mr::TRGEN::write(mr, afec::mr::trgen::EN);  // Hardware trigger
            mr = afec::mr::TRGSEL::write(mr, static_cast<uint32_t>(config.trigger));
        }
        hw->MR = mr;

        // 
        m_config = config;

        return Ok();
    }

    /**
     * @brief Enable ADC channel
     *
     * @param channel Channel to enable
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> enableChannel(AdcChannel channel) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // 
        hw->CHER = (1u << static_cast<uint8_t>(channel));

        return Ok();
    }

    /**
     * @brief Disable ADC channel
     *
     * @param channel Channel to disable
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> disableChannel(AdcChannel channel) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // 
        hw->CHDR = (1u << static_cast<uint8_t>(channel));

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

        // Start conversion
        hw->CR = afec::cr::START::mask;

        return Ok();
    }

    /**
     * @brief Read converted value from channel
     *
     * @param channel Channel to read
     * @return Result<uint16_t, ErrorCode> Read converted value from channel     */
    Result<uint16_t, ErrorCode> read(AdcChannel channel) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // 
        uint8_t ch = static_cast<uint8_t>(channel);
        
        // Wait for conversion complete on this channel
        uint32_t timeout = 100000;
        while ((hw->ISR & (1u << ch)) == 0 && timeout > 0) {
            --timeout;
        }
        
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }
        
        // Read converted data register (CDR) for the channel
        // CDR registers are at offset 0x50 + (channel * 4)
        volatile uint32_t* cdr = reinterpret_cast<volatile uint32_t*>(
            reinterpret_cast<uintptr_t>(hw) + 0x50 + (ch * 4)
        );
        
        uint16_t value = static_cast<uint16_t>(*cdr & 0xFFF);  // 12-bit value

        return Ok(uint16_t(value));
    }

    /**
     * @brief Read single channel (enable, start, read, disable)
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
        
        auto read_result = read(channel);
        
        disableChannel(channel);
        
        return read_result;

    }

    /**
     * @brief Enable DMA mode
     *
     * @return Result<void, ErrorCode>     *
     * @note In DMA mode, converted values are automatically transferred to memory by the DMA controller without CPU intervention
     */
    Result<void, ErrorCode> enableDma() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Enable data ready interrupt for DMA
        hw->IER = afec::ier::DRDY::mask;

        return Ok();
    }

    /**
     * @brief Get data register address for DMA
     *
     * @return const volatile void* Get data register address for DMA     *
     * @note The DMA controller needs to know the address of the last converted data register (LCDR) to read from
     */
    const volatile void* getDmaSourceAddress() const {
        auto* hw = get_hw();

        // 
        return &hw->LCDR;

        return &hw->LCDR;
    }

    /**
     * @brief Check if ADC is open
     *
     * @return bool Check if ADC is open     */
    bool isOpen() const {
        return m_opened;
    }

    // ========================================================================
    // Static Utility Methods
    // ========================================================================

    /**
     * @brief Convert ADC value to voltage
     *
     * @param adc_value 12-bit ADC reading (0-4095)
     * @param vref_mv Reference voltage in millivolts
     * @return uint32_t
     */
    static constexpr uint32_t toVoltage(uint16_t adc_value, uint32_t vref_mv = 3300) {
        return (static_cast<uint32_t>(adc_value) * vref_mv) / 4095;
    }


private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
    AdcConfig m_config = {};  ///< Current configuration
};

// ==============================================================================
// Predefined ADC Instances
// ==============================================================================

constexpr uint32_t ADC0_BASE = 0x4003C000;
constexpr uint32_t ADC0_IRQ = 29;

constexpr uint32_t ADC1_BASE = 0x40064000;
constexpr uint32_t ADC1_IRQ = 40;

using Adc0 = Adc<ADC0_BASE, ADC0_IRQ>;  ///< AFEC0 - 12 channels (CH0-CH11)
using Adc1 = Adc<ADC1_BASE, ADC1_IRQ>;  ///< AFEC1 - 8 channels (CH0-CH7)

} // namespace alloy::hal::same70
