/**
 * @file spi_expert.hpp
 * @brief Level 3 Expert API for SPI Configuration
 *
 * Provides an advanced configuration API with consteval validation,
 * detailed error messages, and full control over all parameters.
 *
 * Design Principles:
 * - Configuration as data (declarative style)
 * - Compile-time validation with constexpr
 * - Custom error messages for each validation failure
 * - Support both compile-time and runtime configs
 * - Zero runtime overhead when used at compile-time
 * - Expert-level control over all hardware details
 *
 * Example Usage:
 * @code
 * // Compile-time configuration with validation
 * constexpr SpiExpertConfig config = {
 *     .peripheral = PeripheralId::SPI0,
 *     .mosi_pin = PinId::PA7,
 *     .miso_pin = PinId::PA6,
 *     .sck_pin = PinId::PA5,
 *     .mode = SpiMode::Mode0,
 *     .clock_speed = 2000000,
 *     .bit_order = SpiBitOrder::MsbFirst,
 *     .data_size = SpiDataSize::Bits8,
 *     .enable_mosi = true,
 *     .enable_miso = true,
 *     .enable_dma_tx = false,
 *     .enable_dma_rx = false
 * };
 *
 * // Validate at compile-time
 * static_assert(config.is_valid(), config.error_message());
 *
 * // Apply configuration
 * auto result = expert::configure(config);
 * @endcode
 *
 * @note Part of Phase 6.2: SPI Expert API
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"
#include "hal/signals.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Expert Configuration Structure
// ============================================================================

/**
 * @brief Expert-level SPI configuration
 *
 * Complete configuration structure with all SPI parameters.
 * Can be validated at compile-time or runtime.
 */
struct SpiExpertConfig {
    // Core configuration
    PeripheralId peripheral;
    PinId mosi_pin;
    PinId miso_pin;
    PinId sck_pin;
    PinId nss_pin;  // Optional chip select (hardware NSS)
    SpiMode mode;
    u32 clock_speed;
    SpiBitOrder bit_order;
    SpiDataSize data_size;

    // Advanced options
    bool enable_mosi;
    bool enable_miso;
    bool enable_nss;        // Hardware NSS control
    bool enable_interrupts;
    bool enable_dma_tx;
    bool enable_dma_rx;

    // Hardware-specific options
    bool enable_crc;        // CRC calculation
    u16 crc_polynomial;     // CRC polynomial (if enabled)
    bool enable_ti_mode;    // TI frame format
    bool enable_motorola;   // Motorola frame format (default)

    // ========================================================================
    // Constexpr Validation
    // ========================================================================

    /**
     * @brief Check if configuration is valid
     *
     * Performs comprehensive validation of all parameters.
     * Can be evaluated at compile-time.
     *
     * @return true if valid, false otherwise
     */
    constexpr bool is_valid() const {
        // Must enable at least MOSI or MISO
        if (!enable_mosi && !enable_miso) {
            return false;
        }

        // Clock speed must be reasonable (1 kHz to 50 MHz)
        if (clock_speed < 1000 || clock_speed > 50000000) {
            return false;
        }

        // CRC polynomial must be non-zero if CRC enabled
        if (enable_crc && crc_polynomial == 0) {
            return false;
        }

        // Can't enable both TI and Motorola mode
        if (enable_ti_mode && enable_motorola) {
            return false;
        }

        // If MISO disabled, can't enable DMA RX
        if (!enable_miso && enable_dma_rx) {
            return false;
        }

        // If MOSI disabled, can't enable DMA TX
        if (!enable_mosi && enable_dma_tx) {
            return false;
        }

        return true;
    }

    /**
     * @brief Get detailed error message
     *
     * Provides a descriptive error message explaining what's wrong
     * with the configuration.
     *
     * @return Error message string, or "Valid" if no errors
     */
    constexpr const char* error_message() const {
        if (!enable_mosi && !enable_miso) {
            return "Must enable at least MOSI or MISO";
        }

        if (clock_speed < 1000) {
            return "Clock speed too low (minimum 1 kHz)";
        }

        if (clock_speed > 50000000) {
            return "Clock speed too high (maximum 50 MHz)";
        }

        if (enable_crc && crc_polynomial == 0) {
            return "CRC enabled but polynomial is zero";
        }

        if (enable_ti_mode && enable_motorola) {
            return "Cannot enable both TI and Motorola frame formats";
        }

        if (!enable_miso && enable_dma_rx) {
            return "DMA RX enabled but MISO is disabled";
        }

        if (!enable_mosi && enable_dma_tx) {
            return "DMA TX enabled but MOSI is disabled";
        }

        return "Valid";
    }

    /**
     * @brief Get configuration hint
     *
     * Provides suggestions for fixing invalid configurations.
     *
     * @return Hint string
     */
    constexpr const char* hint() const {
        if (!is_valid()) {
            return "Check error_message() for details on what to fix";
        }
        return "Configuration is valid";
    }

    // ========================================================================
    // Preset Configurations
    // ========================================================================

    /**
     * @brief Create standard Mode 0 configuration (2 MHz)
     *
     * Common configuration for most SPI devices.
     */
    static constexpr SpiExpertConfig standard_mode0_2mhz(
        PeripheralId peripheral,
        PinId mosi_pin,
        PinId miso_pin,
        PinId sck_pin) {
        return SpiExpertConfig{
            .peripheral = peripheral,
            .mosi_pin = mosi_pin,
            .miso_pin = miso_pin,
            .sck_pin = sck_pin,
            .nss_pin = PinId::PA0,  // Unused
            .mode = SpiMode::Mode0,
            .clock_speed = 2000000,  // 2 MHz
            .bit_order = SpiBitOrder::MsbFirst,
            .data_size = SpiDataSize::Bits8,
            .enable_mosi = true,
            .enable_miso = true,
            .enable_nss = false,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_crc = false,
            .crc_polynomial = 0,
            .enable_ti_mode = false,
            .enable_motorola = true
        };
    }

