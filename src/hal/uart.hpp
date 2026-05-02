#pragma once

#include "hal/uart/uart.hpp"

// alloy.device.v2.1 concept-based UART — no descriptor-runtime required.
// Included unconditionally; the internal StUsart concept gates template instantiation.
#include "hal/uart/lite.hpp"
