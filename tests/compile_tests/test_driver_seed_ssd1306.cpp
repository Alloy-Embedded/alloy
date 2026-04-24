// Compile test: SSD1306 seed driver instantiates against the documented public
// I2C HAL surface. Exercises every driver method the seed exposes so that any
// drift in the bus handle's `write()` signature or the driver's own public
// surface fails the build instead of silently regressing.

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/display/ssd1306/ssd1306.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t /*address*/,
                             std::span<const std::uint8_t> /*data*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto read(std::uint16_t /*address*/,
                            std::span<std::uint8_t> /*data*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto write_read(std::uint16_t /*address*/,
                                  std::span<const std::uint8_t> /*tx*/,
                                  std::span<std::uint8_t> /*rx*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_ssd1306_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::display::ssd1306::Device display{bus,
                                                    {.address = 0x3C,
                                                     .external_vcc = false,
                                                     .flip_horizontal = false,
                                                     .flip_vertical = false}};

    (void)display.init();
    display.clear();
    (void)display.draw_pixel(0, 0, true);
    (void)display.draw_text(0, 0, std::string_view{"alloy"});
    (void)display.flush();
    auto fb = display.framebuffer();
    (void)fb.size();
}

}  // namespace
