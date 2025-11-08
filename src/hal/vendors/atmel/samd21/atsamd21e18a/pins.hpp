#pragma once

#include <cstdint>

namespace alloy::hal::atmel::samd21::atsamd21e18a::pins {

// ============================================================================
// Pin Definitions for ATSAMD21E18A
// Package: TQFP32
// ============================================================================

// Port A pins
constexpr uint8_t PA02 = 2;   // A2
constexpr uint8_t PA03 = 3;   // A3
constexpr uint8_t PA04 = 4;   // A4
constexpr uint8_t PA05 = 5;   // A5
constexpr uint8_t PA06 = 6;   // A6
constexpr uint8_t PA07 = 7;   // A7
constexpr uint8_t PA08 = 8;   // A8
constexpr uint8_t PA09 = 9;   // A9
constexpr uint8_t PA14 = 14;  // A14
constexpr uint8_t PA15 = 15;  // A15
constexpr uint8_t PA16 = 16;  // A16
constexpr uint8_t PA17 = 17;  // A17
constexpr uint8_t PA22 = 22;  // A22
constexpr uint8_t PA23 = 23;  // A23
constexpr uint8_t PA24 = 24;  // A24
constexpr uint8_t PA25 = 25;  // A25
constexpr uint8_t PA27 = 27;  // A27
constexpr uint8_t PA28 = 28;  // A28
constexpr uint8_t PA30 = 30;  // A30
constexpr uint8_t PA31 = 31;  // A31


// Port indices
enum class Port : uint8_t {
    A = 0,
    B = 1,
};

}  // namespace alloy::hal::atmel::samd21::atsamd21e18a::pins
