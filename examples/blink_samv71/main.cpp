/// Blinky Example for SAMV71 Xplained Ultra
///
/// This example demonstrates basic GPIO usage by blinking the onboard LED.
///
/// Hardware: SAMV71 Xplained Ultra board
/// - LED0 connected to PA23
///
/// Expected behavior:
/// - LED0 blinks at 1 Hz (500ms on, 500ms off)

#include "hal/vendors/atmel/samv71/atsamv71q21/gpio.hpp"
#include "platform/delay.hpp"

using namespace alloy::hal::atmel::samv71::atsamv71q21;
using namespace alloy::platform;

int main() {
    // Create GPIO pin for LED0 (PA23)
    GPIOPin<pins::PA23> led;

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
