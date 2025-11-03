#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace alloy::hal::stm32f103::stm32f103rc::pins {

// ============================================================================
// Auto-generated pin definitions for STM32F103RC
// Generated from SVD: STM32F103
// Package: LQFP64
// GPIO Pins: 67
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

// Port D pins (3 pins)
constexpr uint8_t PD0    = 48;
constexpr uint8_t PD1    = 49;
constexpr uint8_t PD2    = 50;

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


// ============================================================================
// Compile-time pin validation
// ============================================================================

namespace detail {
    // Lookup table for valid pins (constexpr array)
    constexpr uint8_t valid_pins[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79
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
        "Invalid pin for STM32F103RC (LQFP64)! "
        "Check the datasheet for available pins on this package.");
}

// Constants
constexpr size_t TOTAL_PIN_COUNT = detail::PIN_COUNT;  // 67 pins
constexpr size_t GPIO_PORT_COUNT = 5;  // 5 ports

}  // namespace alloy::hal::stm32f103::stm32f103rc::pins
