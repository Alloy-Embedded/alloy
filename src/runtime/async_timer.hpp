#pragma once

// Async hardware-timer runtime adapter.
//
// Two operations:
//
//   async::timer::wait_period<Peripheral>(port)
//      -> Result<operation<timer_event::token<Peripheral>>, ErrorCode>
//
//      Suspends the calling task until the next timer update event
//      (period rollover on STM32 TIM, RC compare on SAME70 TC). The
//      caller is responsible for having configured the timer to
//      generate periodic update interrupts.
//
//   async::timer::delay<Peripheral>(port, duration)
//      -> Result<operation<timer_event::token<Peripheral>>, ErrorCode>
//
//      One-shot delay. Reprograms the timer's auto-reload to fire after
//      `duration`, arms the update interrupt, and returns an awaitable
//      operation. Distinct from `runtime::tasks::delay(Duration)` —
//      that uses SysTick; this uses a hardware peripheral so the
//      SysTick budget stays free for the cooperative scheduler.

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "runtime/async.hpp"
#include "runtime/time.hpp"
#include "runtime/timer_event.hpp"

namespace alloy::runtime::async::timer {

template <device::PeripheralId Peripheral, typename PortHandle>
[[nodiscard]] auto wait_period(const PortHandle& port)
    -> core::Result<operation<alloy::runtime::timer_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::timer_event::token<Peripheral>>;
    const auto start_result = port.arm_period_async();
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

template <device::PeripheralId Peripheral, typename PortHandle>
[[nodiscard]] auto delay(const PortHandle& port, time::Duration duration)
    -> core::Result<operation<alloy::runtime::timer_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::timer_event::token<Peripheral>>;
    const auto start_result = port.arm_oneshot_async(duration);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

}  // namespace alloy::runtime::async::timer
