// Compile test: ILI9341 seed driver instantiates against the documented public
// SPI HAL surface. Exercises every driver method the seed exposes so that any
// drift in the bus handle's `transfer()` signature, the DcPolicy/CsPolicy
// interface, or the driver's own public surface fails the build instead of
// silently regressing.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/display/ili9341/ili9341.hpp"

namespace {

namespace drv = alloy::drivers::display::ili9341;

// ── Mock SPI bus ──────────────────────────────────────────────────────────────

struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

// ── Mock GPIO pins (DC and CS) ────────────────────────────────────────────────

struct MockDcPin {
    [[nodiscard]] auto set_high() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto set_low() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};

struct MockCsPin {
    [[nodiscard]] auto set_high() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto set_low() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};

// ── Compile exercise ──────────────────────────────────────────────────────────

[[maybe_unused]] void compile_ili9341_against_public_spi_handle() {
    MockSpiBus bus;
    MockDcPin  dc_pin;
    MockCsPin  cs_pin;

    drv::GpioDcPolicy<MockDcPin> dc{dc_pin};
    drv::GpioCsPolicy<MockCsPin> cs{cs_pin};

    drv::Device<MockSpiBus, drv::GpioDcPolicy<MockDcPin>,
                             drv::GpioCsPolicy<MockCsPin>>
        display{bus, dc, cs, {.madctl = 0x48u, .pixfmt = 0x55u}};

    // init()
    (void)display.init();

    // fill_rect()
    (void)display.fill_rect(0u, 0u, drv::kWidth, drv::kHeight, 0x001Fu);  // blue

    // draw_pixel()
    (void)display.draw_pixel(10u, 20u, 0xF800u);  // red

    // blit()
    std::array<std::uint16_t, 4> pixels{0xFFFFu, 0x0000u, 0xF800u, 0x001Fu};
    (void)display.blit(0u, 0u, 2u, 2u,
                       std::span<const std::uint16_t>{pixels});
}

// Also verify the default (NoOp) policy combination compiles.
[[maybe_unused]] void compile_ili9341_noop_policies() {
    MockSpiBus bus;
    drv::Device<MockSpiBus> display{bus};
    (void)display.init();
    (void)display.fill_rect(0u, 0u, 10u, 10u, 0x07E0u);  // green
    (void)display.draw_pixel(5u, 5u, 0xFFFFu);
    std::array<std::uint16_t, 1> px{0x001Fu};
    (void)display.blit(0u, 0u, 1u, 1u, std::span<const std::uint16_t>{px});
}

}  // namespace
