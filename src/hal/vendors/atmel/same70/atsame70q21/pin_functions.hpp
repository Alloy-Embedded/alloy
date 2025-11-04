#pragma once

#include <cstdint>
#include "pins.hpp"

namespace alloy::hal::atmel::same70::atsame70q21::pin_functions {

// ============================================================================
// Pin Alternate Functions for ATSAME70Q21
// Based on SAME70 I/O Multiplexing Table
// ============================================================================

// Peripheral function selection
enum class PeripheralFunction : uint8_t {
    PIO = 0,     // GPIO mode
    A = 1,       // Peripheral A
    B = 2,       // Peripheral B
    C = 3,       // Peripheral C
    D = 4,       // Peripheral D
};

// ============================================================================
// ADC Peripheral Functions
// ============================================================================

// PA8: AFE0_ADTRG (Peripheral C)
template<>
struct PinFunction<pins::PA8, PeripheralFunction::C> {
    static constexpr const char* name = "AFE0_ADTRG";
    static constexpr const char* peripheral_type = "ADC";
};

// PB0: AFE0_AD10 (Peripheral A)
template<>
struct PinFunction<pins::PB0, PeripheralFunction::A> {
    static constexpr const char* name = "AFE0_AD10";
    static constexpr const char* peripheral_type = "ADC";
};

// PB1: AFE1_AD0 (Peripheral A)
template<>
struct PinFunction<pins::PB1, PeripheralFunction::A> {
    static constexpr const char* name = "AFE1_AD0";
    static constexpr const char* peripheral_type = "ADC";
};

// ============================================================================
// CAN Peripheral Functions
// ============================================================================

// PB2: CANTX0 (Peripheral A)
template<>
struct PinFunction<pins::PB2, PeripheralFunction::A> {
    static constexpr const char* name = "CANTX0";
    static constexpr const char* peripheral_type = "CAN";
};

// PB3: CANRX0 (Peripheral A)
template<>
struct PinFunction<pins::PB3, PeripheralFunction::A> {
    static constexpr const char* name = "CANRX0";
    static constexpr const char* peripheral_type = "CAN";
};

// ============================================================================
// DAC Peripheral Functions
// ============================================================================

// PA2: DACC_DATRG (Peripheral C)
template<>
struct PinFunction<pins::PA2, PeripheralFunction::C> {
    static constexpr const char* name = "DACC_DATRG";
    static constexpr const char* peripheral_type = "DAC";
};

// ============================================================================
// EBI Peripheral Functions
// ============================================================================

// PC0: EBIA10 (Peripheral A)
template<>
struct PinFunction<pins::PC0, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA10";
    static constexpr const char* peripheral_type = "EBI";
};

// PC1: EBIA11 (Peripheral A)
template<>
struct PinFunction<pins::PC1, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA11";
    static constexpr const char* peripheral_type = "EBI";
};

// PC10: EBIA20 (Peripheral A)
template<>
struct PinFunction<pins::PC10, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA20";
    static constexpr const char* peripheral_type = "EBI";
};

// PC11: EBIA21 (Peripheral A)
template<>
struct PinFunction<pins::PC11, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA21";
    static constexpr const char* peripheral_type = "EBI";
};

// PC12: EBIA22 (Peripheral A)
template<>
struct PinFunction<pins::PC12, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA22";
    static constexpr const char* peripheral_type = "EBI";
};

// PC13: EBIA23 (Peripheral A)
template<>
struct PinFunction<pins::PC13, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA23";
    static constexpr const char* peripheral_type = "EBI";
};

// PC14: EBINWR0 (Peripheral A)
template<>
struct PinFunction<pins::PC14, PeripheralFunction::A> {
    static constexpr const char* name = "EBINWR0";
    static constexpr const char* peripheral_type = "EBI";
};

// PC15: EBINCS0 (Peripheral A)
template<>
struct PinFunction<pins::PC15, PeripheralFunction::A> {
    static constexpr const char* name = "EBINCS0";
    static constexpr const char* peripheral_type = "EBI";
};

// PC16: EBID16 (Peripheral A)
template<>
struct PinFunction<pins::PC16, PeripheralFunction::A> {
    static constexpr const char* name = "EBID16";
    static constexpr const char* peripheral_type = "EBI";
};

// PC17: EBID17 (Peripheral A)
template<>
struct PinFunction<pins::PC17, PeripheralFunction::A> {
    static constexpr const char* name = "EBID17";
    static constexpr const char* peripheral_type = "EBI";
};

// PC18: EBID18 (Peripheral A)
template<>
struct PinFunction<pins::PC18, PeripheralFunction::A> {
    static constexpr const char* name = "EBID18";
    static constexpr const char* peripheral_type = "EBI";
};

// PC19: EBID19 (Peripheral A)
template<>
struct PinFunction<pins::PC19, PeripheralFunction::A> {
    static constexpr const char* name = "EBID19";
    static constexpr const char* peripheral_type = "EBI";
};

// PC2: EBIA12 (Peripheral A)
template<>
struct PinFunction<pins::PC2, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA12";
    static constexpr const char* peripheral_type = "EBI";
};

// PC20: EBID20 (Peripheral A)
template<>
struct PinFunction<pins::PC20, PeripheralFunction::A> {
    static constexpr const char* name = "EBID20";
    static constexpr const char* peripheral_type = "EBI";
};

// PC21: EBID21 (Peripheral A)
template<>
struct PinFunction<pins::PC21, PeripheralFunction::A> {
    static constexpr const char* name = "EBID21";
    static constexpr const char* peripheral_type = "EBI";
};

// PC22: EBID22 (Peripheral A)
template<>
struct PinFunction<pins::PC22, PeripheralFunction::A> {
    static constexpr const char* name = "EBID22";
    static constexpr const char* peripheral_type = "EBI";
};

// PC23: EBID23 (Peripheral A)
template<>
struct PinFunction<pins::PC23, PeripheralFunction::A> {
    static constexpr const char* name = "EBID23";
    static constexpr const char* peripheral_type = "EBI";
};

