#pragma once

// uart_event::token<PeripheralId, InterruptKind> — static completion signal
// per (peripheral, interrupt-kind) pair.
//
// Each unique (Peripheral, Kind) pair instantiates a distinct
// event::completion<tag<Peripheral, Kind>>, which holds an independent static
// `signaled` flag.  The vendor UART ISR calls:
//
//   uart_event::token<P, InterruptKind::IdleLine>::signal();
//
// The async::uart::wait_for<Kind>(port) wrapper resets the token, arms the
// interrupt via port.enable_interrupt(Kind), then returns
// operation<uart_event::token<P, Kind>> which the caller polls or awaits.
//
// Disarming: the caller is responsible for calling port.disable_interrupt(Kind)
// after the operation completes to prevent re-signalling on subsequent events.

#include "device/runtime.hpp"
#include "hal/uart/detail/backend.hpp"  // InterruptKind enum
#include "runtime/event.hpp"

namespace alloy::runtime::uart_event {

/// Tag type — uniquely identifies one (peripheral, interrupt kind) combination.
/// The struct is empty; the template parameters carry all the identity.
template <device::PeripheralId Peripheral, hal::uart::InterruptKind Kind>
struct tag {
    static constexpr auto peripheral = Peripheral;
    static constexpr auto kind       = Kind;
};

/// Completion token.  Distinct static state per (Peripheral, Kind) pair.
/// The vendor ISR calls token<P, K>::signal(); the runtime wrapper calls
/// token<P, K>::reset() before arming.
template <device::PeripheralId Peripheral, hal::uart::InterruptKind Kind>
using token = event::completion<tag<Peripheral, Kind>>;

}  // namespace alloy::runtime::uart_event
