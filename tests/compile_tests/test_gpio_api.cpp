#include <string_view>
#include <type_traits>

#include "hal/gpio.hpp"
#include "hal/detail/gpio_schema_concept.hpp"

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

// Task 3.3: schema_type is always present and satisfies GpioSchemaImpl.
template <typename PinHandle>
consteval auto gpio_has_schema_type() -> bool {
    return alloy::hal::gpio::detail::GpioSchemaImpl<typename PinHandle::schema_type>;
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using LedHandle = alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA5>>;
static_assert(LedHandle::valid);
static_assert(LedHandle::peripheral_name == std::string_view{"GPIOA"});
static_assert(LedHandle::line_index == 5);
static_assert(gpio_is_usable<LedHandle>());
static_assert(gpio_has_schema_type<LedHandle>());  // task 3.3
static_assert(std::is_same_v<LedHandle::schema_type, alloy::hal::gpio::detail::StGpioSchema>);
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
static_assert(gpio_has_schema_type<LedHandle>());  // task 3.3
static_assert(std::is_same_v<LedHandle::schema_type, alloy::hal::gpio::detail::StGpioSchema>);
using LedViaApi = decltype(alloy::hal::gpio::open<alloy::device::pin<alloy::device::PinId::PA5>>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedViaApi::valid);
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using LedHandle = alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA23>>;
static_assert(LedHandle::valid);
static_assert(LedHandle::peripheral_name == std::string_view{"GPIOA"});
static_assert(LedHandle::line_index == 23);
static_assert(gpio_is_usable<LedHandle>());
static_assert(gpio_has_schema_type<LedHandle>());  // task 3.3
static_assert(std::is_same_v<LedHandle::schema_type, alloy::hal::gpio::detail::MicrochipPioSchema>);
using LedViaApi = decltype(alloy::hal::gpio::open<alloy::device::pin<alloy::device::PinId::PA23>>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedViaApi::valid);

// Task 5.1: gpio::configure<PinId> one-shot free function
// PC8 = SAME70 Xplained Ultra LED (active-low, no peripheral signal)
[[maybe_unused]] void compile_same70_gpio_configure() {
    [[maybe_unused]] auto result =
        alloy::hal::gpio::configure<alloy::device::PinId::PC8>(alloy::hal::PinDirection::Output,
                                                                alloy::hal::PinState::High);
}
#endif