// PC24: EBID24 (Peripheral A)
template<>
struct PinFunction<pins::PC24, PeripheralFunction::A> {
    static constexpr const char* name = "EBID24";
    static constexpr const char* peripheral_type = "EBI";
};

// PC25: EBID25 (Peripheral A)
template<>
struct PinFunction<pins::PC25, PeripheralFunction::A> {
    static constexpr const char* name = "EBID25";
    static constexpr const char* peripheral_type = "EBI";
};

// PC26: EBID26 (Peripheral A)
template<>
struct PinFunction<pins::PC26, PeripheralFunction::A> {
    static constexpr const char* name = "EBID26";
    static constexpr const char* peripheral_type = "EBI";
};

// PC27: EBID27 (Peripheral A)
template<>
struct PinFunction<pins::PC27, PeripheralFunction::A> {
    static constexpr const char* name = "EBID27";
    static constexpr const char* peripheral_type = "EBI";
};

// PC28: EBID28 (Peripheral A)
template<>
struct PinFunction<pins::PC28, PeripheralFunction::A> {
    static constexpr const char* name = "EBID28";
    static constexpr const char* peripheral_type = "EBI";
};

// PC29: EBID29 (Peripheral A)
template<>
struct PinFunction<pins::PC29, PeripheralFunction::A> {
    static constexpr const char* name = "EBID29";
    static constexpr const char* peripheral_type = "EBI";
};

// PC3: EBIA13 (Peripheral A)
template<>
struct PinFunction<pins::PC3, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA13";
    static constexpr const char* peripheral_type = "EBI";
};

// PC30: EBID30 (Peripheral A)
template<>
struct PinFunction<pins::PC30, PeripheralFunction::A> {
    static constexpr const char* name = "EBID30";
    static constexpr const char* peripheral_type = "EBI";
};

// PC31: EBID31 (Peripheral A)
template<>
struct PinFunction<pins::PC31, PeripheralFunction::A> {
    static constexpr const char* name = "EBID31";
    static constexpr const char* peripheral_type = "EBI";
};

// PC4: EBIA14 (Peripheral A)
template<>
struct PinFunction<pins::PC4, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA14";
    static constexpr const char* peripheral_type = "EBI";
};

// PC5: EBIA15 (Peripheral A)
template<>
struct PinFunction<pins::PC5, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA15";
    static constexpr const char* peripheral_type = "EBI";
};

// PC6: EBIA16 (Peripheral A)
template<>
struct PinFunction<pins::PC6, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA16";
    static constexpr const char* peripheral_type = "EBI";
};

// PC7: EBIA17 (Peripheral A)
template<>
struct PinFunction<pins::PC7, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA17";
    static constexpr const char* peripheral_type = "EBI";
};

// PC8: EBIA18 (Peripheral A)
template<>
struct PinFunction<pins::PC8, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA18";
    static constexpr const char* peripheral_type = "EBI";
};

// PC9: EBIA19 (Peripheral A)
template<>
struct PinFunction<pins::PC9, PeripheralFunction::A> {
    static constexpr const char* name = "EBIA19";
    static constexpr const char* peripheral_type = "EBI";
};

// ============================================================================
// GMAC Peripheral Functions
// ============================================================================

