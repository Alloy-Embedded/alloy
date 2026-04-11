#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "hal/claim.hpp"
#include "hal/connect.hpp"
#include "hal/types.hpp"
#include "hal/uart/detail/backend.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/descriptors.hpp"

namespace alloy::hal::uart {

using Config = UartConfig;

namespace detail {

[[nodiscard]] constexpr auto as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto strings_equal(const char* lhs, std::string_view rhs) -> bool {
    return as_string(lhs) == rhs;
}

[[nodiscard]] consteval auto find_peripheral_base(std::string_view peripheral_name)
    -> const device::descriptors::startup::PeripheralBase* {
    for (const auto& descriptor : device::descriptors::tables::peripheral_bases) {
        if (strings_equal(descriptor.name, peripheral_name)) {
            return &descriptor;
        }
    }
    return nullptr;
}

}  // namespace detail

template <typename Connector>
requires(Connector::valid) class port_handle {
   public:
    using connector_type = Connector;
    using connector_claim = claim::connector_claim<Connector>;

    static constexpr bool valid = Connector::valid;
    static constexpr auto package_name = Connector::package_name;
    static constexpr auto peripheral_name = Connector::peripheral_type::name;
    static constexpr auto peripheral_base = detail::find_peripheral_base(peripheral_name);

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral_base == nullptr ? 0u : peripheral_base->address;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_uart(*this);
    }

    [[nodiscard]] auto write(std::span<const std::byte> buffer) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::write_uart(*this, buffer);
    }

    [[nodiscard]] auto write_byte(std::byte value) const -> core::Result<void, core::ErrorCode> {
        return detail::write_uart_byte(*this, value);
    }

    [[nodiscard]] auto read(std::span<std::byte> buffer) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::read_uart(*this, buffer);
    }

    [[nodiscard]] auto flush() const -> core::Result<void, core::ErrorCode> {
        return detail::flush_uart(*this);
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(Connector::valid,
                  "Requested UART connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return port_handle<Connector>{config};
}

}  // namespace alloy::hal::uart
