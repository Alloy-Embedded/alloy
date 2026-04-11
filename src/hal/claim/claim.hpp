#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "hal/connect/connect.hpp"

namespace alloy::hal::claim {

template <typename Pin>
struct pin_claim {
    using pin_type = Pin;
    static constexpr auto pin_name = Pin::name;
};

template <typename Peripheral>
struct peripheral_claim {
    using peripheral_type = Peripheral;
    static constexpr auto peripheral_name = Peripheral::name;
};

template <connection::FixedString InterruptName>
struct interrupt_claim {
    static constexpr auto interrupt_name = std::string_view{InterruptName};
};

template <connection::FixedString ControllerName, connection::FixedString RequestLineName>
struct dma_claim {
    static constexpr auto controller_name = std::string_view{ControllerName};
    static constexpr auto request_line_name = std::string_view{RequestLineName};
};

template <typename Connector>
requires(Connector::valid) struct connector_claim {
    using connector_type = Connector;
    using peripheral = peripheral_claim<typename Connector::peripheral_type>;

    static constexpr auto pin_count = Connector::binding_count;
    static constexpr auto requirement_count = Connector::requirements().size();
    static constexpr auto operation_count = Connector::operations().size();

    template <std::size_t Index>
    [[nodiscard]] static consteval auto pin_name_at() -> std::string_view {
        using binding_type = typename Connector::template binding_type<Index>;
        return binding_type::pin_type::name;
    }

    template <std::size_t Index>
    [[nodiscard]] static consteval auto signal_name_at() -> std::string_view {
        using binding_type = typename Connector::template binding_type<Index>;
        return binding_type::signal_type::name;
    }

    [[nodiscard]] static consteval auto pins() {
        return []<std::size_t... Index>(std::index_sequence<Index...>) {
            return std::array<std::string_view, sizeof...(Index)>{pin_name_at<Index>()...};
        }
        (std::make_index_sequence<Connector::binding_count>{});
    }

    [[nodiscard]] static consteval auto signals() {
        return []<std::size_t... Index>(std::index_sequence<Index...>) {
            return std::array<std::string_view, sizeof...(Index)>{signal_name_at<Index>()...};
        }
        (std::make_index_sequence<Connector::binding_count>{});
    }
};

}  // namespace alloy::hal::claim
