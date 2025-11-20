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
 * - Built on SpiBase CRTP for code reuse
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
 * spi.value().transfer_byte(0x55);  // Now has transfer methods from SpiBase!
 * @endcode
 *
 * @note Part of Phase 1.9.2: Refactor SpiFluent (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"
#include "hal/core/signals.hpp"
#include "hal/api/spi_simple.hpp"
#include "hal/api/spi_base.hpp"

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
 * Inherits from SpiBase to provide all SPI transfer methods.
 */
struct FluentSpiConfig : public SpiBase<FluentSpiConfig> {
    using Base = SpiBase<FluentSpiConfig>;
    friend Base;

    PeripheralId peripheral;
    PinId mosi_pin;
    PinId miso_pin;
    PinId sck_pin;
    SpiConfig config;
    bool tx_only;

    // ========================================================================
    // Inherited Interface from SpiBase (CRTP)
    // ========================================================================

    // Inherit all common SPI methods from base
    using Base::transfer;         // Full-duplex transfer
    using Base::transmit;         // TX-only
    using Base::receive;          // RX-only
    using Base::transfer_byte;    // Single-byte transfer
    using Base::transmit_byte;    // Single-byte TX
    using Base::receive_byte;     // Single-byte RX
    using Base::configure;        // Configuration
    using Base::set_mode;         // Set SPI mode
    using Base::set_speed;        // Set clock speed
    using Base::is_busy;          // Check if busy
    using Base::is_ready;         // Check if ready

    /**
     * @brief Apply configuration to hardware
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> apply() const {
        // TODO: Apply configuration to hardware
        // - Configure GPIO pins with alternate functions
        // - Set SPI mode, clock speed, bit order, data size
        // - Enable SPI peripheral
        return Ok();
    }

    // ========================================================================
    // Implementation Methods (public for concept checking)
    // ========================================================================

    /**
     * @brief Full-duplex transfer implementation
     *
     * @note TX-only configuration returns NotSupported
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transfer_impl(
        std::span<const u8> tx_buffer,
        std::span<u8> rx_buffer
    ) noexcept {
        if (tx_only) {
            return Err(ErrorCode::NotSupported);
        }
        // TODO: Implement hardware transfer
        (void)tx_buffer;
        (void)rx_buffer;
        return Ok();
    }

    /**
     * @brief Transmit-only implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transmit_impl(
        std::span<const u8> tx_buffer
    ) noexcept {
        // TODO: Implement hardware transmit
        (void)tx_buffer;
        return Ok();
    }

    /**
     * @brief Receive-only implementation
     *
     * @note TX-only configuration returns NotSupported
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> receive_impl(
        std::span<u8> rx_buffer
    ) noexcept {
        if (tx_only) {
            return Err(ErrorCode::NotSupported);
        }
        // TODO: Implement hardware receive
        (void)rx_buffer;
        return Ok();
    }

    /**
     * @brief Configure SPI implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_impl(
        const SpiConfig& new_config
    ) noexcept {
        // TODO: Apply configuration to hardware
        (void)new_config;
        return Ok();
    }

    /**
     * @brief Check if busy implementation
     */
    [[nodiscard]] constexpr bool is_busy_impl() const noexcept {
        // TODO: Check hardware busy status
        return false;
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
        if (validation.is_err()) {
            return Err(std::move(validation).err());
        }

        // Create configuration (can't use aggregate init due to protected base constructor)
        FluentSpiConfig config;
        config.peripheral = PeriphId;
        config.mosi_pin = mosi_pin_id_;
        config.miso_pin = miso_pin_id_;
        config.sck_pin = sck_pin_id_;
        config.config = SpiConfig{mode_, clock_speed_, bit_order_, data_size_};
        config.tx_only = !state_.has_miso;  // TX-only if no MISO

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
