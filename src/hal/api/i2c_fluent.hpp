/**
 * @file i2c_fluent.hpp
 * @brief Level 2 Fluent API for I2C Configuration
 *
 * Provides a builder pattern with method chaining for readable
 * I2C configuration.
 *
 * Design Principles:
 * - Self-documenting method chaining
 * - Incremental validation
 * - Preset configurations (Standard, Fast, FastPlus)
 * - Flexible parameter control
 * - State tracking for validation
 * - Built on I2cBase CRTP for code reuse
 *
 * Example Usage:
 * @code
 * auto i2c = I2cBuilder<PeripheralId::I2C0>()
 *     .with_sda<I2c0_SDA>()
 *     .with_scl<I2c0_SCL>()
 *     .speed(I2cSpeed::Fast)       // 400kHz
 *     .addressing_7bit()
 *     .initialize();
 * i2c.value().read_register(0x50, 0x10);  // Now has I2C methods from I2cBase!
 * @endcode
 *
 * @note Part of Phase 1.11.2: Refactor I2cFluent (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"
#include "hal/core/signals.hpp"
#include "hal/api/i2c_simple.hpp"
#include "hal/api/i2c_base.hpp"

#include <span>

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Builder State Tracking
// ============================================================================

/**
 * @brief Tracks builder configuration state
 *
 * Used for incremental validation during fluent API building.
 */
struct I2cBuilderState {
    bool has_sda = false;
    bool has_scl = false;
    bool has_speed = false;

    /**
     * @brief Check if configuration is valid
     *
     * @return true if all required pins configured
     */
    constexpr bool is_valid() const {
        return has_sda && has_scl;
    }
};

// ============================================================================
// Fluent I2C Configuration Result
// ============================================================================

/**
 * @brief Fluent I2C configuration result
 *
 * Contains validated configuration from builder.
 * Inherits from I2cBase to provide all I2C transfer methods.
 */
struct FluentI2cConfig : public I2cBase<FluentI2cConfig> {
    using Base = I2cBase<FluentI2cConfig>;
    friend Base;

    PeripheralId peripheral;
    PinId sda_pin;
    PinId scl_pin;
    I2cConfig config;

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
        // TODO: Implement hardware read
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
        // TODO: Implement hardware write
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
        // TODO: Implement hardware write-read
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
};

// ============================================================================
// I2C Fluent Builder
// ============================================================================

/**
 * @brief Fluent I2C configuration builder (Level 2)
 *
 * Provides method chaining for readable I2C configuration.
 *
 * @tparam PeriphId I2C peripheral identifier
 */
template <PeripheralId PeriphId>
class I2cBuilder {
public:
    /**
     * @brief Constructor - initialize with defaults
     */
    constexpr I2cBuilder()
        : sda_pin_id_(PinId::PA0),
          scl_pin_id_(PinId::PA0),
          speed_(I2cDefaults::speed),
          addressing_(I2cDefaults::addressing),
          state_() {}

    /**
     * @brief Set SDA pin
     *
     * @tparam SdaPin SDA pin type
     * @return Reference to builder for chaining
     */
    template <typename SdaPin>
    constexpr I2cBuilder& with_sda() {
        sda_pin_id_ = SdaPin::get_pin_id();
        state_.has_sda = true;
        return *this;
    }

    /**
     * @brief Set SCL pin
     *
     * @tparam SclPin SCL pin type
     * @return Reference to builder for chaining
     */
    template <typename SclPin>
    constexpr I2cBuilder& with_scl() {
        scl_pin_id_ = SclPin::get_pin_id();
        state_.has_scl = true;
        return *this;
    }

    /**
     * @brief Set both pins at once
     *
     * @tparam SdaPin SDA pin type
     * @tparam SclPin SCL pin type
     * @return Reference to builder for chaining
     */
    template <typename SdaPin, typename SclPin>
    constexpr I2cBuilder& with_pins() {
        return with_sda<SdaPin>()
               .template with_scl<SclPin>();
    }

    /**
     * @brief Set I2C speed
     *
     * @param s I2C bus speed
     * @return Reference to builder for chaining
     */
    constexpr I2cBuilder& speed(I2cSpeed s) {
        speed_ = s;
        state_.has_speed = true;
        return *this;
    }

    /**
     * @brief Set 7-bit addressing mode
     *
     * @return Reference to builder for chaining
     */
    constexpr I2cBuilder& addressing_7bit() {
        addressing_ = I2cAddressing::SevenBit;
        return *this;
    }

    /**
     * @brief Set 10-bit addressing mode
     *
     * @return Reference to builder for chaining
     */
    constexpr I2cBuilder& addressing_10bit() {
        addressing_ = I2cAddressing::TenBit;
        return *this;
    }

    /**
     * @brief Preset: Standard mode (100kHz, 7-bit)
     *
     * @return Reference to builder for chaining
     */
    constexpr I2cBuilder& standard_mode() {
        speed_ = I2cSpeed::Standard;
        addressing_ = I2cAddressing::SevenBit;
        state_.has_speed = true;
        return *this;
    }

    /**
     * @brief Preset: Fast mode (400kHz, 7-bit)
     *
     * @return Reference to builder for chaining
     */
    constexpr I2cBuilder& fast_mode() {
        speed_ = I2cSpeed::Fast;
        addressing_ = I2cAddressing::SevenBit;
        state_.has_speed = true;
        return *this;
    }

    /**
     * @brief Preset: Fast Plus mode (1MHz, 7-bit)
     *
     * @return Reference to builder for chaining
     */
    constexpr I2cBuilder& fast_plus_mode() {
        speed_ = I2cSpeed::FastPlus;
        addressing_ = I2cAddressing::SevenBit;
        state_.has_speed = true;
        return *this;
    }

    /**
     * @brief Validate current configuration
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> validate() const {
        if (!state_.has_sda) {
            return Err(ErrorCode::InvalidParameter);
        }

        if (!state_.has_scl) {
            return Err(ErrorCode::InvalidParameter);
        }

        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        return Ok();
    }

    /**
     * @brief Initialize I2C with configured parameters
     *
     * Validates configuration and returns result.
     *
     * @return Result containing configuration or error
     */
    Result<FluentI2cConfig, ErrorCode> initialize() const {
        // Validate
        auto validation = validate();
        if (validation.is_err()) {
            return Err(std::move(validation).err());
        }

        // Create configuration (can't use aggregate init due to protected base constructor)
        FluentI2cConfig config;
        config.peripheral = PeriphId;
        config.sda_pin = sda_pin_id_;
        config.scl_pin = scl_pin_id_;
        config.config = I2cConfig{speed_, addressing_};

        return Ok(std::move(config));
    }

private:
    PinId sda_pin_id_;
    PinId scl_pin_id_;
    I2cSpeed speed_;
    I2cAddressing addressing_;
    I2cBuilderState state_;
};

}  // namespace alloy::hal
