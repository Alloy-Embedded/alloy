#pragma once

#include "device/runtime.hpp"

namespace alloy::hal::connection {

namespace detail {

template <typename T = device::SignalId>
consteval auto miso_default_signal_id() -> device::SignalId {
    if constexpr (requires { T::signal_miso; }) {
        return T::signal_miso;
    } else {
        return T::none;
    }
}

template <typename T = device::SignalId>
consteval auto mosi_default_signal_id() -> device::SignalId {
    if constexpr (requires { T::signal_mosi; }) {
        return T::signal_mosi;
    } else {
        return T::none;
    }
}

enum class role_id : unsigned char {
    tx,
    rx,
    cts,
    rts,
    sck,
    miso,
    mosi,
    scl,
    sda,
};

struct tx_role {
    static constexpr auto id = role_id::tx;
    static constexpr auto default_signal_id = device::SignalId::signal_tx;
};

struct rx_role {
    static constexpr auto id = role_id::rx;
    static constexpr auto default_signal_id = device::SignalId::signal_rx;
};

struct cts_role {
    static constexpr auto id = role_id::cts;
    static constexpr auto default_signal_id = device::SignalId::signal_cts;
};

struct rts_role {
    static constexpr auto id = role_id::rts;
    static constexpr auto default_signal_id = device::SignalId::signal_rts;
};

struct sck_role {
    static constexpr auto id = role_id::sck;
    static constexpr auto default_signal_id = device::SignalId::signal_sck;
};

struct miso_role {
    static constexpr auto id = role_id::miso;
    static constexpr auto default_signal_id = miso_default_signal_id();
};

struct mosi_role {
    static constexpr auto id = role_id::mosi;
    static constexpr auto default_signal_id = mosi_default_signal_id();
};

struct scl_role {
    static constexpr auto id = role_id::scl;
    static constexpr auto default_signal_id = device::SignalId::signal_scl;
};

struct sda_role {
    static constexpr auto id = role_id::sda;
    static constexpr auto default_signal_id = device::SignalId::signal_sda;
};

template <typename Role, device::PinId PinIdValue,
          device::SignalId SignalIdValue = device::SignalId::none>
struct role_binding {
    using role_type = Role;
    struct pin_type {
        static constexpr auto id = PinIdValue;
        static constexpr auto name = device::pin<PinIdValue>::name;
    };

    static constexpr auto pin_id = PinIdValue;
    static constexpr auto requested_signal_id = SignalIdValue;
    static constexpr auto default_signal_id = Role::default_signal_id;
    static constexpr auto role_id = Role::id;
};

}  // namespace detail

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

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using tx = connection::detail::role_binding<connection::detail::tx_role, PinIdValue,
                                            SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using rx = connection::detail::role_binding<connection::detail::rx_role, PinIdValue,
                                            SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using cts = connection::detail::role_binding<connection::detail::cts_role, PinIdValue,
                                             SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using rts = connection::detail::role_binding<connection::detail::rts_role, PinIdValue,
                                             SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using sck = connection::detail::role_binding<connection::detail::sck_role, PinIdValue,
                                             SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using miso = connection::detail::role_binding<connection::detail::miso_role, PinIdValue,
                                              SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using mosi = connection::detail::role_binding<connection::detail::mosi_role, PinIdValue,
                                              SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using scl = connection::detail::role_binding<connection::detail::scl_role, PinIdValue,
                                             SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using sda = connection::detail::role_binding<connection::detail::sda_role, PinIdValue,
                                             SignalIdValue>;

}  // namespace alloy::hal
