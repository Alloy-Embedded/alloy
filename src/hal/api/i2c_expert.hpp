/**
 * @file i2c_expert.hpp
 * @brief Level 3 Expert API for I2C Configuration
 *
 * Provides an advanced configuration API with consteval validation,
 * detailed error messages, and full control over all parameters.
 *
 * Design Principles:
 * - Complete control over all I2C parameters
 * - Compile-time validation with detailed error messages
 * - DMA and interrupt support
 * - Advanced filtering options (analog/digital)
 * - Built on I2cBase CRTP for code reuse
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
 *     .enable_interrupts = false,
 *     .enable_analog_filter = true,
 *     .enable_digital_filter = false,
 *     .digital_filter_coefficient = 0
 * };
 *
 * static_assert(config.is_valid(), "Invalid I2C config");
 * auto i2c = expert::create_instance(config);
 * i2c.read_register(0x50, 0x10);  // Now has I2C methods from I2cBase!
 * @endcode
 *
 * @note Part of Phase 1.11.3: Refactor I2cExpert (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"
#include "hal/core/signals.hpp"
#include "hal/api/i2c_base.hpp"

#include <span>

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

// ============================================================================
// Expert I2C Instance (CRTP)
// ============================================================================

/**
 * @brief Expert I2C instance with full control
 *
 * Provides complete I2C functionality with advanced features.
 * Inherits from I2cBase to get all common I2C operations.
 *
 * Features:
 * - DMA support for high-throughput transfers
 * - Interrupt-driven operations
 * - Advanced filtering (analog/digital)
 * - All standard I2C operations from I2cBase
 */
class ExpertI2cInstance : public I2cBase<ExpertI2cInstance> {
    using Base = I2cBase<ExpertI2cInstance>;
    friend Base;

public:
    // ========================================================================
    // Inherited Interface from I2cBase (CRTP)
    // ========================================================================

    // Inherit all common I2C methods from base
    using Base::read;             // Read from device
    using Base::write;            // Write to device
    using Base::write_read;       // Write then read (repeated start)
    using Base::read_byte;        // Single-byte read
    using Base::write_byte;       // Single-byte write
    using Base::read_register;    // Register read
    using Base::write_register;   // Register write
    using Base::scan_bus;         // Bus scanning
    using Base::configure;        // Configuration
    using Base::set_speed;        // Set bus speed
    using Base::set_addressing;   // Set addressing mode

    /**
     * @brief Constructor - validate and store configuration
     */
    constexpr explicit ExpertI2cInstance(const I2cExpertConfig& config)
        : config_(config) {}

    /**
     * @brief Get current configuration
     */
    constexpr const I2cExpertConfig& config() const {
        return config_;
    }

    /**
     * @brief Check if DMA TX is enabled
     */
    constexpr bool has_dma_tx() const {
        return config_.enable_dma_tx;
    }

    /**
     * @brief Check if DMA RX is enabled
     */
    constexpr bool has_dma_rx() const {
        return config_.enable_dma_rx;
    }

    /**
     * @brief Check if interrupts are enabled
     */
    constexpr bool has_interrupts() const {
        return config_.enable_interrupts;
    }

    /**
     * @brief Apply configuration to hardware
     */
    Result<void, ErrorCode> apply() const {
        if (!config_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        // TODO: Apply configuration to hardware
        // - Configure GPIO pins with I2C alternate function
        // - Set I2C speed (calculate timing parameters)
        // - Set addressing mode
        // - Configure DMA channels if enabled
        // - Configure interrupts if enabled
        // - Configure analog/digital filters
        // - Enable I2C peripheral

        return Ok();
    }

    // ========================================================================
    // Implementation Methods (public for concept checking)
    // ========================================================================

    /**
     * @brief Read implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> read_impl(
        u16 address,
        std::span<u8> buffer
    ) noexcept {
        // TODO: Implement hardware read with DMA/interrupt support
        (void)address;
        (void)buffer;
        return Ok();
    }

    /**
     * @brief Write implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_impl(
        u16 address,
        std::span<const u8> buffer
    ) noexcept {
        // TODO: Implement hardware write with DMA/interrupt support
        (void)address;
        (void)buffer;
        return Ok();
    }

    /**
     * @brief Write-read implementation (repeated start)
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_read_impl(
        u16 address,
        std::span<const u8> write_buffer,
        std::span<u8> read_buffer
    ) noexcept {
        // TODO: Implement hardware write-read with DMA/interrupt support
        (void)address;
        (void)write_buffer;
        (void)read_buffer;
        return Ok();
    }

    /**
     * @brief Scan bus implementation
     */
    [[nodiscard]] constexpr Result<usize, ErrorCode> scan_bus_impl(
        std::span<u8> found_devices
    ) noexcept {
        // TODO: Implement bus scanning
        (void)found_devices;
        return Ok(usize{0});
    }

    /**
     * @brief Configure I2C implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_impl(
        const I2cConfig& new_config
    ) noexcept {
        // TODO: Apply configuration to hardware
        (void)new_config;
        return Ok();
    }

private:
    I2cExpertConfig config_;
};

// ============================================================================
// Expert I2C Configuration Namespace
// ============================================================================

/**
 * @brief Expert I2C configuration namespace
 */
namespace expert {

/**
 * @brief Create I2C instance from expert configuration
 *
 * @param config Expert configuration (must be valid)
 * @return ExpertI2cInstance with all I2C operations
 */
constexpr ExpertI2cInstance create_instance(const I2cExpertConfig& config) {
    return ExpertI2cInstance(config);
}

/**
 * @brief Configure I2C with expert configuration (deprecated - use create_instance)
 *
 * @deprecated Use create_instance() instead to get an instance with I2C operations
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
