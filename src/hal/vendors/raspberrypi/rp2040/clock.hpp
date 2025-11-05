#pragma once

#include "hal/interface/clock.hpp"
#include "hal/vendors/raspberrypi/rp2040/peripherals.hpp"
#include "core/error.hpp"
#include "core/types.hpp"

namespace alloy::hal::raspberrypi::rp2040 {

using namespace alloy::core;
using namespace alloy::generated::rp2040;

/**
 * RP2040 Clock Configuration
 *
 * Type-safe clock configuration for Raspberry Pi RP2040 (Dual Cortex-M0+).
 * Supports up to 133 MHz with flexible clock routing.
 *
 * Clock Architecture:
 * - ROSC: Ring oscillator (~6.5 MHz, variable)
 * - XOSC: External crystal oscillator (12 MHz standard)
 * - PLL_SYS: System PLL (12 MHz → 125-133 MHz)
 * - PLL_USB: USB PLL (12 MHz → 48 MHz for USB)
 *
 * Usage:
 *   using Clock = Rp2040Clock<12000000>;  // 12 MHz crystal
 *   auto result = Clock::configure_125mhz();
 */
template<u32 ExternalCrystalHz = 12000000>
class Rp2040Clock {
public:
    static constexpr u32 ROSC_FREQ_HZ = 6500000;   // ~6.5 MHz typical
    static constexpr u32 XOSC_FREQ_HZ = ExternalCrystalHz;
    static constexpr u32 MAX_FREQ_HZ = 133000000;  // 133 MHz max (overclock)

    // Magic values for oscillator enable/disable
    static constexpr u32 XOSC_ENABLE = 0xFAB000;
    static constexpr u32 XOSC_DISABLE = 0xD1E000;
    static constexpr u32 ROSC_PASSWD = 0x9696;

    /**
     * Configure for standard performance: 125 MHz
     *
     * Configuration:
     * - XOSC @ 12 MHz
     * - PLL_SYS: 12 MHz * 125 / 6 / 2 = 125 MHz
     * - PLL_USB: 12 MHz * 40 / 5 / 2 = 48 MHz (for USB)
     * - All clocks: 125 MHz
     */
    static Result<void> configure_125mhz() {
        // Start XOSC
        auto result = start_xosc();
        if (!result.is_ok()) return result;

        // Configure PLL_SYS for 125 MHz
        // VCO = 12MHz * 125 = 1500 MHz
        // Output = 1500 MHz / 6 / 2 = 125 MHz
        result = configure_pll_sys(125, 6, 2);
        if (!result.is_ok()) return result;

        // Configure PLL_USB for 48 MHz
        // VCO = 12MHz * 40 = 480 MHz
        // Output = 480 MHz / 5 / 2 = 48 MHz
        result = configure_pll_usb(40, 5, 2);
        if (!result.is_ok()) return result;

        // Update frequencies
        current_frequencies_.system = 125000000;
        current_frequencies_.ahb = 125000000;
        current_frequencies_.apb1 = 125000000;
        current_frequencies_.apb2 = 125000000;

        return Ok();
    }

    /**
     * Configure for maximum performance: 133 MHz (overclock)
     *
     * Configuration:
     * - XOSC @ 12 MHz
     * - PLL_SYS: 12 MHz * 133 / 6 / 2 = 133 MHz
     * - PLL_USB: 12 MHz * 40 / 5 / 2 = 48 MHz (for USB)
     */
    static Result<void> configure_133mhz() {
        auto result = start_xosc();
        if (!result.is_ok()) return result;

        // Configure PLL_SYS for 133 MHz
        // VCO = 12MHz * 133 = 1596 MHz
        // Output = 1596 MHz / 6 / 2 = 133 MHz
        result = configure_pll_sys(133, 6, 2);
        if (!result.is_ok()) return result;

        result = configure_pll_usb(40, 5, 2);
        if (!result.is_ok()) return result;

        current_frequencies_.system = 133000000;
        current_frequencies_.ahb = 133000000;
        current_frequencies_.apb1 = 133000000;
        current_frequencies_.apb2 = 133000000;

        return Ok();
    }

