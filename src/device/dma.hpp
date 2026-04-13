#pragma once

#include <type_traits>
#include <span>
#include <utility>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
namespace dma {

namespace device_contract = selected::dma_device_contract;

using DmaBindingId = device_contract::DmaBindingId;
using DmaControllerId = device_contract::DmaControllerId;
using DmaRequestLineId = device_contract::DmaRequestLineId;
using DmaRouteId = device_contract::DmaRouteId;
using DmaConflictGroupId = device_contract::DmaConflictGroupId;
using DmaBindingDescriptor = device_contract::DmaBindingDescriptor;
using PeripheralId =
    std::remove_cvref_t<decltype(std::declval<DmaBindingDescriptor>().peripheral_id)>;
using SignalId =
    std::remove_cvref_t<decltype(std::declval<DmaBindingDescriptor>().signal_id)>;

inline constexpr auto bindings = std::span{device_contract::kDmaBindings};

}  // namespace dma
#endif

struct SelectedDmaBindings {
    static constexpr bool available = selected::dma_bindings_available;
};

}  // namespace alloy::device
