/**
 * @file spi_fluent.hpp
 * @brief Level 2 Fluent API for SPI Configuration
 *
 * Provides a builder pattern with method chaining for readable
 * SPI configuration.
 *
 * Design Principles:
 * - Self-documenting method chaining
 * - Incremental validation
 * - Preset configurations (Mode0, Mode3, etc)
 * - Flexible parameter control
 * - State tracking for validation
 *
 * Example Usage:
 * @code
 * auto spi = SpiBuilder<PeripheralId::SPI0>()
 *     .with_mosi<Spi0_MOSI>()
 *     .with_miso<Spi0_MISO>()
 *     .with_sck<Spi0_SCK>()
 *     .clock_speed(2000000)  // 2 MHz
 *     .mode(SpiMode::Mode0)
 *     .msb_first()
 *     .initialize();
 * @endcode
 *
 * @note Part of Phase 6.2: SPI Implementation
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"
#include "hal/signals.hpp"
#include "hal/spi_simple.hpp"

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
struct SpiBuilderState {
    bool has_mosi = false;
    bool has_miso = false;
    bool has_sck = false;
    bool has_clock_speed = false;
    bool has_mode = false;

    /**
     * @brief Check if configuration is valid for full-duplex
     *
     * @return true if all required pins configured
     */
    constexpr bool is_full_duplex_valid() const {
        return has_mosi && has_miso && has_sck && has_clock_speed;
    }

    /**
     * @brief Check if configuration is valid for TX-only
     *
     * @return true if TX pins configured
     */
    constexpr bool is_tx_only_valid() const {
        return has_mosi && !has_miso && has_sck && has_clock_speed;
    }

    /**
     * @brief Check if configuration is valid
     *
     * @return true if either full-duplex or TX-only valid
     */
    constexpr bool is_valid() const {
        return is_full_duplex_valid() || is_tx_only_valid();
    }
};

// ============================================================================
// Fluent SPI Configuration Result
// ============================================================================

/**
 * @brief Fluent SPI configuration result
 *
 * Contains validated configuration from builder.
 */
struct FluentSpiConfig {
    PeripheralId peripheral;
    PinId mosi_pin;
    PinId miso_pin;
    PinId sck_pin;
    SpiConfig config;
    bool tx_only;

    /**
     * @brief Apply configuration to hardware
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> apply() const {
        // TODO: Apply configuration to hardware
        return Ok();
    }
};

// ============================================================================
// SPI Fluent Builder
// ============================================================================

/**
 * @brief Fluent SPI configuration builder (Level 2)
 *
 * Provides method chaining for readable SPI configuration.
 *
 * @tparam PeriphId SPI peripheral identifier
 */
template <PeripheralId PeriphId>
class SpiBuilder {
public:
    /**
     * @brief Constructor - initialize with defaults
     */
    constexpr SpiBuilder()
        : mosi_pin_id_(PinId::PA0),
          miso_pin_id_(PinId::PA0),
          sck_pin_id_(PinId::PA0),
          mode_(SpiDefaults::mode),
          clock_speed_(SpiDefaults::clock_speed),
          bit_order_(SpiDefaults::bit_order),
          data_size_(SpiDefaults::data_size),
          state_() {}

    /**
     * @brief Set MOSI pin
     *
     * @tparam MosiPin MOSI pin type
     * @return Reference to builder for chaining
     */
    template <typename MosiPin>
    constexpr SpiBuilder& with_mosi() {
        mosi_pin_id_ = MosiPin::get_pin_id();
        state_.has_mosi = true;
        return *this;
    }

    /**
     * @brief Set MISO pin
     *
     * @tparam MisoPin MISO pin type
     * @return Reference to builder for chaining
     */
    template <typename MisoPin>
    constexpr SpiBuilder& with_miso() {
        miso_pin_id_ = MisoPin::get_pin_id();
        state_.has_miso = true;
        return *this;
    }

