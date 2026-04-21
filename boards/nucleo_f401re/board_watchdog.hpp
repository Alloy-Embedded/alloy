#pragma once

#include "device/runtime.hpp"
#include "hal/watchdog.hpp"

namespace board {

/// IWDG is the independent watchdog on STM32F401.
/// NOTE: IWDG cannot be disabled once started.  The probe only calls
/// refresh() — safe even when IWDG has not been armed.
using BoardWatchdog = alloy::hal::watchdog::handle<
    alloy::device::runtime::PeripheralId::IWDG>;

inline constexpr bool kBoardWatchdogCanDisable = false;

[[nodiscard]] inline auto make_watchdog(
    alloy::hal::watchdog::Config config = {}) -> BoardWatchdog {
    return alloy::hal::watchdog::open<alloy::device::runtime::PeripheralId::IWDG>(config);
}

}  // namespace board
