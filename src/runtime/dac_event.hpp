#pragma once

// dac_event::token<PeripheralId, InterruptKind> — static completion signal
// per (peripheral, interrupt-kind) pair.
//
// Vendor DAC ISR hooks call `dac_event::token<P, Kind>::signal()`.
// `async::dac::wait_for<Kind>(handle)` resets the token, arms the
// interrupt, and returns operation<dac_event::token<P, Kind>>.

#include "device/runtime.hpp"
#include "hal/dac.hpp"
#include "runtime/event.hpp"

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE

namespace alloy::runtime::dac_event {

/// Tag type — uniquely identifies one (peripheral, interrupt kind) pair.
template <device::PeripheralId Peripheral, hal::dac::InterruptKind Kind>
struct tag {
    static constexpr auto peripheral = Peripheral;
    static constexpr auto kind       = Kind;
};

/// Completion token. Distinct static state per (Peripheral, Kind) pair.
template <device::PeripheralId Peripheral, hal::dac::InterruptKind Kind>
using token = event::completion<tag<Peripheral, Kind>>;

}  // namespace alloy::runtime::dac_event

#endif  // ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