    /**
     * @brief Set SCK pin
     *
     * @tparam SckPin SCK pin type
     * @return Reference to builder for chaining
     */
    template <typename SckPin>
    constexpr SpiBuilder& with_sck() {
        sck_pin_id_ = SckPin::get_pin_id();
        state_.has_sck = true;
        return *this;
    }

    /**
     * @brief Set all pins at once
     *
     * @tparam MosiPin MOSI pin type
     * @tparam MisoPin MISO pin type
     * @tparam SckPin SCK pin type
     * @return Reference to builder for chaining
     */
    template <typename MosiPin, typename MisoPin, typename SckPin>
    constexpr SpiBuilder& with_pins() {
        return with_mosi<MosiPin>()
               .template with_miso<MisoPin>()
               .template with_sck<SckPin>();
    }

    /**
     * @brief Set clock speed
     *
     * @param speed Clock speed in Hz
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& clock_speed(u32 speed) {
        clock_speed_ = speed;
        state_.has_clock_speed = true;
        return *this;
    }

    /**
     * @brief Set SPI mode
     *
     * @param m SPI mode (Mode0-Mode3)
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& mode(SpiMode m) {
        mode_ = m;
        state_.has_mode = true;
        return *this;
    }

    /**
     * @brief Set MSB first bit order
     *
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& msb_first() {
        bit_order_ = SpiBitOrder::MsbFirst;
        return *this;
    }

    /**
     * @brief Set LSB first bit order
     *
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& lsb_first() {
        bit_order_ = SpiBitOrder::LsbFirst;
        return *this;
    }

    /**
     * @brief Set 8-bit data size
     *
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& data_8bit() {
        data_size_ = SpiDataSize::Bits8;
        return *this;
    }

    /**
     * @brief Set 16-bit data size
     *
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& data_16bit() {
        data_size_ = SpiDataSize::Bits16;
        return *this;
    }

    /**
     * @brief Preset: Standard Mode 0 configuration
     *
     * CPOL=0, CPHA=0 (most common)
     *
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& standard_mode0() {
        mode_ = SpiMode::Mode0;
        bit_order_ = SpiBitOrder::MsbFirst;
        data_size_ = SpiDataSize::Bits8;
        state_.has_mode = true;
        return *this;
    }

    /**
     * @brief Preset: Standard Mode 3 configuration
     *
     * CPOL=1, CPHA=1 (common for some sensors)
     *
     * @return Reference to builder for chaining
     */
    constexpr SpiBuilder& standard_mode3() {
        mode_ = SpiMode::Mode3;
        bit_order_ = SpiBitOrder::MsbFirst;
        data_size_ = SpiDataSize::Bits8;
        state_.has_mode = true;
        return *this;
    }

    /**
     * @brief Validate current configuration
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> validate() const {
        if (!state_.has_mosi) {
            return Err(ErrorCode::InvalidParameter);
        }

        if (!state_.has_sck) {
            return Err(ErrorCode::InvalidParameter);
        }

        if (!state_.has_clock_speed) {
            return Err(ErrorCode::InvalidParameter);
        }

        // MISO is optional (for TX-only mode)
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        return Ok();
    }

    /**
     * @brief Initialize SPI with configured parameters
     *
     * Validates configuration and returns result.
     *
     * @return Result containing configuration or error
     */
    Result<FluentSpiConfig, ErrorCode> initialize() const {
        // Validate
        auto validation = validate();
        if (!validation.is_ok()) {
            ErrorCode error_copy = validation.error();
            return Err(std::move(error_copy));
        }

        // Create configuration
        FluentSpiConfig config{
            PeriphId,
            mosi_pin_id_,
            miso_pin_id_,
            sck_pin_id_,
            SpiConfig{mode_, clock_speed_, bit_order_, data_size_},
            !state_.has_miso  // TX-only if no MISO
        };

        return Ok(std::move(config));
    }

private:
    PinId mosi_pin_id_;
    PinId miso_pin_id_;
    PinId sck_pin_id_;
    SpiMode mode_;
    u32 clock_speed_;
    SpiBitOrder bit_order_;
    SpiDataSize data_size_;
    SpiBuilderState state_;
};

}  // namespace alloy::hal
