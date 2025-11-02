/// Blink example for Waveshare RP2040-Zero
///
/// This example demonstrates:
/// - WS2812 RGB LED control
/// - Color cycling through rainbow
/// - Board initialization
///
/// The onboard WS2812 LED will cycle through colors:
/// Red -> Green -> Blue -> Yellow -> Cyan -> Magenta -> White

#include "board.hpp"

using namespace alloy::hal::raspberrypi::rp2040;

int main() {
    // Initialize board (clock, peripherals, etc.)
    Board::initialize();

    // Initialize RGB LED
    Board::Led::init();

    // Set brightness to 50% (WS2812 can be very bright!)
    Board::Led::set_brightness(128);

    // Array of colors to cycle through
    const RgbColor colors[] = {
        Colors::Red,
        Colors::Green,
        Colors::Blue,
        Colors::Yellow,
        Colors::Cyan,
        Colors::Magenta,
        Colors::White
    };
    constexpr size_t num_colors = sizeof(colors) / sizeof(colors[0]);

    // Blink and cycle colors forever
    size_t color_index = 0;
    while (true) {
        // Turn on with current color
        Board::Led::on(colors[color_index]);
        Board::delay_ms(500);

        // Turn off
        Board::Led::off();
        Board::delay_ms(500);

        // Move to next color (avoid modulo operator to save code size)
        color_index++;
        if (color_index >= num_colors) {
            color_index = 0;
        }
    }

    return 0;
}
