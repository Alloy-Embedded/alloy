#pragma once

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"

namespace alloy::hal::connection {

template <device::PeripheralId PeripheralIdValue, typename Binding>
using connector_binding = runtime_binding<
    Binding, Binding::pin_type::id,
    detail::runtime_connector_detail::resolve_binding_signal_id<Binding, PeripheralIdValue>()>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using tx = alloy::hal::tx<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using rx = alloy::hal::rx<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using cts = alloy::hal::cts<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using rts = alloy::hal::rts<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using sck = alloy::hal::sck<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using miso = alloy::hal::miso<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using mosi = alloy::hal::mosi<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using scl = alloy::hal::scl<PinIdValue, SignalIdValue>;

template <device::PinId PinIdValue, device::SignalId SignalIdValue = device::SignalId::none>
using sda = alloy::hal::sda<PinIdValue, SignalIdValue>;

template <device::PeripheralId PeripheralIdValue, typename... Bindings>
using connector = runtime_connector<peripheral<PeripheralIdValue>, PeripheralIdValue,
                                    connector_binding<PeripheralIdValue, Bindings>...>;

}  // namespace alloy::hal::connection
