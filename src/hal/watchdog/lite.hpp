/// @file hal/watchdog/lite.hpp
/// Lightweight, direct-MMIO IWDG/WWDG driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
///
/// Supports:
///   StIwdg  — STM32 IWDG (all families: F0/F4/G0/G4/H7/…)
///     kTemplate = "iwdg", kIpVersion = "iwdg1_v1_1" or "iwdg1_v2_0"
///     v2_0 adds WINR (window register) — available on most modern parts.
///
///   StWwdg  — STM32 WWDG (windowed watchdog, clocked from APB1)
///     kTemplate = "wwdg", kIpVersion = "wwdg1_v…"
///
/// IWDG source clock: always internal LSI (32 kHz nominal; ~17–47 kHz range).
/// Timeout ≈ (reload + 1) × prescaler_div / LSI_Hz
///
/// Quick-start (IWDG at ~1 s timeout, assuming LSI ≈ 32 kHz):
/// @code
///   #include "hal/watchdog.hpp"
///   using Wdg = alloy::hal::watchdog::lite::iwdg_port<dev::iwdg>;
///
///   Wdg::start(alloy::hal::watchdog::lite::IwdgPrescaler::Div32, 999u);
///   // reload = 999, div = 32 → timeout ≈ 1000 × 32 / 32000 = 1.0 s
///   ...
///   Wdg::refresh();   // called periodically to prevent reset
/// @endcode
///
/// WWDG quick-start (timeout driven by APB1 clock):
/// @code
///   using Wdg = alloy::hal::watchdog::lite::wwdg_port<dev::wwdg>;
///   Wdg::configure(WwdgPrescaler::Div8, 0x7Fu, 0x50u); // window = 0x50
///   Wdg::enable();
///   ...
///   Wdg::refresh(0x7Fu);   // write counter before it falls below window
/// @endcode
#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::watchdog::lite {

// ============================================================================
// Detail
// ============================================================================

namespace detail {

// IWDG register offsets (same for v1_1 and v2_0)
inline constexpr std::uintptr_t kIwdgOfsKr  = 0x00u;  ///< Key register (wo)
inline constexpr std::uintptr_t kIwdgOfsPr  = 0x04u;  ///< Prescaler register
inline constexpr std::uintptr_t kIwdgOfsRlr = 0x08u;  ///< Reload register
inline constexpr std::uintptr_t kIwdgOfsSr  = 0x0Cu;  ///< Status register (ro)
inline constexpr std::uintptr_t kIwdgOfsWinr= 0x10u;  ///< Window register (v2_0 only)

// KR magic values
inline constexpr std::uint32_t kIwdgKrRefresh = 0xAAAAu;  ///< Pet the dog
inline constexpr std::uint32_t kIwdgKrUnlock  = 0x5555u;  ///< Enable PR/RLR write
inline constexpr std::uint32_t kIwdgKrStart   = 0xCCCCu;  ///< Start IWDG (irreversible)

// SR bits
inline constexpr std::uint32_t kIwdgSrPvu = 1u << 0;  ///< Prescaler update busy
inline constexpr std::uint32_t kIwdgSrRvu = 1u << 1;  ///< Reload update busy
inline constexpr std::uint32_t kIwdgSrWvu = 1u << 2;  ///< Window update busy (v2_0)

// WWDG register offsets
inline constexpr std::uintptr_t kWwdgOfsCr  = 0x00u;  ///< Control register
inline constexpr std::uintptr_t kWwdgOfsCfr = 0x04u;  ///< Configuration register
inline constexpr std::uintptr_t kWwdgOfsSr  = 0x08u;  ///< Status register

// WWDG CR bits
inline constexpr std::uint32_t kWwdgCrWdga  = 1u << 7;   ///< Activation bit (irreversible)
inline constexpr std::uint32_t kWwdgCrTMask = 0x7Fu;      ///< T[6:0] counter

// WWDG CFR bits
inline constexpr std::uint32_t kWwdgCfrWdgtbMask  = 3u << 7;   ///< WDGTB[8:7] prescaler
inline constexpr std::uint32_t kWwdgCfrWdgtbShift = 7u;
inline constexpr std::uint32_t kWwdgCfrEwi         = 1u << 9;   ///< Early wakeup interrupt
inline constexpr std::uint32_t kWwdgCfrWMask        = 0x7Fu;     ///< W[6:0] window

[[nodiscard]] consteval auto is_iwdg(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "iwdg";
}
[[nodiscard]] consteval auto is_wwdg(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "wwdg";
}
[[nodiscard]] consteval auto is_iwdg_v2(const char* ip) -> bool {
    return std::string_view{ip}.starts_with("iwdg1_v2");
}

}  // namespace detail

// ============================================================================
// Concepts
// ============================================================================

template <typename P>
concept StIwdg =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_iwdg(P::kTemplate);

template <typename P>
concept StWwdg =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_wwdg(P::kTemplate);

// ============================================================================
// IWDG prescaler
// ============================================================================