// PD0: GTSUCOMP (Peripheral A)
template<>
struct PinFunction<pins::PD0, PeripheralFunction::A> {
    static constexpr const char* name = "GTSUCOMP";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD1: GRXCK (Peripheral A)
template<>
struct PinFunction<pins::PD1, PeripheralFunction::A> {
    static constexpr const char* name = "GRXCK";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD10: GTX0 (Peripheral A)
template<>
struct PinFunction<pins::PD10, PeripheralFunction::A> {
    static constexpr const char* name = "GTX0";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD11: GTX1 (Peripheral A)
template<>
struct PinFunction<pins::PD11, PeripheralFunction::A> {
    static constexpr const char* name = "GTX1";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD12: GTXER (Peripheral A)
template<>
struct PinFunction<pins::PD12, PeripheralFunction::A> {
    static constexpr const char* name = "GTXER";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD13: GCOL (Peripheral A)
template<>
struct PinFunction<pins::PD13, PeripheralFunction::A> {
    static constexpr const char* name = "GCOL";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD14: GRXCK (Peripheral A)
template<>
struct PinFunction<pins::PD14, PeripheralFunction::A> {
    static constexpr const char* name = "GRXCK";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD15: GTX2 (Peripheral A)
template<>
struct PinFunction<pins::PD15, PeripheralFunction::A> {
    static constexpr const char* name = "GTX2";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD16: GTX3 (Peripheral A)
template<>
struct PinFunction<pins::PD16, PeripheralFunction::A> {
    static constexpr const char* name = "GTX3";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD17: GRX2 (Peripheral A)
template<>
struct PinFunction<pins::PD17, PeripheralFunction::A> {
    static constexpr const char* name = "GRX2";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD18: GRX3 (Peripheral A)
template<>
struct PinFunction<pins::PD18, PeripheralFunction::A> {
    static constexpr const char* name = "GRX3";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD19: GCRS (Peripheral A)
template<>
struct PinFunction<pins::PD19, PeripheralFunction::A> {
    static constexpr const char* name = "GCRS";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD2: GTXCK (Peripheral A)
template<>
struct PinFunction<pins::PD2, PeripheralFunction::A> {
    static constexpr const char* name = "GTXCK";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD3: GTXEN (Peripheral A)
template<>
struct PinFunction<pins::PD3, PeripheralFunction::A> {
    static constexpr const char* name = "GTXEN";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD4: GRXDV (Peripheral A)
template<>
struct PinFunction<pins::PD4, PeripheralFunction::A> {
    static constexpr const char* name = "GRXDV";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD5: GRX0 (Peripheral A)
template<>
struct PinFunction<pins::PD5, PeripheralFunction::A> {
    static constexpr const char* name = "GRX0";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD6: GRX1 (Peripheral A)
template<>
struct PinFunction<pins::PD6, PeripheralFunction::A> {
    static constexpr const char* name = "GRX1";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD7: GRXER (Peripheral A)
template<>
struct PinFunction<pins::PD7, PeripheralFunction::A> {
    static constexpr const char* name = "GRXER";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD8: GMDC (Peripheral A)
template<>
struct PinFunction<pins::PD8, PeripheralFunction::A> {
    static constexpr const char* name = "GMDC";
    static constexpr const char* peripheral_type = "GMAC";
};

// PD9: GMDIO (Peripheral A)
template<>
struct PinFunction<pins::PD9, PeripheralFunction::A> {
    static constexpr const char* name = "GMDIO";
    static constexpr const char* peripheral_type = "GMAC";
};

// ============================================================================
// HSMCI Peripheral Functions
// ============================================================================

// PA12: MCCI (Peripheral A)
template<>
struct PinFunction<pins::PA12, PeripheralFunction::A> {
    static constexpr const char* name = "MCCI";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA13: MCCK (Peripheral A)
template<>
struct PinFunction<pins::PA13, PeripheralFunction::A> {
    static constexpr const char* name = "MCCK";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA14: MCDA0 (Peripheral A)
template<>
struct PinFunction<pins::PA14, PeripheralFunction::A> {
    static constexpr const char* name = "MCDA0";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA15: MCDA1 (Peripheral A)
template<>
struct PinFunction<pins::PA15, PeripheralFunction::A> {
    static constexpr const char* name = "MCDA1";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA16: MCDA2 (Peripheral A)
template<>
struct PinFunction<pins::PA16, PeripheralFunction::A> {
    static constexpr const char* name = "MCDA2";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA17: MCDA3 (Peripheral A)
template<>
struct PinFunction<pins::PA17, PeripheralFunction::A> {
    static constexpr const char* name = "MCDA3";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA19: MCDA1 (Peripheral A)
template<>
struct PinFunction<pins::PA19, PeripheralFunction::A> {
    static constexpr const char* name = "MCDA1";
    static constexpr const char* peripheral_type = "HSMCI";
};

// PA20: MCDA0 (Peripheral A)
template<>
struct PinFunction<pins::PA20, PeripheralFunction::A> {
    static constexpr const char* name = "MCDA0";
    static constexpr const char* peripheral_type = "HSMCI";
};

// ============================================================================
// ISI Peripheral Functions
// ============================================================================

// PA10: ISI_D4 (Peripheral B)
template<>
struct PinFunction<pins::PA10, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D4";
    static constexpr const char* peripheral_type = "ISI";
};

// PA18: ISI_D6 (Peripheral B)
template<>
struct PinFunction<pins::PA18, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D6";
    static constexpr const char* peripheral_type = "ISI";
};

// PA19: ISI_D7 (Peripheral B)
template<>
struct PinFunction<pins::PA19, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D7";
    static constexpr const char* peripheral_type = "ISI";
};

// PA20: ISI_D8 (Peripheral B)
template<>
struct PinFunction<pins::PA20, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D8";
    static constexpr const char* peripheral_type = "ISI";
};

// PA9: ISI_D3 (Peripheral B)
template<>
struct PinFunction<pins::PA9, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D3";
    static constexpr const char* peripheral_type = "ISI";
};

// PB10: ISI_D1 (Peripheral B)
template<>
struct PinFunction<pins::PB10, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D1";
    static constexpr const char* peripheral_type = "ISI";
};

// PB11: ISI_D2 (Peripheral B)
template<>
struct PinFunction<pins::PB11, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D2";
    static constexpr const char* peripheral_type = "ISI";
};

// PB12: ISI_D5 (Peripheral B)
template<>
struct PinFunction<pins::PB12, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D5";
    static constexpr const char* peripheral_type = "ISI";
};

// PB6: ISI_VSYNC (Peripheral B)
template<>
struct PinFunction<pins::PB6, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_VSYNC";
    static constexpr const char* peripheral_type = "ISI";
};

// PB7: ISI_HSYNC (Peripheral B)
template<>
struct PinFunction<pins::PB7, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_HSYNC";
    static constexpr const char* peripheral_type = "ISI";
};

// PB8: ISI_PCK (Peripheral B)
template<>
struct PinFunction<pins::PB8, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_PCK";
    static constexpr const char* peripheral_type = "ISI";
};

// PB9: ISI_D0 (Peripheral B)
template<>
struct PinFunction<pins::PB9, PeripheralFunction::B> {
    static constexpr const char* name = "ISI_D0";
    static constexpr const char* peripheral_type = "ISI";
};

// ============================================================================
// LCD Peripheral Functions
// ============================================================================

// PA3: LCDDAT3 (Peripheral C)
template<>
struct PinFunction<pins::PA3, PeripheralFunction::C> {
    static constexpr const char* name = "LCDDAT3";
    static constexpr const char* peripheral_type = "LCD";
};

// PE0: LCDDAT0 (Peripheral A)
template<>
struct PinFunction<pins::PE0, PeripheralFunction::A> {
    static constexpr const char* name = "LCDDAT0";
    static constexpr const char* peripheral_type = "LCD";
};

// PE1: LCDDAT1 (Peripheral A)
template<>
struct PinFunction<pins::PE1, PeripheralFunction::A> {
    static constexpr const char* name = "LCDDAT1";
    static constexpr const char* peripheral_type = "LCD";
};

// PE2: LCDDAT2 (Peripheral A)
template<>
struct PinFunction<pins::PE2, PeripheralFunction::A> {
    static constexpr const char* name = "LCDDAT2";
    static constexpr const char* peripheral_type = "LCD";
};

// PE3: LCDDAT3 (Peripheral A)
template<>
struct PinFunction<pins::PE3, PeripheralFunction::A> {
    static constexpr const char* name = "LCDDAT3";
    static constexpr const char* peripheral_type = "LCD";
};

// PE4: LCDDAT4 (Peripheral A)
template<>
struct PinFunction<pins::PE4, PeripheralFunction::A> {
    static constexpr const char* name = "LCDDAT4";
    static constexpr const char* peripheral_type = "LCD";
};

// PE5: LCDDAT5 (Peripheral A)
template<>
struct PinFunction<pins::PE5, PeripheralFunction::A> {
    static constexpr const char* name = "LCDDAT5";
    static constexpr const char* peripheral_type = "LCD";
};

// ============================================================================
// PMC Peripheral Functions
// ============================================================================

// PA18: PCK0 (Peripheral A)
template<>
struct PinFunction<pins::PA18, PeripheralFunction::A> {
    static constexpr const char* name = "PCK0";
    static constexpr const char* peripheral_type = "PMC";
};

// PA21: PCK1 (Peripheral B)
template<>
struct PinFunction<pins::PA21, PeripheralFunction::B> {
    static constexpr const char* name = "PCK1";
    static constexpr const char* peripheral_type = "PMC";
};

// PB13: PCK0 (Peripheral B)
template<>
struct PinFunction<pins::PB13, PeripheralFunction::B> {
    static constexpr const char* name = "PCK0";
    static constexpr const char* peripheral_type = "PMC";
};

// PD27: PCK0 (Peripheral A)
template<>
struct PinFunction<pins::PD27, PeripheralFunction::A> {
    static constexpr const char* name = "PCK0";
    static constexpr const char* peripheral_type = "PMC";
};

// ============================================================================
// PWM Peripheral Functions
// ============================================================================

// PA0: PWMC0_PWMH0 (Peripheral A)
template<>
struct PinFunction<pins::PA0, PeripheralFunction::A> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PA1: PWMC0_PWMH1 (Peripheral A)
template<>
struct PinFunction<pins::PA1, PeripheralFunction::A> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PA11: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PA11, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PA12: PWMC0_PWMH1 (Peripheral B)
template<>
struct PinFunction<pins::PA12, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PA2: PWMC0_PWMH2 (Peripheral A)
template<>
struct PinFunction<pins::PA2, PeripheralFunction::A> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PA30: PWMC0_PWMH2 (Peripheral A)
template<>
struct PinFunction<pins::PA30, PeripheralFunction::A> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PA7: PWMC0_PWMH3 (Peripheral A)
template<>
struct PinFunction<pins::PA7, PeripheralFunction::A> {
    static constexpr const char* name = "PWMC0_PWMH3";
    static constexpr const char* peripheral_type = "PWM";
};

// PA8: PWMC0_PWML3 (Peripheral A)
template<>
struct PinFunction<pins::PA8, PeripheralFunction::A> {
    static constexpr const char* name = "PWMC0_PWML3";
    static constexpr const char* peripheral_type = "PWM";
};

// PB0: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PB0, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PB1: PWMC0_PWMH1 (Peripheral B)
template<>
struct PinFunction<pins::PB1, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PB4: PWMC0_PWMH2 (Peripheral B)
template<>
struct PinFunction<pins::PB4, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PB5: PWMC0_PWML0 (Peripheral B)
template<>
struct PinFunction<pins::PB5, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML0";
    static constexpr const char* peripheral_type = "PWM";
};

// PC0: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PC0, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PC1: PWMC0_PWMH1 (Peripheral B)
template<>
struct PinFunction<pins::PC1, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PC10: PWMC0_PWML2 (Peripheral B)
template<>
struct PinFunction<pins::PC10, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML2";
    static constexpr const char* peripheral_type = "PWM";
};

// PC11: PWMC0_PWML3 (Peripheral B)
template<>
struct PinFunction<pins::PC11, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML3";
    static constexpr const char* peripheral_type = "PWM";
};

// PC12: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PC12, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PC13: PWMC0_PWMH1 (Peripheral B)
template<>
struct PinFunction<pins::PC13, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PC14: PWMC0_PWMH2 (Peripheral B)
template<>
struct PinFunction<pins::PC14, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PC15: PWMC0_PWMH3 (Peripheral B)
template<>
struct PinFunction<pins::PC15, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH3";
    static constexpr const char* peripheral_type = "PWM";
};

// PC2: PWMC0_PWMH2 (Peripheral B)
template<>
struct PinFunction<pins::PC2, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PC3: PWMC0_PWMH3 (Peripheral B)
template<>
struct PinFunction<pins::PC3, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH3";
    static constexpr const char* peripheral_type = "PWM";
};

// PC8: PWMC0_PWML0 (Peripheral B)
template<>
struct PinFunction<pins::PC8, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML0";
    static constexpr const char* peripheral_type = "PWM";
};

// PC9: PWMC0_PWML1 (Peripheral B)
template<>
struct PinFunction<pins::PC9, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML1";
    static constexpr const char* peripheral_type = "PWM";
};

// PD0: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PD0, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PD1: PWMC0_PWMH1 (Peripheral B)
template<>
struct PinFunction<pins::PD1, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PD10: PWMC0_PWMEXTRG0 (Peripheral B)
template<>
struct PinFunction<pins::PD10, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMEXTRG0";
    static constexpr const char* peripheral_type = "PWM";
};

// PD11: PWMC0_PWMEXTRG1 (Peripheral B)
template<>
struct PinFunction<pins::PD11, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMEXTRG1";
    static constexpr const char* peripheral_type = "PWM";
};

// PD12: PWMC0_PWMFI0 (Peripheral B)
template<>
struct PinFunction<pins::PD12, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMFI0";
    static constexpr const char* peripheral_type = "PWM";
};

// PD13: PWMC0_PWMFI1 (Peripheral B)
template<>
struct PinFunction<pins::PD13, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMFI1";
    static constexpr const char* peripheral_type = "PWM";
};

// PD14: PWMC0_PWMFI2 (Peripheral B)
template<>
struct PinFunction<pins::PD14, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMFI2";
    static constexpr const char* peripheral_type = "PWM";
};

// PD2: PWMC0_PWMH2 (Peripheral B)
template<>
struct PinFunction<pins::PD2, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PD24: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PD24, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PD3: PWMC0_PWMH3 (Peripheral B)
template<>
struct PinFunction<pins::PD3, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH3";
    static constexpr const char* peripheral_type = "PWM";
};

