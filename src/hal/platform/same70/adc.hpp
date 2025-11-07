/**
 * @file adc.hpp
 * @brief Template-based AFEC (ADC) implementation for SAME70 (ARM Cortex-M7)
 *
 * This file implements the AFEC (Analog Front-End Controller) for SAME70
 * using templates with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - DMA support: Can work with DMA for continuous high-speed conversion
 *
 * SAME70 AFEC Features:
 * - 12-bit resolution
 * - Up to 2 MSPS (Million Samples Per Second)
 * - 12 channels (AFEC0) and 8 channels (AFEC1)
 * - Programmable gain amplifier
 * - Hardware averaging
 * - DMA support
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/registers/afec0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/afec0_bitfields.hpp"
#include "hal/platform/same70/clock.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
namespace afec = alloy::hal::atmel::same70::afec0;  // Alias for easier bitfield access

/**
 * @brief ADC Resolution
 */
enum class AdcResolution : uint8_t {
    Bits12 = 0,  // 12-bit resolution (default)
    Bits13 = 1,  // 13-bit with oversampling (not all devices)
};

/**
 * @brief ADC Trigger Source
 */
enum class AdcTrigger : uint8_t {
    Software = 0,      // Software trigger (manual)
    External = 1,      // External trigger pin
    Timer = 2,         // Timer/Counter trigger
    PwmEvent = 3,      // PWM event trigger
    Continuous = 4,    // Free-running mode
};

/**
 * @brief ADC Channel Number
 */
enum class AdcChannel : uint8_t {
    CH0 = 0,
    CH1 = 1,
    CH2 = 2,
    CH3 = 3,
    CH4 = 4,
    CH5 = 5,
    CH6 = 6,
    CH7 = 7,
    CH8 = 8,
    CH9 = 9,
    CH10 = 10,
    CH11 = 11,
};

/**
 * @brief ADC Configuration
 */
struct AdcConfig {
    AdcResolution resolution = AdcResolution::Bits12;
    AdcTrigger trigger = AdcTrigger::Software;
    uint32_t sample_rate = 1000000;  ///< Target sample rate in Hz (up to 2 MHz)
    bool use_dma = false;             ///< Enable DMA transfers
    uint8_t channels = 1;             ///< Number of channels to scan
};

