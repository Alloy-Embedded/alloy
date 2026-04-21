#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_CAPABILITIES_AVAILABLE
namespace capabilities {

namespace device_contract = selected::runtime_capability_device_contract;
namespace family_contract = selected::runtime_family_contract;

using CapabilityId = device_contract::CapabilityId;
using ScopeId = device_contract::CapabilityScopeId;
using NameId = device_contract::CapabilityNameId;
using ValueId = device_contract::CapabilityValueId;
using Descriptor = device_contract::CapabilityDescriptor;
using PeripheralId = device_contract::PeripheralId;
using PeripheralClassId = family_contract::PeripheralClassId;

inline constexpr auto all = std::span{device_contract::kCapabilities};

template <CapabilityId Id>
using Traits = device_contract::CapabilityTraits<Id>;

template <PeripheralId Id>
using PeripheralTraits = device_contract::PeripheralCapabilityTraits<Id>;

template <PeripheralClassId Id>
using PeripheralClassTraits = device_contract::PeripheralClassCapabilityTraits<Id>;

}  // namespace capabilities
#endif

struct SelectedCapabilities {
    static constexpr bool available = selected::capabilities_available;
};

}  // namespace alloy::device
