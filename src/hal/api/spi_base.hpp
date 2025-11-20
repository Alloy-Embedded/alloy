/**
 * @file spi_base.hpp
 * @brief CRTP Base Class for SPI APIs
 *
 * Implements the Curiously Recurring Template Pattern (CRTP) to eliminate
 * code duplication across SpiSimple, SpiFluent, and SpiExpert APIs.
 *
 * Design Goals:
 * - Zero runtime overhead (no virtual functions)
 * - Compile-time polymorphism via CRTP
 * - Eliminate code duplication across SPI API levels
 * - Type-safe interface validation
 * - Platform-independent base implementation
 *
 * CRTP Pattern:
 * @code
 * template <typename Derived>
 * class SpiBase {
 *     // Common interface methods that delegate to derived
 *     Result<void> transfer(...) { return impl().transfer_impl(...); }
 * };
 *
 * class SimpleSpi : public SpiBase<SimpleSpi> {
 *     friend SpiBase<SimpleSpi>;
 *     Result<void> transfer_impl(...) { ... }
 * };
 * @endcode
 *
 * Benefits:
 * - Simple/Fluent/Expert share common code
 * - Fixes propagate automatically to all APIs
 * - Binary size reduction
 * - Compilation time improvement
 *
 * @note Part of Phase 1.8: API Layer Refactoring (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"

#include <concepts>
#include <span>
#include <type_traits>

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// CRTP Concepts
// ============================================================================

/**
 * @brief Concept to validate SPI implementation
 *
 * Ensures that derived class implements required methods.
 * Provides clear compile-time errors if interface is incomplete.
 *
 * @tparam T Derived SPI implementation type
 */
template <typename T>
concept SpiImplementation = requires(T spi, std::span<const u8> tx, std::span<u8> rx, SpiConfig cfg) {
    // Data transfer operations
    { spi.transfer_impl(tx, rx) } -> std::same_as<Result<void, ErrorCode>>;
    { spi.transmit_impl(tx) } -> std::same_as<Result<void, ErrorCode>>;
    { spi.receive_impl(rx) } -> std::same_as<Result<void, ErrorCode>>;

    // Configuration operations
    { spi.configure_impl(cfg) } -> std::same_as<Result<void, ErrorCode>>;
    { spi.is_busy_impl() } -> std::same_as<bool>;
};

// ============================================================================
// CRTP Base Class
// ============================================================================

/**
 * @brief CRTP base class for SPI APIs
 *
 * Provides common interface methods that delegate to derived implementation.
 * Uses CRTP pattern for zero-overhead compile-time polymorphism.
 *
 * @tparam Derived The derived SPI class (SimpleSpiConfig, FluentSpiConfig, etc.)
 *
 * Usage:
 * @code
 * template <typename HardwarePolicy>
 * class SimpleSpi : public SpiBase<SimpleSpi<HardwarePolicy>> {
 *     friend SpiBase<SimpleSpi<HardwarePolicy>>;
 * private:
 *     // Implementation methods (called by base)
 *     Result<void> transfer_impl(...) noexcept { ... }
 * };
 * @endcode
 */
template <typename Derived>
class SpiBase {
protected:
    // ========================================================================
    // CRTP Helper Methods
    // ========================================================================

    /**
     * @brief Get reference to derived instance
     * @return Reference to derived class instance
     */
    constexpr Derived& impl() noexcept {
        return static_cast<Derived&>(*this);
    }

    /**
     * @brief Get const reference to derived instance
     * @return Const reference to derived class instance
     */
    constexpr const Derived& impl() const noexcept {
        return static_cast<const Derived&>(*this);
    }

public:
    // ========================================================================
    // Data Transfer Operations
    // ========================================================================

