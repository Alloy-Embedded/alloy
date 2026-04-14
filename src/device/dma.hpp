#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
namespace dma {

namespace device_contract = selected::runtime_dma_device_contract;
namespace driver_contract = selected::runtime_dma_driver_contract;

using DmaBindingId = device_contract::DmaBindingId;
using DmaControllerId = device_contract::DmaControllerId;
using DmaRequestLineId = device_contract::DmaRequestLineId;
using DmaRouteId = device_contract::DmaRouteId;
using DmaConflictGroupId = device_contract::DmaConflictGroupId;
using DmaBindingDescriptor = device_contract::DmaBindingDescriptor;
using PeripheralId = device_contract::PeripheralId;
using SignalId = selected::runtime_family_contract::SignalId;

inline constexpr auto bindings = std::span{device_contract::kDmaBindings};
inline constexpr auto controllers = std::span{device_contract::kDmaControllers};
inline constexpr auto semantic_peripherals = std::span{driver_contract::kDmaSemanticPeripherals};

template <PeripheralId Peripheral, SignalId Signal>
using BindingTraits = device_contract::BindingTraits<Peripheral, Signal>;

template <DmaControllerId Id>
using ControllerTraits = device_contract::ControllerTraits<Id>;

template <PeripheralId Peripheral, SignalId Signal>
using SemanticTraits = driver_contract::DmaSemanticTraits<Peripheral, Signal>;

}  // namespace dma
#endif

struct SelectedDmaBindings {
    static constexpr bool available = selected::dma_bindings_available;
};

}  // namespace alloy::device