/**
 * @brief Template-based ADC peripheral for SAME70
 *
 * @tparam BASE_ADDR AFEC base address
 * @tparam IRQ_ID Peripheral IRQ ID
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Adc {
public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    static inline volatile atmel::same70::afec0::AFEC0_Registers* get_hw() {
#ifdef ALLOY_ADC_MOCK_HW
        return ALLOY_ADC_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::afec0::AFEC0_Registers*>(BASE_ADDR);
#endif
    }

    constexpr Adc() = default;

    /**
     * @brief Initialize ADC peripheral
     */
    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        auto* hw = get_hw();

        // Enable AFEC clock using Clock class
        auto clock_result = Clock::enablePeripheralClock(IRQ_ID);
        if (!clock_result.is_ok()) {
            return Result<void>::error(clock_result.error());
        }

        // Reset AFEC using type-safe bitfield
        hw->CR = afec::cr::SWRST::mask;

        // Set prescaler for ADC clock using type-safe bitfields
        // ADC_CLK = MCK / ((PRESCAL+1) * 2)
        // Calculate PRESCAL dynamically from actual MCK for 1 MHz ADC clock
        uint32_t mck = Clock::getMasterClockFrequency();
        uint32_t prescal = (mck / (2 * 1000000)) - 1;  // Target 1 MHz ADC clock

        uint32_t mr = 0;
        mr = afec::mr::PRESCAL::write(mr, prescal);
        mr = afec::mr::STARTUP::write(mr, afec::mr::startup::SUT0);  // Fastest startup
        mr = afec::mr::TRACKTIM::write(mr, 0);  // Fastest tracking
        mr = afec::mr::TRANSFER::write(mr, 0);
        mr = afec::mr::USEQ::write(mr, afec::mr::useq::NUM_ORDER);  // Channel order CH0-CH11
        mr = afec::mr::ONE::set(mr);  // Must be set to 1 (ONE bit)

        hw->MR = mr;

        m_opened = true;
        return Result<void>::ok();
    }

    /**
     * @brief Close ADC peripheral
     */
    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Disable all channels
        hw->CHDR = 0xFFFFFFFF;

        m_opened = false;
        return Result<void>::ok();
    }

    /**
     * @brief Configure ADC
     */
    Result<void> configure(const AdcConfig& config) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Configure extended mode register (EMR) using type-safe bitfields
        uint32_t emr = 0;
        // Map AdcResolution enum to AFEC RES field values
        uint32_t res_value = afec::emr::res::NO_AVERAGE;  // 12-bit default
        if (config.resolution == AdcResolution::Bits13) {
            res_value = afec::emr::res::OSR4;  // 13-bit with oversampling
        }
        emr = afec::emr::RES::write(emr, res_value);
        emr = afec::emr::TAG::write(emr, 0);  // No tag
        emr = afec::emr::CMPMODE::write(emr, afec::emr::cmpmode::LOW);  // No comparison

        hw->EMR = emr;

        // Configure trigger mode in MR using type-safe bitfields
        uint32_t mr = hw->MR;
        if (config.trigger == AdcTrigger::Software) {
            mr = afec::mr::TRGEN::write(mr, afec::mr::trgen::DIS);  // Software trigger
        } else {
            mr = afec::mr::TRGEN::write(mr, afec::mr::trgen::EN);  // Hardware trigger
            mr = afec::mr::TRGSEL::write(mr, static_cast<uint32_t>(config.trigger));
        }
        hw->MR = mr;

        m_config = config;
        return Result<void>::ok();
    }

    /**
     * @brief Enable ADC channel
     */
    Result<void> enableChannel(AdcChannel channel) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->CHER = (1u << static_cast<uint8_t>(channel));

        return Result<void>::ok();
    }

    /**
     * @brief Disable ADC channel
     */
    Result<void> disableChannel(AdcChannel channel) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->CHDR = (1u << static_cast<uint8_t>(channel));

        return Result<void>::ok();
    }

    /**
     * @brief Start ADC conversion (software trigger)
     */
    Result<void> startConversion() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->CR = afec::cr::START::mask;  // Start conversion

        return Result<void>::ok();
    }

    /**
     * @brief Read converted value from channel
     */
    Result<uint16_t> read(AdcChannel channel) {
        if (!m_opened) {
            return Result<uint16_t>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        uint8_t ch = static_cast<uint8_t>(channel);

        // Wait for conversion complete on this channel
        uint32_t timeout = 100000;
        while ((hw->ISR & (1u << ch)) == 0 && timeout > 0) {
            --timeout;
        }

        if (timeout == 0) {
            return Result<uint16_t>::error(ErrorCode::Timeout);
        }

        // Read converted data register (CDR) for the channel
        // CDR registers are at offset 0x50 + (channel * 4)
        volatile uint32_t* cdr = reinterpret_cast<volatile uint32_t*>(
            reinterpret_cast<uintptr_t>(hw) + 0x50 + (ch * 4)
        );

        uint16_t value = static_cast<uint16_t>(*cdr & 0xFFF);  // 12-bit value

        return Result<uint16_t>::ok(value);
    }

    /**
     * @brief Read single channel (enable, start, read, disable)
     *
     * Convenience method for one-shot readings.
     */
    Result<uint16_t> readSingle(AdcChannel channel) {
        auto enable_result = enableChannel(channel);
        if (!enable_result.is_ok()) {
            return Result<uint16_t>::error(enable_result.error());
        }

        auto start_result = startConversion();
        if (!start_result.is_ok()) {
            return Result<uint16_t>::error(start_result.error());
        }

        auto read_result = read(channel);

        disableChannel(channel);

        return read_result;
    }

    /**
     * @brief Convert ADC value to voltage
     *
     * @param adc_value 12-bit ADC reading (0-4095)
     * @param vref_mv Reference voltage in millivolts (typically 3300 mV)
     * @return Voltage in millivolts
     */
    static constexpr uint32_t toVoltage(uint16_t adc_value, uint32_t vref_mv = 3300) {
        return (static_cast<uint32_t>(adc_value) * vref_mv) / 4095;
    }

    /**
     * @brief Enable DMA mode
     *
     * In DMA mode, converted values are automatically transferred to memory
     * by the DMA controller without CPU intervention.
     */
    Result<void> enableDma() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Enable data ready interrupt for DMA
        hw->IER = afec::ier::DRDY::mask;

        return Result<void>::ok();
    }

    /**
     * @brief Get data register address for DMA
     *
     * The DMA controller needs to know the address of the last converted
     * data register (LCDR) to read from.
     */
    const volatile void* getDmaSourceAddress() const {
        auto* hw = get_hw();
        return &hw->LCDR;  // Last Converted Data Register
    }

    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;
    AdcConfig m_config;
};

// ============================================================================
// Predefined ADC instances for SAME70
// ============================================================================

constexpr uint32_t AFEC0_BASE = 0x4003C000;
constexpr uint32_t AFEC0_IRQ = 29;

constexpr uint32_t AFEC1_BASE = 0x40064000;
constexpr uint32_t AFEC1_IRQ = 40;

using Adc0 = Adc<AFEC0_BASE, AFEC0_IRQ>;  // 12 channels (CH0-CH11)
using Adc1 = Adc<AFEC1_BASE, AFEC1_IRQ>;  // 8 channels (CH0-CH7)

} // namespace alloy::hal::same70
