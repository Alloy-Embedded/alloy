#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_LOW_POWER_AVAILABLE
namespace low_power {

namespace device_contract = selected::runtime_low_power_device_contract;

using ModeId = device_contract::LowPowerModeId;
using ModeDescriptor = device_contract::LowPowerModeDescriptor;
using WakeupTagId = device_contract::WakeupTagId;
using WakeupSourceId = device_contract::WakeupSourceId;
using WakeupSourceDescriptor = device_contract::WakeupSourceDescriptor;
using PinId = device_contract::PinId;

inline constexpr auto modes = std::span{device_contract::kLowPowerModes};
inline constexpr auto wakeup_sources = std::span{device_contract::kWakeupSources};

template <ModeId Id>
using ModeTraits = device_contract::LowPowerModeTraits<Id>;

template <WakeupSourceId Id>
using WakeupSourceTraits = device_contract::WakeupSourceTraits<Id>;

template <PinId Pin>
using WakeupPinTraits = device_contract::WakeupPinTraits<Pin>;

}  // namespace low_power
#endif

struct SelectedLowPower {
    static constexpr bool available = selected::low_power_available;
};

}  // namespace alloy::device
