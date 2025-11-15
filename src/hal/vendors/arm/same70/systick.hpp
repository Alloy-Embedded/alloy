#pragma once

/**
 * @file systick.hpp
 * @brief SAME70 SysTick Platform Integration
 */

#include "hal/vendors/atmel/same70/systick_hardware_policy.hpp"

namespace alloy::hal::same70 {

// SysTick instance (ARM Cortex-M7 @ 300MHz)
using SysTick = SysTickHardware;

} // namespace alloy::hal::same70