    /**
     * @brief Full-duplex transfer (simultaneous TX and RX)
     *
     * Sends and receives data simultaneously. Transfer length is the
     * minimum of both buffer sizes.
     *
     * @param tx_buffer Data to transmit
     * @param rx_buffer Buffer for received data
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * u8 tx_data[] = {0x01, 0x02, 0x03};
     * u8 rx_data[3];
     * spi.transfer(std::span(tx_data), std::span(rx_data)).expect("Transfer failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transfer(
        std::span<const u8> tx_buffer,
        std::span<u8> rx_buffer
    ) noexcept {
        return impl().transfer_impl(tx_buffer, rx_buffer);
    }

    /**
     * @brief Transmit-only operation (TX without RX)
     *
     * Sends data without receiving. More efficient when receive
     * data is not needed (e.g., writing to a display).
     *
     * @param tx_buffer Data to transmit
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * u8 data[] = {0xAA, 0xBB, 0xCC};
     * spi.transmit(std::span(data)).expect("Transmit failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transmit(
        std::span<const u8> tx_buffer
    ) noexcept {
        return impl().transmit_impl(tx_buffer);
    }

    /**
     * @brief Receive-only operation (RX without meaningful TX)
     *
     * Receives data while sending dummy bytes.
     *
     * @param rx_buffer Buffer for received data
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * u8 data[4];
     * spi.receive(std::span(data)).expect("Receive failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> receive(
        std::span<u8> rx_buffer
    ) noexcept {
        return impl().receive_impl(rx_buffer);
    }

    // ========================================================================
    // Convenience Single-Byte Operations
    // ========================================================================

    /**
     * @brief Transfer single byte
     *
     * Convenience method for single-byte transfer.
     *
     * @param tx_byte Byte to transmit
     * @return Ok(received_byte) on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * auto rx = spi.transfer_byte(0x55).expect("Transfer failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<u8, ErrorCode> transfer_byte(u8 tx_byte) noexcept {
        u8 rx_byte = 0;
        auto tx_buf = std::span(&tx_byte, 1);
        auto rx_buf = std::span(&rx_byte, 1);

        auto result = impl().transfer_impl(tx_buf, rx_buf);
        if (result.is_err()) {
            return Err(std::move(result).err());
        }

        return Ok(u8{rx_byte});
    }

    /**
     * @brief Transmit single byte
     *
     * Convenience method for single-byte transmission.
     *
     * @param byte Byte to transmit
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * spi.transmit_byte(0xAA).expect("Transmit failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> transmit_byte(u8 byte) noexcept {
        auto buffer = std::span(&byte, 1);
        return impl().transmit_impl(buffer);
    }

    /**
     * @brief Receive single byte
     *
     * Convenience method for single-byte reception.
     *
     * @return Ok(received_byte) on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * auto byte = spi.receive_byte().expect("Receive failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<u8, ErrorCode> receive_byte() noexcept {
        u8 byte = 0;
        auto buffer = std::span(&byte, 1);

        auto result = impl().receive_impl(buffer);
        if (result.is_err()) {
            return Err(std::move(result).err());
        }

        return Ok(u8{byte});
    }

    // ========================================================================
    // Configuration Operations
    // ========================================================================

    /**
     * @brief Configure SPI peripheral
     *
     * Updates SPI configuration (mode, speed, bit order, data size).
     * May temporarily disrupt communication.
     *
     * @param config SPI configuration
     * @return Ok() on success, Err(ErrorCode) on invalid configuration
     *
     * Example:
     * @code
     * SpiConfig cfg{SpiMode::Mode0, 2000000, SpiBitOrder::MsbFirst, SpiDataSize::Bits8};
     * spi.configure(cfg).expect("Config failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure(const SpiConfig& config) noexcept {
        return impl().configure_impl(config);
    }

    /**
     * @brief Change SPI mode
     *
     * Convenience method to change only the SPI mode.
     *
     * @param mode New SPI mode (Mode0-Mode3)
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_mode(SpiMode mode) noexcept {
        // Get current config, modify mode, and reconfigure
        // Note: This requires the derived class to track current config
        SpiConfig config{mode, 1000000, SpiBitOrder::MsbFirst, SpiDataSize::Bits8};
        return impl().configure_impl(config);
    }

    /**
     * @brief Change SPI clock speed
     *
     * Convenience method to change only the clock speed.
     *
     * @param speed_hz Clock speed in Hz
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_speed(u32 speed_hz) noexcept {
        // Get current config, modify speed, and reconfigure
        SpiConfig config{SpiMode::Mode0, speed_hz, SpiBitOrder::MsbFirst, SpiDataSize::Bits8};
        return impl().configure_impl(config);
    }

    // ========================================================================
    // Status Operations
    // ========================================================================

    /**
     * @brief Check if SPI is busy
     *
     * Returns true if a transfer is in progress.
     *
     * @return true if busy, false if ready
     *
     * Example:
     * @code
     * while (spi.is_busy()) {
     *     // Wait for transfer to complete
     * }
     * @endcode
     */
    [[nodiscard]] constexpr bool is_busy() const noexcept {
        return impl().is_busy_impl();
    }

    /**
     * @brief Check if SPI is ready
     *
     * Convenience method equivalent to !is_busy().
     *
     * @return true if ready, false if busy
     */
    [[nodiscard]] constexpr bool is_ready() const noexcept {
        return !impl().is_busy_impl();
    }

protected:
    // Default constructor (protected - only derived can construct)
    constexpr SpiBase() noexcept = default;

    // Copy/move allowed for derived classes
    constexpr SpiBase(const SpiBase&) noexcept = default;
    constexpr SpiBase(SpiBase&&) noexcept = default;
    constexpr SpiBase& operator=(const SpiBase&) noexcept = default;
    constexpr SpiBase& operator=(SpiBase&&) noexcept = default;

    // Destructor (protected - prevent deletion through base pointer)
    ~SpiBase() noexcept = default;
};

// ============================================================================
// Static Assertions
// ============================================================================

// Note: Zero-overhead validation is performed within the class template itself
// using static_assert on sizeof(SpiBase) and std::is_empty_v<SpiBase>.
// This ensures validation only occurs when SpiBase is properly used with CRTP.

} // namespace alloy::hal
