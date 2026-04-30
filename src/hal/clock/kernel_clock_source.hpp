#pragma once

// kernel_clock_source.hpp — Task 3.2 (add-clock-management-hal)
//
// KernelClockSource: vendor-neutral kernel clock MUX values.
// Vendors map these enum values to their peripheral-clock-selector bitfield values.

#include <cstdint>

namespace alloy::hal::clock {

/// Vendor-neutral kernel clock source selector.
/// Maps to the RCC/PMC/OSCCTRL peripheral-clock-selector (e.g. RCC_CCIPR USART*SEL).
enum class KernelClockSource : std::uint8_t {
    /// Peripheral bus clock (APB1/APB2/PCLK — default after reset).
    pclk = 0,
    /// Internal high-speed oscillator (HSI16, HSI48, HSI, etc.).
    hsi16 = 1,
    /// Low-speed external oscillator (LSE, 32.768 kHz).
    lse = 2,
    /// System clock (SYSCLK).
    sysclk = 3,
    /// PLL2-Q output (STM32H7 / STM32U5).
    pll2_q = 4,
    /// External high-speed oscillator (HSE).
    hse = 5,
    /// USB PHY clock (48 MHz via CRS-tuned HSI48 or PLL).
    hsi48 = 6,
    /// Main PLL-Q output (PLLQ, for SDMMC / USB on STM32F4/G4).
    pll_q = 7,
};

}  // namespace alloy::hal::clock
