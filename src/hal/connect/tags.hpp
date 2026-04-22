#pragma once

#include "device/runtime.hpp"

namespace alloy::hal::connection {

template <device::PeripheralId Id>
struct peripheral {
    static constexpr auto id = Id;
    static constexpr auto name = device::peripheral<Id>::name;
};

template <device::PinId Id>
struct pin {
    static constexpr auto id = Id;
    static constexpr auto name = device::pin<Id>::name;
};

template <device::SignalId Id>
struct signal {
    static constexpr auto id = Id;
    static constexpr auto name = device::signal<Id>::name;
};

template <typename Signal, typename Pin>
struct binding {
    using signal_type = Signal;
    using pin_type = Pin;
};

}  // namespace alloy::hal::connection

namespace alloy::hal {

template <device::PeripheralId Id>
using peripheral = connection::peripheral<Id>;

template <device::PinId Id>
using pin = connection::pin<Id>;

template <device::SignalId Id>
using signal = connection::signal<Id>;

template <typename Signal, typename Pin>
using bind = connection::binding<Signal, Pin>;

template <typename Pin, typename Signal>
using tx = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using rx = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using cts = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using rts = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using sck = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using miso = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using mosi = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using scl = bind<Signal, Pin>;

template <typename Pin, typename Signal>
using sda = bind<Signal, Pin>;

}  // namespace alloy::hal
