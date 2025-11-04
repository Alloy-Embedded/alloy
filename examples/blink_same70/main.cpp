/// Blinky Example for SAME70 Xplained
///
/// This example demonstrates basic GPIO usage by blinking the onboard LED.
///
/// Hardware: SAME70 Xplained board
/// - LED0 connected to PC8
///
/// Expected behavior:
/// - LED0 blinks at 1 Hz (500ms on, 500ms off)

#include "hal/vendors/atmel/same70/atsame70q21/gpio.hpp"
#include "platform/delay.hpp"

using namespace alloy::hal::atmel::same70::atsame70q21;
using namespace alloy::platform;

int main() {
    // Create GPIO pin for LED0 (PC8)
    GPIOPin<pins::PC8> led;

    // Configure as output
    led.configureOutput();

    // Blink forever
    while (true) {
        led.set();          // Turn LED on
        delay_ms(500);      // Wait 500ms

        led.clear();        // Turn LED off
        delay_ms(500);      // Wait 500ms
    }

    return 0;
}
