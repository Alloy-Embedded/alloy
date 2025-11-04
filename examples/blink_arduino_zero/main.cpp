/// Blinky Example for Arduino Zero
///
/// This example demonstrates basic GPIO usage by blinking the onboard LED.
///
/// Hardware: Arduino Zero board (ATSAMD21G18A)
/// - LED connected to PA17 (D13)
///
/// Expected behavior:
/// - LED blinks at 1 Hz (500ms on, 500ms off)

#include "hal/vendors/atmel/samd21/atsamd21g18a/gpio.hpp"
#include "platform/delay.hpp"

using namespace alloy::hal::atmel::samd21::atsamd21g18a;
using namespace alloy::platform;

int main() {
    // Create GPIO pin for LED (PA17 = D13 on Arduino Zero)
    // Port A = 0, Pin 17
    GPIOPin<0, pins::PA17> led;

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
