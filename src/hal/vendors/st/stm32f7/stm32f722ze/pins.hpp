#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace alloy::hal::stm32f7::stm32f722ze::pins {

// ============================================================================
// Auto-generated pin definitions for STM32F722ZE
// Generated from SVD: STM32F7x2
// Package: LQFP144
// GPIO Pins: 112
// ============================================================================

// Port A pins (16 pins)
constexpr uint8_t PA0    = 0;
constexpr uint8_t PA1    = 1;
constexpr uint8_t PA2    = 2;
constexpr uint8_t PA3    = 3;
constexpr uint8_t PA4    = 4;
constexpr uint8_t PA5    = 5;
constexpr uint8_t PA6    = 6;
constexpr uint8_t PA7    = 7;
constexpr uint8_t PA8    = 8;
constexpr uint8_t PA9    = 9;
constexpr uint8_t PA10   = 10;
constexpr uint8_t PA11   = 11;
constexpr uint8_t PA12   = 12;
constexpr uint8_t PA13   = 13;
constexpr uint8_t PA14   = 14;
constexpr uint8_t PA15   = 15;

// Port B pins (16 pins)
constexpr uint8_t PB0    = 16;
constexpr uint8_t PB1    = 17;
constexpr uint8_t PB2    = 18;
constexpr uint8_t PB3    = 19;
constexpr uint8_t PB4    = 20;
constexpr uint8_t PB5    = 21;
constexpr uint8_t PB6    = 22;
constexpr uint8_t PB7    = 23;
constexpr uint8_t PB8    = 24;
constexpr uint8_t PB9    = 25;
constexpr uint8_t PB10   = 26;
constexpr uint8_t PB11   = 27;
constexpr uint8_t PB12   = 28;
constexpr uint8_t PB13   = 29;
constexpr uint8_t PB14   = 30;
constexpr uint8_t PB15   = 31;

// Port C pins (16 pins)
constexpr uint8_t PC0    = 32;
constexpr uint8_t PC1    = 33;
constexpr uint8_t PC2    = 34;
constexpr uint8_t PC3    = 35;
constexpr uint8_t PC4    = 36;
constexpr uint8_t PC5    = 37;
constexpr uint8_t PC6    = 38;
constexpr uint8_t PC7    = 39;
constexpr uint8_t PC8    = 40;
constexpr uint8_t PC9    = 41;
constexpr uint8_t PC10   = 42;
constexpr uint8_t PC11   = 43;
constexpr uint8_t PC12   = 44;
constexpr uint8_t PC13   = 45;
constexpr uint8_t PC14   = 46;
constexpr uint8_t PC15   = 47;

// Port D pins (16 pins)
constexpr uint8_t PD0    = 48;
constexpr uint8_t PD1    = 49;
constexpr uint8_t PD2    = 50;
constexpr uint8_t PD3    = 51;
constexpr uint8_t PD4    = 52;
constexpr uint8_t PD5    = 53;
constexpr uint8_t PD6    = 54;
constexpr uint8_t PD7    = 55;
constexpr uint8_t PD8    = 56;
constexpr uint8_t PD9    = 57;
constexpr uint8_t PD10   = 58;
constexpr uint8_t PD11   = 59;
constexpr uint8_t PD12   = 60;
constexpr uint8_t PD13   = 61;
constexpr uint8_t PD14   = 62;
constexpr uint8_t PD15   = 63;

// Port E pins (16 pins)
constexpr uint8_t PE0    = 64;
constexpr uint8_t PE1    = 65;
constexpr uint8_t PE2    = 66;
constexpr uint8_t PE3    = 67;
constexpr uint8_t PE4    = 68;
constexpr uint8_t PE5    = 69;
constexpr uint8_t PE6    = 70;
constexpr uint8_t PE7    = 71;
constexpr uint8_t PE8    = 72;
constexpr uint8_t PE9    = 73;
constexpr uint8_t PE10   = 74;
constexpr uint8_t PE11   = 75;
constexpr uint8_t PE12   = 76;
constexpr uint8_t PE13   = 77;
constexpr uint8_t PE14   = 78;
constexpr uint8_t PE15   = 79;

// Port F pins (16 pins)
constexpr uint8_t PF0    = 80;
constexpr uint8_t PF1    = 81;
constexpr uint8_t PF2    = 82;
constexpr uint8_t PF3    = 83;
constexpr uint8_t PF4    = 84;
constexpr uint8_t PF5    = 85;
constexpr uint8_t PF6    = 86;
constexpr uint8_t PF7    = 87;
constexpr uint8_t PF8    = 88;
constexpr uint8_t PF9    = 89;
constexpr uint8_t PF10   = 90;
constexpr uint8_t PF11   = 91;
constexpr uint8_t PF12   = 92;
constexpr uint8_t PF13   = 93;
constexpr uint8_t PF14   = 94;
constexpr uint8_t PF15   = 95;

// Port G pins (16 pins)
constexpr uint8_t PG0    = 96;
constexpr uint8_t PG1    = 97;
constexpr uint8_t PG2    = 98;
constexpr uint8_t PG3    = 99;
constexpr uint8_t PG4    = 100;
constexpr uint8_t PG5    = 101;
constexpr uint8_t PG6    = 102;
constexpr uint8_t PG7    = 103;
constexpr uint8_t PG8    = 104;
constexpr uint8_t PG9    = 105;
constexpr uint8_t PG10   = 106;
constexpr uint8_t PG11   = 107;
constexpr uint8_t PG12   = 108;
constexpr uint8_t PG13   = 109;
constexpr uint8_t PG14   = 110;
constexpr uint8_t PG15   = 111;


// ============================================================================
// Compile-time pin validation
// ============================================================================

namespace detail {
    // Lookup table for valid pins (constexpr array)
    constexpr uint8_t valid_pins[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111
    };

    constexpr size_t PIN_COUNT = sizeof(valid_pins) / sizeof(valid_pins[0]);

    // Check if pin exists in valid_pins array (constexpr)
    constexpr bool is_valid_pin(uint8_t pin) {
        for (size_t i = 0; i < PIN_COUNT; ++i) {
            if (valid_pins[i] == pin) return true;
        }
        return false;
    }
}

// Type trait for compile-time validation
template<uint8_t Pin>
inline constexpr bool is_valid_pin_v = detail::is_valid_pin(Pin);

// C++20 concept for compile-time pin validation
template<uint8_t Pin>
concept ValidPin = is_valid_pin_v<Pin>;

// Helper to trigger compile error with clear message
template<uint8_t Pin>
constexpr void validate_pin() {
    static_assert(ValidPin<Pin>,
        "Invalid pin for STM32F722ZE (LQFP144)! "
        "Check the datasheet for available pins on this package.");
}

// Constants
constexpr size_t TOTAL_PIN_COUNT = detail::PIN_COUNT;  // 112 pins
constexpr size_t GPIO_PORT_COUNT = 7;  // 7 ports

}  // namespace alloy::hal::stm32f7::stm32f722ze::pins
