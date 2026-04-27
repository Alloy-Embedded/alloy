#pragma once

// Async watchdog runtime adapter — interrupt-driven crash-state capture.
//
//   async::watchdog::wait_for<Kind>(handle)
//      -> Result<operation<watchdog_event::token<P, Kind>>, ErrorCode>
//
// The canonical use is to register a coroutine that captures crash state
// (PC, SP, recent message log, …) into a backup register before the
// inevitable reset. This is NOT a "recover from watchdog timeout" path —
// recovery defeats the watchdog's safety guarantee. The vendor ISR calls
// `watchdog_event::token<P, Kind>::signal()` when EWI fires.

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/watchdog.hpp"
#include "runtime/async.hpp"

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
#include "runtime/watchdog_event.hpp"
#endif

namespace alloy::runtime::async::watchdog {

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE

/// Interrupt-driven single-event wait.
///
/// Resets the watchdog_event::token<P, Kind> static flag, arms the
/// early-warning interrupt via handle.enable_interrupt(Kind), and returns
/// an awaitable operation.
///
/// The vendor watchdog ISR must call:
///   watchdog_event::token<P, InterruptKind::EarlyWarning>::signal();
///
/// Usage (crash-state capture before reset):
///   auto op = async::watchdog::wait_for<InterruptKind::EarlyWarning>(wdg);
///   op.wait_until<SysTickSource>(deadline);
///   // Now in early-warning window — dump state to backup register here.
template <hal::watchdog::InterruptKind Kind, typename Handle>
[[nodiscard]] auto wait_for(const Handle& handle)
    -> core::Result<
        operation<alloy::runtime::watchdog_event::token<Handle::peripheral_id, Kind>>,
        core::ErrorCode> {
    using token_type =
        alloy::runtime::watchdog_event::token<Handle::peripheral_id, Kind>;
    using operation_type = operation<token_type>;

    token_type::reset();

    const auto arm_result = handle.enable_interrupt(Kind);
    if (arm_result.is_err()) {
        return core::Err(core::ErrorCode{arm_result.err()});
    }
    return core::Ok(operation_type{});
}

#endif  // ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE

}  // namespace alloy::runtime::async::watchdog
