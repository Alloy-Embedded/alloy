#pragma once

#include "device/runtime.hpp"
#include "hal/watchdog.hpp"

namespace board {

/// WDT is the primary general-purpose watchdog on ATSAME70Q21B.
/// board::init() already disables both WDT and RSWDT, so this handle is
/// safe to open and use (disable is idempotent, refresh key-write is benign
/// when the watchdog is not running).
using BoardWatchdog = alloy::hal::watchdog::handle<
    alloy::device::PeripheralId::WDT>;

inline constexpr bool kBoardWatchdogCanDisable = true;

[[nodiscard]] inline auto make_watchdog(
    alloy::hal::watchdog::Config config = {}) -> BoardWatchdog {
    return alloy::hal::watchdog::open<alloy::device::PeripheralId::WDT>(config);
}

}  // namespace board
