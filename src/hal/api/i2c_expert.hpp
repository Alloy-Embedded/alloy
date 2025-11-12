/**
 * @file i2c_expert.hpp
 * @brief Level 3 Expert API for I2C Configuration
 *
 * Provides an advanced configuration API with consteval validation,
 * detailed error messages, and full control over all parameters.
 *
 * Example Usage:
 * @code
 * constexpr I2cExpertConfig config = {
 *     .peripheral = PeripheralId::I2C0,
 *     .sda_pin = PinId::PA10,
 *     .scl_pin = PinId::PA9,
 *     .speed = I2cSpeed::Fast,
 *     .addressing = I2cAddressing::SevenBit,
 *     .enable_dma_tx = false,
 *     .enable_dma_rx = false,
 *     .enable_interrupts = false
 * };
 *
 * static_assert(config.is_valid(), "Invalid I2C config");
 * @endcode
 *
 * @note Part of Phase 6.3: I2C Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"
#include "hal/core/signals.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

/**
 * @brief Expert-level I2C configuration
 *
 * Complete configuration structure with all I2C parameters.
 * Can be validated at compile-time or runtime.
 */
struct I2cExpertConfig {
    // Core configuration
    PeripheralId peripheral;
    PinId sda_pin;
    PinId scl_pin;
    I2cSpeed speed;
    I2cAddressing addressing;

    // Advanced options
    bool enable_interrupts;
    bool enable_dma_tx;
    bool enable_dma_rx;
    bool enable_analog_filter;
    bool enable_digital_filter;
    u8 digital_filter_coefficient;  // 0-15

    /**
     * @brief Check if configuration is valid
     */
    constexpr bool is_valid() const {
        // Digital filter coefficient must be 0-15
        if (enable_digital_filter && digital_filter_coefficient > 15) {
            return false;
        }

        return true;
    }

    /**
     * @brief Get detailed error message
     */
    constexpr const char* error_message() const {
        if (enable_digital_filter && digital_filter_coefficient > 15) {
            return "Digital filter coefficient must be 0-15";
        }

        return "Valid";
    }

    /**
     * @brief Create standard configuration (100kHz)
     */
    static constexpr I2cExpertConfig standard(
        PeripheralId peripheral,
        PinId sda_pin,
        PinId scl_pin) {
        
        return I2cExpertConfig{
            .peripheral = peripheral,
            .sda_pin = sda_pin,
            .scl_pin = scl_pin,
            .speed = I2cSpeed::Standard,
            .addressing = I2cAddressing::SevenBit,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_analog_filter = true,
            .enable_digital_filter = false,
            .digital_filter_coefficient = 0
        };
    }

    /**
     * @brief Create fast mode configuration (400kHz)
     */
    static constexpr I2cExpertConfig fast(
        PeripheralId peripheral,
        PinId sda_pin,
        PinId scl_pin) {
        
        return I2cExpertConfig{
            .peripheral = peripheral,
            .sda_pin = sda_pin,
            .scl_pin = scl_pin,
            .speed = I2cSpeed::Fast,
            .addressing = I2cAddressing::SevenBit,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_analog_filter = true,
            .enable_digital_filter = false,
            .digital_filter_coefficient = 0
        };
    }

    /**
     * @brief Create DMA-enabled configuration
     */
    static constexpr I2cExpertConfig dma(
        PeripheralId peripheral,
        PinId sda_pin,
        PinId scl_pin,
        I2cSpeed speed) {
        
        return I2cExpertConfig{
            .peripheral = peripheral,
            .sda_pin = sda_pin,
            .scl_pin = scl_pin,
            .speed = speed,
            .addressing = I2cAddressing::SevenBit,
            .enable_interrupts = true,
            .enable_dma_tx = true,
            .enable_dma_rx = true,
            .enable_analog_filter = true,
            .enable_digital_filter = false,
            .digital_filter_coefficient = 0
        };
    }
};

/**
 * @brief Expert I2C configuration namespace
 */
namespace expert {

/**
 * @brief Configure I2C with expert configuration
 */
inline Result<void, ErrorCode> configure(const I2cExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // TODO: Apply configuration to hardware

    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
