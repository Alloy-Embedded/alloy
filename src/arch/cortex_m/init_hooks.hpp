#pragma once

#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"

namespace alloy::arch::cortex_m {

inline void late_init() {
    alloy::hal::arm::late_init();
}

}  // namespace alloy::arch::cortex_m
