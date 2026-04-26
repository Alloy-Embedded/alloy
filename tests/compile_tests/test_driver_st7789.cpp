// Compile test: ST7789 TFT display driver instantiates against the documented
// SPI bus surface, DC policy, and CS policy. Exercises init(), fill_rect(),
// draw_pixel(), and blit() so that any drift in the public API fails the build.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/display/st7789/st7789.hpp"

namespace {

// ── Mock SPI bus ──────────────────────────────────────────────────────────────

struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

// ── Mock GPIO DC pin ──────────────────────────────────────────────────────────

struct MockDcPin {
    [[nodiscard]] auto set_low()
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto set_high()
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        return alloy::core::Ok();
    }
};

// ── Mock GPIO CS pin ──────────────────────────────────────────────────────────

struct MockCsPin {
    [[nodiscard]] auto set_low()
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto set_high()
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        return alloy::core::Ok();
    }
};

// ── Compile exercise ──────────────────────────────────────────────────────────

[[maybe_unused]] void compile_st7789_against_public_spi_handle() {
    namespace st7789 = alloy::drivers::display::st7789;

    MockSpiBus bus;
    MockDcPin  dc_pin;
    MockCsPin  cs_pin;

    st7789::GpioDcPolicy<MockDcPin> dc_policy{dc_pin};
    st7789::GpioCsPolicy<MockCsPin> cs_policy{cs_pin};

    st7789::Device<MockSpiBus,
                   st7789::GpioDcPolicy<MockDcPin>,
                   st7789::GpioCsPolicy<MockCsPin>> display{bus, dc_policy, cs_policy};

    // init()
    (void)display.init();

    // fill_rect() — fill a 10×10 red rectangle at (0, 0).
    constexpr std::uint16_t kRed = 0xF800u;  // RGB565 red
    (void)display.fill_rect(0u, 0u, 10u, 10u, kRed);

    // draw_pixel() — draw a single white pixel at centre.
    constexpr std::uint16_t kWhite = 0xFFFFu;
    (void)display.draw_pixel(120u, 160u, kWhite);

    // blit() — upload a small user buffer.
    constexpr std::size_t kBufPixels = 4u;
    const std::array<std::uint16_t, kBufPixels> pixels{kWhite, kWhite, kWhite, kWhite};
    (void)display.blit(0u, 0u, 2u, 2u,
                       std::span<const std::uint16_t>{pixels.data(), pixels.size()});
}

}  // namespace
