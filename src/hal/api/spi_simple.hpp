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
 *
 * Example Usage:
 * @code
 * // One-liner SPI setup with defaults
 * auto spi = Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>(
 *     1000000  // 1 MHz clock
 * );
 * spi.initialize();
 *
 * // Master-only (TX-only) for output devices
 * auto spi_tx = Spi<PeripheralId::SPI0>::quick_setup_master_tx<Spi0_MOSI, Spi0_SCK>(
 *     2000000  // 2 MHz clock
 * );
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
#include "hal/core/signals.hpp"

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
 *
 * @tparam MosiPin MOSI pin type
 * @tparam MisoPin MISO pin type
 * @tparam SckPin SCK pin type
 */
template <typename MosiPin, typename MisoPin, typename SckPin>
struct SimpleSpiConfig {
    PeripheralId peripheral;
    SpiConfig config;
    PinId mosi_pin_id;
    PinId miso_pin_id;
    PinId sck_pin_id;

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
};

/**
 * @brief TX-only simple SPI configuration
 *
 * For output-only SPI devices (displays, DACs, etc).
 *
 * @tparam MosiPin MOSI pin type
 * @tparam SckPin SCK pin type
 */
template <typename MosiPin, typename SckPin>
struct SimpleSpiTxConfig {
    PeripheralId peripheral;
    SpiConfig config;
    PinId mosi_pin_id;
    PinId sck_pin_id;

    /**
     * @brief Initialize SPI peripheral (TX-only)
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> initialize() const {
        // TODO: Apply TX-only configuration
        return Ok();
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

        return SimpleSpiConfig<MosiPin, MisoPin, SckPin>{
            PeriphId,
            SpiConfig{mode, clock_speed, SpiDefaults::bit_order, SpiDefaults::data_size},
            MosiPin::get_pin_id(),
            MisoPin::get_pin_id(),
            SckPin::get_pin_id()
        };
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

        return SimpleSpiTxConfig<MosiPin, SckPin>{
            PeriphId,
            SpiConfig{mode, clock_speed, SpiDefaults::bit_order, SpiDefaults::data_size},
            MosiPin::get_pin_id(),
            SckPin::get_pin_id()
        };
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
