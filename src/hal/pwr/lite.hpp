/// @file hal/pwr/lite.hpp
/// Power controller (PWR) — alloy.device.v2.1 lite driver.
///
/// Sleep / Stop / Standby mode entry helpers and wakeup-flag management.
/// Works alongside the Cortex-M System Control Register (SCB.SCR) which
/// controls whether WFI/WFE enter normal Sleep or deep Sleep.
///
/// Address-template variant — no codegen format required:
/// @code
///   #include "hal/pwr/lite.hpp"
///
///   // STM32G0 / G4 / F4 / L4 PWR base = 0x40007000
///   using Pwr = alloy::hal::pwr::lite::controller<0x40007000u>;
///
///   // Enter Sleep (CPU halts, peripherals continue)
///   Pwr::sleep_on_wfi();
///
///   // Enter Stop 1 (main regulator off, SRAM retained)
///   Pwr::clear_wakeup_flags();
///   Pwr::stop(alloy::hal::pwr::lite::StopMode::Stop1);
/// @endcode
///
/// Register layout (STM32G0/G4/F4/L4/WB — 0x40007000):
///   0x00 CR1    — LPMS[2:0], FPDS, FPDSSTOP, DBP, VOS, LPR
///   0x04 CR2    — PVDE, PLS, PVMEN (not needed for mode entry)
///   0x08 CR3    — EWUP1..5, RRS, APC, EIWUL
///   0x0C CR4    — WP1..5, VBE, VBRS
///   0x10 SR1    — WUF1..5(4:0), SBF(8), WUFI(15)  [read-only]
///   0x14 SR2    — status [read-only]
///   0x18 SCR    — CWUF1..5(4:0), CSBF(8)  [write-1-to-clear]
///
/// Cortex-M SCB.SCR (0xE000_ED10):
///   SLEEPONEXIT(1), SLEEPDEEP(2), SEVONPEND(4)
#pragma once

#include <cstdint>

namespace alloy::hal::pwr::lite {

namespace detail {

// Cortex-M System Control Register — universal address
inline constexpr std::uintptr_t kScbScrAddr    = 0xE000ED10u;
inline constexpr std::uint32_t  kScrSleepDeep  = 1u << 2;
inline constexpr std::uint32_t  kScrSleepOnExit = 1u << 1;

// PWR CR1 LPMS encoding (STM32G0/G4/L4/WB)
inline constexpr std::uint32_t kCr1LpmsMask    = 0x7u;
inline constexpr std::uint32_t kLpmsStop0      = 0x0u;
inline constexpr std::uint32_t kLpmsStop1      = 0x1u;
inline constexpr std::uint32_t kLpmsStop2      = 0x2u;   ///< G0 / L4 only
inline constexpr std::uint32_t kLpmsStandby    = 0x3u;
inline constexpr std::uint32_t kLpmsShutdown   = 0x4u;

// PWR SR1 / SCR flags
inline constexpr std::uint32_t kWufMask        = 0x1Fu;  ///< WUF1..5 in SR1
inline constexpr std::uint32_t kSbfBit         = 1u << 8; ///< Standby flag in SR1
inline constexpr std::uint32_t kScrCwufAll     = 0x1Fu;  ///< Clear WUF1..5 in SCR
inline constexpr std::uint32_t kScrCsbf        = 1u << 8; ///< Clear SBF in SCR

// PWR register offsets
inline constexpr std::uintptr_t kCr1Ofs  = 0x00u;
inline constexpr std::uintptr_t kSr1Ofs  = 0x10u;
inline constexpr std::uintptr_t kScrOfs  = 0x18u;

[[nodiscard]] inline auto scb_scr() noexcept -> volatile std::uint32_t& {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<volatile std::uint32_t*>(kScbScrAddr);
}

}  // namespace detail

/// Low-power mode selector for `controller::stop()`.
///
/// Values map directly to PWR.CR1.LPMS[2:0].
enum class StopMode : std::uint32_t {
    Stop0    = detail::kLpmsStop0,    ///< All regulators on — fastest wakeup
    Stop1    = detail::kLpmsStop1,    ///< Main regulator off — lower power
    Stop2    = detail::kLpmsStop2,    ///< Low-power regulator only (G0/L4)
    Standby  = detail::kLpmsStandby,  ///< SRAM lost; wakeup via WKUP pins / RTC
    Shutdown = detail::kLpmsShutdown, ///< Vcore off — lowest power, no SRAM
};

/// Power mode controller.
///
/// `PwrBase` is the PWR peripheral base address:
///   - STM32G0 / G4 / F4 / L4 / WB : 0x40007000
///   - STM32H7                       : 0x58024800
template <std::uintptr_t PwrBase>
class controller {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 PWR (power controller) — G0/G4/L4/WB/H7
   //
   // CR1/CR2/CR3/SR1/SR2 layout and voltage-scaling / stop-mode encoding are
   // STM32-specific.  Do NOT use for SAME70 SUPC/PMC or nRF52 POWER module.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/pwr/lite.hpp: PWR controller layout is STM32-only. "
        "Use a vendor-specific power driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto pwr(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(PwrBase + ofs);
    }

