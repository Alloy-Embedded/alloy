#include "board.hpp"

#include <cstdint>

#include "hal/gpio.hpp"

#include "device/clock_config.hpp"
#include "device/runtime.hpp"
#include "device/system_clock.hpp"

using namespace alloy::hal;

namespace board {

namespace {

using BoardLed = alloy::device::pin<alloy::device::PinId::GPIO8>;

static bool board_initialized = false;

auto& led_handle() {
    static auto handle = alloy::hal::gpio::open<BoardLed>({
        .direction     = PinDirection::Output,
        .drive         = PinDrive::PushPull,
        .pull          = PinPull::None,
        .initial_state = LedConfig::led_green_active_high ? PinState::Low : PinState::High,
    });
    return handle;
}

}  // namespace

namespace led {

void init() {
    led_handle().configure().unwrap();
    off();
}

void on() {
    if constexpr (LedConfig::led_green_active_high) {
        led_handle().set_high().unwrap();
    } else {
        led_handle().set_low().unwrap();
    }
}

void off() {
    if constexpr (LedConfig::led_green_active_high) {
        led_handle().set_low().unwrap();
    } else {
        led_handle().set_high().unwrap();
    }
}

void toggle() { led_handle().toggle().unwrap(); }

}  // namespace led

void init() {
    if (board_initialized) {
        return;
    }

#if ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    alloy::device::clock_config::apply_profile<ClockConfig::system_clock_profile>();
#else
    alloy::device::system_clock::apply_default();
#endif

    led::init();

    board_initialized = true;
}

}  // namespace board
