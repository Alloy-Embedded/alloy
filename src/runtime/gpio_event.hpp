#pragma once

// GPIO edge-event completion token. The external-interrupt line (EXTI on
// STM32, PIO interrupt on SAME70) signals the token when a configured
// edge is detected. `gpio_event::token<PinId>` is keyed on the pin so
// each line carries its own static state — multiple async waits on
// distinct pins do not alias.
//
// `async_gpio.hpp` returns `operation<gpio_event::token<Pin>>` from
// `async::gpio::wait_edge<Pin>(edge)`.

#include "device/runtime.hpp"
#include "runtime/event.hpp"

namespace alloy::runtime::gpio_event {

template <device::PinId Pin>
struct tag {
    static constexpr auto value = Pin;
};

template <device::PinId Pin>
using token = event::completion<tag<Pin>>;

}  // namespace alloy::runtime::gpio_event
