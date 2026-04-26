// Compile test: TM1637 4-digit 7-segment display driver instantiates against
// the documented GPIO pin surface. Exercises every public method so that any
// drift in the pin surface or driver API fails the build.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/display/tm1637/tm1637.hpp"

namespace {

struct MockGpioPin {
    [[nodiscard]] auto set_high() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto set_low() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_tm1637_against_gpio_pin_surface() {
    MockGpioPin clk;
    MockGpioPin dio;

    alloy::drivers::display::tm1637::Device<MockGpioPin, MockGpioPin> display{clk, dio};

    (void)display.init();
    (void)display.display_number(1234u);
    (void)display.display_number(42u, /*leading_zeros=*/false);
    (void)display.display_number(7u,  /*leading_zeros=*/true);
    (void)display.set_brightness(3u);
    (void)display.display_digits(0x3Fu, 0x06u, 0x5Bu, 0x4Fu);
    (void)display.clear();
}

}  // namespace
