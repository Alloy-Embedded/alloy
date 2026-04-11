#pragma once

#include <string_view>

#include "device/selected.hpp"

namespace alloy::device {

struct SelectedDeviceTraits {
    static constexpr bool available = selected::available;
    static constexpr std::string_view vendor = selected::vendor;
    static constexpr std::string_view family = selected::family;
    static constexpr std::string_view name = selected::name;
    static constexpr std::string_view arch = selected::arch;
};

}  // namespace alloy::device