    /**
     * @brief Create TX-only configuration
     *
     * Optimized for output-only devices (displays, DACs).
     */
    static constexpr SpiExpertConfig tx_only_config(
        PeripheralId peripheral,
        PinId mosi_pin,
        PinId sck_pin,
        u32 clock_speed) {
        return SpiExpertConfig{
            .peripheral = peripheral,
            .mosi_pin = mosi_pin,
            .miso_pin = PinId::PA0,  // Unused
            .sck_pin = sck_pin,
            .nss_pin = PinId::PA0,  // Unused
            .mode = SpiMode::Mode0,
            .clock_speed = clock_speed,
            .bit_order = SpiBitOrder::MsbFirst,
            .data_size = SpiDataSize::Bits8,
            .enable_mosi = true,
            .enable_miso = false,  // TX only
            .enable_nss = false,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_crc = false,
            .crc_polynomial = 0,
            .enable_ti_mode = false,
            .enable_motorola = true
        };
    }

    /**
     * @brief Create DMA-enabled configuration
     *
     * High-performance configuration with DMA.
     */
    static constexpr SpiExpertConfig dma_config(
        PeripheralId peripheral,
        PinId mosi_pin,
        PinId miso_pin,
        PinId sck_pin,
        u32 clock_speed) {
        return SpiExpertConfig{
            .peripheral = peripheral,
            .mosi_pin = mosi_pin,
            .miso_pin = miso_pin,
            .sck_pin = sck_pin,
            .nss_pin = PinId::PA0,  // Unused
            .mode = SpiMode::Mode0,
            .clock_speed = clock_speed,
            .bit_order = SpiBitOrder::MsbFirst,
            .data_size = SpiDataSize::Bits8,
            .enable_mosi = true,
            .enable_miso = true,
            .enable_nss = false,
            .enable_interrupts = true,  // DMA needs interrupts
            .enable_dma_tx = true,
            .enable_dma_rx = true,
            .enable_crc = false,
            .crc_polynomial = 0,
            .enable_ti_mode = false,
            .enable_motorola = true
        };
    }

    /**
     * @brief Create high-speed configuration
     *
     * For fast devices (10+ MHz).
     */
    static constexpr SpiExpertConfig high_speed_config(
        PeripheralId peripheral,
        PinId mosi_pin,
        PinId miso_pin,
        PinId sck_pin,
        u32 clock_speed) {
        return SpiExpertConfig{
            .peripheral = peripheral,
            .mosi_pin = mosi_pin,
            .miso_pin = miso_pin,
            .sck_pin = sck_pin,
            .nss_pin = PinId::PA0,  // Unused
            .mode = SpiMode::Mode0,
            .clock_speed = clock_speed,
            .bit_order = SpiBitOrder::MsbFirst,
            .data_size = SpiDataSize::Bits8,
            .enable_mosi = true,
            .enable_miso = true,
            .enable_nss = false,
            .enable_interrupts = true,
            .enable_dma_tx = true,
            .enable_dma_rx = true,
            .enable_crc = false,
            .crc_polynomial = 0,
            .enable_ti_mode = false,
            .enable_motorola = true
        };
    }
};

// ============================================================================
// Expert Configuration API
// ============================================================================

/**
 * @brief Expert SPI configuration namespace
 *
 * Provides functions for working with expert configurations.
 */
namespace expert {

/**
 * @brief Configure SPI with expert configuration
 *
 * Applies a validated configuration to the SPI peripheral.
 *
 * @param config Expert configuration struct
 * @return Result indicating success or error
 *
 * @note Configuration must be valid (config.is_valid() == true)
 */
inline Result<void, ErrorCode> configure(const SpiExpertConfig& config) {
    // Validate configuration
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // TODO: Apply configuration to hardware
    // - Configure GPIO pins with alternate functions
    // - Set SPI mode, clock speed, bit order, data size
    // - Configure frame format (Motorola/TI)
    // - Enable MOSI/MISO as needed
    // - Configure DMA if enabled
    // - Enable interrupts if needed
    // - Configure CRC if enabled

    return Ok();
}

}  // namespace expert

// ============================================================================
// Compile-Time Validation Helpers
// ============================================================================

/**
 * @brief Validate SPI configuration at compile-time
 *
 * Use in static_assert to ensure configuration is valid.
 *
 * Example:
 * @code
 * constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(...);
 * static_assert(validate_spi_config(config), "Invalid SPI config");
 * @endcode
 */
constexpr bool validate_spi_config(const SpiExpertConfig& config) {
    return config.is_valid();
}

/**
 * @brief Check specific validation rules
 */
constexpr bool has_valid_clock_speed(const SpiExpertConfig& config) {
    return config.clock_speed >= 1000 && config.clock_speed <= 50000000;
}

constexpr bool has_enabled_direction(const SpiExpertConfig& config) {
    return config.enable_mosi || config.enable_miso;
}

constexpr bool has_valid_crc_config(const SpiExpertConfig& config) {
    return !config.enable_crc || config.crc_polynomial != 0;
}

constexpr bool has_valid_frame_format(const SpiExpertConfig& config) {
    return !(config.enable_ti_mode && config.enable_motorola);
}

constexpr bool has_valid_dma_config(const SpiExpertConfig& config) {
    if (config.enable_dma_tx && !config.enable_mosi) return false;
    if (config.enable_dma_rx && !config.enable_miso) return false;
    return true;
}

}  // namespace alloy::hal
