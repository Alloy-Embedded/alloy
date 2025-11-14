/**
 * @file watchdog_hardware_policy.hpp
 * @brief Hardware Policy for SAME70 Watchdog Timer (WDT and RSWDT)
 *
 * Provides platform-specific watchdog control using Hardware Policy pattern.
 * Supports both WDT (standard watchdog) and RSWDT (reinforced watchdog).
 *
 * @note Part of Alloy HAL Vendor Layer
 */

#pragma once

#include "core/types.hpp"
#include "hal/vendors/atmel/same70/registers/wdt_registers.hpp"
#include "hal/vendors/atmel/same70/registers/rswdt_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/wdt_bitfields.hpp"
#include "hal/vendors/atmel/same70/bitfields/rswdt_bitfields.hpp"

namespace alloy::hal::atmel::same70 {

using namespace alloy::core;

/**
 * @brief SAME70 Watchdog Hardware Policy
 *
 * Template parameter is the WDT base address (WDT or RSWDT).
 *
 * Usage:
 * @code
 * using WDT_Policy = Same70WatchdogHardwarePolicy<peripherals::WDT>;
 * using RSWDT_Policy = Same70WatchdogHardwarePolicy<peripherals::RSWDT>;
 * @endcode
 *
 * @tparam WDT_BASE Base address of WDT or RSWDT
 */
template <uintptr_t WDT_BASE>
struct Same70WatchdogHardwarePolicy {
    /**
     * @brief Disable watchdog
     *
     * Sets WDDIS bit in Mode Register.
     * NOTE: On SAME70, WDT MR is write-once after reset!
     * Once configured, it cannot be changed without a reset.
     */
    static void disable() {
        auto* wdt = get_wdt();
        // WDT MR is write-once, set WDDIS=1 to disable
        wdt->MR = wdt::mr::WDDIS::set(0xFFF000);  // Also set safe timeout values
    }

    /**
     * @brief Enable watchdog with default timeout
     *
     * Configures watchdog with 1 second timeout.
     */
    static void enable() {
        auto* wdt = get_wdt();
        // Calculate counter value for 1 second (assuming 128 Hz slow clock)
        u16 wdv = 128;  // 1 second at 128 Hz

        u32 mr = 0;
        mr = wdt::mr::WDV::write(mr, wdv);
        mr = wdt::mr::WDD::write(mr, wdv);
        mr = wdt::mr::WDRSTEN::set(mr);  // Enable reset
        // WDDIS=0 (enabled) - default
        wdt->MR = mr;
    }

    /**
     * @brief Enable watchdog with custom timeout
     *
     * @param timeout_ms Timeout in milliseconds
     */
    static void enable_with_timeout(u16 timeout_ms) {
        auto* wdt = get_wdt();
        // SAME70 watchdog runs at 128 Hz (32768 / 256)
        constexpr u32 WDT_FREQ_HZ = 128;

        // Calculate counter value
        u16 wdv = (timeout_ms * WDT_FREQ_HZ) / 1000;
        if (wdv > 0xFFF) wdv = 0xFFF;  // Max 12 bits
        if (wdv == 0) wdv = 1;  // Minimum

        u32 mr = 0;
        mr = wdt::mr::WDV::write(mr, wdv);
        mr = wdt::mr::WDD::write(mr, wdv);
        mr = wdt::mr::WDRSTEN::set(mr);
        wdt->MR = mr;
    }

    /**
     * @brief Feed/kick the watchdog
     *
     * Restarts the watchdog counter.
     */
    static void feed() {
        auto* wdt = get_wdt();
        u32 cr = 0;
        cr = wdt::cr::WDRSTT::set(cr);
        cr = wdt::cr::KEY::write(cr, wdt::cr::key::PASSWD);
        wdt->CR = cr;
    }

    /**
     * @brief Check if watchdog is enabled
     *
     * @return true if enabled, false if disabled
     */
    static bool is_enabled() {
        auto* wdt = get_wdt();
        return !(wdt->MR & wdt::mr::WDDIS::mask);
    }

    /**
     * @brief Configure watchdog with expert settings
     *
     * @param timeout_ms Timeout in milliseconds
     * @param reset_enable Enable reset on timeout
     * @param interrupt_enable Enable interrupt on timeout
     */
    static void configure(u16 timeout_ms, bool reset_enable, bool interrupt_enable) {
        auto* wdt = get_wdt();
        constexpr u32 WDT_FREQ_HZ = 128;

        u16 wdv = (timeout_ms * WDT_FREQ_HZ) / 1000;
        if (wdv > 0xFFF) wdv = 0xFFF;
        if (wdv == 0) wdv = 1;

        u32 mr = 0;
        mr = wdt::mr::WDV::write(mr, wdv);
        mr = wdt::mr::WDD::write(mr, wdv);

        if (reset_enable) {
            mr = wdt::mr::WDRSTEN::set(mr);
        }

        if (interrupt_enable) {
            mr = wdt::mr::WDFIEN::set(mr);
        }

        wdt->MR = mr;
    }

    /**
     * @brief Set interrupt priority
     *
     * @param priority Interrupt priority (0-15, 0=highest)
     */
    static void set_interrupt_priority(u8 priority) {
        // WDT interrupt is peripheral ID 4 on SAME70
        // RSWDT interrupt is peripheral ID 5
        // TODO: Configure NVIC priority
        // This would require access to NVIC, which should be in interrupt.hpp
        (void)priority;  // Suppress unused warning for now
    }

    /**
     * @brief Get remaining time before timeout
     *
     * @return Remaining time in milliseconds (0 if not supported)
     */
    static u16 get_remaining_time_ms() {
        // SAME70 WDT doesn't provide a way to read the current counter value
        return 0;
    }

private:
    static inline auto* get_wdt() {
        return reinterpret_cast<volatile wdt::WDT_Registers*>(WDT_BASE);
    }
};

}  // namespace alloy::hal::atmel::same70
