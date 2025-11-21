/**
 * @file pwm.hpp
 * @brief SAME70 PWM Platform Integration
 */

#pragma once

#include "hal/vendors/atmel/same70/pwm_hardware_policy.hpp"

namespace ucore::hal::same70 {

// Type aliases for PWM instances
using Pwm0 = Pwm0Hardware;
using Pwm1 = Pwm1Hardware;

}  // namespace ucore::hal::same70
