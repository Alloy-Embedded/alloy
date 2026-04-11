#pragma once

#include "device/descriptors.hpp"

namespace alloy::arch::cortex_m {

struct SelectedStartupContract {
    static constexpr bool available = device::SelectedDescriptors::available;
};

#if ALLOY_DEVICE_CONTRACT_AVAILABLE
inline constexpr auto& kVectorSlots = alloy::device::descriptors::startup::kVectorSlots;
inline constexpr auto& kStartupDescriptors = alloy::device::descriptors::startup::kStartupDescriptors;
#endif

}  // namespace alloy::arch::cortex_m
