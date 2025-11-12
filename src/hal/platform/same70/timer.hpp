/**
 * @file timer.hpp
 * @brief SAME70 Timer Platform Integration
 */

#pragma once

#include "hal/vendors/atmel/same70/timer_hardware_policy.hpp"

namespace alloy::hal::same70 {

// Type aliases for Timer instances
using Timer0 = Timer0Ch0Hardware;
using Timer1 = Timer1Ch0Hardware;
using Timer2 = Timer2Ch0Hardware;
using Timer3 = Timer3Ch0Hardware;

}  // namespace alloy::hal::same70
