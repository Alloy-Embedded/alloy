/// Blinky Example
///
/// This example demonstrates basic GPIO usage by toggling an LED.
/// On host platforms, it prints GPIO state changes to the console.
/// On embedded targets, it will blink a physical LED.
///
/// Hardware setup (embedded):
/// - LED connected to GPIO pin 25 (or onboard LED)
///
/// Expected output (host):
/// [GPIO Mock] Pin 25 configured as Output
/// [GPIO Mock] Pin 25 set HIGH
/// [GPIO Mock] Pin 25 toggled to LOW
/// [GPIO Mock] Pin 25 toggled to HIGH
/// ...

#include "hal/host/gpio.hpp"
#include "platform/delay.hpp"
#include <iostream>

using namespace alloy::hal;
using namespace alloy::platform;

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "Alloy Blinky Example (Host Mode)" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;

    // Create GPIO pin 25 (commonly used for onboard LED on many boards)
    host::GpioPin<25> led;

    // Configure as output
    led.configure(PinMode::Output);

    // Initial state: HIGH
    led.set_high();

    std::cout << std::endl;
    std::cout << "Starting blink loop (Ctrl+C to stop)..." << std::endl;
    std::cout << std::endl;

    // Blink forever
    while (true) {
        delay_ms(500);  // Wait 500ms
        led.toggle();   // Toggle LED state
    }

    return 0;
}
