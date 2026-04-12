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
#include "hal/spi/detail/backend.hpp"
#include "hal/spi/types.hpp"

namespace alloy::hal::spi {

using Config = SpiConfig;
using Mode = SpiMode;
using BitOrder = SpiBitOrder;
using DataSize = SpiDataSize;

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
            ? detail::runtime::SpiSchema::unknown
            : detail::runtime::spi_schema_from_id(
                  detail::runtime::as_string(peripheral->backend_schema_id));
    static constexpr bool valid =
        Connector::valid && peripheral != nullptr && schema != detail::runtime::SpiSchema::unknown;

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral == nullptr ? 0u : peripheral->base_address;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_spi(*this);
    }

    [[nodiscard]] auto transfer(std::span<const std::uint8_t> tx_buffer,
                                std::span<std::uint8_t> rx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::transfer_spi(*this, tx_buffer, rx_buffer);
    }

    [[nodiscard]] auto transmit(std::span<const std::uint8_t> tx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::transmit_spi(*this, tx_buffer);
    }

    [[nodiscard]] auto receive(std::span<std::uint8_t> rx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::receive_spi(*this, rx_buffer);
    }

    [[nodiscard]] auto is_busy() const -> bool { return detail::spi_is_busy(*this); }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested SPI connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return port_handle<Connector>{config};
}

}  // namespace alloy::hal::spi
