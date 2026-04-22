#include <string_view>

#include "hal/gpio.hpp"

#include "device/runtime.hpp"
#include "device/traits.hpp"

namespace {

template <typename PinHandle>
consteval auto gpio_is_usable() -> bool {
    if constexpr (!PinHandle::valid) {
        return false;
    }

    return !PinHandle::peripheral_name.empty() && PinHandle::base_address() != 0u &&
           PinHandle::operations().size() >= 1;
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using LedHandle = alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA5>>;
static_assert(LedHandle::valid);
static_assert(LedHandle::peripheral_name == std::string_view{"GPIOA"});
static_assert(LedHandle::line_index == 5);
static_assert(gpio_is_usable<LedHandle>());
using LedViaApi = decltype(alloy::hal::gpio::open<alloy::device::pin<alloy::device::PinId::PA5>>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedViaApi::valid);

using InvalidLed = alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::none>>;
static_assert(!InvalidLed::valid);
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using LedHandle = alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA5>>;
static_assert(LedHandle::valid);
static_assert(LedHandle::peripheral_name == std::string_view{"GPIOA"});
static_assert(LedHandle::line_index == 5);
static_assert(gpio_is_usable<LedHandle>());
using LedViaApi = decltype(alloy::hal::gpio::open<alloy::device::pin<alloy::device::PinId::PA5>>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedViaApi::valid);
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using LedHandle = alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA23>>;
static_assert(LedHandle::valid);
static_assert(LedHandle::peripheral_name == std::string_view{"GPIOA"});
static_assert(LedHandle::line_index == 23);
static_assert(gpio_is_usable<LedHandle>());
using LedViaApi = decltype(alloy::hal::gpio::open<alloy::device::pin<alloy::device::PinId::PA23>>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedViaApi::valid);
#endif