// PD4: PWMC0_PWML0 (Peripheral B)
template<>
struct PinFunction<pins::PD4, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML0";
    static constexpr const char* peripheral_type = "PWM";
};

// PD5: PWMC0_PWML1 (Peripheral B)
template<>
struct PinFunction<pins::PD5, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML1";
    static constexpr const char* peripheral_type = "PWM";
};

// PD6: PWMC0_PWML2 (Peripheral B)
template<>
struct PinFunction<pins::PD6, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML2";
    static constexpr const char* peripheral_type = "PWM";
};

// PD7: PWMC0_PWML3 (Peripheral B)
template<>
struct PinFunction<pins::PD7, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML3";
    static constexpr const char* peripheral_type = "PWM";
};

// PD8: PWMC0_PWMFI0 (Peripheral B)
template<>
struct PinFunction<pins::PD8, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMFI0";
    static constexpr const char* peripheral_type = "PWM";
};

// PD9: PWMC0_PWMFI1 (Peripheral B)
template<>
struct PinFunction<pins::PD9, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMFI1";
    static constexpr const char* peripheral_type = "PWM";
};

// PE0: PWMC0_PWMH0 (Peripheral B)
template<>
struct PinFunction<pins::PE0, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH0";
    static constexpr const char* peripheral_type = "PWM";
};

