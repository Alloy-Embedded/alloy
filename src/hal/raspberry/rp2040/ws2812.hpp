/// WS2812 RGB LED Driver for RP2040
///
/// Supports WS2812, WS2812B, and compatible RGB LEDs (NeoPixel)
/// Uses bit-banging for precise timing (can be upgraded to PIO later)
///
/// Timing requirements:
/// - T0H: 0.4µs ±150ns (0 bit high time)
/// - T0L: 0.85µs ±150ns (0 bit low time)
/// - T1H: 0.8µs ±150ns (1 bit high time)
/// - T1L: 0.45µs ±150ns (1 bit low time)
/// - RES: >50µs (reset time)

#ifndef ALLOY_HAL_RP2040_WS2812_HPP
#define ALLOY_HAL_RP2040_WS2812_HPP

#include "gpio.hpp"
#include <cstdint>

// ARM Cortex-M IRQ control inline functions
static inline void disable_interrupts() {
    __asm__ volatile ("cpsid i" : : : "memory");
}

static inline void enable_interrupts() {
    __asm__ volatile ("cpsie i" : : : "memory");
}

namespace alloy::hal::rp2040 {

/// RGB color structure
struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr RgbColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : r(red), g(green), b(blue) {}

    /// Create color from 24-bit RGB value
    static constexpr RgbColor from_rgb(uint32_t rgb) {
        return RgbColor(
            (rgb >> 16) & 0xFF,
            (rgb >> 8) & 0xFF,
            rgb & 0xFF
        );
    }

    /// Convert to 24-bit GRB format (WS2812 native format)
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
    constexpr RgbColor Orange{255, 165, 0};
    constexpr RgbColor Purple{128, 0, 128};
}

/// WS2812 RGB LED driver
/// @tparam PIN GPIO pin number (0-29)
template<uint8_t PIN>
class WS2812 {
public:
    /// Initialize the LED
    void init() {
        pin_.configure(PinMode::Output);
        pin_.set_low();
        delay_us(100);  // Initial reset
    }

    /// Set LED color
    /// @param color RGB color to display
    void set_color(const RgbColor& color) {
        send_byte((color.to_grb() >> 16) & 0xFF);  // Green
        send_byte((color.to_grb() >> 8) & 0xFF);   // Red
        send_byte(color.to_grb() & 0xFF);          // Blue
        delay_us(60);  // Reset time (>50µs)
    }

    /// Turn off the LED
    void off() {
        set_color(Colors::Black);
    }

    /// Set brightness (0-255)
    void set_brightness(uint8_t brightness) {
        brightness_ = brightness;
    }

    /// Set color with brightness applied
    void set_color_scaled(const RgbColor& color) {
        RgbColor scaled(
            (color.r * brightness_) / 255,
            (color.g * brightness_) / 255,
            (color.b * brightness_) / 255
        );
        set_color(scaled);
    }

private:
    GpioPin<PIN> pin_;
    uint8_t brightness_ = 255;

    /// Send a single byte to WS2812
    void send_byte(uint8_t byte) {
        for (int i = 7; i >= 0; i--) {
            if (byte & (1 << i)) {
                send_one();
            } else {
                send_zero();
            }
        }
    }

    /// Send a '1' bit
    /// T1H: 0.8µs, T1L: 0.45µs
    inline void send_one() {
        // Disable interrupts for precise timing
        disable_interrupts();

        pin_.set_high();
        // T1H: 0.8µs @ 125MHz = 100 cycles
        // Each instruction ~8ns, so ~100 instructions
        delay_cycles(100);

        pin_.set_low();
        // T1L: 0.45µs @ 125MHz = 56 cycles
        delay_cycles(56);

        enable_interrupts();
    }

    /// Send a '0' bit
    /// T0H: 0.4µs, T0L: 0.85µs
    inline void send_zero() {
        // Disable interrupts for precise timing
        disable_interrupts();

        pin_.set_high();
        // T0H: 0.4µs @ 125MHz = 50 cycles
        delay_cycles(50);

        pin_.set_low();
        // T0L: 0.85µs @ 125MHz = 106 cycles
        delay_cycles(106);

        enable_interrupts();
    }

    /// Precise cycle delay using volatile loop
    /// @param cycles Number of CPU cycles to delay
    static inline void delay_cycles(uint32_t cycles) {
        // Simple volatile loop for delay
        // Each iteration is approximately 3-4 cycles on Cortex-M0+
        volatile uint32_t count = cycles / 4;
        while (count > 0) {
            count--;
        }
    }

    /// Microsecond delay
    /// @param us Microseconds to delay
    static void delay_us(uint32_t us) {
        // At 125MHz: 125 cycles per microsecond
        delay_cycles(us * 125);
    }
};

} // namespace alloy::hal::rp2040

#endif // ALLOY_HAL_RP2040_WS2812_HPP
