#pragma once

#include "hal/vendors/atmel/same70/atsame70q21/peripherals.hpp"
#include <cstdint>

namespace alloy::hal::atmel::same70::clock {

using namespace alloy::generated::atsame70q21;

/// Clock configuration constants
namespace config {
    /// External crystal frequency (12 MHz on SAME70-XPLD)
    constexpr uint32_t XTAL_FREQ_HZ = 12000000;

    /// Target CPU frequency (300 MHz max for SAME70)
    constexpr uint32_t TARGET_CPU_FREQ_HZ = 300000000;

    /// PLL multiplier (MULA): 12MHz * 25 = 300MHz
    constexpr uint32_t PLL_MUL = 25;

    /// PLL divider (DIVA): divide by 1
    constexpr uint32_t PLL_DIV = 1;

    /// Master clock divider (MCK = CPU/2 = 150MHz)
    constexpr uint32_t MASTER_CLK_DIV = 2;

    /// Flash wait states for 300MHz @ 3.3V
    /// SAME70 requires 6 wait states for 300MHz operation
    constexpr uint32_t FLASH_WAIT_STATES = 6;
}

/// PMC register key for write protection
constexpr uint32_t PMC_KEY = 0x37 << 16;

/// Current clock frequencies (updated by init())
namespace current {
    inline uint32_t cpu_freq_hz = 0;
    inline uint32_t master_freq_hz = 0;
}

/**
 * @brief Initialize system clocks to 300MHz CPU, 150MHz Master Clock
 *
 * Clock configuration:
 * - External Crystal: 12 MHz
 * - PLLA: 12 MHz / 1 * 25 = 300 MHz
 * - CPU Clock (HCLK): 300 MHz
 * - Master Clock (MCK): 150 MHz (CPU/2)
 * - Flash Wait States: 6 cycles
 */
inline void init() {
    // 1. Enable main crystal oscillator (12MHz external crystal)
    pmc::PMC->CKGR_MOR = PMC_KEY |
                         pmc::ckgr_mor_bits::MOSCXTEN |    // Enable crystal
                         pmc::ckgr_mor_bits::MOSCRCEN |    // Keep RC enabled during transition
                         (0xFF << 8);                       // Startup time: 8ms * 12MHz / 8 = 0xFF

    // Wait for crystal to stabilize
    while (!(pmc::PMC->SR & pmc::sr_bits::MOSCXTS));

    // 2. Switch main clock to crystal oscillator
    pmc::PMC->CKGR_MOR = PMC_KEY |
                         pmc::ckgr_mor_bits::MOSCXTEN |
                         pmc::ckgr_mor_bits::MOSCRCEN |
                         pmc::ckgr_mor_bits::MOSCSEL |     // Select crystal as main clock
                         (0xFF << 8);

    // Wait for selection to complete
    while (!(pmc::PMC->SR & pmc::sr_bits::MOSCSELS));

    // 3. Set flash wait states BEFORE increasing frequency
    // SAME70 @ 300MHz requires 6 wait states (FWS=6)
    efc::EFC->EEFC_FMR = (config::FLASH_WAIT_STATES << 8);

    // 4. Configure PLLA: 12MHz / 1 * 25 = 300MHz
    // DIVA = 1 (divide by 1)
    // MULA = 24 (multiply by 25, MULA = MUL - 1)
    // PLLACOUNT = 0x3F (startup time)
    pmc::PMC->CKGR_PLLAR = (1U << 29) |                   // ONE bit (must be set)
                           ((config::PLL_MUL - 1) << 16) | // MULA (25-1=24)
                           (0x3F << 8) |                   // PLLACOUNT
                           config::PLL_DIV;                // DIVA

    // Wait for PLLA to lock
    while (!(pmc::PMC->SR & pmc::sr_bits::LOCKA));

    // 5. Switch master clock to PLLA with prescaler and divider
    // First set prescaler (PRES=0, no prescale)
    pmc::PMC->MCKR = (pmc::PMC->MCKR & ~(0x7 << 4)) | (0 << 4);
    while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY));

    // Set master clock divider (MDIV=2, MCK=PCK/2)
    // This gives us MCK = 300MHz / 2 = 150MHz
    pmc::PMC->MCKR = (pmc::PMC->MCKR & ~(0x3 << 8)) | (0x1 << 8);
    while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY));

    // Finally switch to PLLA as clock source
    pmc::PMC->MCKR = (pmc::PMC->MCKR & ~0x3) | 0x2;  // CSS=2 (PLLA)
    while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY));

    // Update current frequency globals
    current::cpu_freq_hz = config::TARGET_CPU_FREQ_HZ;
    current::master_freq_hz = config::TARGET_CPU_FREQ_HZ / config::MASTER_CLK_DIV;
}

/**
 * @brief Get current CPU clock frequency
 * @return CPU frequency in Hz
 */
inline uint32_t get_cpu_freq() {
    return current::cpu_freq_hz;
}

/**
 * @brief Get current master clock frequency
 * @return Master clock (MCK) frequency in Hz
 */
inline uint32_t get_master_freq() {
    return current::master_freq_hz;
}

/**
 * @brief Enable peripheral clock
 * @param pid Peripheral ID (0-63)
 */
inline void enable_peripheral_clock(uint8_t pid) {
    if (pid < 32) {
        pmc::PMC->PCER0 = (1U << pid);
    } else {
        pmc::PMC->PCER1 = (1U << (pid - 32));
    }
}

/**
 * @brief Disable peripheral clock
 * @param pid Peripheral ID (0-63)
 */
inline void disable_peripheral_clock(uint8_t pid) {
    if (pid < 32) {
        pmc::PMC->PCDR0 = (1U << pid);
    } else {
        pmc::PMC->PCDR1 = (1U << (pid - 32));
    }
}

}  // namespace alloy::hal::atmel::same70::clock
