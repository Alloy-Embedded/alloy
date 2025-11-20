/**
 * @file i2c_base.hpp
 * @brief CRTP Base Class for I2C APIs
 *
 * Implements the Curiously Recurring Template Pattern (CRTP) to eliminate
 * code duplication across I2cSimple, I2cFluent, and I2cExpert APIs.
 *
 * Design Goals:
 * - Zero runtime overhead (no virtual functions)
 * - Compile-time polymorphism via CRTP
 * - Eliminate code duplication across I2C API levels
 * - Type-safe interface validation
 * - Platform-independent base implementation
 *
 * CRTP Pattern:
 * @code
 * template <typename Derived>
 * class I2cBase {
 *     // Common interface methods that delegate to derived
 *     Result<void> read(...) { return impl().read_impl(...); }
 * };
 *
 * class SimpleI2c : public I2cBase<SimpleI2c> {
 *     friend I2cBase<SimpleI2c>;
 *     Result<void> read_impl(...) { ... }
 * };
 * @endcode
 *
 * Benefits:
 * - Simple/Fluent/Expert share common code
 * - Fixes propagate automatically to all APIs
 * - Binary size reduction
 * - Compilation time improvement
 *
 * @note Part of Phase 1.10: Implement I2cBase (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"

#include <concepts>
#include <span>
#include <type_traits>

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// CRTP Concepts
// ============================================================================

/**
 * @brief Concept to validate I2C implementation
 *
 * Ensures that derived class implements required methods.
 * Provides clear compile-time errors if interface is incomplete.
 *
 * @tparam T Derived I2C implementation type
 */
template <typename T>
concept I2cImplementation = requires(
    T i2c,
    u16 address,
    std::span<const u8> write_buffer,
    std::span<u8> read_buffer,
    I2cConfig cfg
) {
    // Data transfer operations
    { i2c.read_impl(address, read_buffer) } -> std::same_as<Result<void, ErrorCode>>;
    { i2c.write_impl(address, write_buffer) } -> std::same_as<Result<void, ErrorCode>>;
    { i2c.write_read_impl(address, write_buffer, read_buffer) } -> std::same_as<Result<void, ErrorCode>>;

    // Configuration operations
    { i2c.configure_impl(cfg) } -> std::same_as<Result<void, ErrorCode>>;

    // Bus scanning
    { i2c.scan_bus_impl(read_buffer) } -> std::same_as<Result<usize, ErrorCode>>;
};

// ============================================================================
// CRTP Base Class
// ============================================================================

/**
 * @brief CRTP base class for I2C APIs
 *
 * Provides common interface methods that delegate to derived implementation.
 * Uses CRTP pattern for zero-overhead compile-time polymorphism.
 *
 * @tparam Derived The derived I2C class (SimpleI2cConfig, FluentI2cConfig, etc.)
 *
 * Usage:
 * @code
 * template <typename HardwarePolicy>
 * class SimpleI2c : public I2cBase<SimpleI2c<HardwarePolicy>> {
 *     friend I2cBase<SimpleI2c<HardwarePolicy>>;
 * private:
 *     // Implementation methods (called by base)
 *     Result<void> read_impl(...) noexcept { ... }
 * };
 * @endcode
 */
template <typename Derived>
class I2cBase {
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
     * @brief Read data from I2C device
     *
     * Reads data from the specified I2C device address into the buffer.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @param buffer Buffer to store received data
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Possible errors:
     * - ErrorCode::I2cNack: Device did not acknowledge
     * - ErrorCode::I2cBusBusy: Bus is busy
     * - ErrorCode::Timeout: Operation timed out
     *
     * Example:
     * @code
     * u8 data[4];
     * i2c.read(0x50, std::span(data)).expect("Read failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> read(
        u16 address,
        std::span<u8> buffer
    ) noexcept {
        return impl().read_impl(address, buffer);
    }

