/// ESP32 Clock Configuration Implementation
///
/// Implements the clock interface for ESP32 (Xtensa LX6 Dual-Core, 240MHz max)
/// Uses auto-generated peripheral definitions from SVD
///
/// Clock tree:
/// - XTAL: 40MHz external crystal (default)
/// - PLL: 320MHz or 480MHz
/// - CPU: 80MHz, 160MHz, or 240MHz (from PLL)
/// - APB: 80MHz (fixed)

#ifndef ALLOY_HAL_ESPRESSIF_ESP32_CLOCK_HPP
#define ALLOY_HAL_ESPRESSIF_ESP32_CLOCK_HPP

#include "hal/interface/clock.hpp"
#include "core/types.hpp"
#include "hal/vendors/unknown/esp32/esp32/peripherals.hpp"

namespace alloy::hal::espressif::esp32 {

// Import generated peripherals
using namespace alloy::generated::esp32;

/// ESP32 System Clock implementation
class SystemClock {
public:
    SystemClock() : system_frequency_(80000000) {}  // Default PLL 80MHz

    /// Configure system clock
    core::Result<void> configure(const ClockConfig& config) {
        if (config.source == ClockSource::ExternalCrystal) {
            return configure_xtal_pll(config);
        } else if (config.source == ClockSource::InternalRC) {
            return configure_internal_8m();
        }
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

    /// Set system frequency (high-level API)
    core::Result<void> set_frequency(core::u32 frequency_hz) {
        if (frequency_hz == 80000000) {
            ClockConfig config(
                ClockSource::ExternalCrystal,  // source
                40000000,                        // crystal_hz
                2,                               // pll_mul (40MHz * 8 = 320MHz PLL, then /4 = 80MHz)
                1,                               // pll_div
                1,                               // ahb_div
                1,                               // apb1_div
                1,                               // apb2_div
                80000000                         // target_freq
            );
            return configure(config);
        } else if (frequency_hz == 160000000) {
            ClockConfig config(
                ClockSource::ExternalCrystal,  // source
                40000000,                        // crystal_hz
                4,                               // pll_mul (40MHz * 8 = 320MHz PLL, then /2 = 160MHz)
                1,                               // pll_div
                1,                               // ahb_div
                1,                               // apb1_div
                1,                               // apb2_div
                160000000                        // target_freq
            );
            return configure(config);
        } else if (frequency_hz == 240000000) {
            ClockConfig config(
                ClockSource::ExternalCrystal,  // source
                40000000,                        // crystal_hz
                6,                               // pll_mul (40MHz * 12 = 480MHz PLL, then /2 = 240MHz)
                1,                               // pll_div
                1,                               // ahb_div
                1,                               // apb1_div
                1,                               // apb2_div
                240000000                        // target_freq
            );
            return configure(config);
        }
        return core::Result<void>::error(core::ErrorCode::ClockInvalidFrequency);
    }

    /// Get current system frequency
    core::u32 get_frequency() const {
        return system_frequency_;
    }

    /// Get AHB frequency (same as CPU on ESP32)
    core::u32 get_ahb_frequency() const {
        return system_frequency_;
    }

    /// Get APB frequency (always 80MHz on ESP32)
    core::u32 get_apb1_frequency() const {
        return 80000000;  // APB is fixed at 80MHz
    }

    /// Get APB2 frequency (same as APB1 on ESP32)
    core::u32 get_apb2_frequency() const {
        return 80000000;
    }

    /// Get peripheral frequency
    core::u32 get_peripheral_frequency(Peripheral periph) const {
        // Most peripherals on ESP32 run at APB frequency (80MHz)
        return 80000000;
    }

    /// Enable peripheral clock
    core::Result<void> enable_peripheral(Peripheral periph) {
        // ESP32 uses DPORT_PERIP_CLK_EN_REG for peripheral clock enable
        // For now, this is a minimal implementation
        // Full implementation would require accessing DPORT registers
        return core::Result<void>::ok();
    }

    /// Disable peripheral clock
    core::Result<void> disable_peripheral(Peripheral periph) {
        // ESP32 uses DPORT_PERIP_CLK_EN_REG for peripheral clock disable
        return core::Result<void>::ok();
    }

    /// Set flash latency (not applicable to ESP32 - no-op)
    core::Result<void> set_flash_latency(core::u32 frequency_hz) {
        // ESP32 doesn't use flash latency settings like ARM Cortex-M
        return core::Result<void>::ok();
    }

    /// Configure PLL
    core::Result<void> configure_pll(const PllConfig& config) {
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

private:
    core::u32 system_frequency_;

    /// Configure internal 8MHz oscillator
    core::Result<void> configure_internal_8m() {
        // ESP32 has internal 8MHz oscillator (used during boot)
        // Switching back to it is not commonly done in applications
        system_frequency_ = 8000000;
        return core::Result<void>::ok();
    }

    /// Configure XTAL + PLL (40MHz → 80/160/240MHz)
    core::Result<void> configure_xtal_pll(const ClockConfig& config) {
        // ESP32 clock configuration is complex and typically done by bootloader/ROM
        // In bare-metal, we would configure:
        // 1. RTC_CNTL registers for crystal oscillator
        // 2. DPORT registers for PLL configuration
        // 3. CPU frequency selection

        // For this minimal implementation, we assume the clock is already
        // configured by the bootloader to a reasonable default

        // Calculate target frequency based on multiplier
        // ESP32 PLL: XTAL * (multiplier * 2) / divider
        core::u32 target_freq = config.crystal_frequency_hz * config.pll_multiplier;

        // Validate frequency range
        if (target_freq < 80000000 || target_freq > 240000000) {
            return core::Result<void>::error(core::ErrorCode::ClockInvalidFrequency);
        }

        // In a full implementation, we would:
        // 1. Enable XTAL oscillator via RTC_CNTL
        // 2. Configure PLL via DPORT_CPU_PER_CONF_REG
        // 3. Switch CPU clock source
        // 4. Wait for PLL lock

        system_frequency_ = target_freq;
        return core::Result<void>::ok();
    }
};

// Static assertions to verify concept compliance
static_assert(hal::SystemClock<SystemClock>, "ESP32 SystemClock must satisfy SystemClock concept");

} // namespace alloy::hal::espressif::esp32

#endif // ALLOY_HAL_ESPRESSIF_ESP32_CLOCK_HPP
