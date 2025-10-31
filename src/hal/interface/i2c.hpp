/// Platform-agnostic I2C interface
///
/// Defines I2C concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_I2C_HPP
#define ALLOY_HAL_INTERFACE_I2C_HPP

#include "core/error.hpp"
#include "core/types.hpp"
#include <concepts>
#include <span>

namespace alloy::hal {

/// I2C addressing mode
enum class I2cAddressing : core::u8 {
    SevenBit  = 7,   ///< 7-bit addressing (most common)
    TenBit    = 10   ///< 10-bit addressing (extended)
};

/// I2C bus speed
enum class I2cSpeed : core::u32 {
    Standard  = 100000,   ///< Standard mode: 100 kHz
    Fast      = 400000,   ///< Fast mode: 400 kHz
    FastPlus  = 1000000,  ///< Fast mode plus: 1 MHz
    HighSpeed = 3400000   ///< High-speed mode: 3.4 MHz
};

/// I2C configuration parameters
///
/// Contains all parameters needed to configure an I2C peripheral.
struct I2cConfig {
    I2cSpeed speed;
    I2cAddressing addressing;

    /// Constructor with default configuration
    constexpr I2cConfig(I2cSpeed spd = I2cSpeed::Standard,
                       I2cAddressing addr = I2cAddressing::SevenBit)
        : speed(spd), addressing(addr) {}
};

/// I2C master device concept
///
/// Defines the interface that all I2C master implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Error codes specific to I2C:
/// - ErrorCode::I2cNack: Device did not acknowledge (not present or busy)
/// - ErrorCode::I2cBusBusy: Bus is busy (another master is active)
/// - ErrorCode::I2cArbitrationLost: Lost arbitration in multi-master setup
/// - ErrorCode::Timeout: Operation timed out
/// - ErrorCode::InvalidParameter: Invalid address or buffer
template<typename T>
concept I2cMaster = requires(T device, const T const_device,
                             core::u16 address,
                             std::span<core::u8> buffer,
                             std::span<const core::u8> const_buffer,
                             I2cConfig config) {
    /// Read data from I2C device
    ///
    /// @param address 7-bit or 10-bit I2C device address
    /// @param buffer Buffer to store received data
    /// @return Ok on success, error code on failure
    { device.read(address, buffer) } -> std::same_as<core::Result<void>>;

    /// Write data to I2C device
    ///
    /// @param address 7-bit or 10-bit I2C device address
    /// @param buffer Buffer containing data to send
    /// @return Ok on success, error code on failure
    { device.write(address, const_buffer) } -> std::same_as<core::Result<void>>;

    /// Write then read from I2C device (repeated start)
    ///
    /// Useful for register reads: write register address, then read value.
    ///
    /// @param address 7-bit or 10-bit I2C device address
    /// @param write_buffer Buffer containing data to write
    /// @param read_buffer Buffer to store received data
    /// @return Ok on success, error code on failure
    { device.write_read(address, const_buffer, buffer) } -> std::same_as<core::Result<void>>;

    /// Scan I2C bus for devices
    ///
    /// Attempts to communicate with all possible addresses and returns
    /// a list of addresses that responded.
    ///
    /// @param found_devices Buffer to store found device addresses
    /// @return Number of devices found, or error code
    { device.scan_bus(buffer) } -> std::same_as<core::Result<core::usize>>;

    /// Configure I2C peripheral
    ///
    /// @param config I2C configuration (speed, addressing mode)
    /// @return Ok on success, error code on failure
    { device.configure(config) } -> std::same_as<core::Result<void>>;
};

/// Helper function to read a single byte from I2C device
///
/// @tparam Device I2C device type satisfying I2cMaster concept
/// @param device I2C device instance
/// @param address 7-bit or 10-bit I2C device address
/// @return Byte value or error code
template<I2cMaster Device>
core::Result<core::u8> i2c_read_byte(Device& device, core::u16 address) {
    core::u8 byte = 0;
    auto buffer = std::span(&byte, 1);
    auto result = device.read(address, buffer);

    if (result.is_error()) {
        return core::Result<core::u8>::error(result.error());
    }

    return core::Result<core::u8>::ok(byte);
}

/// Helper function to write a single byte to I2C device
///
/// @tparam Device I2C device type satisfying I2cMaster concept
/// @param device I2C device instance
/// @param address 7-bit or 10-bit I2C device address
/// @param byte Byte to write
/// @return Ok on success, error code on failure
template<I2cMaster Device>
core::Result<void> i2c_write_byte(Device& device, core::u16 address, core::u8 byte) {
    auto buffer = std::span(&byte, 1);
    return device.write(address, buffer);
}

/// Helper function to read a register from I2C device
///
/// Common pattern: write register address, then read register value.
///
/// @tparam Device I2C device type satisfying I2cMaster concept
/// @param device I2C device instance
/// @param address 7-bit or 10-bit I2C device address
/// @param reg_addr Register address to read from
/// @return Register value or error code
template<I2cMaster Device>
core::Result<core::u8> i2c_read_register(Device& device, core::u16 address, core::u8 reg_addr) {
    core::u8 value = 0;
    auto write_buf = std::span(&reg_addr, 1);
    auto read_buf = std::span(&value, 1);

    auto result = device.write_read(address, write_buf, read_buf);

    if (result.is_error()) {
        return core::Result<core::u8>::error(result.error());
    }

    return core::Result<core::u8>::ok(value);
}

/// Helper function to write a register to I2C device
///
/// Common pattern: write register address followed by register value.
///
/// @tparam Device I2C device type satisfying I2cMaster concept
/// @param device I2C device instance
/// @param address 7-bit or 10-bit I2C device address
/// @param reg_addr Register address to write to
/// @param value Value to write
/// @return Ok on success, error code on failure
template<I2cMaster Device>
core::Result<void> i2c_write_register(Device& device, core::u16 address,
                                     core::u8 reg_addr, core::u8 value) {
    core::u8 buffer[2] = {reg_addr, value};
    return device.write(address, std::span(buffer, 2));
}

} // namespace alloy::hal

#endif // ALLOY_HAL_INTERFACE_I2C_HPP
