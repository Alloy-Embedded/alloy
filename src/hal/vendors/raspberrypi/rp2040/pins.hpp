#pragma once

#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040::pins {

// ============================================================================
// Pin Definitions for RP2040
// ============================================================================

// GPIO pins
constexpr uint8_t GPIO0  = 0;
constexpr uint8_t GPIO1  = 1;
constexpr uint8_t GPIO2  = 2;
constexpr uint8_t GPIO3  = 3;
constexpr uint8_t GPIO4  = 4;
constexpr uint8_t GPIO5  = 5;
constexpr uint8_t GPIO6  = 6;
constexpr uint8_t GPIO7  = 7;
constexpr uint8_t GPIO8  = 8;
constexpr uint8_t GPIO9  = 9;
constexpr uint8_t GPIO10 = 10;
constexpr uint8_t GPIO11 = 11;
constexpr uint8_t GPIO12 = 12;
constexpr uint8_t GPIO13 = 13;
constexpr uint8_t GPIO14 = 14;
constexpr uint8_t GPIO15 = 15;
constexpr uint8_t GPIO16 = 16;
constexpr uint8_t GPIO17 = 17;
constexpr uint8_t GPIO18 = 18;
constexpr uint8_t GPIO19 = 19;
constexpr uint8_t GPIO20 = 20;
constexpr uint8_t GPIO21 = 21;
constexpr uint8_t GPIO22 = 22;
constexpr uint8_t GPIO23 = 23;
constexpr uint8_t GPIO24 = 24;
constexpr uint8_t GPIO25 = 25;  // On-board LED on Pico
constexpr uint8_t GPIO26 = 26;  // ADC0
constexpr uint8_t GPIO27 = 27;  // ADC1
constexpr uint8_t GPIO28 = 28;  // ADC2
constexpr uint8_t GPIO29 = 29;  // ADC3

// Convenience aliases for Pico board
constexpr uint8_t LED = GPIO25;

}  // namespace alloy::hal::raspberrypi::rp2040::pins
