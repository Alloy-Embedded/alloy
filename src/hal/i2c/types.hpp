#pragma once

#include <concepts>
#include <span>

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

enum class I2cAddressing : u8 {
    SevenBit = 7,
    TenBit = 10,
};

enum class I2cSpeed : u32 {
    Standard = 100000,
    Fast = 400000,
    FastPlus = 1000000,
    HighSpeed = 3400000,
};

struct I2cConfig {
    I2cSpeed speed;
    I2cAddressing addressing;
    u32 peripheral_clock_hz;

    constexpr I2cConfig(I2cSpeed spd = I2cSpeed::Standard,
                        I2cAddressing addr = I2cAddressing::SevenBit,
                        u32 peripheral_clock = 0u)
        : speed(spd),
          addressing(addr),
          peripheral_clock_hz(peripheral_clock) {}
};

template <typename T>
concept I2cMaster =
    requires(T device, const T const_device, u16 address, std::span<u8> buffer,
             std::span<const u8> const_buffer, I2cConfig config) {
        { device.read(address, buffer) } -> std::same_as<Result<void, ErrorCode>>;
        { device.write(address, const_buffer) } -> std::same_as<Result<void, ErrorCode>>;
        { device.write_read(address, const_buffer, buffer) } -> std::same_as<Result<void, ErrorCode>>;
        { device.scan_bus(buffer) } -> std::same_as<Result<usize, ErrorCode>>;
        { device.configure(config) } -> std::same_as<Result<void, ErrorCode>>;
        { const_device.config() } -> std::same_as<const I2cConfig&>;
    };

template <I2cMaster Device>
inline auto i2c_read_byte(Device& device, u16 address) -> Result<u8, ErrorCode> {
    u8 byte = 0;
    auto buffer = std::span(&byte, 1);
    auto result = device.read(address, buffer);

    if (!result.is_ok()) {
        return Err(std::move(result).error());
    }

    return Ok(static_cast<u8>(byte));
}

template <I2cMaster Device>
inline auto i2c_write_byte(Device& device, u16 address, u8 byte)
    -> Result<void, ErrorCode> {
    auto buffer = std::span(&byte, 1);
    return device.write(address, buffer);
}

template <I2cMaster Device>
inline auto i2c_read_register(Device& device, u16 address, u8 reg_addr)
    -> Result<u8, ErrorCode> {
    u8 value = 0;
    auto write_buf = std::span(&reg_addr, 1);
    auto read_buf = std::span(&value, 1);

    auto result = device.write_read(address, write_buf, read_buf);
    if (!result.is_ok()) {
        return Err(std::move(result).error());
    }

    return Ok(static_cast<u8>(value));
}

template <I2cMaster Device>
inline auto i2c_write_register(Device& device, u16 address, u8 reg_addr, u8 value)
    -> Result<void, ErrorCode> {
    u8 buffer[2] = {reg_addr, value};
    return device.write(address, std::span(buffer, 2));
}

}  // namespace alloy::hal