/// IWDG prescaler divider.  Timeout = (reload+1) × div / LSI_Hz.
enum class IwdgPrescaler : std::uint8_t {
    Div4   = 0,  ///< ÷4   — max timeout ≈  512 ms @ 32 kHz LSI
    Div8   = 1,  ///< ÷8   — max timeout ≈  1.0 s
    Div16  = 2,  ///< ÷16  — max timeout ≈  2.0 s
    Div32  = 3,  ///< ÷32  — max timeout ≈  4.1 s
    Div64  = 4,  ///< ÷64  — max timeout ≈  8.2 s
    Div128 = 5,  ///< ÷128 — max timeout ≈ 16.4 s
    Div256 = 6,  ///< ÷256 — max timeout ≈ 32.8 s
};

// ============================================================================
// WWDG prescaler
// ============================================================================

/// WWDG prescaler.  Timeout = (T[5:0]+1) × 4096 × div / APB1_Hz.
enum class WwdgPrescaler : std::uint8_t {
    Div1 = 0,
    Div2 = 1,
    Div4 = 2,
    Div8 = 3,
};

// ============================================================================
// iwdg_port<P>
// ============================================================================

/// Direct-MMIO IWDG driver.  P must satisfy StIwdg.
///
/// IMPORTANT: once `start()` is called, the IWDG cannot be stopped.
/// Calling `start()` again with different parameters is only valid if the
/// IWDG has not been started yet or if the MCU is being reset.
template <typename P>
    requires StIwdg<P>
class iwdg_port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

    /// True if this IWDG instance supports the window register (v2_0).
    static constexpr bool kHasWindow =
        requires { { P::kIpVersion } -> std::convertible_to<const char*>; } &&
        detail::is_iwdg_v2(P::kIpVersion);

   private:
    [[nodiscard]] static auto reg(std::uintptr_t offset) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + offset);
    }

    static void unlock() noexcept {
        reg(detail::kIwdgOfsKr) = detail::kIwdgKrUnlock;
    }

   public:
    /// Configure prescaler + reload value and start the IWDG.
    ///
    /// Waits for PR and RLR update flags to clear before returning, ensuring
    /// the values are latched into the IWDG clock domain.
    ///
    /// @param psc     Prescaler divider (see IwdgPrescaler).
    /// @param reload  12-bit reload value (0–4095).  Must be > 0.
    static void start(IwdgPrescaler psc, std::uint16_t reload) noexcept {
        unlock();
        reg(detail::kIwdgOfsPr)  = static_cast<std::uint32_t>(psc) & 0x7u;
        reg(detail::kIwdgOfsRlr) = static_cast<std::uint32_t>(reload) & 0xFFFu;
        // Wait for the values to be taken into the LSI clock domain.
        while ((reg(detail::kIwdgOfsSr) &
                (detail::kIwdgSrPvu | detail::kIwdgSrRvu)) != 0u) { /* spin */ }
        reg(detail::kIwdgOfsKr) = detail::kIwdgKrStart;
    }

    /// Set the WINR window value (v2_0 IWDG only).
    ///
    /// Refresh is only valid when the counter is between `window` and the
    /// RLR reload value.  Refreshing outside this window triggers an
    /// immediate reset.
    ///
    /// Must be called after `start()` (KR=0x5555 unlock is needed again).
    ///
    /// @param window  12-bit window value (must be ≤ reload).
    static void set_window(std::uint16_t window) noexcept
        requires (kHasWindow)
    {
        unlock();
        reg(detail::kIwdgOfsWinr) = static_cast<std::uint32_t>(window) & 0xFFFu;
        while ((reg(detail::kIwdgOfsSr) & detail::kIwdgSrWvu) != 0u) { /* spin */ }
    }

    /// Reload the IWDG counter (pet the watchdog).
    ///
    /// Call this more frequently than the configured timeout to prevent reset.
    static void refresh() noexcept {
        reg(detail::kIwdgOfsKr) = detail::kIwdgKrRefresh;
    }

    /// Returns true while any update register is busy propagating to the
    /// LSI clock domain (PR, RLR, or WINR).
    [[nodiscard]] static auto busy() noexcept -> bool {
        constexpr std::uint32_t kAllBusy =
            detail::kIwdgSrPvu | detail::kIwdgSrRvu |
            (kHasWindow ? detail::kIwdgSrWvu : 0u);
        return (reg(detail::kIwdgOfsSr) & kAllBusy) != 0u;
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    /// Note: IWDG has an early-wakeup IRQ only on select STM32 families.
    /// The `if constexpr` guard handles devices where the field is absent.
    [[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
        -> std::uint32_t {
        if constexpr (requires { P::kIrqLines[0]; }) {
            return static_cast<std::uint32_t>(P::kIrqLines[idx]);
        } else {
            static_assert(sizeof(P) == 0,
                "P::kIrqLines not present; upgrade device artifact to v2.1");
            return 0u;
        }
    }

    /// Returns the number of IRQ lines for this peripheral.
    /// Sourced from `P::kIrqCount` (flat-struct v2.1). Returns 0 if absent.
    [[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
        if constexpr (requires { P::kIrqCount; }) {
            return static_cast<std::size_t>(P::kIrqCount);
        }
        return 0u;
    }

    // -----------------------------------------------------------------------
    // Clock gate — sourced from alloy.device.v2.1 flat-struct kRccEnable
    // -----------------------------------------------------------------------

    /// Enable the peripheral clock and deassert reset (if kRccReset present).
    static void clock_on() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) |= P::kRccEnable.mask;
        }
        if constexpr (requires { P::kRccReset; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) |=  P::kRccReset.mask;
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) &= ~P::kRccReset.mask;
        }
    }

    /// Disable the peripheral clock.
    static void clock_off() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) &= ~P::kRccEnable.mask;
        }
    }

    iwdg_port() = delete;
};

