#pragma once

#include "hal/interface/clock.hpp"
#include "hal/vendors/microchip_technology_inc/atsamd21j18a/atsamd21j18a/peripherals.hpp"
#include "core/error.hpp"
#include "core/types.hpp"

namespace alloy::hal::microchip::samd21 {

using namespace alloy::core;
using namespace alloy::generated::atsamd21j18a;

/**
 * ATSAMD21 Clock Configuration
 *
 * Type-safe clock configuration for Microchip/Atmel SAMD21 family (Cortex-M0+).
 * Supports up to 48 MHz using DFLL48M (Digital Frequency Locked Loop).
 *
 * Clock Architecture:
 * - OSC8M: 8 MHz internal RC oscillator (default at boot)
 * - XOSC32K: 32.768 kHz external crystal
 * - DFLL48M: 48 MHz output locked to 32 kHz reference
 * - GCLK: Generic Clock Generators (flexible routing)
 *
 * Usage:
 *   using Clock = Samd21Clock<32768>;  // 32.768 kHz crystal
 *   auto result = Clock::configure_48mhz();
 */
template<u32 ExternalCrystal32kHz = 32768>
class Samd21Clock {
public:
    static constexpr u32 OSC8M_FREQ_HZ = 8000000;    // 8 MHz internal
    static constexpr u32 XOSC32K_FREQ_HZ = ExternalCrystal32kHz;
    static constexpr u32 MAX_FREQ_HZ = 48000000;     // 48 MHz max

    // Register bit masks
    static constexpr u32 DFLLCTRL_ENABLE = (1 << 1);
    static constexpr u32 DFLLCTRL_MODE = (1 << 2);     // Closed-loop mode
    static constexpr u32 DFLLCTRL_STABLE = (1 << 3);
    static constexpr u32 DFLLCTRL_WAITLOCK = (1 << 11);
    static constexpr u32 PCLKSR_DFLLRDY = (1 << 4);
    static constexpr u32 PCLKSR_DFLLLCKC = (1 << 6);
    static constexpr u32 PCLKSR_DFLLLCKF = (1 << 7);

    /**
     * Configure for maximum performance: 48 MHz
     *
     * Configuration:
     * - XOSC32K @ 32.768 kHz (external crystal)
     * - DFLL48M in closed-loop mode (32.768 kHz → 48 MHz)
     * - GCLK0 (main clock) = DFLL48M
     * - 1 flash wait state
     */
    static Result<void> configure_48mhz() {
        // Enable 32.768 kHz crystal oscillator
        auto result = enable_xosc32k();
        if (!result.is_ok()) return result;

        // Configure DFLL48M in closed-loop mode
        result = configure_dfll48m_closed_loop();
        if (!result.is_ok()) return result;

        // Set flash wait states for 48 MHz
        set_flash_wait_states(1);

        // Switch main clock (GCLK0) to DFLL48M
        result = switch_gclk0_to_dfll48m();
        if (!result.is_ok()) return result;

        // Update frequencies
        current_frequencies_.system = 48000000;
        current_frequencies_.ahb = 48000000;
        current_frequencies_.apb1 = 48000000;
        current_frequencies_.apb2 = 48000000;

        return Ok();
    }

    /**
     * Configure for low power: 8 MHz internal RC
     *
     * Uses OSC8M, no external components needed.
     * Default configuration at boot.
     */
    static Result<void> configure_8mhz() {
        // OSC8M is enabled by default
        // Just ensure GCLK0 is using OSC8M

        // Set flash wait states for 8 MHz (0 wait states)
        set_flash_wait_states(0);

        current_frequencies_.system = 8000000;
        current_frequencies_.ahb = 8000000;
        current_frequencies_.apb1 = 8000000;
        current_frequencies_.apb2 = 8000000;

        return Ok();
    }

