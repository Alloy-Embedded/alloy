#pragma once

#include "hal/i2c/i2c.hpp"

// alloy.device.v2.1 concept-based I2C — no descriptor-runtime required.
// Supports modern ST I2C (i2c2_v*: F3/G0/G4/H7/L4/WB — TIMINGR layout).
#include "hal/i2c/lite.hpp"
