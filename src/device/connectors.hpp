#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_CONNECTORS_AVAILABLE
namespace connectors {

namespace device_contract = selected::runtime_connector_device_contract;
namespace family_contract = selected::runtime_family_contract;

using ConnectorId = device_contract::ConnectorId;
using ConnectorDescriptor = device_contract::ConnectorDescriptor;
using PinId = device_contract::PinId;
using PeripheralId = device_contract::PeripheralId;
using SignalId = family_contract::SignalId;

inline constexpr auto all = std::span{device_contract::kConnectors};

template <PinId Pin, PeripheralId Peripheral, SignalId Signal>
using ConnectorTraits = device_contract::ConnectorTraits<Pin, Peripheral, Signal>;

template <PeripheralId Peripheral, SignalId Signal>
using SignalTraits = device_contract::ConnectorSignalTraits<Peripheral, Signal>;

}  // namespace connectors
#endif

struct SelectedConnectors {
    static constexpr bool available = selected::connectors_available;
};

}  // namespace alloy::device
