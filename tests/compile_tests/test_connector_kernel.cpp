#include <string_view>

#include "hal/claim.hpp"
#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"

#include "device/traits.hpp"

namespace {

using namespace alloy::hal;

template <typename Connector>
consteval auto connector_is_usable() -> bool {
    if constexpr (!Connector::valid) {
        return false;
    }

    const auto operations = Connector::operations();
    return operations.size() > 0;
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using DebugUart = connection::runtime_connector<
    peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    connection::runtime_binding<tx<pin<"PA2">>, alloy::device::runtime::PinId::PA2,
                                alloy::device::runtime::SignalId::signal_tx>,
    connection::runtime_binding<rx<pin<"PA3">>, alloy::device::runtime::PinId::PA3,
                                alloy::device::runtime::SignalId::signal_rx>>;

using InvalidUart = connection::runtime_connector<
    peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    connection::runtime_binding<tx<pin<"PA2">>, alloy::device::runtime::PinId::PA2,
                                alloy::device::runtime::SignalId::signal_tx>,
    connection::runtime_binding<rx<pin<"PZ99">>, alloy::device::runtime::PinId::none,
                                alloy::device::runtime::SignalId::signal_rx>>;

static_assert(DebugUart::valid);
static_assert(connector_is_usable<DebugUart>());
static_assert(DebugUart::operations().size() >= 3);
static_assert(!InvalidUart::valid);
using DebugUartViaFunction = DebugUart;
static_assert(DebugUartViaFunction::valid);

using DebugUartClaim = claim::connector_claim<DebugUart>;
static_assert(DebugUartClaim::pin_count == 2);
static_assert(DebugUartClaim::pins()[0] == std::string_view{"PA2"});
static_assert(DebugUartClaim::signals()[1] == std::string_view{"rx"});
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using DebugUart = connection::runtime_connector<
    peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    connection::runtime_binding<tx<pin<"PA2">>, alloy::device::runtime::PinId::PA2,
                                alloy::device::runtime::SignalId::signal_tx>,
    connection::runtime_binding<rx<pin<"PA3">>, alloy::device::runtime::PinId::PA3,
                                alloy::device::runtime::SignalId::signal_rx>>;

using InvalidUart = connection::runtime_connector<
    peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    connection::runtime_binding<tx<pin<"PA2">>, alloy::device::runtime::PinId::PA2,
                                alloy::device::runtime::SignalId::signal_tx>,
    connection::runtime_binding<rx<pin<"PZ99">>, alloy::device::runtime::PinId::none,
                                alloy::device::runtime::SignalId::signal_rx>>;

static_assert(DebugUart::valid);
static_assert(connector_is_usable<DebugUart>());
static_assert(!InvalidUart::valid);
using DebugUartViaFunction = DebugUart;
static_assert(DebugUartViaFunction::valid);
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using DebugUart = connection::runtime_connector<
    peripheral<"USART0">, alloy::device::runtime::PeripheralId::USART0,
    connection::runtime_binding<tx<pin<"PB1">>, alloy::device::runtime::PinId::PB1,
                                alloy::device::runtime::SignalId::signal_txd0>,
    connection::runtime_binding<rx<pin<"PB0">>, alloy::device::runtime::PinId::PB0,
                                alloy::device::runtime::SignalId::signal_rxd0>>;

using InvalidUart = connection::runtime_connector<
    peripheral<"USART0">, alloy::device::runtime::PeripheralId::USART0,
    connection::runtime_binding<tx<pin<"PB1">>, alloy::device::runtime::PinId::PB1,
                                alloy::device::runtime::SignalId::signal_txd0>,
    connection::runtime_binding<rx<pin<"PZ99">>, alloy::device::runtime::PinId::none,
                                alloy::device::runtime::SignalId::signal_rxd0>>;

static_assert(DebugUart::valid);
static_assert(connector_is_usable<DebugUart>());
static_assert(!InvalidUart::valid);
using DebugUartViaFunction = DebugUart;
static_assert(DebugUartViaFunction::valid);
#endif
