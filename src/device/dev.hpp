#pragma once

namespace alloy::dev {

#if ALLOY_DEVICE_RUNTIME_AVAILABLE

namespace pin {
using enum device::PinId;
}

namespace periph {
using enum device::PeripheralId;
}

namespace sig {
using enum device::SignalId;

inline constexpr auto tx = device::SignalId::signal_tx;
inline constexpr auto rx = device::SignalId::signal_rx;
inline constexpr auto cts = device::SignalId::signal_cts;
inline constexpr auto rts = device::SignalId::signal_rts;
inline constexpr auto sck = device::SignalId::signal_sck;
inline constexpr auto miso = device::SignalId::signal_miso;
inline constexpr auto mosi = device::SignalId::signal_mosi;
inline constexpr auto scl = device::SignalId::signal_scl;
inline constexpr auto sda = device::SignalId::signal_sda;
}

#endif

}  // namespace alloy::dev
