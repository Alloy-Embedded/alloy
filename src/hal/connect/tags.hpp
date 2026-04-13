#pragma once

#include "hal/connect/fixed_string.hpp"

namespace alloy::hal::connection {

template <FixedString Name>
struct peripheral {
    static constexpr auto name = std::string_view{Name};
};

template <FixedString Name>
struct pin {
    static constexpr auto name = std::string_view{Name};
};

template <FixedString Name>
struct signal {
    static constexpr auto name = std::string_view{Name};
};

template <typename Signal, typename Pin>
struct binding {
    using signal_type = Signal;
    using pin_type = Pin;
};

}  // namespace alloy::hal::connection

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

}  // namespace alloy::hal
