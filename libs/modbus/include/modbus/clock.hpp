// The default microsecond timebase adapter, shared by client and server.
//
// Both bind their framers to an injected clock (house rule: inject the
// clock, never read it) — this is the one implementation that reads the
// board's timebase, and the only place in the library that does. Tests
// substitute a stepping clock over testkit's virtual_clock instead.

#pragma once

#include <cstdint>

#include "alloy/time.hpp"

namespace alloy::lib::modbus {

// Any type with now_us() -> uint32_t substitutes via the Clock parameter.
struct uptime_clock {
    [[nodiscard]] std::uint32_t now_us() const { return alloy::uptime_us(); }
};

}  // namespace alloy::lib::modbus