// ============================================================================
// wwdg_port<P>
// ============================================================================

/// Direct-MMIO WWDG driver.  P must satisfy StWwdg.
///
/// WWDG is clocked from the APB1 bus (after prescaler).  Unlike IWDG, it
/// can be enabled and disabled in software, but once enabled via the WDGA
/// bit it cannot be disabled again until the next reset.
///
/// Typical use: enable WWDG late in init (after all peripherals are ready)
/// to catch hangs in the main loop.  Refresh once per loop iteration.
template <typename P>
    requires StWwdg<P>
class wwdg_port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

   private:
    [[nodiscard]] static auto reg(std::uintptr_t offset) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + offset);
    }

   public:
    /// Configure the WWDG (does not enable it yet — WDGA is not set).
    ///
    /// @param psc      Prescaler (see WwdgPrescaler).
    /// @param counter  Initial counter value T[6:0].  Bit 6 must be set (0x40–0x7F).
    /// @param window   Window value W[6:0].  Refresh is valid when T > window.
    static void configure(WwdgPrescaler psc,
                          std::uint8_t  counter,
                          std::uint8_t  window) noexcept {
        clock_on();
        reg(detail::kWwdgOfsCr) = static_cast<std::uint32_t>(counter) & detail::kWwdgCrTMask;
        reg(detail::kWwdgOfsCfr) =
            (static_cast<std::uint32_t>(psc) << detail::kWwdgCfrWdgtbShift) |
            (static_cast<std::uint32_t>(window) & detail::kWwdgCfrWMask);
    }

    /// Enable the WWDG by setting WDGA (irreversible until next reset).
    static void enable() noexcept {
        reg(detail::kWwdgOfsCr) |= detail::kWwdgCrWdga;
    }

    /// Enable the early-wakeup interrupt (EWI in CFR, cleared in SR).
    static void enable_early_wakeup_interrupt() noexcept {
        reg(detail::kWwdgOfsCfr) |= detail::kWwdgCfrEwi;
    }

    /// Clear the early-wakeup interrupt flag (call inside EWI ISR).
    static void clear_early_wakeup_flag() noexcept {
        reg(detail::kWwdgOfsSr) = 0u;
    }

    /// Refresh the WWDG by writing a new counter value.
    ///
    /// Must be called while T > window (i.e. within the valid refresh window).
    /// Bit 6 of `counter` must be set (value in range 0x40–0x7F); writing a
    /// value with bit 6 clear immediately triggers a reset.
    ///
    /// @param counter  New T[6:0] counter value (0x40–0x7F).
    static void refresh(std::uint8_t counter) noexcept {
        reg(detail::kWwdgOfsCr) =
            detail::kWwdgCrWdga |
            (static_cast<std::uint32_t>(counter) & detail::kWwdgCrTMask);
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    /// Compiles to a constexpr constant — zero overhead.
    [[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
        -> std::uint32_t {
        if constexpr (requires { P::kIrqLines[0]; }) {
            return static_cast<std::uint32_t>(P::kIrqLines[idx]);
        } else {
            static_assert(sizeof(P) == 0,
                "P::kIrqLines not present; upgrade device artifact to v2.1");
            return 0u;
        }
    }

    /// Returns the number of IRQ lines for this peripheral.
    /// Sourced from `P::kIrqCount` (flat-struct v2.1). Returns 0 if absent.
    [[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
        if constexpr (requires { P::kIrqCount; }) {
            return static_cast<std::size_t>(P::kIrqCount);
        }
        return 0u;
    }

    // -----------------------------------------------------------------------
    // Clock gate — sourced from alloy.device.v2.1 flat-struct kRccEnable
    // -----------------------------------------------------------------------

    /// Enable the peripheral clock and deassert reset (if kRccReset present).
    static void clock_on() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) |= P::kRccEnable.mask;
        }
        if constexpr (requires { P::kRccReset; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) |=  P::kRccReset.mask;
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) &= ~P::kRccReset.mask;
        }
    }

    /// Disable the peripheral clock.
    static void clock_off() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) &= ~P::kRccEnable.mask;
        }
    }

    wwdg_port() = delete;
};

}  // namespace alloy::hal::watchdog::lite
