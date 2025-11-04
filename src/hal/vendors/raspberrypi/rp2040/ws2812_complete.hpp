/// Complete WS2812 Driver using RP2040 PIO
///
/// This is a fully bare-metal implementation without pico-sdk dependency.

#ifndef ALLOY_HAL_RP2040_WS2812_COMPLETE_HPP
#define ALLOY_HAL_RP2040_WS2812_COMPLETE_HPP

#include "pio.hpp"
#include "resets.hpp"
#include "ws2812_pio.h"
#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040 {

/// RGB color structure
struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr RgbColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : r(red), g(green), b(blue) {}

    /// Convert to 24-bit GRB format (WS2812 native)
    constexpr uint32_t to_grb() const {
        return (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(r) << 8) |
               static_cast<uint32_t>(b);
    }
};

/// Common colors
namespace Colors {
    constexpr RgbColor Black{0, 0, 0};
    constexpr RgbColor White{255, 255, 255};
    constexpr RgbColor Red{255, 0, 0};
    constexpr RgbColor Green{0, 255, 0};
    constexpr RgbColor Blue{0, 0, 255};
    constexpr RgbColor Yellow{255, 255, 0};
    constexpr RgbColor Cyan{0, 255, 255};
    constexpr RgbColor Magenta{255, 0, 255};
}

/// WS2812 driver using PIO
template<uint8_t PIN>
class WS2812Complete {
public:
    /// Initialize WS2812 with PIO
    void init() {
        pio_ = get_pio0();
        sm_ = 0;

        // 1. Initialize PIO peripheral (release from reset)
        pio_init(pio_);

        // 2. Also need to release IO_BANK0 and PADS_BANK0 from reset
        reset_peripheral_blocking(ResetBits::IO_BANK0);
        reset_peripheral_blocking(ResetBits::PADS_BANK0);

        // 3. Load WS2812 program into PIO instruction memory
        pio_load_program(pio_, ws2812_program_instructions, 4, 0);

        // 4. Configure GPIO for PIO control
        pio_gpio_init(pio_, sm_, PIN);

        // 5. Configure state machine
        configure_sm();

        // 6. Enable state machine
        pio_sm_set_enabled(pio_, sm_, true);
    }

    /// Set LED color
    void set_color(const RgbColor& color) {
        uint32_t grb = color.to_grb();
        // WS2812 expects 24 bits, shifted left by 8
        pio_sm_put_blocking(pio_, sm_, grb << 8);
    }

    /// Turn off LED
    void off() {
        set_color(Colors::Black);
    }

    /// Set brightness
    void set_brightness(uint8_t brightness) {
        brightness_ = brightness;
    }

    /// Set color with brightness
    void set_color_scaled(const RgbColor& color) {
        RgbColor scaled(
            (color.r * brightness_) / 255,
            (color.g * brightness_) / 255,
            (color.b * brightness_) / 255
        );
        set_color(scaled);
    }

private:
    PIO_Registers* pio_;
    uint8_t sm_;
    uint8_t brightness_ = 255;

    /// Configure PIO state machine for WS2812
    void configure_sm() {
        // Configure clock divider for 800kHz WS2812 timing
        // System clock is 125MHz (default after boot2)
        // We need to slow it down for WS2812 timing
        // cycles_per_bit = T1 + T2 + T3 = 2 + 5 + 3 = 10
        // Target frequency = 800kHz * 10 = 8MHz
        // Divider = 125MHz / 8MHz = 15.625
        // In 16.16 fixed point: 15.625 * 65536 = 1024000
        uint32_t div = (15 << 16) | (40960);  // 15.625 in 16.16 fixed point
        pio_sm_set_clkdiv(pio_, sm_, div);

        // Configure execution control
        // Set wrap top and wrap bottom
        pio_->SM[sm_].EXECCTRL = (3 << 12) | (0 << 7);  // wrap_top=3, wrap_bottom=0

        // Configure shift control
        // autopull=1, pull_thresh=24 (for 24-bit GRB)
        pio_->SM[sm_].SHIFTCTRL = (1 << 17) | (24 << 25) | (0 << 16);  // autopull, 24 bits, shift right

        // Configure pin control
        // sideset_base = PIN, sideset_count = 1
        pio_->SM[sm_].PINCTRL = (PIN << 5) | (1 << 29);  // sideset pins
    }
};

} // namespace alloy::hal::raspberrypi::rp2040

#endif // ALLOY_HAL_RP2040_WS2812_COMPLETE_HPP
