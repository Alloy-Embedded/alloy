#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_CONTRACT_AVAILABLE
namespace descriptors {

namespace family = selected::family_contract;
namespace device_contract = selected::device_contract;
namespace startup = selected::startup_contract;

namespace tables {

inline constexpr auto runtime_profiles = std::span{family::kRuntimeProfiles};
inline constexpr auto signal_endpoints = std::span{family::kSignalEndpoints};
inline constexpr auto route_requirements = std::span{family::kRouteRequirements};
inline constexpr auto route_operations = std::span{family::kRouteOperations};
inline constexpr auto connection_candidates = std::span{family::kConnectionCandidates};
inline constexpr auto candidate_requirement_refs = std::span{family::kCandidateRequirementRefs};
inline constexpr auto candidate_operation_refs = std::span{family::kCandidateOperationRefs};
inline constexpr auto connection_groups = std::span{family::kConnectionGroups};
inline constexpr auto connection_group_signals = std::span{family::kConnectionGroupSignals};
inline constexpr auto connection_group_candidate_refs =
    std::span{family::kConnectionGroupCandidateRefs};
inline constexpr auto candidate_capability_refs = std::span{family::kCandidateCapabilityRefs};
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
inline constexpr auto device_descriptor = startup::kDeviceDescriptor;
inline constexpr auto pins = std::span{device_contract::kPins};
inline constexpr auto pin_signals = std::span{device_contract::kPinSignals};
inline constexpr auto capability_overlays = std::span{device_contract::kCapabilityOverlays};
inline constexpr auto peripheral_instances = std::span{device_contract::kPeripheralInstances};
inline constexpr auto registers = std::span{device_contract::kRegisters};
inline constexpr auto register_fields = std::span{device_contract::kRegisterFields};
inline constexpr auto interrupt_bindings = std::span{device_contract::kInterruptBindings};
inline constexpr auto interrupt_binding_aliases =
    std::span{device_contract::kInterruptBindingAliases};
inline constexpr auto dma_bindings = std::span{device_contract::kDmaBindings};
inline constexpr auto peripheral_bases = std::span{device_contract::kPeripheralBases};
inline constexpr auto vector_slots = std::span{device_contract::kVectorSlots};
inline constexpr auto startup_descriptors = std::span{device_contract::kStartupDescriptors};

}  // namespace tables

}  // namespace descriptors
#endif

struct SelectedDescriptors {
    static constexpr bool available = selected::available;
};

}  // namespace alloy::device
