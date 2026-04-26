// Compile test: SSD1306 SPI transport variant instantiates against the
// documented SPI bus surface and GPIO DC/CS pin policies. Exercises every
// public method so that any drift in the surface fails the build.

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/display/ssd1306/ssd1306.hpp"

namespace {

// Minimal SPI bus mock. Surface: transfer(tx, rx).
struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

// Minimal GPIO pin mock for DC and CS policies.
struct MockGpioPin {
    [[nodiscard]] auto set_high() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto set_low() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};

using DcPolicy = alloy::drivers::display::ssd1306::GpioDcPolicy<MockGpioPin>;
using CsPolicy = alloy::drivers::display::ssd1306::GpioCsPolicy<MockGpioPin>;
using SpiOled  = alloy::drivers::display::ssd1306::SpiDevice<MockSpiBus, DcPolicy, CsPolicy>;

[[maybe_unused]] void compile_ssd1306_spi_against_public_surface() {
    MockSpiBus spi;
    MockGpioPin dc_pin;
    MockGpioPin cs_pin;

    SpiOled display{spi, DcPolicy{dc_pin}, CsPolicy{cs_pin},
                    {.external_vcc = false,
                     .flip_horizontal = false,
                     .flip_vertical = false}};

    (void)display.init();
    display.clear();
    (void)display.draw_pixel(0u, 0u, true);
    (void)display.draw_text(0u, 0u, std::string_view{"alloy"});
    (void)display.flush();
    auto fb = display.framebuffer();
    (void)fb.size();
}

// Verify NoOpDcPolicy / NoOpCsPolicy variants also compile.
[[maybe_unused]] void compile_ssd1306_spi_noop_policies() {
    MockSpiBus spi;
    alloy::drivers::display::ssd1306::SpiDevice<MockSpiBus> display{spi};
    (void)display.init();
    display.clear();
    (void)display.flush();
}

}  // namespace
