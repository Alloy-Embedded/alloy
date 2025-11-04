/// Blinky Example for Raspberry Pi Pico
///
/// This example demonstrates basic GPIO usage by blinking the onboard LED.
///
/// Hardware: Raspberry Pi Pico board (RP2040)
/// - LED connected to GPIO25
///
/// Expected behavior:
/// - LED blinks at 1 Hz (500ms on, 500ms off)

#include "hal/vendors/raspberrypi/rp2040/gpio.hpp"
#include "platform/delay.hpp"

using namespace alloy::hal::raspberrypi::rp2040;
using namespace alloy::platform;

int main() {
    // Create GPIO pin for LED (GPIO25 = on-board LED on Pico)
    GPIOPin<pins::LED> led;

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
