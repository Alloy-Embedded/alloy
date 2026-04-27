#pragma once

// watchdog_event::token<PeripheralId, InterruptKind> — static completion
// signal per (peripheral, interrupt-kind) pair.
//
// The vendor watchdog ISR calls:
//
//   watchdog_event::token<P, InterruptKind::EarlyWarning>::signal();
//
// The async::watchdog::wait_for<Kind>(handle) wrapper resets the token,
// arms the interrupt, and returns operation<watchdog_event::token<P, Kind>>.
//
// IMPORTANT: this is for crash-state capture only. The watchdog WILL still
// reset the system after the early-warning fires — recovery is contradictory
// to the watchdog's safety guarantee.

#include "device/runtime.hpp"
#include "hal/watchdog.hpp"  // InterruptKind enum
#include "runtime/event.hpp"

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE

namespace alloy::runtime::watchdog_event {

/// Tag type — uniquely identifies one (peripheral, interrupt kind) pair.
template <device::PeripheralId Peripheral, hal::watchdog::InterruptKind Kind>
struct tag {
    static constexpr auto peripheral = Peripheral;
    static constexpr auto kind       = Kind;
};

/// Completion token. Distinct static state per (Peripheral, Kind) pair.
template <device::PeripheralId Peripheral, hal::watchdog::InterruptKind Kind>
using token = event::completion<tag<Peripheral, Kind>>;

}  // namespace alloy::runtime::watchdog_event

#endif  // ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
