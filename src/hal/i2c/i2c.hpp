#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/descriptors.hpp"
#include "hal/claim.hpp"
#include "hal/connect.hpp"
#include "hal/i2c/detail/backend.hpp"
#include "hal/i2c/types.hpp"

namespace alloy::hal::i2c {

using Config = I2cConfig;
using Addressing = I2cAddressing;
using Speed = I2cSpeed;

namespace detail {

namespace runtime = alloy::hal::detail::runtime;

}  // namespace detail

template <typename Connector>
requires(Connector::valid) class port_handle {
   public:
    using connector_type = Connector;
    using connector_claim = claim::connector_claim<Connector>;

    static constexpr auto package_name = Connector::package_name;
    static constexpr auto peripheral_name = Connector::peripheral_type::name;
    static constexpr auto peripheral = detail::runtime::find_peripheral_descriptor(peripheral_name);
    static constexpr auto schema =
        peripheral == nullptr
            ? detail::runtime::I2cSchema::unknown
            : detail::runtime::i2c_schema_from_id(
                  detail::runtime::as_string(peripheral->backend_schema_id));
    static constexpr bool valid =
        Connector::valid && peripheral != nullptr && schema != detail::runtime::I2cSchema::unknown;

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral == nullptr ? 0u : peripheral->base_address;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_i2c(*this);
    }

    [[nodiscard]] auto read(std::uint16_t address, std::span<std::uint8_t> buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::read_i2c(*this, address, buffer);
    }

    [[nodiscard]] auto write(std::uint16_t address,
                             std::span<const std::uint8_t> buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::write_i2c(*this, address, buffer);
    }

    [[nodiscard]] auto write_read(std::uint16_t address,
                                  std::span<const std::uint8_t> write_buffer,
                                  std::span<std::uint8_t> read_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::write_read_i2c(*this, address, write_buffer, read_buffer);
    }

    [[nodiscard]] auto scan_bus(std::span<std::uint8_t> found_devices) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::scan_i2c_bus(*this, found_devices);
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested I2C connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return port_handle<Connector>{config};
}

}  // namespace alloy::hal::i2c