   public:
    // -----------------------------------------------------------------------
    // Sleep mode (SLEEPDEEP = 0, CPU halts, all clocks/peripherals continue)
    // -----------------------------------------------------------------------

    /// Enter Sleep mode via WFI (wake on next interrupt).
    ///
    /// Clears SCB.SCR.SLEEPDEEP so the SoC does NOT enter the deep-sleep
    /// voltage domain — all peripheral clocks keep running.
    static void sleep_on_wfi() noexcept {
        detail::scb_scr() &= ~detail::kScrSleepDeep;
        __asm volatile("dsb" ::: "memory");
        __asm volatile("wfi" ::: "memory");
    }

    /// Enter Sleep mode via WFE (wake on event or interrupt).
    static void sleep_on_wfe() noexcept {
        detail::scb_scr() &= ~detail::kScrSleepDeep;
        __asm volatile("dsb" ::: "memory");
        __asm volatile("wfe" ::: "memory");
    }

    /// Control SLEEPONEXIT: when true the core re-enters Sleep automatically
    /// after returning from each interrupt handler.
    ///
    /// Ideal for interrupt-driven designs with no background work — the CPU
    /// sleeps between interrupts without an explicit WFI in the main loop.
    static void set_sleep_on_exit(bool enable) noexcept {
        if (enable) {
            detail::scb_scr() |= detail::kScrSleepOnExit;
        } else {
            detail::scb_scr() &= ~detail::kScrSleepOnExit;
        }
    }

    // -----------------------------------------------------------------------
    // Deep-sleep modes (SLEEPDEEP = 1, clock tree stopped)
    // -----------------------------------------------------------------------

    /// Enter a deep-sleep power mode (Stop / Standby / Shutdown) via WFI.
    ///
    /// Sequence:
    ///   1. Write LPMS to PWR.CR1.
    ///   2. Set SCB.SCR.SLEEPDEEP = 1.
    ///   3. DSB + WFI.
    ///   4. On wakeup: clear SLEEPDEEP (so subsequent `sleep_on_wfi()` works).
    ///
    /// Caller must call `clear_wakeup_flags()` BEFORE entering to guarantee
    /// a clean entry when a wakeup source is already pending.
    ///
    /// @param mode  Target power mode (Stop0, Stop1, Stop2, Standby, Shutdown).
    static void stop(StopMode mode) noexcept {
        auto& cr1 = pwr(detail::kCr1Ofs);
        cr1 = (cr1 & ~detail::kCr1LpmsMask) | static_cast<std::uint32_t>(mode);
        detail::scb_scr() |= detail::kScrSleepDeep;
        __asm volatile("dsb" ::: "memory");
        __asm volatile("wfi" ::: "memory");
        // Clear SLEEPDEEP after wakeup so normal Sleep mode still works.
        detail::scb_scr() &= ~detail::kScrSleepDeep;
    }

    // -----------------------------------------------------------------------
    // Wakeup flags
    // -----------------------------------------------------------------------

    /// Clear all wakeup flags (WUF1..5) and the standby flag (SBF) via SCR.
    ///
    /// Must be called before entering Stop or Standby — if a wakeup source
    /// flag is already set the hardware will refuse to enter deep sleep.
    static void clear_wakeup_flags() noexcept {
        pwr(detail::kScrOfs) = detail::kScrCwufAll | detail::kScrCsbf;
    }

    /// True if any wakeup flag (WUF1..5) is set in SR1.
    [[nodiscard]] static auto any_wakeup_flag() noexcept -> bool {
        return (pwr(detail::kSr1Ofs) & detail::kWufMask) != 0u;
    }

    /// True if the standby flag (SBF) is set — the MCU returned from Standby.
    ///
    /// Check this early in `main()` to distinguish a cold boot from a Standby
    /// wakeup and restore any state saved to backup registers / SRAM.
    [[nodiscard]] static auto standby_flag() noexcept -> bool {
        return (pwr(detail::kSr1Ofs) & detail::kSbfBit) != 0u;
    }

    controller() = delete;
};

}  // namespace alloy::hal::pwr::lite
