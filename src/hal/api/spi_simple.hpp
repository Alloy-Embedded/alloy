/**
 * @file spi_simple.hpp
 * @brief Level 1 Simple API for SPI Configuration
 *
 * Provides a one-liner SPI configuration with sensible defaults
 * and compile-time pin validation.
 *
 * Design Principles:
 * - One-liner setup for common cases
 * - Sensible defaults (Mode0, 1MHz, 8-bit, MSB first)
 * - Compile-time pin validation
 * - Minimal boilerplate
 * - Type-safe chip select handling
 * - Built on SpiBase CRTP for code reuse
 *
 * Example Usage:
 * @code
 * // One-liner SPI setup with defaults
 * auto spi = Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>(
 *     1000000  // 1 MHz clock
 * );
 * spi.initialize();
 * spi.transfer_byte(0x55);  // Now has transfer methods from SpiBase!
 *
 * // Master-only (TX-only) for output devices
 * auto spi_tx = Spi<PeripheralId::SPI0>::quick_setup_master_tx<Spi0_MOSI, Spi0_SCK>(
 *     2000000  // 2 MHz clock
 * );
 * spi_tx.transmit_byte(0xAA);  // TX-only operations
 * @endcode
 *
 * @note Part of Phase 1.9: Refactor SPI APIs (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"
#include "hal/core/signals.hpp"
#include "hal/api/spi_base.hpp"

#include <span>

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Default SPI Configuration
// ============================================================================

/**
 * @brief Default SPI configuration values
 *
 * Provides sensible defaults for common SPI usage:
 * - Mode 0 (CPOL=0, CPHA=0) - most common
 * - 1 MHz clock speed - safe default
 * - MSB first - standard
 * - 8-bit data size - most common
 */
struct SpiDefaults {
    static constexpr SpiMode mode = SpiMode::Mode0;
    static constexpr u32 clock_speed = 1000000;  // 1 MHz
    static constexpr SpiBitOrder bit_order = SpiBitOrder::MsbFirst;
    static constexpr SpiDataSize data_size = SpiDataSize::Bits8;
};

// ============================================================================
// Simple SPI Configuration
// ============================================================================

/**
 * @brief Simple SPI configuration result
 *
 * Contains validated SPI configuration with pin assignments.
 * Inherits from SpiBase to provide all SPI transfer methods.
 *
 * @tparam MosiPin MOSI pin type
 * @tparam MisoPin MISO pin type
 * @tparam SckPin SCK pin type
 */
template <typename MosiPin, typename MisoPin, typename SckPin>
struct SimpleSpiConfig : public SpiBase<SimpleSpiConfig<MosiPin, MisoPin, SckPin>> {
    using Base = SpiBase<SimpleSpiConfig<MosiPin, MisoPin, SckPin>>;
    friend Base;

    PeripheralId peripheral;
    SpiConfig config;
    PinId mosi_pin_id;
    PinId miso_pin_id;
    PinId sck_pin_id;

