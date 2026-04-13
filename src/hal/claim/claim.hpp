#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/dma/bindings.hpp"

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

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
template <dma::PeripheralId Peripheral, dma::SignalId Signal>
requires(dma::BindingTraits<Peripheral, Signal>::kPresent) struct dma_claim {
    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto signal_id = Signal;
    static constexpr auto binding_id = dma::BindingTraits<Peripheral, Signal>::descriptor().binding_id;
    static constexpr auto controller_id = dma::BindingTraits<Peripheral, Signal>::kControllerId;
    static constexpr auto request_line_id = dma::BindingTraits<Peripheral, Signal>::kRequestLineId;
    static constexpr auto route_id = dma::BindingTraits<Peripheral, Signal>::kRouteId;
    static constexpr auto conflict_group_id =
        dma::BindingTraits<Peripheral, Signal>::kConflictGroupId;
};
#endif

template <typename Connector>
requires(Connector::valid) struct connector_claim {
    using connector_type = Connector;
    using peripheral = peripheral_claim<typename Connector::peripheral_type>;

    static constexpr auto pin_count = Connector::binding_count;

    [[nodiscard]] static consteval auto requirement_count() -> std::size_t {
        return Connector::requirements().size();
    }

    [[nodiscard]] static consteval auto operation_count() -> std::size_t {
        return Connector::operations().size();
    }

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
