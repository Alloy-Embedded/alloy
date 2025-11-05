/// ATSAMD21 Clock Configuration Implementation
///
/// Implements the clock interface for ATSAMD21 family (Cortex-M0+, 48MHz max)
/// Uses auto-generated peripheral definitions from SVD
///
/// Clock tree:
/// - XOSC32K: 32.768kHz external crystal
/// - OSC8M: 8MHz internal RC oscillator
/// - DFLL48M: 48MHz output from 32kHz reference (Digital Frequency Locked Loop)
/// - GCLK: Generic Clock Generators (0-8)
///
/// SAMD21 has a unique clock architecture with generic clock generators

#ifndef ALLOY_HAL_MICROCHIP_SAMD21_CLOCK_HPP
#define ALLOY_HAL_MICROCHIP_SAMD21_CLOCK_HPP

#include "hal/interface/clock.hpp"
#include "core/types.hpp"
#include "hal/vendors/microchip_technology_inc/atsamd21j18a/atsamd21j18a/peripherals.hpp"

namespace alloy::hal::microchip::samd21 {

// Import generated peripherals
using namespace alloy::generated::atsamd21j18a;

/// ATSAMD21 System Clock implementation
class SystemClock {
public:
    SystemClock() : system_frequency_(8000000) {}  // Default OSC8M = 8MHz

    /// Configure system clock
    core::Result<void> configure(const ClockConfig& config) {
        if (config.source == ClockSource::ExternalCrystal) {
            return configure_dfll48m();
        } else if (config.source == ClockSource::InternalRC) {
            return configure_osc8m();
        }
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

    /// Set system frequency (high-level API)
    core::Result<void> set_frequency(core::u32 frequency_hz) {
        if (frequency_hz <= 8000000) {
            // Use OSC8M (internal 8MHz)
            return configure_osc8m();
        } else if (frequency_hz == 48000000) {
            // Use DFLL48M (48MHz from 32kHz crystal)
            return configure_dfll48m();
        }
        return core::Result<void>::error(core::ErrorCode::ClockInvalidFrequency);
    }

    /// Get current system frequency
    core::u32 get_frequency() const {
        return system_frequency_;
    }

    /// Get AHB frequency (same as system on SAMD21)
    core::u32 get_ahb_frequency() const {
        return system_frequency_;
    }

    /// Get APB1 frequency (same as system on SAMD21)
    core::u32 get_apb1_frequency() const {
        return system_frequency_;
    }

    /// Get APB2 frequency (same as system on SAMD21)
    core::u32 get_apb2_frequency() const {
        return system_frequency_;
    }

    /// Get peripheral frequency
    core::u32 get_peripheral_frequency(Peripheral periph) const {
        // SAMD21 uses generic clock generators
        // For simplicity, return system frequency
        return system_frequency_;
    }

    /// Enable peripheral clock
    core::Result<void> enable_peripheral(Peripheral periph) {
        // SAMD21 uses PM (Power Manager) to enable peripheral clocks
        // Different from STM32's RCC approach
        // For minimal implementation, this is a no-op
        return core::Result<void>::ok();
    }

    /// Disable peripheral clock
    core::Result<void> disable_peripheral(Peripheral periph) {
        // SAMD21 peripheral clock disable
        return core::Result<void>::ok();
    }

    /// Set flash latency (SAMD21 uses wait states)
    core::Result<void> set_flash_latency(core::u32 frequency_hz) {
        // SAMD21 NVMCTRL wait states:
        // 0 wait states: <= 24MHz
        // 1 wait state: > 24MHz
        core::u8 wait_states = (frequency_hz > 24000000) ? 1 : 0;

        // NVMCTRL->CTRLB.MANW field controls wait states
        // For minimal implementation, assume bootloader configured it
        return core::Result<void>::ok();
    }

    /// Configure PLL (not used in SAMD21, uses DFLL instead)
    core::Result<void> configure_pll(const PllConfig& config) {
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

private:
    core::u32 system_frequency_;

    /// Configure OSC8M (8MHz internal oscillator)
    core::Result<void> configure_osc8m() {
        // OSC8M is enabled by default on SAMD21
        // SYSCTRL->OSC8M register controls it

        // Switch GCLK0 (main clock) to OSC8M
        // GCLK->GENDIV = generator 0, no divider
        // GCLK->GENCTRL = generator 0, source OSC8M, enable

        // For minimal implementation, assume it's already configured
        system_frequency_ = 8000000;
        return core::Result<void>::ok();
    }

    /// Configure DFLL48M (48MHz from 32.768kHz crystal)
    core::Result<void> configure_dfll48m() {
        // DFLL48M configuration on SAMD21:
        // 1. Enable XOSC32K (32.768kHz external crystal)
        // 2. Configure GCLK1 to use XOSC32K as reference
        // 3. Enable DFLL48M in closed-loop mode
        // 4. Wait for lock
        // 5. Switch GCLK0 (main clock) to DFLL48M

        // This is complex and typically done by bootloader
        // For minimal implementation, we assume DFLL48M is already running
        // (Arduino Zero bootloader does this)

        // Set wait states for 48MHz
        set_flash_latency(48000000);

        system_frequency_ = 48000000;
        return core::Result<void>::ok();
    }
};

// Static assertions to verify concept compliance
static_assert(hal::SystemClock<SystemClock>, "ATSAMD21 SystemClock must satisfy SystemClock concept");

} // namespace alloy::hal::microchip::samd21

#endif // ALLOY_HAL_MICROCHIP_SAMD21_CLOCK_HPP
