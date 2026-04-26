#pragma once

// I2C completion token — interrupt-driven (BTF/TC on STM32, TXCOMP/RXRDY on
// SAME70 TWIHS). Each peripheral instance gets its own static `signaled`
// flag because the tag type is distinct per `PeripheralId`. The vendor ISR
// hook calls `i2c_event::token<P>::signal()` from the I2C event interrupt.
//
// The runtime header `async_i2c.hpp` returns
// `operation<i2c_event::token<P>>` from `async::i2c::write` /
// `async::i2c::read` / `async::i2c::write_read`.

#include "device/runtime.hpp"
#include "runtime/event.hpp"

namespace alloy::runtime::i2c_event {

// Distinct tag per (kind=I2C, PeripheralId) so completion<...> instantiates
// a unique static state per peripheral. The struct itself is empty — the
// `value` member exists only to keep the tag type printable / debuggable.
template <device::PeripheralId Peripheral>
struct tag {
    static constexpr auto value = Peripheral;
};

template <device::PeripheralId Peripheral>
using token = event::completion<tag<Peripheral>>;

}  // namespace alloy::runtime::i2c_event