// PE1: PWMC0_PWMH1 (Peripheral B)
template<>
struct PinFunction<pins::PE1, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH1";
    static constexpr const char* peripheral_type = "PWM";
};

// PE2: PWMC0_PWMH2 (Peripheral B)
template<>
struct PinFunction<pins::PE2, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH2";
    static constexpr const char* peripheral_type = "PWM";
};

// PE3: PWMC0_PWMH3 (Peripheral B)
template<>
struct PinFunction<pins::PE3, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWMH3";
    static constexpr const char* peripheral_type = "PWM";
};

// PE4: PWMC0_PWML0 (Peripheral B)
template<>
struct PinFunction<pins::PE4, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML0";
    static constexpr const char* peripheral_type = "PWM";
};

// PE5: PWMC0_PWML1 (Peripheral B)
template<>
struct PinFunction<pins::PE5, PeripheralFunction::B> {
    static constexpr const char* name = "PWMC0_PWML1";
    static constexpr const char* peripheral_type = "PWM";
};

// ============================================================================
// QSPI Peripheral Functions
// ============================================================================

// PA11: QSP_I_CS (Peripheral A)
template<>
struct PinFunction<pins::PA11, PeripheralFunction::A> {
    static constexpr const char* name = "QSP_I_CS";
    static constexpr const char* peripheral_type = "QSPI";
};

// PC20: QSPI_IO3 (Peripheral C)
template<>
struct PinFunction<pins::PC20, PeripheralFunction::C> {
    static constexpr const char* name = "QSPI_IO3";
    static constexpr const char* peripheral_type = "QSPI";
};

// PC21: QSPI_IO2 (Peripheral C)
template<>
struct PinFunction<pins::PC21, PeripheralFunction::C> {
    static constexpr const char* name = "QSPI_IO2";
    static constexpr const char* peripheral_type = "QSPI";
};

// PC22: QSPI_IO1 (Peripheral C)
template<>
struct PinFunction<pins::PC22, PeripheralFunction::C> {
    static constexpr const char* name = "QSPI_IO1";
    static constexpr const char* peripheral_type = "QSPI";
};

// PC23: QSPI_IO0 (Peripheral C)
template<>
struct PinFunction<pins::PC23, PeripheralFunction::C> {
    static constexpr const char* name = "QSPI_IO0";
    static constexpr const char* peripheral_type = "QSPI";
};

// PC24: QSPI_SCK (Peripheral C)
template<>
struct PinFunction<pins::PC24, PeripheralFunction::C> {
    static constexpr const char* name = "QSPI_SCK";
    static constexpr const char* peripheral_type = "QSPI";
};

// PC25: QSPI_CS (Peripheral C)
template<>
struct PinFunction<pins::PC25, PeripheralFunction::C> {
    static constexpr const char* name = "QSPI_CS";
    static constexpr const char* peripheral_type = "QSPI";
};

// ============================================================================
// SPI Peripheral Functions
// ============================================================================

// PA22: SPI0_SPCK (Peripheral C)
template<>
struct PinFunction<pins::PA22, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_SPCK";
    static constexpr const char* peripheral_type = "SPI";
};

// PA23: SPI0_MOSI (Peripheral C)
template<>
struct PinFunction<pins::PA23, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_MOSI";
    static constexpr const char* peripheral_type = "SPI";
};

// PA24: SPI0_MISO (Peripheral C)
template<>
struct PinFunction<pins::PA24, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_MISO";
    static constexpr const char* peripheral_type = "SPI";
};

// PA25: SPI0_NPCS0 (Peripheral C)
template<>
struct PinFunction<pins::PA25, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_NPCS0";
    static constexpr const char* peripheral_type = "SPI";
};

// PA30: SPI0_NPCS2 (Peripheral B)
template<>
struct PinFunction<pins::PA30, PeripheralFunction::B> {
    static constexpr const char* name = "SPI0_NPCS2";
    static constexpr const char* peripheral_type = "SPI";
};

// PA31: SPI0_NPCS3 (Peripheral A)
template<>
struct PinFunction<pins::PA31, PeripheralFunction::A> {
    static constexpr const char* name = "SPI0_NPCS3";
    static constexpr const char* peripheral_type = "SPI";
};

// PC16: SPI0_MISO (Peripheral C)
template<>
struct PinFunction<pins::PC16, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_MISO";
    static constexpr const char* peripheral_type = "SPI";
};

