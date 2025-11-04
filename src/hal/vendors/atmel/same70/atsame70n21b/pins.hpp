#pragma once

#include <cstdint>

namespace alloy::hal::atmel::same70::atsame70n21b::pins {

// ============================================================================
// Pin Definitions for ATSAME70N21B
// Package: LQFP100
// ============================================================================

// Port A pins
constexpr uint8_t PA0 = 0;  // A0
constexpr uint8_t PA1 = 1;  // A1
constexpr uint8_t PA2 = 2;  // A2
constexpr uint8_t PA3 = 3;  // A3
constexpr uint8_t PA4 = 4;  // A4
constexpr uint8_t PA5 = 5;  // A5
constexpr uint8_t PA6 = 6;  // A6
constexpr uint8_t PA7 = 7;  // A7
constexpr uint8_t PA8 = 8;  // A8
constexpr uint8_t PA9 = 9;  // A9
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
constexpr uint8_t PA24 = 24;  // A24

// Port B pins
constexpr uint8_t PB0 = 0;  // B0
constexpr uint8_t PB1 = 1;  // B1
constexpr uint8_t PB2 = 2;  // B2
constexpr uint8_t PB3 = 3;  // B3
constexpr uint8_t PB4 = 4;  // B4
constexpr uint8_t PB5 = 5;  // B5
constexpr uint8_t PB6 = 6;  // B6
constexpr uint8_t PB7 = 7;  // B7
constexpr uint8_t PB8 = 8;  // B8
constexpr uint8_t PB9 = 9;  // B9
constexpr uint8_t PB10 = 10;  // B10
constexpr uint8_t PB11 = 11;  // B11
constexpr uint8_t PB12 = 12;  // B12
constexpr uint8_t PB13 = 13;  // B13

// Port C pins
constexpr uint8_t PC0 = 0;  // C0
constexpr uint8_t PC1 = 1;  // C1
constexpr uint8_t PC2 = 2;  // C2
constexpr uint8_t PC3 = 3;  // C3
constexpr uint8_t PC4 = 4;  // C4
constexpr uint8_t PC5 = 5;  // C5
constexpr uint8_t PC6 = 6;  // C6
constexpr uint8_t PC7 = 7;  // C7
constexpr uint8_t PC8 = 8;  // C8
constexpr uint8_t PC9 = 9;  // C9
constexpr uint8_t PC10 = 10;  // C10
constexpr uint8_t PC11 = 11;  // C11
constexpr uint8_t PC12 = 12;  // C12
constexpr uint8_t PC13 = 13;  // C13
constexpr uint8_t PC14 = 14;  // C14
constexpr uint8_t PC15 = 15;  // C15
constexpr uint8_t PC16 = 16;  // C16
constexpr uint8_t PC17 = 17;  // C17
constexpr uint8_t PC18 = 18;  // C18
constexpr uint8_t PC19 = 19;  // C19
constexpr uint8_t PC20 = 20;  // C20
constexpr uint8_t PC21 = 21;  // C21
constexpr uint8_t PC22 = 22;  // C22
constexpr uint8_t PC23 = 23;  // C23
constexpr uint8_t PC24 = 24;  // C24
constexpr uint8_t PC25 = 25;  // C25
constexpr uint8_t PC26 = 26;  // C26
constexpr uint8_t PC27 = 27;  // C27
constexpr uint8_t PC28 = 28;  // C28
constexpr uint8_t PC29 = 29;  // C29
constexpr uint8_t PC30 = 30;  // C30
constexpr uint8_t PC31 = 31;  // C31

// Port D pins
constexpr uint8_t PD0 = 0;  // D0
constexpr uint8_t PD1 = 1;  // D1
constexpr uint8_t PD2 = 2;  // D2
constexpr uint8_t PD3 = 3;  // D3
constexpr uint8_t PD4 = 4;  // D4
constexpr uint8_t PD5 = 5;  // D5
constexpr uint8_t PD6 = 6;  // D6
constexpr uint8_t PD7 = 7;  // D7
constexpr uint8_t PD8 = 8;  // D8
constexpr uint8_t PD9 = 9;  // D9
constexpr uint8_t PD10 = 10;  // D10
constexpr uint8_t PD11 = 11;  // D11
constexpr uint8_t PD12 = 12;  // D12
constexpr uint8_t PD13 = 13;  // D13
constexpr uint8_t PD14 = 14;  // D14
constexpr uint8_t PD15 = 15;  // D15
constexpr uint8_t PD16 = 16;  // D16
constexpr uint8_t PD17 = 17;  // D17
constexpr uint8_t PD18 = 18;  // D18
constexpr uint8_t PD19 = 19;  // D19
constexpr uint8_t PD20 = 20;  // D20
constexpr uint8_t PD21 = 21;  // D21
constexpr uint8_t PD22 = 22;  // D22
constexpr uint8_t PD23 = 23;  // D23
constexpr uint8_t PD24 = 24;  // D24
constexpr uint8_t PD25 = 25;  // D25
constexpr uint8_t PD26 = 26;  // D26
constexpr uint8_t PD27 = 27;  // D27


// Port base indices for pin addressing
enum class Port : uint8_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
};

// Helper to get port from pin name (compile-time)
template<char PortChar>
constexpr Port get_port() {
    if constexpr (PortChar == 'A') return Port::A;
    if constexpr (PortChar == 'B') return Port::B;
    if constexpr (PortChar == 'C') return Port::C;
    if constexpr (PortChar == 'D') return Port::D;
    else static_assert(PortChar >= 'A' && PortChar <= 'E', "Invalid port");
}

}  // namespace alloy::hal::atmel::same70::atsame70n21b::pins
