#pragma once

#include "hal/interface/clock.hpp"
#include "hal/vendors/atmel/same70/atsame70q21/peripherals.hpp"
#include "core/error.hpp"
#include "core/types.hpp"

namespace alloy::hal::atmel::same70 {

using namespace alloy::core;
using namespace alloy::generated::atsame70q21;

/**
 * SAME70 Clock Configuration
 *
 * Type-safe clock configuration for Atmel/Microchip SAME70 family.
 * Supports up to 300 MHz with advanced PLL configuration.
 *
 * Usage:
 *   using Clock = Same70Clock<12000000>;  // 12 MHz crystal
 *   auto result = Clock::configure_300mhz();
 */
template<u32 ExternalCrystalHz = 12000000>
class Same70Clock {
public:
    static constexpr u32 MAINCK_FREQ_HZ = 12000000;  // Main clock (RC or XTAL)
    static constexpr u32 HSE_FREQ_HZ = ExternalCrystalHz;
    static constexpr u32 MAX_FREQ_HZ = 300000000;
    static constexpr u32 PMC_KEY = 0x37 << 16;

    /**
     * Configure for maximum performance: 300 MHz CPU, 150 MHz Master Clock
     *
     * Configuration:
     * - External Crystal: 12 MHz
     * - PLLA: 12 MHz / 1 * 25 = 300 MHz
     * - CPU Clock (HCLK): 300 MHz
     * - Master Clock (MCK): 150 MHz (CPU/2)
     * - Flash Wait States: 6 cycles
     */
    static Result<void> configure_300mhz() {
        // Enable main crystal oscillator (12MHz)
        pmc::PMC->CKGR_MOR = PMC_KEY |
                            pmc::ckgr_mor_bits::MOSCXTEN |
                            pmc::ckgr_mor_bits::MOSCRCEN |
                            (0xFF << 8);  // Startup time

        // Wait for crystal stabilization
        u32 timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MOSCXTS) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        // Switch main clock to crystal
        pmc::PMC->CKGR_MOR = PMC_KEY |
                            pmc::ckgr_mor_bits::MOSCXTEN |
                            pmc::ckgr_mor_bits::MOSCRCEN |
                            pmc::ckgr_mor_bits::MOSCSEL |
                            (0xFF << 8);

        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MOSCSELS) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        // Set flash wait states BEFORE increasing frequency
        efc::EFC->EEFC_FMR = (6 << 8);  // 6 wait states for 300 MHz

        // Configure PLLA: 12MHz / 1 * 25 = 300MHz
        pmc::PMC->CKGR_PLLAR = (1U << 29) |      // ONE bit
                              (24 << 16) |        // MULA (25-1)
                              (0x3F << 8) |       // PLLACOUNT
                              1;                  // DIVA

        // Wait for PLLA lock
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::LOCKA) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::PllLockFailed);

        // Switch master clock to PLLA with divider
        pmc::PMC->MCKR = (pmc::PMC->MCKR & ~(0x7 << 4)) | (0 << 4);  // PRES=0
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY) && timeout--) {}

        pmc::PMC->MCKR = (pmc::PMC->MCKR & ~(0x3 << 8)) | (0x1 << 8);  // MDIV=2
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY) && timeout--) {}

        pmc::PMC->MCKR = (pmc::PMC->MCKR & ~0x3) | 0x2;  // CSS=PLLA
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        // Update frequencies
        current_frequencies_.system = 300000000;
        current_frequencies_.ahb = 300000000;
        current_frequencies_.apb1 = 150000000;
        current_frequencies_.apb2 = 150000000;

        return Ok();
    }

    /**
     * Configure for lower power: 150 MHz CPU, 75 MHz Master Clock
     */
    static Result<void> configure_150mhz() {
        // Similar to 300MHz but with different multiplier
        pmc::PMC->CKGR_MOR = PMC_KEY | pmc::ckgr_mor_bits::MOSCXTEN |
                            pmc::ckgr_mor_bits::MOSCRCEN | (0xFF << 8);

        u32 timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MOSCXTS) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        pmc::PMC->CKGR_MOR = PMC_KEY | pmc::ckgr_mor_bits::MOSCXTEN |
                            pmc::ckgr_mor_bits::MOSCRCEN |
                            pmc::ckgr_mor_bits::MOSCSEL | (0xFF << 8);

        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MOSCSELS) && timeout--) {}

        efc::EFC->EEFC_FMR = (3 << 8);  // 3 wait states for 150 MHz

        // PLLA: 12MHz / 1 * 13 = 156 MHz (close to 150)
        pmc::PMC->CKGR_PLLAR = (1U << 29) | (12 << 16) | (0x3F << 8) | 1;

        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::LOCKA) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::PllLockFailed);

        pmc::PMC->MCKR = (pmc::PMC->MCKR & ~(0x7 << 4)) | (0 << 4);
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY) && timeout--) {}

        pmc::PMC->MCKR = (pmc::PMC->MCKR & ~(0x3 << 8)) | (0x1 << 8);
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY) && timeout--) {}

        pmc::PMC->MCKR = (pmc::PMC->MCKR & ~0x3) | 0x2;
        timeout = 100000;
        while (!(pmc::PMC->SR & pmc::sr_bits::MCKRDY) && timeout--) {}

        current_frequencies_.system = 156000000;
        current_frequencies_.ahb = 156000000;
        current_frequencies_.apb1 = 78000000;
        current_frequencies_.apb2 = 78000000;

        return Ok();
    }

    static u32 get_frequency() { return current_frequencies_.system; }
    static u32 get_ahb_frequency() { return current_frequencies_.ahb; }
    static u32 get_apb1_frequency() { return current_frequencies_.apb1; }
    static u32 get_apb2_frequency() { return current_frequencies_.apb2; }

    /**
     * Enable peripheral clock
     */
    static Result<void> enable_peripheral(hal::Peripheral peripheral) {
        u16 pid = static_cast<u16>(peripheral) & 0xFF;

        if (pid < 32) {
            pmc::PMC->PCER0 = (1U << pid);
        } else {
            pmc::PMC->PCER1 = (1U << (pid - 32));
        }

        return Ok();
    }

    /**
     * Disable peripheral clock
     */
    static Result<void> disable_peripheral(hal::Peripheral peripheral) {
        u16 pid = static_cast<u16>(peripheral) & 0xFF;

        if (pid < 32) {
            pmc::PMC->PCDR0 = (1U << pid);
        } else {
            pmc::PMC->PCDR1 = (1U << (pid - 32));
        }

        return Ok();
    }

private:
    struct Frequencies {
        u32 system = MAINCK_FREQ_HZ;
        u32 ahb = MAINCK_FREQ_HZ;
        u32 apb1 = MAINCK_FREQ_HZ;
        u32 apb2 = MAINCK_FREQ_HZ;
    };

    static inline Frequencies current_frequencies_;
};

using Same70Clock12MHz = Same70Clock<12000000>;

} // namespace alloy::hal::atmel::same70
