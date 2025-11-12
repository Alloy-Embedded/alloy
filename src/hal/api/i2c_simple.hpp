/**
 * @file i2c_simple.hpp
 * @brief Level 1 Simple API for I2C Configuration
 *
 * Provides a minimalist one-liner API for common I2C use cases.
 *
 * Design Principles:
 * - One-liner setup with sensible defaults
 * - Compile-time pin validation
 * - Common speed presets (Standard 100kHz, Fast 400kHz)
 * - 7-bit addressing (most common)
 * - Zero configuration for typical use cases
 *
 * Example Usage:
 * @code
 * // Simplest I2C setup - just specify pins
 * constexpr auto i2c = I2c<PeripheralId::I2C0>::quick_setup<
 *     I2c0_SDA,
 *     I2c0_SCL
 * >();  // Defaults: Standard mode (100kHz), 7-bit addressing
 *
 * // With custom speed
 * constexpr auto fast_i2c = I2c<PeripheralId::I2C0>::quick_setup<
 *     I2c0_SDA,
 *     I2c0_SCL
 * >(I2cSpeed::Fast);  // 400kHz
 * @endcode
 *
 * @note Part of Phase 6.3: I2C Implementation
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
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

// ============================================================================
// Default I2C Configuration
// ============================================================================

/**
 * @brief Default I2C configuration values
 *
 * Provides sensible defaults for most I2C devices:
 * - Standard mode: 100 kHz (compatible with all devices)
 * - 7-bit addressing (most common)
 */
struct I2cDefaults {
    static constexpr I2cSpeed speed = I2cSpeed::Standard;
    static constexpr I2cAddressing addressing = I2cAddressing::SevenBit;
};

// ============================================================================
// Simple I2C Configuration Result
// ============================================================================

/**
 * @brief Simple I2C configuration result
 *
 * Contains validated pin configuration from quick_setup.
 *
 * @tparam SdaPin SDA (data) pin type
 * @tparam SclPin SCL (clock) pin type
 */
template <typename SdaPin, typename SclPin>
struct SimpleI2cConfig {
    PinId sda_pin_id;
    PinId scl_pin_id;
    I2cSpeed speed;
    I2cAddressing addressing;

    /**
     * @brief Apply configuration to hardware
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> apply() const {
        // TODO: Apply configuration to hardware
        // - Configure GPIO pins with I2C alternate function
        // - Set I2C speed (calculate timing parameters)
        // - Set addressing mode
        // - Enable I2C peripheral
        return Ok();
    }
};

// ============================================================================
// Simple I2C API
// ============================================================================

/**
 * @brief Simple I2C configuration API (Level 1)
 *
 * One-liner setup for common I2C configurations.
 *
 * @tparam PeriphId I2C peripheral identifier
 */
template <PeripheralId PeriphId>
class I2c {
public:
    /**
     * @brief Quick setup with default configuration
     *
     * Simplest I2C setup - just specify pins.
     * Uses Standard mode (100kHz) and 7-bit addressing.
     *
     * @tparam SdaPin SDA (data) pin type
     * @tparam SclPin SCL (clock) pin type
     * @param speed I2C bus speed (default: Standard 100kHz)
     * @param addressing Addressing mode (default: 7-bit)
     * @return Simple I2C configuration
     */
    template <typename SdaPin, typename SclPin>
    static constexpr auto quick_setup(I2cSpeed speed = I2cDefaults::speed,
                                       I2cAddressing addressing = I2cDefaults::addressing) {
        // Compile-time pin validation
        static_assert(is_valid_sda_pin<SdaPin>(),
                     "SDA pin is not compatible with this I2C peripheral");
        static_assert(is_valid_scl_pin<SclPin>(),
                     "SCL pin is not compatible with this I2C peripheral");

        return SimpleI2cConfig<SdaPin, SclPin>{
            SdaPin::get_pin_id(),
            SclPin::get_pin_id(),
            speed,
            addressing
        };
    }

    /**
     * @brief Quick setup for Fast mode (400kHz)
     *
     * Convenience preset for Fast mode I2C.
     *
     * @tparam SdaPin SDA pin type
     * @tparam SclPin SCL pin type
     * @return Simple I2C configuration with Fast mode
     */
    template <typename SdaPin, typename SclPin>
    static constexpr auto quick_setup_fast() {
        return quick_setup<SdaPin, SclPin>(I2cSpeed::Fast,
                                           I2cDefaults::addressing);
    }

    /**
     * @brief Quick setup for Fast Plus mode (1MHz)
     *
     * Convenience preset for Fast Plus mode I2C.
     *
     * @tparam SdaPin SDA pin type
     * @tparam SclPin SCL pin type
     * @return Simple I2C configuration with Fast Plus mode
     */
    template <typename SdaPin, typename SclPin>
    static constexpr auto quick_setup_fast_plus() {
        return quick_setup<SdaPin, SclPin>(I2cSpeed::FastPlus,
                                           I2cDefaults::addressing);
    }

private:
    /**
     * @brief Validate SDA pin at compile-time
     *
     * Checks if pin is compatible with this I2C peripheral's SDA signal.
     *
     * @tparam Pin Pin type to validate
     * @return true if valid, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_sda_pin() {
        // TODO: Implement actual pin validation using signal routing tables
        // For now, just check that pin has get_pin_id() method
        return requires { Pin::get_pin_id(); };
    }

    /**
     * @brief Validate SCL pin at compile-time
     *
     * Checks if pin is compatible with this I2C peripheral's SCL signal.
     *
     * @tparam Pin Pin type to validate
     * @return true if valid, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_scl_pin() {
        // TODO: Implement actual pin validation using signal routing tables
        // For now, just check that pin has get_pin_id() method
        return requires { Pin::get_pin_id(); };
    }
};

// ============================================================================
// Preset Configurations
// ============================================================================

/**
 * @brief Create standard I2C configuration (100kHz, 7-bit)
 *
 * Most compatible configuration for all I2C devices.
 *
 * @tparam PeriphId I2C peripheral ID
 * @tparam SdaPin SDA pin type
 * @tparam SclPin SCL pin type
 * @return Simple I2C configuration
 */
template <PeripheralId PeriphId, typename SdaPin, typename SclPin>
constexpr auto create_i2c_standard() {
    return I2c<PeriphId>::template quick_setup<SdaPin, SclPin>(
        I2cSpeed::Standard,
        I2cAddressing::SevenBit
    );
}

/**
 * @brief Create fast I2C configuration (400kHz, 7-bit)
 *
 * For devices that support fast mode.
 *
 * @tparam PeriphId I2C peripheral ID
 * @tparam SdaPin SDA pin type
 * @tparam SclPin SCL pin type
 * @return Simple I2C configuration
 */
template <PeripheralId PeriphId, typename SdaPin, typename SclPin>
constexpr auto create_i2c_fast() {
    return I2c<PeriphId>::template quick_setup<SdaPin, SclPin>(
        I2cSpeed::Fast,
        I2cAddressing::SevenBit
    );
}

/**
 * @brief Create fast plus I2C configuration (1MHz, 7-bit)
 *
 * For high-speed devices with Fast Plus support.
 *
 * @tparam PeriphId I2C peripheral ID
 * @tparam SdaPin SDA pin type
 * @tparam SclPin SCL pin type
 * @return Simple I2C configuration
 */
template <PeripheralId PeriphId, typename SdaPin, typename SclPin>
constexpr auto create_i2c_fast_plus() {
    return I2c<PeriphId>::template quick_setup<SdaPin, SclPin>(
        I2cSpeed::FastPlus,
        I2cAddressing::SevenBit
    );
}

}  // namespace alloy::hal