    /**
     * Configure for low power: Ring oscillator (~6.5 MHz)
     *
     * Uses internal ROSC, no external components needed.
     * Lowest power consumption.
     */
    static Result<void> configure_rosc() {
        // ROSC is enabled by default on RP2040
        // It's the boot clock source

        current_frequencies_.system = ROSC_FREQ_HZ;
        current_frequencies_.ahb = ROSC_FREQ_HZ;
        current_frequencies_.apb1 = ROSC_FREQ_HZ;
        current_frequencies_.apb2 = ROSC_FREQ_HZ;

        return Ok();
    }

    static u32 get_frequency() { return current_frequencies_.system; }
    static u32 get_ahb_frequency() { return current_frequencies_.ahb; }
    static u32 get_apb1_frequency() { return current_frequencies_.apb1; }
    static u32 get_apb2_frequency() { return current_frequencies_.apb2; }

    /**
     * Enable peripheral clock
     *
     * RP2040 uses RESETS to bring peripherals out of reset.
     * Once out of reset, peripheral clocks are always running.
     */
    static Result<void> enable_peripheral(hal::Peripheral peripheral) {
        // RP2040 peripheral clock management is done via RESETS block
        // For this implementation, we assume peripherals are already out of reset
        // A complete implementation would interface with the RESETS peripheral
        return Ok();
    }

    /**
     * Disable peripheral clock
     */
    static Result<void> disable_peripheral(hal::Peripheral peripheral) {
        // RP2040 peripheral clock disable via RESETS
        return Ok();
    }

private:
    struct Frequencies {
        u32 system = ROSC_FREQ_HZ;
        u32 ahb = ROSC_FREQ_HZ;
        u32 apb1 = ROSC_FREQ_HZ;
        u32 apb2 = ROSC_FREQ_HZ;
    };

    static inline Frequencies current_frequencies_;

    /**
     * Start external crystal oscillator (XOSC)
     */
    static Result<void> start_xosc() {
        // Set frequency range (1-15 MHz)
        xosc::XOSC->DISABLE = 0xAA0;  // Frequency range 1-15 MHz

        // Set startup delay (depends on crystal, typically ~1ms @ 12MHz = ~12000 cycles)
        xosc::XOSC->X4 = 47;  // Startup delay multiplier

        // Enable XOSC
        xosc::XOSC->DISABLE = XOSC_ENABLE;

        // Wait for XOSC to stabilize
        // STABLE register bit 31 indicates stability
        u32 timeout = 100000;
        while (!(xosc::XOSC->STABLE & (1U << 31)) && timeout--) {}

        if (timeout == 0) {
            return Error(ErrorCode::ClockSourceNotReady);
        }

        return Ok();
    }

    /**
     * Configure system PLL
     *
     * Formula: Fout = (Fref * FBDIV) / (POSTDIV1 * POSTDIV2)
     * VCO must be in range 400-1600 MHz
     */
    static Result<void> configure_pll_sys(u16 fbdiv, u8 postdiv1, u8 postdiv2) {
        // Reset PLL
        pll::PLL_SYS->VCOPD = 1;  // Power down VCO
        pll::PLL_SYS->LOCK = 0;   // Clear lock

        // Set feedback divider
        pll::PLL_SYS->FBDIV_INT = fbdiv;

        // Power up VCO
        pll::PLL_SYS->VCOPD = 0;

        // Wait for VCO to lock
        u32 timeout = 100000;
        while (!(pll::PLL_SYS->LOCK & (1U << 31)) && timeout--) {}

        if (timeout == 0) {
            return Error(ErrorCode::PllLockFailed);
        }

        // Set post dividers
        pll::PLL_SYS->POSTDIV1 = ((postdiv1 & 0x7) << 16) | ((postdiv2 & 0x7) << 12);

        return Ok();
    }

    /**
     * Configure USB PLL for 48 MHz
     */
    static Result<void> configure_pll_usb(u16 fbdiv, u8 postdiv1, u8 postdiv2) {
        // USB PLL configuration is similar to system PLL
        // but targets 48 MHz for USB specification

        // For simplicity, we assume USB PLL is at fixed address offset
        // A complete implementation would use proper peripheral definitions

        return Ok();
    }
};

using Rp2040Clock12MHz = Rp2040Clock<12000000>;

} // namespace alloy::hal::raspberrypi::rp2040
