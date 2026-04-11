#pragma once

#include "hal/connect/connect.hpp"

namespace alloy::hal {

template <connection::FixedString Name>
using peripheral = connection::peripheral<Name>;

template <connection::FixedString Name>
using pin = connection::pin<Name>;

template <connection::FixedString Name>
using signal = connection::signal<Name>;

template <typename Signal, typename Pin>
using bind = connection::binding<Signal, Pin>;

template <typename Pin>
using tx = bind<signal<"tx">, Pin>;

template <typename Pin>
using rx = bind<signal<"rx">, Pin>;

template <typename Pin>
using cts = bind<signal<"cts">, Pin>;

template <typename Pin>
using rts = bind<signal<"rts">, Pin>;

template <typename Pin>
using sck = bind<signal<"sck">, Pin>;

template <typename Pin>
using miso = bind<signal<"miso">, Pin>;

template <typename Pin>
using mosi = bind<signal<"mosi">, Pin>;

template <typename Pin>
using scl = bind<signal<"scl">, Pin>;

template <typename Pin>
using sda = bind<signal<"sda">, Pin>;

template <typename Peripheral, typename... Bindings>
[[nodiscard]] consteval auto connect() -> connection::connector<Peripheral, Bindings...> {
    return connection::resolve<Peripheral, Bindings...>();
}

}  // namespace alloy::hal
