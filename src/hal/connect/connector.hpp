#pragma once

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"

namespace alloy::hal::connection {

template <typename Binding>
using connector_binding = runtime_binding<Binding, Binding::pin_type::id, Binding::signal_type::id>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using tx = alloy::hal::tx<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using rx = alloy::hal::rx<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using cts = alloy::hal::cts<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using rts = alloy::hal::rts<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using sck = alloy::hal::sck<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using miso = alloy::hal::miso<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using mosi = alloy::hal::mosi<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using scl = alloy::hal::scl<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue>
using sda = alloy::hal::sda<pin<PinIdValue>, signal<SignalIdValue>>;

template <device::PeripheralId PeripheralIdValue, typename... Bindings>
using connector = runtime_connector<peripheral<PeripheralIdValue>, PeripheralIdValue,
                                    connector_binding<Bindings>...>;

}  // namespace alloy::hal::connection
