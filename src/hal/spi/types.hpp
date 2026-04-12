#pragma once

#include <concepts>
#include <span>

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

enum class SpiMode : u8 {
    Mode0 = 0,
    Mode1 = 1,
    Mode2 = 2,
    Mode3 = 3,
};

enum class SpiBitOrder : u8 {
    MsbFirst = 0,
    LsbFirst = 1,
};

enum class SpiDataSize : u8 {
    Bits8 = 8,
    Bits16 = 16,
};

struct SpiConfig {
    SpiMode mode;
    u32 clock_speed;
    SpiBitOrder bit_order;
    SpiDataSize data_size;
    u32 peripheral_clock_hz;

    constexpr SpiConfig(SpiMode m = SpiMode::Mode0, u32 speed = 1000000,
                        SpiBitOrder order = SpiBitOrder::MsbFirst,
                        SpiDataSize size = SpiDataSize::Bits8,
                        u32 peripheral_clock = 0u)
        : mode(m),
          clock_speed(speed),
          bit_order(order),
          data_size(size),
          peripheral_clock_hz(peripheral_clock) {}
};

template <typename T>
concept SpiMaster =
    requires(T device, const T const_device, std::span<u8> buffer,
             std::span<const u8> const_buffer, SpiConfig config) {
        { device.transfer(const_buffer, buffer) } -> std::same_as<Result<void, ErrorCode>>;
        { device.transmit(const_buffer) } -> std::same_as<Result<void, ErrorCode>>;
        { device.receive(buffer) } -> std::same_as<Result<void, ErrorCode>>;
        { device.configure(config) } -> std::same_as<Result<void, ErrorCode>>;
        { const_device.is_busy() } -> std::same_as<bool>;
        { const_device.config() } -> std::same_as<const SpiConfig&>;
    };

template <SpiMaster Device>
inline auto spi_transfer_byte(Device& device, u8 tx_byte) -> Result<u8, ErrorCode> {
    u8 rx_byte = 0;
    auto tx_buf = std::span(&tx_byte, 1);
    auto rx_buf = std::span(&rx_byte, 1);

    auto result = device.transfer(tx_buf, rx_buf);
    if (!result.is_ok()) {
        return Err(std::move(result).error());
    }

    return Ok(static_cast<u8>(rx_byte));
}

template <SpiMaster Device>
inline auto spi_write_byte(Device& device, u8 byte) -> Result<void, ErrorCode> {
    auto buffer = std::span(&byte, 1);
    return device.transmit(buffer);
}

template <SpiMaster Device>
inline auto spi_read_byte(Device& device) -> Result<u8, ErrorCode> {
    u8 byte = 0;
    auto buffer = std::span(&byte, 1);

    auto result = device.receive(buffer);
    if (!result.is_ok()) {
        return Err(std::move(result).error());
    }

    return Ok(static_cast<u8>(byte));
}

template <typename GpioPin>
class SpiChipSelect {
   public:
    explicit SpiChipSelect(GpioPin& pin) : pin_(pin) { pin_.set_low(); }
    ~SpiChipSelect() { pin_.set_high(); }

    SpiChipSelect(const SpiChipSelect&) = delete;
    auto operator=(const SpiChipSelect&) -> SpiChipSelect& = delete;
    SpiChipSelect(SpiChipSelect&&) = delete;
    auto operator=(SpiChipSelect&&) -> SpiChipSelect& = delete;

   private:
    GpioPin& pin_;
};

}  // namespace alloy::hal