// PC17: SPI0_MOSI (Peripheral C)
template<>
struct PinFunction<pins::PC17, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_MOSI";
    static constexpr const char* peripheral_type = "SPI";
};

// PC18: SPI0_SPCK (Peripheral C)
template<>
struct PinFunction<pins::PC18, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_SPCK";
    static constexpr const char* peripheral_type = "SPI";
};

// PC19: SPI0_NPCS0 (Peripheral C)
template<>
struct PinFunction<pins::PC19, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_NPCS0";
    static constexpr const char* peripheral_type = "SPI";
};

// PC4: SPI0_NPCS1 (Peripheral C)
template<>
struct PinFunction<pins::PC4, PeripheralFunction::C> {
    static constexpr const char* name = "SPI0_NPCS1";
    static constexpr const char* peripheral_type = "SPI";
};

// PD28: SPI0_NPCS3 (Peripheral A)
template<>
struct PinFunction<pins::PD28, PeripheralFunction::A> {
    static constexpr const char* name = "SPI0_NPCS3";
    static constexpr const char* peripheral_type = "SPI";
};

// ============================================================================
// TC Peripheral Functions
// ============================================================================

// PA0: TC0_TIOA0 (Peripheral B)
template<>
struct PinFunction<pins::PA0, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOA0";
    static constexpr const char* peripheral_type = "TC";
};

// PA1: TC0_TIOB0 (Peripheral B)
template<>
struct PinFunction<pins::PA1, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOB0";
    static constexpr const char* peripheral_type = "TC";
};

// PA13: TC0_TCLK0 (Peripheral B)
template<>
struct PinFunction<pins::PA13, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TCLK0";
    static constexpr const char* peripheral_type = "TC";
};

// PA14: TC0_TIOA1 (Peripheral B)
template<>
struct PinFunction<pins::PA14, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOA1";
    static constexpr const char* peripheral_type = "TC";
};

// PA15: TC0_TIOB1 (Peripheral B)
template<>
struct PinFunction<pins::PA15, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOB1";
    static constexpr const char* peripheral_type = "TC";
};

// PA16: TC0_TIOA2 (Peripheral B)
template<>
struct PinFunction<pins::PA16, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOA2";
    static constexpr const char* peripheral_type = "TC";
};

// PA17: TC0_TIOB2 (Peripheral B)
template<>
struct PinFunction<pins::PA17, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOB2";
    static constexpr const char* peripheral_type = "TC";
};

// PA26: TC0_TIOA2 (Peripheral B)
template<>
struct PinFunction<pins::PA26, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOA2";
    static constexpr const char* peripheral_type = "TC";
};

// PA27: TC0_TIOB2 (Peripheral B)
template<>
struct PinFunction<pins::PA27, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOB2";
    static constexpr const char* peripheral_type = "TC";
};

// PA28: TC0_TCLK1 (Peripheral B)
template<>
struct PinFunction<pins::PA28, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TCLK1";
    static constexpr const char* peripheral_type = "TC";
};

// PA29: TC0_TCLK2 (Peripheral B)
template<>
struct PinFunction<pins::PA29, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TCLK2";
    static constexpr const char* peripheral_type = "TC";
};

// PA31: TC0_TIOA0 (Peripheral B)
template<>
struct PinFunction<pins::PA31, PeripheralFunction::B> {
    static constexpr const char* name = "TC0_TIOA0";
    static constexpr const char* peripheral_type = "TC";
};

// PA4: TCLK0 (Peripheral B)
template<>
struct PinFunction<pins::PA4, PeripheralFunction::B> {
    static constexpr const char* name = "TCLK0";
    static constexpr const char* peripheral_type = "TC";
};

// PC26: TC1_TIOA1 (Peripheral C)
template<>
struct PinFunction<pins::PC26, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TIOA1";
    static constexpr const char* peripheral_type = "TC";
};

// PC27: TC1_TIOB1 (Peripheral C)
template<>
struct PinFunction<pins::PC27, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TIOB1";
    static constexpr const char* peripheral_type = "TC";
};

// PC28: TC1_TCLK1 (Peripheral C)
template<>
struct PinFunction<pins::PC28, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TCLK1";
    static constexpr const char* peripheral_type = "TC";
};

// PC29: TC1_TIOA2 (Peripheral C)
template<>
struct PinFunction<pins::PC29, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TIOA2";
    static constexpr const char* peripheral_type = "TC";
};

// PC30: TC1_TIOB2 (Peripheral C)
template<>
struct PinFunction<pins::PC30, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TIOB2";
    static constexpr const char* peripheral_type = "TC";
};

// PC31: TC1_TCLK2 (Peripheral C)
template<>
struct PinFunction<pins::PC31, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TCLK2";
    static constexpr const char* peripheral_type = "TC";
};

// PC5: TC1_TIOA0 (Peripheral C)
template<>
struct PinFunction<pins::PC5, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TIOA0";
    static constexpr const char* peripheral_type = "TC";
};

// PC6: TC1_TIOB0 (Peripheral C)
template<>
struct PinFunction<pins::PC6, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TIOB0";
    static constexpr const char* peripheral_type = "TC";
};

// PC7: TC1_TCLK0 (Peripheral C)
template<>
struct PinFunction<pins::PC7, PeripheralFunction::C> {
    static constexpr const char* name = "TC1_TCLK0";
    static constexpr const char* peripheral_type = "TC";
};

// PD15: TC2_TIOA0 (Peripheral C)
template<>
struct PinFunction<pins::PD15, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TIOA0";
    static constexpr const char* peripheral_type = "TC";
};

// PD16: TC2_TIOB0 (Peripheral C)
template<>
struct PinFunction<pins::PD16, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TIOB0";
    static constexpr const char* peripheral_type = "TC";
};

// PD17: TC2_TCLK0 (Peripheral C)
template<>
struct PinFunction<pins::PD17, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TCLK0";
    static constexpr const char* peripheral_type = "TC";
};

// PD18: TC2_TIOA1 (Peripheral C)
template<>
struct PinFunction<pins::PD18, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TIOA1";
    static constexpr const char* peripheral_type = "TC";
};

