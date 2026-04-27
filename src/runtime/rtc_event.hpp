#pragma once

// rtc_event::token<PeripheralId, InterruptKind> — static completion signal
// per (peripheral, interrupt-kind) pair.
//
// The vendor RTC ISR calls:
//
//   rtc_event::token<P, InterruptKind::Alarm>::signal();
//
// The async::rtc::wait_for<Kind>(handle) wrapper resets the token, arms the
// interrupt, and returns operation<rtc_event::token<P, Kind>>.

#include "device/runtime.hpp"
#include "hal/rtc.hpp"
#include "runtime/event.hpp"

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE

namespace alloy::runtime::rtc_event {

/// Tag type — uniquely identifies one (peripheral, interrupt kind) pair.
template <device::PeripheralId Peripheral, hal::rtc::InterruptKind Kind>
struct tag {
    static constexpr auto peripheral = Peripheral;
    static constexpr auto kind       = Kind;
};

/// Completion token. Distinct static state per (Peripheral, Kind) pair.
template <device::PeripheralId Peripheral, hal::rtc::InterruptKind Kind>
using token = event::completion<tag<Peripheral, Kind>>;

}  // namespace alloy::runtime::rtc_event

#endif  // ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
