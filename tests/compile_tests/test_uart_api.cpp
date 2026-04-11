#include "device/traits.hpp"
#include "hal/connect.hpp"
#include "hal/uart.hpp"

#include <string_view>

namespace {

template <typename UartHandle>
consteval auto uart_is_usable() -> bool {
    if constexpr (!UartHandle::valid) {
        return false;
    }

    return !UartHandle::peripheral_name.empty() && UartHandle::base_address() != 0u &&
           UartHandle::requirements().size() >= 3 && UartHandle::operations().size() >= 2;
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using DebugUartConnector =
    decltype(alloy::hal::connect<alloy::hal::peripheral<"USART2">,
                                 alloy::hal::tx<alloy::hal::pin<"PA2">>,
                                 alloy::hal::rx<alloy::hal::pin<"PA3">>>());
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::package_name == std::string_view{"lqfp64"});
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});
static_assert(uart_is_usable<DebugUart>());
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using DebugUartConnector =
    decltype(alloy::hal::connect<alloy::hal::peripheral<"USART2">,
                                 alloy::hal::tx<alloy::hal::pin<"PA2">>,
                                 alloy::hal::rx<alloy::hal::pin<"PA3">>>());
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::package_name == std::string_view{"lqfp64"});
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});
static_assert(uart_is_usable<DebugUart>());
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using DebugUartConnector =
    decltype(alloy::hal::connect<alloy::hal::peripheral<"USART0">,
                                 alloy::hal::tx<alloy::hal::pin<"PB1">>,
                                 alloy::hal::rx<alloy::hal::pin<"PB0">>>());
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::package_name == std::string_view{"lqfp144"});
static_assert(DebugUart::peripheral_name == std::string_view{"USART0"});
static_assert(uart_is_usable<DebugUart>());
#endif
