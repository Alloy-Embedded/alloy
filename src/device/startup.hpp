#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_STARTUP_AVAILABLE
namespace startup {

namespace device_contract = selected::startup_device_contract;

using StartupMemoryRegionId = device_contract::StartupMemoryRegionId;
using StartupSymbolId = device_contract::StartupSymbolId;
using StartupDescriptorId = device_contract::StartupDescriptorId;
using VectorSlotDescriptor = device_contract::VectorSlotDescriptor;
using StartupDescriptor = device_contract::StartupDescriptor;

inline constexpr auto vector_slots = std::span{device_contract::kVectorSlots};
inline constexpr auto descriptors = std::span{device_contract::kStartupDescriptors};

}  // namespace startup
#endif

struct SelectedStartupDescriptors {
    static constexpr bool available = selected::startup_available;
};

}  // namespace alloy::device