// PD19: TC2_TIOB1 (Peripheral C)
template<>
struct PinFunction<pins::PD19, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TIOB1";
    static constexpr const char* peripheral_type = "TC";
};

// PD20: TC2_TCLK1 (Peripheral C)
template<>
struct PinFunction<pins::PD20, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TCLK1";
    static constexpr const char* peripheral_type = "TC";
};

// PD21: TC2_TIOA2 (Peripheral C)
template<>
struct PinFunction<pins::PD21, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TIOA2";
    static constexpr const char* peripheral_type = "TC";
};

// PD22: TC2_TIOB2 (Peripheral C)
template<>
struct PinFunction<pins::PD22, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TIOB2";
    static constexpr const char* peripheral_type = "TC";
};

// PD23: TC2_TCLK2 (Peripheral C)
template<>
struct PinFunction<pins::PD23, PeripheralFunction::C> {
    static constexpr const char* name = "TC2_TCLK2";
    static constexpr const char* peripheral_type = "TC";
};

// PD25: TC0_TIOA0 (Peripheral C)
template<>
struct PinFunction<pins::PD25, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TIOA0";
    static constexpr const char* peripheral_type = "TC";
};

// PD26: TC0_TIOB0 (Peripheral C)
template<>
struct PinFunction<pins::PD26, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TIOB0";
    static constexpr const char* peripheral_type = "TC";
};

// PD27: TC0_TCLK0 (Peripheral C)
template<>
struct PinFunction<pins::PD27, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TCLK0";
    static constexpr const char* peripheral_type = "TC";
};

// PD28: TC0_TIOA1 (Peripheral C)
template<>
struct PinFunction<pins::PD28, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TIOA1";
    static constexpr const char* peripheral_type = "TC";
};

// PD29: TC0_TIOB1 (Peripheral C)
template<>
struct PinFunction<pins::PD29, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TIOB1";
    static constexpr const char* peripheral_type = "TC";
};

// PD30: TC0_TCLK1 (Peripheral C)
template<>
struct PinFunction<pins::PD30, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TCLK1";
    static constexpr const char* peripheral_type = "TC";
};

// PD31: TC0_TIOA2 (Peripheral C)
template<>
struct PinFunction<pins::PD31, PeripheralFunction::C> {
    static constexpr const char* name = "TC0_TIOA2";
    static constexpr const char* peripheral_type = "TC";
};

// ============================================================================
// TWI Peripheral Functions
// ============================================================================

// PA3: TWD0 (Peripheral A)
template<>
struct PinFunction<pins::PA3, PeripheralFunction::A> {
    static constexpr const char* name = "TWD0";
    static constexpr const char* peripheral_type = "TWI";
};

// PA4: TWCK0 (Peripheral A)
template<>
struct PinFunction<pins::PA4, PeripheralFunction::A> {
    static constexpr const char* name = "TWCK0";
    static constexpr const char* peripheral_type = "TWI";
};

// PB12: TWD0 (Peripheral A)
template<>
struct PinFunction<pins::PB12, PeripheralFunction::A> {
    static constexpr const char* name = "TWD0";
    static constexpr const char* peripheral_type = "TWI";
};

// PB13: TWCK0 (Peripheral A)
template<>
struct PinFunction<pins::PB13, PeripheralFunction::A> {
    static constexpr const char* name = "TWCK0";
    static constexpr const char* peripheral_type = "TWI";
};

// PB2: TWD1 (Peripheral B)
template<>
struct PinFunction<pins::PB2, PeripheralFunction::B> {
    static constexpr const char* name = "TWD1";
    static constexpr const char* peripheral_type = "TWI";
};

// PB3: TWCK1 (Peripheral B)
template<>
struct PinFunction<pins::PB3, PeripheralFunction::B> {
    static constexpr const char* name = "TWCK1";
    static constexpr const char* peripheral_type = "TWI";
};

// PB4: TWD1 (Peripheral A)
template<>
struct PinFunction<pins::PB4, PeripheralFunction::A> {
    static constexpr const char* name = "TWD1";
    static constexpr const char* peripheral_type = "TWI";
};

// PB5: TWCK1 (Peripheral A)
template<>
struct PinFunction<pins::PB5, PeripheralFunction::A> {
    static constexpr const char* name = "TWCK1";
    static constexpr const char* peripheral_type = "TWI";
};

// ============================================================================
// UART Peripheral Functions
// ============================================================================

// PA5: RXDA (Peripheral C)
template<>
struct PinFunction<pins::PA5, PeripheralFunction::C> {
    static constexpr const char* name = "RXDA";
    static constexpr const char* peripheral_type = "UART";
};

// PA6: TXDA (Peripheral C)
template<>
struct PinFunction<pins::PA6, PeripheralFunction::C> {
    static constexpr const char* name = "TXDA";
    static constexpr const char* peripheral_type = "UART";
};

// ============================================================================
// USART Peripheral Functions
// ============================================================================

// PA10: UTXD0 (Peripheral A)
template<>
struct PinFunction<pins::PA10, PeripheralFunction::A> {
    static constexpr const char* name = "UTXD0";
    static constexpr const char* peripheral_type = "USART";
};

// PA21: RXD1 (Peripheral A)
template<>
struct PinFunction<pins::PA21, PeripheralFunction::A> {
    static constexpr const char* name = "RXD1";
    static constexpr const char* peripheral_type = "USART";
};

// PA22: TXD1 (Peripheral A)
template<>
struct PinFunction<pins::PA22, PeripheralFunction::A> {
    static constexpr const char* name = "TXD1";
    static constexpr const char* peripheral_type = "USART";
};

// PA23: SCK1 (Peripheral A)
template<>
struct PinFunction<pins::PA23, PeripheralFunction::A> {
    static constexpr const char* name = "SCK1";
    static constexpr const char* peripheral_type = "USART";
};

