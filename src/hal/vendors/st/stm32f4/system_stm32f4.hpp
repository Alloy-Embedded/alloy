// Alloy Framework - STM32F4 System Initialization
//
// Provides system initialization for STM32F4 family (Cortex-M4F)
//
// Features:
// - Cortex-M4F core initialization (FPU enabled)
// - Clock configuration integration
// - Flash latency configuration
// - Voltage scaling configuration (PWR)
//
// Usage in board startup.cpp:
//   #include "hal/vendors/st/stm32f4/system_stm32f4.hpp"
//
//   void SystemInit() {
//       alloy::hal::st::stm32f4::system_init();
//   }

#pragma once

#include "../../../../startup/arm_cortex_m4/cortex_m4_init.hpp"
#include "clock.hpp"
#include <cstdint>

namespace alloy::hal::st::stm32f4 {

// Flash latency settings based on SYSCLK frequency and voltage range
// From STM32F4 reference manual (VOS Scale 1, 2.7-3.6V):
// 0 wait states: 0 < SYSCLK <= 30 MHz
// 1 wait state:  30 MHz < SYSCLK <= 60 MHz
// 2 wait states: 60 MHz < SYSCLK <= 90 MHz
// 3 wait states: 90 MHz < SYSCLK <= 120 MHz
// 4 wait states: 120 MHz < SYSCLK <= 150 MHz
// 5 wait states: 150 MHz < SYSCLK <= 168 MHz

namespace flash {
    constexpr uint32_t BASE = 0x40023C00;  // FLASH base address

    struct Registers {
        volatile uint32_t ACR;       // Flash access control register
        volatile uint32_t KEYR;      // Flash key register
        volatile uint32_t OPTKEYR;   // Option byte key register
        volatile uint32_t SR;        // Status register
        volatile uint32_t CR;        // Control register
        volatile uint32_t OPTCR;     // Option control register
        volatile uint32_t OPTCR1;    // Option control register 1
    };

    constexpr Registers* FLASH = reinterpret_cast<Registers*>(BASE);

    // ACR (Access Control Register) bit definitions
    namespace acr {
        constexpr uint32_t LATENCY_Pos = 0;
        constexpr uint32_t LATENCY_Msk = (15UL << LATENCY_Pos);
        constexpr uint32_t PRFTEN      = (1UL << 8);   // Prefetch enable
        constexpr uint32_t ICEN        = (1UL << 9);   // Instruction cache enable
        constexpr uint32_t DCEN        = (1UL << 10);  // Data cache enable
        constexpr uint32_t ICRST       = (1UL << 11);  // Instruction cache reset
        constexpr uint32_t DCRST       = (1UL << 12);  // Data cache reset
    }

    /// Set Flash latency based on SYSCLK frequency
    /// @param sysclk_hz: System clock frequency in Hz
    inline void set_latency(uint32_t sysclk_hz) {
        uint32_t latency;

        if (sysclk_hz <= 30000000) {
            latency = 0;
        } else if (sysclk_hz <= 60000000) {
            latency = 1;
        } else if (sysclk_hz <= 90000000) {
            latency = 2;
        } else if (sysclk_hz <= 120000000) {
            latency = 3;
        } else if (sysclk_hz <= 150000000) {
            latency = 4;
        } else {
            latency = 5;  // Up to 168 MHz
        }

        // Set latency
        uint32_t acr = FLASH->ACR;
        acr &= ~acr::LATENCY_Msk;
        acr |= latency;
        FLASH->ACR = acr;

        // Wait for latency to be applied
        while ((FLASH->ACR & acr::LATENCY_Msk) != latency) {}
    }

    /// Enable Flash features (prefetch, instruction cache, data cache)
    inline void enable_features() {
        FLASH->ACR |= acr::PRFTEN | acr::ICEN | acr::DCEN;
    }
}

// Power Control (PWR) for voltage scaling
namespace pwr {
    constexpr uint32_t BASE = 0x40007000;  // PWR base address

    struct Registers {
        volatile uint32_t CR;   // Power control register
        volatile uint32_t CSR;  // Power control/status register
    };

    constexpr Registers* PWR = reinterpret_cast<Registers*>(BASE);

