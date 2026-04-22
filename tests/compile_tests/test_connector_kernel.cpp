#include <string_view>

#include "hal/claim.hpp"
#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"

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
using DebugUart = connection::connector<
    alloy::device::PeripheralId::USART2,
    connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    connection::rx<alloy::device::PinId::PA3, alloy::device::SignalId::signal_rx>>;

using InvalidUart = connection::connector<
    alloy::device::PeripheralId::USART2,
    connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    connection::rx<alloy::device::PinId::none, alloy::device::SignalId::signal_rx>>;

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
using DebugUart = connection::connector<
    alloy::device::PeripheralId::USART2,
    connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    connection::rx<alloy::device::PinId::PA3, alloy::device::SignalId::signal_rx>>;

using InvalidUart = connection::connector<
    alloy::device::PeripheralId::USART2,
    connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    connection::rx<alloy::device::PinId::none, alloy::device::SignalId::signal_rx>>;

static_assert(DebugUart::valid);
static_assert(connector_is_usable<DebugUart>());
static_assert(!InvalidUart::valid);
using DebugUartViaFunction = DebugUart;
static_assert(DebugUartViaFunction::valid);
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using DebugUart = connection::connector<
    alloy::device::PeripheralId::USART1,
    connection::tx<alloy::device::PinId::PB4, alloy::device::SignalId::signal_txd1>,
    connection::rx<alloy::device::PinId::PA21, alloy::device::SignalId::signal_rxd1>>;

using InvalidUart = connection::connector<
    alloy::device::PeripheralId::USART1,
    connection::tx<alloy::device::PinId::PB4, alloy::device::SignalId::signal_txd1>,
    connection::rx<alloy::device::PinId::none, alloy::device::SignalId::signal_rxd1>>;

static_assert(DebugUart::valid);
static_assert(connector_is_usable<DebugUart>());
static_assert(!InvalidUart::valid);
using DebugUartViaFunction = DebugUart;
static_assert(DebugUartViaFunction::valid);
#endif