// PA24: RTS1 (Peripheral A)
template<>
struct PinFunction<pins::PA24, PeripheralFunction::A> {
    static constexpr const char* name = "RTS1";
    static constexpr const char* peripheral_type = "USART";
};

// PA25: CTS1 (Peripheral A)
template<>
struct PinFunction<pins::PA25, PeripheralFunction::A> {
    static constexpr const char* name = "CTS1";
    static constexpr const char* peripheral_type = "USART";
};

// PA26: DCD1 (Peripheral A)
template<>
struct PinFunction<pins::PA26, PeripheralFunction::A> {
    static constexpr const char* name = "DCD1";
    static constexpr const char* peripheral_type = "USART";
};

// PA27: DTR1 (Peripheral A)
template<>
struct PinFunction<pins::PA27, PeripheralFunction::A> {
    static constexpr const char* name = "DTR1";
    static constexpr const char* peripheral_type = "USART";
};

// PA28: DSR1 (Peripheral A)
template<>
struct PinFunction<pins::PA28, PeripheralFunction::A> {
    static constexpr const char* name = "DSR1";
    static constexpr const char* peripheral_type = "USART";
};

// PA29: RI1 (Peripheral A)
template<>
struct PinFunction<pins::PA29, PeripheralFunction::A> {
    static constexpr const char* name = "RI1";
    static constexpr const char* peripheral_type = "USART";
};

// PA5: URXD0 (Peripheral A)
template<>
struct PinFunction<pins::PA5, PeripheralFunction::A> {
    static constexpr const char* name = "URXD0";
    static constexpr const char* peripheral_type = "USART";
};

// PA6: UTXD0 (Peripheral A)
template<>
struct PinFunction<pins::PA6, PeripheralFunction::A> {
    static constexpr const char* name = "UTXD0";
    static constexpr const char* peripheral_type = "USART";
};

// PA7: RTS0 (Peripheral C)
template<>
struct PinFunction<pins::PA7, PeripheralFunction::C> {
    static constexpr const char* name = "RTS0";
    static constexpr const char* peripheral_type = "USART";
};

// PA9: URXD0 (Peripheral A)
template<>
struct PinFunction<pins::PA9, PeripheralFunction::A> {
    static constexpr const char* name = "URXD0";
    static constexpr const char* peripheral_type = "USART";
};

// PB10: TXD3 (Peripheral A)
template<>
struct PinFunction<pins::PB10, PeripheralFunction::A> {
    static constexpr const char* name = "TXD3";
    static constexpr const char* peripheral_type = "USART";
};

// PB11: RXD3 (Peripheral A)
template<>
struct PinFunction<pins::PB11, PeripheralFunction::A> {
    static constexpr const char* name = "RXD3";
    static constexpr const char* peripheral_type = "USART";
};

// PB6: TXD1 (Peripheral A)
template<>
struct PinFunction<pins::PB6, PeripheralFunction::A> {
    static constexpr const char* name = "TXD1";
    static constexpr const char* peripheral_type = "USART";
};

// PB7: RXD1 (Peripheral A)
template<>
struct PinFunction<pins::PB7, PeripheralFunction::A> {
    static constexpr const char* name = "RXD1";
    static constexpr const char* peripheral_type = "USART";
};

// PB8: TXD2 (Peripheral A)
template<>
struct PinFunction<pins::PB8, PeripheralFunction::A> {
    static constexpr const char* name = "TXD2";
    static constexpr const char* peripheral_type = "USART";
};

// PB9: RXD2 (Peripheral A)
template<>
struct PinFunction<pins::PB9, PeripheralFunction::A> {
    static constexpr const char* name = "RXD2";
    static constexpr const char* peripheral_type = "USART";
};

// PD20: URXD2 (Peripheral A)
template<>
struct PinFunction<pins::PD20, PeripheralFunction::A> {
    static constexpr const char* name = "URXD2";
    static constexpr const char* peripheral_type = "USART";
};

// PD21: UTXD2 (Peripheral A)
template<>
struct PinFunction<pins::PD21, PeripheralFunction::A> {
    static constexpr const char* name = "UTXD2";
    static constexpr const char* peripheral_type = "USART";
};

// PD22: SCK2 (Peripheral A)
template<>
struct PinFunction<pins::PD22, PeripheralFunction::A> {
    static constexpr const char* name = "SCK2";
    static constexpr const char* peripheral_type = "USART";
};

// PD23: RTS2 (Peripheral A)
template<>
struct PinFunction<pins::PD23, PeripheralFunction::A> {
    static constexpr const char* name = "RTS2";
    static constexpr const char* peripheral_type = "USART";
};

// PD24: CTS2 (Peripheral A)
template<>
struct PinFunction<pins::PD24, PeripheralFunction::A> {
    static constexpr const char* name = "CTS2";
    static constexpr const char* peripheral_type = "USART";
};

// PD25: RXD2 (Peripheral A)
template<>
struct PinFunction<pins::PD25, PeripheralFunction::A> {
    static constexpr const char* name = "RXD2";
    static constexpr const char* peripheral_type = "USART";
};

// PD26: TXD2 (Peripheral A)
template<>
struct PinFunction<pins::PD26, PeripheralFunction::A> {
    static constexpr const char* name = "TXD2";
    static constexpr const char* peripheral_type = "USART";
};

// PD29: TXD3 (Peripheral A)
template<>
struct PinFunction<pins::PD29, PeripheralFunction::A> {
    static constexpr const char* name = "TXD3";
    static constexpr const char* peripheral_type = "USART";
};

// PD30: RXD3 (Peripheral A)
template<>
struct PinFunction<pins::PD30, PeripheralFunction::A> {
    static constexpr const char* name = "RXD3";
    static constexpr const char* peripheral_type = "USART";
};

// PD31: SCK3 (Peripheral A)
template<>
struct PinFunction<pins::PD31, PeripheralFunction::A> {
    static constexpr const char* name = "SCK3";
    static constexpr const char* peripheral_type = "USART";
};


}  // namespace alloy::hal::atmel::same70::atsame70q21::pin_functions
