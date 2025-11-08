/// ATSAME70 Xplained Blink Example
///
/// Simple LED blink example demonstrating:
/// - Board-level abstraction
/// - LED control
/// - Delay functions
/// - Cortex-M7 initialization (FPU, no cache yet)
///
/// Hardware:
/// - Board: ATSAME70 Xplained
/// - MCU:   ATSAME70Q21B (Cortex-M7 @ 300MHz max)
/// - LED:   User LED on PC8 (active LOW)
///
/// Expected behavior:
/// - User LED blinks at 1Hz (500ms ON, 500ms OFF)

#include "../../boards/atmel_same70_xpld/board.hpp"

int main() {
    // Initialize board (clocks, peripherals)
    Board::initialize();

    // Initialize LED
    Board::Led::init();

    // Blink forever
    while (true) {
        Board::Led::on();      // Turn LED ON
        Board::delay_ms(500);  // Wait 500ms

        Board::Led::off();     // Turn LED OFF
        Board::delay_ms(500);  // Wait 500ms
    }

    return 0;
}