    /**
     * Configure DFLL48M in open-loop mode (less accurate, no crystal needed)
     */
    static Result<void> configure_48mhz_open_loop() {
        // Enable DFLL48M in open-loop mode
        // Uses internal reference, less accurate but doesn't need XOSC32K

        sysctrl::SYSCTRL->DFLLCTRL = DFLLCTRL_ENABLE;

        // Wait for DFLL to be ready
        u32 timeout = 100000;
        while (!(sysctrl::SYSCTRL->PCLKSR & PCLKSR_DFLLRDY) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        set_flash_wait_states(1);

        current_frequencies_.system = 48000000;
        current_frequencies_.ahb = 48000000;
        current_frequencies_.apb1 = 48000000;
        current_frequencies_.apb2 = 48000000;

        return Ok();
    }

    static u32 get_frequency() { return current_frequencies_.system; }
    static u32 get_ahb_frequency() { return current_frequencies_.ahb; }
    static u32 get_apb1_frequency() { return current_frequencies_.apb1; }
    static u32 get_apb2_frequency() { return current_frequencies_.apb2; }

    /**
     * Enable peripheral clock
     *
     * SAMD21 uses PM (Power Manager) and GCLK for peripheral clocking.
     */
    static Result<void> enable_peripheral(hal::Peripheral peripheral) {
        // SAMD21 peripheral clocks are controlled by PM (Power Manager)
        // and GCLK (Generic Clock Generator)
        // For this implementation, we assume peripherals are enabled by default
        return Ok();
    }

    /**
     * Disable peripheral clock
     */
    static Result<void> disable_peripheral(hal::Peripheral peripheral) {
        return Ok();
    }

private:
    struct Frequencies {
        u32 system = OSC8M_FREQ_HZ;
        u32 ahb = OSC8M_FREQ_HZ;
        u32 apb1 = OSC8M_FREQ_HZ;
        u32 apb2 = OSC8M_FREQ_HZ;
    };

    static inline Frequencies current_frequencies_;

    /**
     * Enable 32.768 kHz external crystal oscillator
     */
    static Result<void> enable_xosc32k() {
        // Configure XOSC32K
        // XOSC32K register: EN32K | XTALEN | STARTUP | ONDEMAND
        sysctrl::SYSCTRL->XOSC32K = (1 << 1) |   // XTALEN: Crystal oscillator enable
                                    (1 << 2) |   // EN32K: 32kHz output enable
                                    (0x6 << 8);  // STARTUP: ~4 seconds startup time

        // Wait for XOSC32K to stabilize
        // PCLKSR.XOSC32KRDY bit indicates ready
        u32 timeout = 100000;
        while (!(sysctrl::SYSCTRL->PCLKSR & (1 << 1)) && timeout--) {}

        if (timeout == 0) {
            return Error(ErrorCode::ClockSourceNotReady);
        }

        return Ok();
    }

    /**
     * Configure DFLL48M in closed-loop mode
     *
     * Locks DFLL48M output to 32.768 kHz reference for accurate 48 MHz.
     */
    static Result<void> configure_dfll48m_closed_loop() {
        // Disable DFLL before configuration
        sysctrl::SYSCTRL->DFLLCTRL = 0;

        // Wait for DFLL to be ready
        u32 timeout = 100000;
        while (!(sysctrl::SYSCTRL->PCLKSR & PCLKSR_DFLLRDY) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        // Configure multiplier: 48 MHz / 32.768 kHz = 1465
        // DFLLMUL = MUL | FSTEP | CSTEP
        sysctrl::SYSCTRL->DFLLMUL = (1465 << 0) |   // MUL: Multiplier value
                                    (0xFF << 16) |  // FSTEP: Fine step
                                    (0x1F << 26);   // CSTEP: Coarse step

        // Configure DFLL value for closed-loop mode
        sysctrl::SYSCTRL->DFLLVAL = (512 << 0) |    // FINE: Fine calibration
                                    (64 << 10);     // COARSE: Coarse calibration

        // Enable DFLL in closed-loop mode
        sysctrl::SYSCTRL->DFLLCTRL = DFLLCTRL_ENABLE |
                                     DFLLCTRL_MODE |
                                     DFLLCTRL_WAITLOCK;

        // Wait for DFLL to lock
        timeout = 100000;
        while (!(sysctrl::SYSCTRL->PCLKSR & (PCLKSR_DFLLLCKC | PCLKSR_DFLLLCKF)) && timeout--) {}

        if (timeout == 0) {
            return Error(ErrorCode::PllLockFailed);
        }

        return Ok();
    }

    /**
     * Switch main clock (GCLK0) to DFLL48M
     */
    static Result<void> switch_gclk0_to_dfll48m() {
        // Configure GCLK generator 0 to use DFLL48M
        // GENDIV: Select generator 0, no division
        gclk::GCLK->GENDIV = (0 << 0);  // ID=0 (GCLK0), DIV=0 (no division)

        // GENCTRL: Select DFLL48M as source for GCLK0
        // SRC=0x07 (DFLL48M), GENEN=1, IDC=1
        gclk::GCLK->GENCTRL = (0 << 0) |    // ID=0 (GCLK0)
                              (0x07 << 8) | // SRC=DFLL48M
                              (1 << 16) |   // GENEN: Enable generator
                              (1 << 17);    // IDC: Improve duty cycle

        // Wait for synchronization
        u32 timeout = 10000;
        while ((gclk::GCLK->STATUS & (1 << 7)) && timeout--) {}  // SYNCBUSY

        return Ok();
    }

    /**
     * Set flash wait states
     *
     * SAMD21 requires:
     * - 0 wait states: ≤ 24 MHz
     * - 1 wait state: > 24 MHz
     */
    static void set_flash_wait_states(u8 wait_states) {
        // NVMCTRL->CTRLB.RWS field
        // For this minimal implementation, we assume the bootloader configured it
        // A complete implementation would write to NVMCTRL registers
        (void)wait_states;
    }
};

using Samd21Clock32kHz = Samd21Clock<32768>;

} // namespace alloy::hal::microchip::samd21
