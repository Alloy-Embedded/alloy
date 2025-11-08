#pragma once

#include <stdint.h>

namespace alloy::hal::atmel::samv71::atsamv71j19b::pins {

// ============================================================================
// Pin Definitions for ATSAMV71J19B
// Package: LQFP64
// ============================================================================

// Port A pins
constexpr uint8_t PA0 = 0;    // A0
constexpr uint8_t PA1 = 1;    // A1
constexpr uint8_t PA2 = 2;    // A2
constexpr uint8_t PA3 = 3;    // A3
constexpr uint8_t PA4 = 4;    // A4
constexpr uint8_t PA5 = 5;    // A5
constexpr uint8_t PA6 = 6;    // A6
constexpr uint8_t PA7 = 7;    // A7
constexpr uint8_t PA8 = 8;    // A8
constexpr uint8_t PA9 = 9;    // A9
constexpr uint8_t PA10 = 10;  // A10
constexpr uint8_t PA11 = 11;  // A11
constexpr uint8_t PA12 = 12;  // A12
constexpr uint8_t PA13 = 13;  // A13
constexpr uint8_t PA14 = 14;  // A14
constexpr uint8_t PA15 = 15;  // A15
constexpr uint8_t PA16 = 16;  // A16
constexpr uint8_t PA17 = 17;  // A17
constexpr uint8_t PA18 = 18;  // A18
constexpr uint8_t PA19 = 19;  // A19
constexpr uint8_t PA20 = 20;  // A20
constexpr uint8_t PA21 = 21;  // A21
constexpr uint8_t PA22 = 22;  // A22
constexpr uint8_t PA23 = 23;  // A23

// Port B pins
constexpr uint8_t PB0 = 32;  // B0
constexpr uint8_t PB1 = 33;  // B1
constexpr uint8_t PB2 = 34;  // B2
constexpr uint8_t PB3 = 35;  // B3

// Port C pins
constexpr uint8_t PC0 = 64;   // C0
constexpr uint8_t PC1 = 65;   // C1
constexpr uint8_t PC2 = 66;   // C2
constexpr uint8_t PC3 = 67;   // C3
constexpr uint8_t PC4 = 68;   // C4
constexpr uint8_t PC5 = 69;   // C5
constexpr uint8_t PC6 = 70;   // C6
constexpr uint8_t PC7 = 71;   // C7
constexpr uint8_t PC8 = 72;   // C8
constexpr uint8_t PC9 = 73;   // C9
constexpr uint8_t PC10 = 74;  // C10
constexpr uint8_t PC11 = 75;  // C11
constexpr uint8_t PC12 = 76;  // C12
constexpr uint8_t PC13 = 77;  // C13
constexpr uint8_t PC14 = 78;  // C14
constexpr uint8_t PC15 = 79;  // C15
constexpr uint8_t PC16 = 80;  // C16
constexpr uint8_t PC17 = 81;  // C17
constexpr uint8_t PC18 = 82;  // C18
constexpr uint8_t PC19 = 83;  // C19

// Port D pins
constexpr uint8_t PD0 = 96;    // D0
constexpr uint8_t PD1 = 97;    // D1
constexpr uint8_t PD2 = 98;    // D2
constexpr uint8_t PD3 = 99;    // D3
constexpr uint8_t PD4 = 100;   // D4
constexpr uint8_t PD5 = 101;   // D5
constexpr uint8_t PD6 = 102;   // D6
constexpr uint8_t PD7 = 103;   // D7
constexpr uint8_t PD8 = 104;   // D8
constexpr uint8_t PD9 = 105;   // D9
constexpr uint8_t PD10 = 106;  // D10
constexpr uint8_t PD11 = 107;  // D11
constexpr uint8_t PD12 = 108;  // D12
constexpr uint8_t PD13 = 109;  // D13
constexpr uint8_t PD14 = 110;  // D14
constexpr uint8_t PD15 = 111;  // D15
constexpr uint8_t PD16 = 112;  // D16
constexpr uint8_t PD17 = 113;  // D17
constexpr uint8_t PD18 = 114;  // D18
constexpr uint8_t PD19 = 115;  // D19
constexpr uint8_t PD20 = 116;  // D20
constexpr uint8_t PD21 = 117;  // D21


// Port base indices for pin addressing
enum class Port : uint8_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
};

// Helper to get port from pin name (compile-time)
template <char PortChar>
constexpr Port get_port() {
    if constexpr (PortChar == 'A')
        return Port::A;
    if constexpr (PortChar == 'B')
        return Port::B;
    if constexpr (PortChar == 'C')
        return Port::C;
    if constexpr (PortChar == 'D')
        return Port::D;
    else
        static_assert(PortChar >= 'A' && PortChar <= 'E', "Invalid port");
}

}  // namespace alloy::hal::atmel::samv71::atsamv71j19b::pins
