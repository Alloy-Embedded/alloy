#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_CONTRACT_AVAILABLE
namespace descriptors {

namespace family = selected::family_contract;
namespace startup = selected::startup_contract;

namespace tables {

inline constexpr auto signal_endpoints = std::span{family::kSignalEndpoints};
inline constexpr auto route_requirements = std::span{family::kRouteRequirements};
inline constexpr auto route_operations = std::span{family::kRouteOperations};
inline constexpr auto connection_candidates = std::span{family::kConnectionCandidates};
inline constexpr auto connection_groups = std::span{family::kConnectionGroups};
inline constexpr auto interrupt_map = std::span{family::kInterruptMap};
inline constexpr auto memory_map = std::span{family::kMemoryMap};
inline constexpr auto package_map = std::span{family::kPackageMap};
inline constexpr auto package_pads = std::span{family::kPackagePads};
inline constexpr auto pin_constraints = std::span{family::kPinConstraints};
inline constexpr auto dma_map = std::span{family::kDmaMap};
inline constexpr auto rcc_map = std::span{family::kRccMap};
inline constexpr auto clock_nodes = std::span{family::kClockNodes};
inline constexpr auto clock_selectors = std::span{family::kClockSelectors};
inline constexpr auto clock_gates = std::span{family::kClockGates};
inline constexpr auto resets = std::span{family::kResets};
inline constexpr auto peripheral_clock_bindings =
    std::span{family::kPeripheralClockBindings};
inline constexpr auto peripheral_bases = std::span{startup::kPeripheralBases};
inline constexpr auto vector_slots = std::span{startup::kVectorSlots};
inline constexpr auto startup_descriptors = std::span{startup::kStartupDescriptors};

}  // namespace tables

}  // namespace descriptors
#endif

struct SelectedDescriptors {
    static constexpr bool available = selected::available;
};

}  // namespace alloy::device
