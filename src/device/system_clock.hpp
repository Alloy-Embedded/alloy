#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_SYSTEM_CLOCK_AVAILABLE
namespace system_clock {

namespace device_contract = selected::runtime_device_contract;

using ProfileId = device_contract::SystemClockProfileId;
using ProfileKindId = device_contract::SystemClockProfileKindId;
using SourceKindId = device_contract::SystemClockSourceKindId;
using ProfileDescriptor = device_contract::SystemClockProfileDescriptor;

inline constexpr auto profiles = std::span{device_contract::kSystemClockProfiles};

template <ProfileId Id>
using ProfileTraits = device_contract::SystemClockProfileTraits<Id>;

template <ProfileId Id>
inline bool apply_profile() {
    return device_contract::template apply_system_clock_profile<Id>();
}

inline bool apply_default() {
    return device_contract::apply_default_system_clock();
}

inline bool apply_safe() {
    return device_contract::apply_safe_system_clock();
}

}  // namespace system_clock
#endif

struct SelectedSystemClockProfiles {
    static constexpr bool available = selected::system_clock_available;
};

}  // namespace alloy::device