    /**
     * @brief Write data to I2C device
     *
     * Writes data from the buffer to the specified I2C device address.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @param buffer Buffer containing data to send
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Possible errors:
     * - ErrorCode::I2cNack: Device did not acknowledge
     * - ErrorCode::I2cBusBusy: Bus is busy
     * - ErrorCode::Timeout: Operation timed out
     *
     * Example:
     * @code
     * u8 data[] = {0x01, 0x02, 0x03};
     * i2c.write(0x50, std::span(data)).expect("Write failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write(
        u16 address,
        std::span<const u8> buffer
    ) noexcept {
        return impl().write_impl(address, buffer);
    }

    /**
     * @brief Write then read from I2C device (repeated start)
     *
     * Performs a write operation followed by a read operation with
     * a repeated start condition (no stop between write and read).
     * This is commonly used for register reads.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @param write_buffer Buffer containing data to write
     * @param read_buffer Buffer to store received data
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Possible errors:
     * - ErrorCode::I2cNack: Device did not acknowledge
     * - ErrorCode::I2cBusBusy: Bus is busy
     * - ErrorCode::Timeout: Operation timed out
     *
     * Example:
     * @code
     * u8 reg_addr = 0x10;
     * u8 value[2];
     * i2c.write_read(0x50, std::span(&reg_addr, 1), std::span(value))
     *    .expect("Register read failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_read(
        u16 address,
        std::span<const u8> write_buffer,
        std::span<u8> read_buffer
    ) noexcept {
        return impl().write_read_impl(address, write_buffer, read_buffer);
    }

    // ========================================================================
    // Convenience Single-Byte Operations
    // ========================================================================

    /**
     * @brief Read single byte from I2C device
     *
     * Convenience method for single-byte read.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @return Ok(byte) on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * auto byte = i2c.read_byte(0x50).expect("Read failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<u8, ErrorCode> read_byte(u16 address) noexcept {
        u8 byte = 0;
        auto buffer = std::span(&byte, 1);

        auto result = impl().read_impl(address, buffer);
        if (result.is_err()) {
            return Err(std::move(result).err());
        }

        return Ok(u8{byte});
    }

    /**
     * @brief Write single byte to I2C device
     *
     * Convenience method for single-byte write.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @param byte Byte to write
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * i2c.write_byte(0x50, 0xAA).expect("Write failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_byte(
        u16 address,
        u8 byte
    ) noexcept {
        auto buffer = std::span(&byte, 1);
        return impl().write_impl(address, buffer);
    }

    /**
     * @brief Read register from I2C device
     *
     * Common pattern: write register address, then read value.
     * Uses repeated start between write and read.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @param reg_addr Register address
     * @return Ok(value) on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * auto value = i2c.read_register(0x50, 0x10).expect("Register read failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<u8, ErrorCode> read_register(
        u16 address,
        u8 reg_addr
    ) noexcept {
        u8 value = 0;
        auto write_buf = std::span(&reg_addr, 1);
        auto read_buf = std::span(&value, 1);

        auto result = impl().write_read_impl(address, write_buf, read_buf);
        if (result.is_err()) {
            return Err(std::move(result).err());
        }

        return Ok(u8{value});
    }

    /**
     * @brief Write register to I2C device
     *
     * Common pattern: write register address followed by value.
     *
     * @param address 7-bit or 10-bit I2C device address
     * @param reg_addr Register address
     * @param value Value to write
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * i2c.write_register(0x50, 0x10, 0xAA).expect("Register write failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_register(
        u16 address,
        u8 reg_addr,
        u8 value
    ) noexcept {
        u8 buffer[2] = {reg_addr, value};
        auto span = std::span(buffer, 2);
        return impl().write_impl(address, span);
    }

    // ========================================================================
    // Bus Scanning Operations
    // ========================================================================

    /**
     * @brief Scan I2C bus for devices
     *
     * Attempts to communicate with all possible addresses and returns
     * addresses that responded.
     *
     * @param found_devices Buffer to store found device addresses (as u8)
     * @return Ok(count) with number of devices found, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * u8 devices[128];
     * auto count = i2c.scan_bus(std::span(devices)).expect("Scan failed");
     * // devices[0..count] contains found addresses
     * @endcode
     */
    [[nodiscard]] constexpr Result<usize, ErrorCode> scan_bus(
        std::span<u8> found_devices
    ) noexcept {
        return impl().scan_bus_impl(found_devices);
    }

    // ========================================================================
    // Configuration Operations
    // ========================================================================

    /**
     * @brief Configure I2C peripheral
     *
     * Updates I2C configuration (speed, addressing mode).
     * May temporarily disrupt communication.
     *
     * @param config I2C configuration
     * @return Ok() on success, Err(ErrorCode) on invalid configuration
     *
     * Example:
     * @code
     * I2cConfig cfg{I2cSpeed::Fast, I2cAddressing::SevenBit};
     * i2c.configure(cfg).expect("Config failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure(
        const I2cConfig& config
    ) noexcept {
        return impl().configure_impl(config);
    }

    /**
     * @brief Change I2C bus speed
     *
     * Convenience method to change only the bus speed.
     *
     * @param speed New I2C speed
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_speed(I2cSpeed speed) noexcept {
        I2cConfig config{speed, I2cAddressing::SevenBit};
        return impl().configure_impl(config);
    }

    /**
     * @brief Change addressing mode
     *
     * Convenience method to change only the addressing mode.
     *
     * @param addressing New addressing mode
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_addressing(
        I2cAddressing addressing
    ) noexcept {
        I2cConfig config{I2cSpeed::Standard, addressing};
        return impl().configure_impl(config);
    }

protected:
    // Default constructor (protected - only derived can construct)
    constexpr I2cBase() noexcept = default;

    // Copy/move allowed for derived classes
    constexpr I2cBase(const I2cBase&) noexcept = default;
    constexpr I2cBase(I2cBase&&) noexcept = default;
    constexpr I2cBase& operator=(const I2cBase&) noexcept = default;
    constexpr I2cBase& operator=(I2cBase&&) noexcept = default;

    // Destructor (protected - prevent deletion through base pointer)
    ~I2cBase() noexcept = default;
};

// ============================================================================
// Static Assertions
// ============================================================================

// Note: Zero-overhead validation is performed within the class template itself
// using static_assert on sizeof(I2cBase) and std::is_empty_v<I2cBase>.
// This ensures validation only occurs when I2cBase is properly used with CRTP.

} // namespace alloy::hal
