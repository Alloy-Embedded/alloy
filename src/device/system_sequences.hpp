#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_SYSTEM_SEQUENCES_AVAILABLE
namespace system_sequences {

namespace device_contract = selected::runtime_sequence_device_contract;

using SequenceId = device_contract::SystemSequenceId;
using StepKindId = device_contract::SystemSequenceStepKindId;
using StepDescriptor = device_contract::SystemSequenceStepDescriptor;
using StartupDescriptorId = device_contract::StartupDescriptorId;
using PeripheralId = device_contract::PeripheralId;
using SystemClockProfileId = device_contract::SystemClockProfileId;

inline constexpr auto steps = std::span{device_contract::kSystemSequenceSteps};

template <SequenceId Id>
using Traits = device_contract::SystemSequenceTraits<Id>;

}  // namespace system_sequences
#endif

struct SelectedSystemSequences {
    static constexpr bool available = selected::system_sequences_available;
};

}  // namespace alloy::device
