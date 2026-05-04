#pragma once

#include "hal/uart/uart.hpp"

// alloy.device.v2.1 concept-based UART — no descriptor-runtime required.
// Included unconditionally; the internal StUsart concept gates template instantiation.
#include "hal/uart/lite.hpp"

// Connector-typed port<Connector> — public user-facing type; wraps the internal
// port_handle<Connector> with static connect(), Guard C (tx/rx signal count),
// and configure-after-connect ordering.
#include "hal/uart/port.hpp"
