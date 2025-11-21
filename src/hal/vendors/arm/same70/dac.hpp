/**
 * @file dac.hpp
 * @brief SAME70 DAC Platform Integration
 */

#pragma once

#include "hal/vendors/atmel/same70/dac_hardware_policy.hpp"

namespace ucore::hal::same70 {

// Type alias for DAC
using Dac = DacHardware;

}  // namespace ucore::hal::same70
