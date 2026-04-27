#pragma once

// Async RTC runtime adapter — interrupt-driven calendar/alarm operations.
//
//   async::rtc::wait_for<Kind>(handle)
//      -> Result<operation<rtc_event::token<P, Kind>>, ErrorCode>
//
// The canonical use is to suspend a task until an RTC event fires (alarm,
// per-second tick, calendar rollover). The vendor RTC ISR must call:
//
//   rtc_event::token<P, Kind>::signal();
//
// to unblock the waiting task.

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/rtc.hpp"
#include "runtime/async.hpp"

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
#include "runtime/rtc_event.hpp"
#endif

namespace alloy::runtime::async::rtc {

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE

/// Interrupt-driven single-event wait.
///
/// Resets the rtc_event::token<P, Kind> static flag, arms the interrupt via
/// handle.enable_interrupt(Kind), and returns an awaitable operation.
///
/// Usage (alarm-driven LED blink):
///   auto op = async::rtc::wait_for<InterruptKind::Alarm>(rtc);
///   op.wait_until<SysTickSource>(deadline);
///   // Alarm fired — toggle LED here.
template <hal::rtc::InterruptKind Kind, typename Handle>
[[nodiscard]] auto wait_for(const Handle& handle)
    -> core::Result<
        operation<alloy::runtime::rtc_event::token<Handle::peripheral_id, Kind>>,
        core::ErrorCode> {
    using token_type     = alloy::runtime::rtc_event::token<Handle::peripheral_id, Kind>;
    using operation_type = operation<token_type>;

    token_type::reset();

    const auto arm_result = handle.enable_interrupt(Kind);
    if (arm_result.is_err()) {
        return core::Err(core::ErrorCode{arm_result.err()});
    }
    return core::Ok(operation_type{});
}

#endif  // ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE

}  // namespace alloy::runtime::async::rtc
