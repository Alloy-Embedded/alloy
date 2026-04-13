#pragma once

#include <cstddef>
#include <utility>

#include "device/dma.hpp"

namespace alloy::hal::dma {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
using PeripheralId = device::dma::PeripheralId;
using SignalId = device::dma::SignalId;
using DmaBindingId = device::dma::DmaBindingId;
using DmaControllerId = device::dma::DmaControllerId;
using DmaRequestLineId = device::dma::DmaRequestLineId;
using DmaRouteId = device::dma::DmaRouteId;
using DmaConflictGroupId = device::dma::DmaConflictGroupId;

namespace detail {

template <PeripheralId Peripheral, SignalId Signal, std::size_t... Index>
[[nodiscard]] consteval auto resolve_binding_index(std::index_sequence<Index...>) -> std::size_t {
    constexpr auto not_found = sizeof...(Index);
    auto resolved = not_found;

    auto match = [&]<std::size_t I>() consteval {
        if (resolved != not_found) {
            return;
        }

        constexpr auto binding = device::dma::bindings[I];
        if (binding.peripheral_id == Peripheral && binding.signal_id == Signal) {
            resolved = I;
        }
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

}  // namespace detail

template <PeripheralId Peripheral, SignalId Signal>
struct BindingTraits {
    static constexpr auto kIndex =
        detail::resolve_binding_index<Peripheral, Signal>(
            std::make_index_sequence<std::size(device::dma::bindings)>{});
    static constexpr bool kPresent = kIndex < std::size(device::dma::bindings);

    [[nodiscard]] static consteval auto descriptor() {
        static_assert(kPresent, "No DMA binding published for the requested peripheral/signal.");
        return device::dma::bindings[kIndex];
    }

   private:
    [[nodiscard]] static consteval auto controller_id_value() {
        if constexpr (kPresent) {
            return descriptor().controller_id;
        } else {
            return DmaControllerId::none;
        }
    }

    [[nodiscard]] static consteval auto request_line_id_value() {
        if constexpr (kPresent) {
            return descriptor().request_line_id;
        } else {
            return DmaRequestLineId::none;
        }
    }

    [[nodiscard]] static consteval auto route_id_value() {
        if constexpr (kPresent) {
            return descriptor().route_id;
        } else {
            return DmaRouteId::none;
        }
    }

    [[nodiscard]] static consteval auto conflict_group_id_value() {
        if constexpr (kPresent) {
            return descriptor().conflict_group_id;
        } else {
            return DmaConflictGroupId::none;
        }
    }

   public:
    static constexpr auto kPeripheralId = Peripheral;
    static constexpr auto kSignalId = Signal;
    static constexpr auto kControllerId = controller_id_value();
    static constexpr auto kRequestLineId = request_line_id_value();
    static constexpr auto kRouteId = route_id_value();
    static constexpr auto kConflictGroupId = conflict_group_id_value();
};
#endif

}  // namespace alloy::hal::dma
