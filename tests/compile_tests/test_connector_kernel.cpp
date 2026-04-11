#include <string_view>

#include "hal/claim.hpp"
#include "hal/connect.hpp"

#include "device/traits.hpp"

namespace {

using namespace alloy::hal;

template <typename Connector>
consteval auto connector_is_usable() -> bool {
    if constexpr (!Connector::valid) {
        return false;
    }

    const auto requirements = Connector::requirements();
    const auto operations = Connector::operations();
    return requirements.size() > 0 && operations.size() > 0;
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using DebugUart = connection::connector<peripheral<"USART2">, tx<pin<"PA2">>, rx<pin<"PA3">>>;

using InvalidUart = connection::connector<peripheral<"USART2">, tx<pin<"PA2">>, rx<pin<"PA4">>>;

static_assert(DebugUart::valid);
static_assert(DebugUart::has_group());
static_assert(DebugUart::package_name == std::string_view{"lqfp64"});
static_assert(connector_is_usable<DebugUart>());
static_assert(DebugUart::requirements().size() >= 4);
static_assert(DebugUart::operations().size() >= 3);
static_assert(!InvalidUart::valid);
using DebugUartViaFunction =
    decltype(connect<peripheral<"USART2">, tx<pin<"PA2">>, rx<pin<"PA3">>>());
static_assert(DebugUartViaFunction::valid);

using DebugUartClaim = claim::connector_claim<DebugUart>;
static_assert(DebugUartClaim::pin_count == 2);
static_assert(DebugUartClaim::pins()[0] == std::string_view{"PA2"});
static_assert(DebugUartClaim::signals()[1] == std::string_view{"rx"});
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using DebugUart = connection::connector<peripheral<"USART2">, tx<pin<"PA2">>, rx<pin<"PA3">>>;

using InvalidUart = connection::connector<peripheral<"USART2">, tx<pin<"PA2">>, rx<pin<"PA4">>>;

static_assert(DebugUart::valid);
static_assert(DebugUart::has_group());
static_assert(DebugUart::package_name == std::string_view{"lqfp64"});
static_assert(connector_is_usable<DebugUart>());
static_assert(!InvalidUart::valid);
using DebugUartViaFunction =
    decltype(connect<peripheral<"USART2">, tx<pin<"PA2">>, rx<pin<"PA3">>>());
static_assert(DebugUartViaFunction::valid);
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using DebugUart = connection::connector<peripheral<"USART0">, tx<pin<"PB1">>, rx<pin<"PB0">>>;

using InvalidUart = connection::connector<peripheral<"USART0">, tx<pin<"PB1">>, rx<pin<"PB4">>>;

static_assert(DebugUart::valid);
static_assert(DebugUart::has_group());
static_assert(DebugUart::package_name == std::string_view{"lqfp144"});
static_assert(connector_is_usable<DebugUart>());
static_assert(!InvalidUart::valid);
using DebugUartViaFunction =
    decltype(connect<peripheral<"USART0">, tx<pin<"PB1">>, rx<pin<"PB0">>>());
static_assert(DebugUartViaFunction::valid);
#endif
