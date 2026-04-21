#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
namespace clock_config {

namespace device_contract = selected::runtime_clock_device_contract;

using ProfileId = device_contract::ClockProfileId;
using ProfileKindId = device_contract::ClockProfileKindId;
using SourceKindId = device_contract::ClockProfileSourceKindId;
using ProfileDescriptor = device_contract::ClockProfileDescriptor;

inline constexpr auto profile_ids = std::span{device_contract::kClockProfileIds};
inline constexpr auto profiles = std::span{device_contract::kClockProfiles};
inline constexpr auto default_profile_id = device_contract::kDefaultClockProfileId;
inline constexpr auto safe_profile_id = device_contract::kSafeClockProfileId;
inline constexpr auto max_profile_id = device_contract::kMaxClockProfileId;
inline constexpr auto max_clock_frequency_hz = device_contract::kMaxClockFrequencyHz;

template <ProfileId Id>
using ProfileTraits = device_contract::ClockProfileTraits<Id>;

template <ProfileId Id>
inline bool apply_profile() {
    return device_contract::template apply_clock_profile<Id>();
}

inline bool apply_default() {
    return device_contract::apply_default_clock_profile();
}

inline bool apply_safe() {
    return device_contract::apply_safe_clock_profile();
}

inline bool apply_max() {
    return device_contract::apply_max_clock_profile();
}

}  // namespace clock_config
#endif

struct SelectedClockConfig {
    static constexpr bool available = selected::clock_config_available;
};

}  // namespace alloy::device
