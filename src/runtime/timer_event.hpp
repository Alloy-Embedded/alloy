#pragma once

// Timer completion token. The update / RC-compare interrupt signals the
// token, allowing tasks to suspend for a hardware-timed period without
// burning the SysTick `delay(Duration)` slot.
//
// `async_timer.hpp` returns `operation<timer_event::token<P>>` from
// `async::timer::wait_period` and `async::timer::delay`. Vendor ISR
// hooks call `signal()` from the timer interrupt.

#include "device/runtime.hpp"
#include "runtime/event.hpp"

namespace alloy::runtime::timer_event {

template <device::PeripheralId Peripheral>
struct tag {
    static constexpr auto value = Peripheral;
};

template <device::PeripheralId Peripheral>
using token = event::completion<tag<Peripheral>>;

}  // namespace alloy::runtime::timer_event
