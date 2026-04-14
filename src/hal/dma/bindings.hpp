#pragma once

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

template <PeripheralId Peripheral, SignalId Signal>
struct BindingTraits {
    using published_traits = device::dma::BindingTraits<Peripheral, Signal>;
    static constexpr bool kPresent = published_traits::kPresent;

    [[nodiscard]] static consteval auto descriptor() {
        static_assert(kPresent, "No DMA binding published for the requested peripheral/signal.");
        return device::dma::DmaBindingDescriptor{
            published_traits::kBindingId,
            Peripheral,
            Signal,
            published_traits::kControllerId,
            published_traits::kRequestLineId,
            published_traits::kRouteId,
            published_traits::kConflictGroupId,
            published_traits::kChannelIndex,
            published_traits::kRequestValue,
            published_traits::kChannelSelector,
        };
    }

   public:
    static constexpr auto kPeripheralId = Peripheral;
    static constexpr auto kSignalId = Signal;
    static constexpr auto kBindingId = published_traits::kBindingId;
    static constexpr auto kControllerId = published_traits::kControllerId;
    static constexpr auto kRequestLineId = published_traits::kRequestLineId;
    static constexpr auto kRouteId = published_traits::kRouteId;
    static constexpr auto kConflictGroupId = published_traits::kConflictGroupId;
    static constexpr auto kChannelIndex = published_traits::kChannelIndex;
    static constexpr auto kRequestValue = published_traits::kRequestValue;
    static constexpr auto kChannelSelector = published_traits::kChannelSelector;
};

template <DmaControllerId Controller>
using ControllerTraits = device::dma::ControllerTraits<Controller>;
#endif

}  // namespace alloy::hal::dma