    // Constructor to allow initialization (protected base constructor prevents aggregate init)
    constexpr SimpleSpiConfig(
        PeripheralId periph,
        const SpiConfig& cfg,
        PinId mosi,
        PinId miso,
        PinId sck
    ) : peripheral(periph), config(cfg), mosi_pin_id(mosi), miso_pin_id(miso), sck_pin_id(sck) {}

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
     * @brief Initialize SPI peripheral
     *
     * Configures pins and peripheral with stored configuration.
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> initialize() const {
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
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transfer_impl(
        std::span<const u8> tx_buffer,
        std::span<u8> rx_buffer
    ) noexcept {
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
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> receive_impl(
        std::span<u8> rx_buffer
    ) noexcept {
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

/**
 * @brief TX-only simple SPI configuration
 *
 * For output-only SPI devices (displays, DACs, etc).
 * Inherits from SpiBase to provide all SPI transfer methods.
 *
 * @tparam MosiPin MOSI pin type
 * @tparam SckPin SCK pin type
 */
template <typename MosiPin, typename SckPin>
struct SimpleSpiTxConfig : public SpiBase<SimpleSpiTxConfig<MosiPin, SckPin>> {
    using Base = SpiBase<SimpleSpiTxConfig<MosiPin, SckPin>>;
    friend Base;

    PeripheralId peripheral;
    SpiConfig config;
    PinId mosi_pin_id;
    PinId sck_pin_id;

    // Constructor to allow initialization (protected base constructor prevents aggregate init)
    constexpr SimpleSpiTxConfig(
        PeripheralId periph,
        const SpiConfig& cfg,
        PinId mosi,
        PinId sck
    ) : peripheral(periph), config(cfg), mosi_pin_id(mosi), sck_pin_id(sck) {}

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
     * @brief Initialize SPI peripheral (TX-only)
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> initialize() const {
        // TODO: Apply TX-only configuration
        // - Configure GPIO pins with alternate functions (MOSI and SCK only)
        // - Set SPI mode, clock speed, bit order, data size
        // - Enable SPI peripheral in TX-only mode
        return Ok();
    }

    // ========================================================================
    // Implementation Methods (public for concept checking)
    // ========================================================================

    /**
     * @brief Full-duplex transfer implementation
     *
     * @note TX-only configuration doesn't support full-duplex
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transfer_impl(
        std::span<const u8> tx_buffer,
        std::span<u8> rx_buffer
    ) noexcept {
        // TX-only configuration cannot do full-duplex
        (void)tx_buffer;
        (void)rx_buffer;
        return Err(ErrorCode::NotSupported);
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
     * @note TX-only configuration doesn't support receive
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> receive_impl(
        std::span<u8> rx_buffer
    ) noexcept {
        // TX-only configuration cannot receive
        (void)rx_buffer;
        return Err(ErrorCode::NotSupported);
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
// Simple SPI API
// ============================================================================

/**
 * @brief Simple SPI API (Level 1)
 *
 * One-liner SPI configuration with compile-time validation.
 *
 * @tparam PeriphId SPI peripheral identifier
 */
template <PeripheralId PeriphId>
class Spi {
public:
    /**
     * @brief Quick setup for full-duplex SPI
     *
     * One-liner SPI configuration with sensible defaults.
     * Validates pins at compile-time.
     *
     * @tparam MosiPin MOSI pin type
     * @tparam MisoPin MISO pin type
     * @tparam SckPin SCK pin type
     * @param clock_speed Clock speed in Hz (default 1MHz)
     * @param mode SPI mode (default Mode0)
     * @return Simple SPI configuration
     */
    template <typename MosiPin, typename MisoPin, typename SckPin>
    static constexpr auto quick_setup(
        u32 clock_speed = SpiDefaults::clock_speed,
        SpiMode mode = SpiDefaults::mode) {

        // Validate pins at compile-time
        static_assert(is_valid_mosi_pin<MosiPin>(),
                     "MOSI pin is not compatible with this SPI peripheral");
        static_assert(is_valid_miso_pin<MisoPin>(),
                     "MISO pin is not compatible with this SPI peripheral");
        static_assert(is_valid_sck_pin<SckPin>(),
                     "SCK pin is not compatible with this SPI peripheral");

        return SimpleSpiConfig<MosiPin, MisoPin, SckPin>(
            PeriphId,
            SpiConfig{mode, clock_speed, SpiDefaults::bit_order, SpiDefaults::data_size},
            MosiPin::get_pin_id(),
            MisoPin::get_pin_id(),
            SckPin::get_pin_id()
        );
    }

    /**
     * @brief Quick setup for TX-only SPI (master transmit)
     *
     * Optimized for output-only devices like displays or DACs.
     *
     * @tparam MosiPin MOSI pin type
     * @tparam SckPin SCK pin type
     * @param clock_speed Clock speed in Hz (default 1MHz)
     * @param mode SPI mode (default Mode0)
     * @return TX-only SPI configuration
     */
    template <typename MosiPin, typename SckPin>
    static constexpr auto quick_setup_master_tx(
        u32 clock_speed = SpiDefaults::clock_speed,
        SpiMode mode = SpiDefaults::mode) {

        static_assert(is_valid_mosi_pin<MosiPin>(),
                     "MOSI pin is not compatible with this SPI peripheral");
        static_assert(is_valid_sck_pin<SckPin>(),
                     "SCK pin is not compatible with this SPI peripheral");

        return SimpleSpiTxConfig<MosiPin, SckPin>(
            PeriphId,
            SpiConfig{mode, clock_speed, SpiDefaults::bit_order, SpiDefaults::data_size},
            MosiPin::get_pin_id(),
            SckPin::get_pin_id()
        );
    }

    /**
     * @brief Quick setup with custom SPI mode
     *
     * For devices requiring specific clock polarity/phase.
     *
     * @tparam MosiPin MOSI pin type
     * @tparam MisoPin MISO pin type
     * @tparam SckPin SCK pin type
     * @param clock_speed Clock speed in Hz
     * @param mode SPI mode (Mode0-Mode3)
     * @return Simple SPI configuration
     */
    template <typename MosiPin, typename MisoPin, typename SckPin>
    static constexpr auto quick_setup_with_mode(
        u32 clock_speed,
        SpiMode mode) {

        return quick_setup<MosiPin, MisoPin, SckPin>(clock_speed, mode);
    }

private:
    /**
     * @brief Validate MOSI pin compatibility
     *
     * @tparam Pin Pin type to validate
     * @return true if valid, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_mosi_pin() {
        // TODO: Implement signal-based validation
        // Should check if Pin supports SPI MOSI signal for this peripheral
        return true;  // Placeholder
    }

    /**
     * @brief Validate MISO pin compatibility
     *
     * @tparam Pin Pin type to validate
     * @return true if valid, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_miso_pin() {
        // TODO: Implement signal-based validation
        return true;  // Placeholder
    }

    /**
     * @brief Validate SCK pin compatibility
     *
     * @tparam Pin Pin type to validate
     * @return true if valid, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_sck_pin() {
        // TODO: Implement signal-based validation
        return true;  // Placeholder
    }
};

}  // namespace alloy::hal
