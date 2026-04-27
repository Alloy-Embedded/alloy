#pragma once

// Async CAN runtime adapter — interrupt-driven single-event wait.
//
//   async::can::wait_for<Kind>(handle)
//      -> Result<operation<can_event::token<P, Kind>>, ErrorCode>
//
// Arms the CAN interrupt for `Kind`, returns an awaitable that resolves
// when the vendor ISR calls `can_event::token<P, Kind>::signal()`.
//
// Typical usage: arm RxFifo0 before a receive loop, or Tx to confirm
// a frame was successfully sent.

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/can.hpp"
#include "runtime/async.hpp"

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
#include "runtime/can_event.hpp"
#endif

namespace alloy::runtime::async::can {

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE

/// Interrupt-driven single-event wait for a CAN peripheral.
///
/// Resets the can_event::token<P, Kind> static flag, arms the interrupt
/// via handle.enable_interrupt(Kind), and returns an awaitable operation.
///
/// The vendor CAN ISR must call:
///   can_event::token<P, Kind>::signal();
///
/// Example:
///   auto op = async::can::wait_for<can::InterruptKind::RxFifo0>(can_handle);
///   op.wait_until<SysTickSource>(deadline);
template <hal::can::InterruptKind Kind, typename Handle>
[[nodiscard]] auto wait_for(const Handle& handle)
    -> core::Result<
        operation<alloy::runtime::can_event::token<Handle::peripheral_id, Kind>>,
        core::ErrorCode> {
    using token_type     = alloy::runtime::can_event::token<Handle::peripheral_id, Kind>;
    using operation_type = operation<token_type>;

    token_type::reset();

    const auto arm_result = handle.enable_interrupt(Kind);
    if (arm_result.is_err()) {
        return core::Err(core::ErrorCode{arm_result.err()});
    }
    return core::Ok(operation_type{});
}

#endif  // ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE

}  // namespace alloy::runtime::async::can
