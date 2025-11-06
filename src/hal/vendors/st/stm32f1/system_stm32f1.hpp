// Alloy Framework - STM32F1 System Initialization
//
// Provides system initialization for STM32F1 family (Cortex-M3)
//
// Features:
// - Cortex-M3 core initialization (no FPU, no cache)
// - Clock configuration integration
// - Flash latency configuration
//
// Usage in board startup.cpp:
//   #include "hal/vendors/st/stm32f1/system_stm32f1.hpp"
//
//   void SystemInit() {
//       alloy::hal::st::stm32f1::system_init();
//   }

#pragma once

#include "../../../../startup/arm_cortex_m/core_common.hpp"
#include "clock.hpp"
#include <cstdint>

namespace alloy::hal::st::stm32f1 {

// Flash latency settings based on SYSCLK frequency
// From STM32F1 reference manual:
// 0 wait states: 0 < SYSCLK <= 24 MHz
// 1 wait state:  24 MHz < SYSCLK <= 48 MHz
// 2 wait states: 48 MHz < SYSCLK <= 72 MHz

namespace flash {
    constexpr uint32_t BASE = 0x40022000;  // FLASH base address

    struct Registers {
        volatile uint32_t ACR;      // Flash access control register
        volatile uint32_t KEYR;     // Flash key register
        volatile uint32_t OPTKEYR;  // Option byte key register
        volatile uint32_t SR;       // Status register
        volatile uint32_t CR;       // Control register
        volatile uint32_t AR;       // Address register
        uint32_t RESERVED;
        volatile uint32_t OBR;      // Option byte register
        volatile uint32_t WRPR;     // Write protection register
    };

    constexpr Registers* FLASH = reinterpret_cast<Registers*>(BASE);

    // ACR (Access Control Register) bit definitions
    namespace acr {
        constexpr uint32_t LATENCY_Pos = 0;
        constexpr uint32_t LATENCY_Msk = (7UL << LATENCY_Pos);
        constexpr uint32_t PRFTBE       = (1UL << 4);  // Prefetch buffer enable
        constexpr uint32_t PRFTBS       = (1UL << 5);  // Prefetch buffer status
    }

    /// Set Flash latency based on SYSCLK frequency
    /// @param sysclk_hz: System clock frequency in Hz
    inline void set_latency(uint32_t sysclk_hz) {
        uint32_t latency;

        if (sysclk_hz <= 24000000) {
            latency = 0;  // 0 wait states
        } else if (sysclk_hz <= 48000000) {
            latency = 1;  // 1 wait state
        } else {
            latency = 2;  // 2 wait states (up to 72 MHz)
        }

        // Set latency
        uint32_t acr = FLASH->ACR;
        acr &= ~acr::LATENCY_Msk;
        acr |= latency;
        FLASH->ACR = acr;
    }

    /// Enable Flash prefetch buffer
    /// Improves performance by prefetching instructions
    inline void enable_prefetch() {
        FLASH->ACR |= acr::PRFTBE;
    }
}

/// Initialize STM32F1 system
/// @param config: Clock configuration (optional)
///
/// This function:
/// 1. Initializes Cortex-M3 core (no FPU, no cache)
/// 2. Configures Flash latency based on clock frequency
/// 3. Enables Flash prefetch buffer
/// 4. Configures system clock if config provided
///
/// Example:
///   void SystemInit() {
///       // Configure for 72MHz from HSE 8MHz
///       ClockConfig config = {
///           .source = ClockSource::HSE,
///           .hse_frequency_hz = 8000000,
///           .use_pll = true,
///           .pll_mul = 9,  // 8MHz * 9 = 72MHz
///           .ahb_prescaler = AHBPrescaler::Div1,
///           .apb1_prescaler = APBPrescaler::Div2,  // Max 36MHz
///           .apb2_prescaler = APBPrescaler::Div1
///       };
///       system_init(config);
///   }
inline void system_init(const ClockConfig* config = nullptr) {
    // 1. Cortex-M3 initialization
    // Note: M3 has no FPU and no cache, so nothing to initialize

    // 2. Configure Flash latency for default HSI (8 MHz)
    flash::set_latency(8000000);

    // 3. Enable Flash prefetch buffer
    flash::enable_prefetch();

    // 4. Configure system clock if provided
    if (config != nullptr) {
        configure_clocks(*config);

        // Update Flash latency for new clock frequency
        uint32_t sysclk = get_sysclk_frequency();
        flash::set_latency(sysclk);
    }

    // 5. Memory barriers
    dsb();
    isb();
}

/// Initialize STM32F1 with default HSI clock (8 MHz)
/// Use this for simple applications that don't need high performance
inline void system_init_default() {
    system_init(nullptr);
}

} // namespace alloy::hal::st::stm32f1
