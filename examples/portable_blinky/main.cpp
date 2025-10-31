/**
 * @file portable_blinky/main.cpp
 * @brief Portable LED blinky example that works on ANY MCU
 *
 * This example demonstrates MCU-agnostic code using generic headers.
 * The SAME code works on STM32, nRF, ESP32, RP2040, etc.
 * Just change ALLOY_MCU in CMake!
 *
 * Supported MCUs (examples):
 * - STM32F103C8 (BluePill)
 * - STM32F446RE (Nucleo)
 * - nRF52840
 * - ESP32
 * - RP2040 (Raspberry Pi Pico)
 *
 * Pin mapping (defined per board in CMakeLists.txt):
 * - BluePill: PC13 (onboard LED)
 * - Nucleo:   PA5 (onboard LED)
 * - Pico:     GPIO25 (onboard LED)
 */

// Generic portable headers - work on any MCU!
#include "alloy/hal/gpio.hpp"
#include "alloy/hal/uart.hpp"
#include "core/types.hpp"

using namespace alloy::hal;
using namespace alloy::core;

// LED pin is configured by CMakeLists.txt based on board
// For example: -DLED_PIN=45 for BluePill PC13
#ifndef LED_PIN
#define LED_PIN 13  // Default fallback
#endif

// Simple delay function (blocking, for demonstration)
void delay_ms(uint32_t ms) {
    // This is a very rough delay - should use timer in production
    for (volatile uint32_t i = 0; i < ms * 1000; i++) {
        __asm volatile ("nop");
    }
}

int main() {
    // Configure LED pin as output
    // This works on ANY MCU - the implementation is vendor-specific
    GpioPin<LED_PIN> led;
    led.configure(PinMode::Output);

    // Optionally configure UART for debug output
    // Comment this out if your board doesn't have UART configured
    #ifdef USE_UART
    UartDevice<UsartId::USART1> serial;
    serial.configure({baud_rates::Baud115200});

    const char* hello = "Portable Blinky Started!\r\n";
    for (const char* p = hello; *p; p++) {
        serial.write_byte(static_cast<u8>(*p));
    }
    #endif

    // Main loop - blink LED forever
    uint32_t count = 0;
    while (true) {
        led.toggle();

        #ifdef USE_UART
        // Send heartbeat message
        if (count % 10 == 0) {
            const char* msg = "Heartbeat\r\n";
            for (const char* p = msg; *p; p++) {
                serial.write_byte(static_cast<u8>(*p));
            }
        }
        #endif

        delay_ms(500);  // 500ms delay
        count++;
    }

    return 0;
}
