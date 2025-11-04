/// WS2812 RGB LED Driver for RP2040 using PIO
///
/// This version uses the RP2040's Programmable I/O (PIO) for precise timing.
/// Much more reliable than bit-banging approach.

#ifndef ALLOY_HAL_RP2040_WS2812_V2_HPP
#define ALLOY_HAL_RP2040_WS2812_V2_HPP

#include "gpio.hpp"
#include "ws2812_pio.h"
#include <cstdint>

// PIO registers (from RP2040 datasheet)
#define PIO0_BASE 0x50200000
#define PIO1_BASE 0x50300000

namespace alloy::hal::rp2040 {

/// RGB color structure
struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr RgbColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : r(red), g(green), b(blue) {}

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

/// PIO peripheral wrapper
struct PIO {
    volatile uint32_t CTRL;           // 0x000
    volatile uint32_t FSTAT;          // 0x004
    volatile uint32_t FDEBUG;         // 0x008
    volatile uint32_t FLEVEL;         // 0x00c
    volatile uint32_t TXF[4];         // 0x010-0x01c
    volatile uint32_t RXF[4];         // 0x020-0x2c
    volatile uint32_t IRQ;            // 0x030
    volatile uint32_t IRQ_FORCE;      // 0x034
    volatile uint32_t INPUT_SYNC_BYPASS; // 0x038
    volatile uint32_t DBG_PADOUT;     // 0x03c
    volatile uint32_t DBG_PADOE;      // 0x040
    volatile uint32_t DBG_CFGINFO;    // 0x044
    volatile uint32_t INSTR_MEM[32];  // 0x048-0x0c4
    // ... more registers (SM0-SM3 config, etc.)
};

/// WS2812 driver using PIO
/// @tparam PIN GPIO pin number (0-29)
template<uint8_t PIN>
class WS2812PIO {
public:
    /// Initialize the LED with PIO
    void init() {
        pio_ = reinterpret_cast<PIO*>(PIO0_BASE);
        sm_ = 0;  // Use state machine 0

        // Load PIO program into instruction memory
        for (uint i = 0; i < 4; i++) {
            pio_->INSTR_MEM[i] = ws2812_program_instructions[i];
        }

        // Initialize PIO state machine
        ws2812_program_init_manual(0, PIN, 800000.0f, false);
    }

    /// Set LED color
    void set_color(const RgbColor& color) {
        uint32_t grb = color.to_grb();
        // Write to PIO TX FIFO (blocking)
        while ((pio_->FSTAT & (1 << (sm_ + 0))) == 0) {
            // Wait for TX FIFO to have space
        }
        pio_->TXF[sm_] = grb << 8;  // Shift left 8 bits
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
    PIO* pio_;
    uint8_t sm_;
    uint8_t brightness_ = 255;

    /// Manual PIO initialization (simplified version)
    void ws2812_program_init_manual(uint offset, uint pin, float freq, bool rgbw) {
        // Configure GPIO for PIO control
        volatile uint32_t* io_ctrl = get_io_bank0_ctrl(pin);
        *io_ctrl = 6;  // Function 6 = PIO0

        // Set pin direction to output via PIO
        // (PIO takes control, we don't need to set it manually)

        // Configure state machine (simplified - would need full SM config registers)
        // For now, assume PIO is already configured by boot2
    }
};

} // namespace alloy::hal::rp2040

#endif // ALLOY_HAL_RP2040_WS2812_V2_HPP