    // CR (Control Register) bit definitions
    namespace cr {
        constexpr uint32_t VOS_Pos = 14;
        constexpr uint32_t VOS_Msk = (3UL << VOS_Pos);
        constexpr uint32_t VOS_Scale3 = (1UL << VOS_Pos);  // Scale 3 mode (max 120 MHz)
        constexpr uint32_t VOS_Scale2 = (2UL << VOS_Pos);  // Scale 2 mode (max 144 MHz)
        constexpr uint32_t VOS_Scale1 = (3UL << VOS_Pos);  // Scale 1 mode (max 168 MHz)
    }

    // CSR (Control/Status Register) bit definitions
    namespace csr {
        constexpr uint32_t VOSRDY = (1UL << 14);  // Voltage scaling ready flag
    }

    /// Configure voltage scaling based on target frequency
    /// @param sysclk_hz: Target system clock frequency
    inline void configure_voltage_scaling(uint32_t sysclk_hz) {
        // Enable PWR clock (RCC_APB1ENR, bit 28)
        constexpr uint32_t RCC_APB1ENR = 0x40023840;
        constexpr uint32_t PWREN = (1UL << 28);
        *reinterpret_cast<volatile uint32_t*>(RCC_APB1ENR) |= PWREN;

        // Select voltage scale based on frequency
        uint32_t vos;
        if (sysclk_hz <= 120000000) {
            vos = cr::VOS_Scale3;  // Up to 120 MHz
        } else if (sysclk_hz <= 144000000) {
            vos = cr::VOS_Scale2;  // Up to 144 MHz
        } else {
            vos = cr::VOS_Scale1;  // Up to 168 MHz
        }

        // Configure voltage scaling
        uint32_t cr = PWR->CR;
        cr &= ~cr::VOS_Msk;
        cr |= vos;
        PWR->CR = cr;

        // Wait for voltage scaling to be ready
        while ((PWR->CSR & csr::VOSRDY) == 0) {}
    }
}

/// Initialize STM32F4 system
/// @param config: Clock configuration (optional)
///
/// This function:
/// 1. Initializes Cortex-M4F core (enables FPU)
/// 2. Configures voltage scaling
/// 3. Configures Flash latency based on clock frequency
/// 4. Enables Flash features (prefetch, caches)
/// 5. Configures system clock if config provided
///
/// Example:
///   void SystemInit() {
///       // Configure for 168MHz from HSE 8MHz
///       ClockConfig config = {
///           .source = ClockSource::PLL,
///           .pll_source = PLLSource::HSE,
///           .pll_m = 8,   // 8MHz / 8 = 1MHz
///           .pll_n = 336, // 1MHz * 336 = 336MHz
///           .pll_p = 2,   // 336MHz / 2 = 168MHz
///           .pll_q = 7,   // 336MHz / 7 = 48MHz (USB)
///           .ahb_prescaler = AHBPrescaler::Div1,
///           .apb1_prescaler = APBPrescaler::Div4,  // Max 42MHz
///           .apb2_prescaler = APBPrescaler::Div2   // Max 84MHz
///       };
///       system_init(config);
///   }
inline void system_init(const ClockConfig* config = nullptr) {
    // 1. Cortex-M4F initialization (enable FPU)
    cortex_m4::initialize(true, true);  // Enable FPU with lazy stacking

    // 2. Configure voltage scaling for default HSI (16 MHz)
    pwr::configure_voltage_scaling(16000000);

    // 3. Configure Flash latency for default HSI (16 MHz)
    flash::set_latency(16000000);

    // 4. Enable Flash features
    flash::enable_features();

    // 5. Configure system clock if provided
    if (config != nullptr) {
        // Get target frequency from config
        uint32_t target_freq = 168000000;  // Default max for STM32F4

        // Update voltage scaling for target frequency
        pwr::configure_voltage_scaling(target_freq);

        // Update Flash latency for target frequency
        flash::set_latency(target_freq);

        // Configure clocks
        configure_clocks(*config);
    }

    // 6. Memory barriers
    dsb();
    isb();
}

/// Initialize STM32F4 with default HSI clock (16 MHz)
/// Use this for simple applications that don't need high performance
inline void system_init_default() {
    system_init(nullptr);
}

} // namespace alloy::hal::st::stm32f4
