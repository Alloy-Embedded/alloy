/// Blinky Example for SAME70 Xplained
///
/// This example demonstrates basic GPIO usage by blinking the onboard LED.
///
/// Hardware: SAME70 Xplained board
/// - LED0 connected to PC8
///
/// Clock configuration:
/// - CPU: 300 MHz (from PLLA)
/// - Master Clock (MCK): 150 MHz
/// - Flash wait states: 6 cycles
///
/// Expected behavior:
/// - LED0 blinks at 1 Hz (500ms on, 500ms off)

#include "hal/vendors/atmel/same70/atsame70q21/gpio.hpp"
#include "hal/vendors/atmel/same70/clock.hpp"
#include "hal/vendors/atmel/same70/delay.hpp"

using namespace alloy::hal::atmel::same70::atsame70q21;
using namespace alloy::hal::atmel::same70;

int main() {
    // Initialize system clocks (300 MHz CPU, 150 MHz MCK)
    clock::init();

    // Create GPIO pin for LED0 (PC8)
    GPIOPin<pins::PC8> led;

    // Configure as output
    led.configureOutput();

    // Blink forever
    while (true) {
        led.set();              // Turn LED on
        delay::delay_ms(500);   // Wait 500ms

        led.clear();            // Turn LED off
        delay::delay_ms(500);   // Wait 500ms
    }

    return 0;
}
