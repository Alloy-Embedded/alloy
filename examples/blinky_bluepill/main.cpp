/// Blinky Example for Blue Pill (STM32F103C8T6)
///
/// This example demonstrates GPIO usage on the STM32F103 (Blue Pill board).
/// It blinks the onboard LED connected to PC13.
///
/// Hardware:
/// - Blue Pill board (STM32F103C8T6)
/// - Onboard LED on PC13 (active low - LED is ON when pin is LOW)
///
/// Build:
///   cmake -B build -DALLOY_BOARD=bluepill
///   cmake --build build --target blinky_bluepill
///
/// Flash:
///   # Using ST-Link
///   st-flash write build/blinky_bluepill.bin 0x8000000
///
///   # Using USB bootloader (DFU)
///   dfu-util -a 0 -s 0x08000000 -D build/blinky_bluepill.bin

#include "hal/stm32f1/gpio.hpp"
#include "hal/stm32f1/delay.hpp"

using namespace alloy::hal;

int main() {
    // PC13 = Port C, Pin 13 = (2 * 16) + 13 = 45
    constexpr uint8_t LED_PIN = 45;

    // Create GPIO pin for onboard LED (PC13)
    stm32f1::GpioPin<LED_PIN> led;

    // Configure as output
    led.configure(PinMode::Output);

    // Blink forever
    // Note: LED on Blue Pill is active-low (LOW = ON, HIGH = OFF)
    while (true) {
        led.set_low();   // Turn LED ON
        stm32f1::delay_ms(500);

        led.set_high();  // Turn LED OFF
        stm32f1::delay_ms(500);
    }

    return 0;
}
