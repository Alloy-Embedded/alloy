/// @file hal/exti/lite.hpp
/// STM32 EXTI (External Interrupt/Event Controller) — alloy.device.v2.1 lite driver.
///
/// Address-template variant — no codegen format required.
///
/// Supports lines 0–31 (first EXTI bank). Lines 0–15 map 1:1 to GPIO pins
/// (configured via SYSCFG); higher lines connect to internal sources (RTC,
/// USB, comparators, etc. — device-specific).
///
/// Typical GPIO pin-change interrupt setup (PA0, rising edge):
/// @code
///   #include "hal/exti.hpp"
///   #include "hal/syscfg.hpp"
///
///   using Exti   = alloy::hal::exti::lite::controller<0x40010400u>;
///   using Syscfg = alloy::hal::syscfg::lite::controller<0x40010000u>;
///   using Nvic   = alloy::hal::nvic::lite::controller;
///
///   dev::peripheral_on<dev::gpioa>();
///   dev::peripheral_on<dev::syscfg>();
///   Gpioa::configure_input(0, Pull::Up);
///   Syscfg::connect_exti(0, alloy::hal::syscfg::lite::GpioPort::PA);
///   Exti::configure(0, alloy::hal::exti::lite::Edge::Rising);
///   Nvic::set_priority(6u, 128u);  // EXTI0 = IRQ 6 on G4
///   Nvic::enable_irq(6u);
///   Nvic::enable_all();
///
///   // ISR:
///   extern "C" void EXTI0_IRQHandler() {
///       if (Exti::pending(0)) { Exti::clear_pending(0); /* ... */ }
///   }
/// @endcode
///
/// Base addresses:
///   STM32F0 / F3 / G0 / G4 / L4 / WB : 0x40010400
///   STM32H7 (EXTI1)                    : 0x58000000
///   STM32U5                            : 0x46021800
///
/// Register layout (lines 0–31, F0 / G0 / G4 / L4 / WB):
///   0x00  IMR1   — Interrupt mask register 1  (1 = interrupt unmasked)
///   0x04  EMR1   — Event mask register 1      (1 = event unmasked)
///   0x08  RTSR1  — Rising-edge trigger selection  (1 = rising enabled)
///   0x0C  FTSR1  — Falling-edge trigger selection (1 = falling enabled)
///   0x10  SWIER1 — Software interrupt event    (write 1 to force trigger)
///   0x14  PR1    — Pending register            (write 1 to clear flag)
///
/// Note: STM32H7 has a different register map (RTSR/FTSR/RPR/FPR instead of
/// PR, separate IMR/EMR per CPU).  Use this driver only for the classic layout.
#pragma once

#include <cstdint>

namespace alloy::hal::exti::lite {

namespace detail {

inline constexpr std::uintptr_t kOfsImr1   = 0x00u;  ///< Interrupt mask
inline constexpr std::uintptr_t kOfsEmr1   = 0x04u;  ///< Event mask
inline constexpr std::uintptr_t kOfsRtsr1  = 0x08u;  ///< Rising-edge trigger
inline constexpr std::uintptr_t kOfsFtsr1  = 0x0Cu;  ///< Falling-edge trigger
inline constexpr std::uintptr_t kOfsSwier1 = 0x10u;  ///< Software trigger (w)
inline constexpr std::uintptr_t kOfsPr1    = 0x14u;  ///< Pending flags (rc_w1)

}  // namespace detail

// ----------------------------------------------------------------------------
// Edge trigger selector
// ----------------------------------------------------------------------------

/// Edge(s) that trigger the EXTI line.
enum class Edge : std::uint8_t {
    None    = 0,  ///< No hardware trigger (disable RTSR + FTSR for this line)
    Rising  = 1,  ///< Rising edge only
    Falling = 2,  ///< Falling edge only
    Both    = 3,  ///< Both rising and falling edges
};

// ----------------------------------------------------------------------------
// controller<ExtiBase>
// ----------------------------------------------------------------------------

/// STM32 EXTI controller — address-template variant.
///
/// `ExtiBase` is the peripheral base address (see file header for values).
/// All methods target the 32-bit EXTI banks 1 (lines 0–31).
template <std::uintptr_t ExtiBase>
class controller {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 EXTI (F0/F1/F3/G0/G4/L4/WB/H7)
   //
   // Register layout: IMR1/EMR1/RTSR1/FTSR1/SWIER1/PR1 at fixed offsets.
   // Do NOT use this driver for SAME70 PIO/PMC, nRF52 GPIOTE, or RP2040 IO —
   // they have incompatible interrupt-line routing.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/exti/lite.hpp: EXTI layout is STM32-only. "
        "Use a vendor-specific interrupt driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(ExtiBase + ofs);
    }

    [[nodiscard]] static constexpr auto bit(std::uint8_t line) noexcept
        -> std::uint32_t {
        return std::uint32_t{1u} << line;
    }

   public:
    // -----------------------------------------------------------------------
    // Full configure
    // -----------------------------------------------------------------------

    /// Configure edge trigger and interrupt/event masks for one EXTI line.
    ///
    /// For GPIO lines (0–15) route the desired port with
    /// `alloy::hal::syscfg::lite::controller::connect_exti()` first.
    ///
    /// @param line   EXTI line number (0–31).
    /// @param edge   Trigger edge(s).  `Edge::None` disables RTSR+FTSR.
    /// @param irq    true → unmask interrupt (IMR1 bit set).
    /// @param event  true → unmask event   (EMR1 bit set; used for WFE).
    static void configure(std::uint8_t line,
                          Edge         edge,
                          bool         irq   = true,
                          bool         event = false) noexcept {
        const std::uint32_t b = bit(line);

        // Rising trigger
        if ((static_cast<std::uint8_t>(edge) & 0x1u) != 0u) {
            reg(detail::kOfsRtsr1) |= b;
        } else {
            reg(detail::kOfsRtsr1) &= ~b;
        }
        // Falling trigger
        if ((static_cast<std::uint8_t>(edge) & 0x2u) != 0u) {
            reg(detail::kOfsFtsr1) |= b;
        } else {
            reg(detail::kOfsFtsr1) &= ~b;
        }
        // Interrupt mask
        if (irq) {
            reg(detail::kOfsImr1) |= b;
        } else {
            reg(detail::kOfsImr1) &= ~b;
        }
        // Event mask
        if (event) {
            reg(detail::kOfsEmr1) |= b;
        } else {
            reg(detail::kOfsEmr1) &= ~b;
        }
    }

    // -----------------------------------------------------------------------
    // Interrupt mask
    // -----------------------------------------------------------------------

    /// Unmask (enable) the interrupt for EXTI `line`.
    static void enable_irq(std::uint8_t line) noexcept {
        reg(detail::kOfsImr1) |= bit(line);
    }

    /// Mask (disable) the interrupt for EXTI `line`.
    static void disable_irq(std::uint8_t line) noexcept {
        reg(detail::kOfsImr1) &= ~bit(line);
    }

    // -----------------------------------------------------------------------
    // Edge triggers
    // -----------------------------------------------------------------------

    /// Enable or disable rising-edge trigger for `line`.
    static void set_rising_trigger(std::uint8_t line, bool enable) noexcept {
        if (enable) { reg(detail::kOfsRtsr1) |=  bit(line); }
        else        { reg(detail::kOfsRtsr1) &= ~bit(line); }
    }

    /// Enable or disable falling-edge trigger for `line`.
    static void set_falling_trigger(std::uint8_t line, bool enable) noexcept {
        if (enable) { reg(detail::kOfsFtsr1) |=  bit(line); }
        else        { reg(detail::kOfsFtsr1) &= ~bit(line); }
    }

    // -----------------------------------------------------------------------
    // Event mask
    // -----------------------------------------------------------------------

    /// Unmask event output for `line` (wakes CPU from WFE on trigger).
    static void enable_event(std::uint8_t line) noexcept {
        reg(detail::kOfsEmr1) |= bit(line);
    }

    /// Mask event output for `line`.
    static void disable_event(std::uint8_t line) noexcept {
        reg(detail::kOfsEmr1) &= ~bit(line);
    }

    // -----------------------------------------------------------------------
    // Pending / software trigger
    // -----------------------------------------------------------------------

    /// True when EXTI `line` has a pending interrupt request (PR1 bit set).
    [[nodiscard]] static auto pending(std::uint8_t line) noexcept -> bool {
        return (reg(detail::kOfsPr1) & bit(line)) != 0u;
    }

    /// Clear the pending flag for EXTI `line` (write 1 to PR1).
    ///
    /// Must be called inside the ISR before returning to prevent
    /// the interrupt from re-triggering immediately.
    static void clear_pending(std::uint8_t line) noexcept {
        reg(detail::kOfsPr1) = bit(line);  // rc_w1: write 1 to clear
    }

    /// Trigger EXTI `line` in software (set SWIER1 bit).
    ///
    /// If the line interrupt is enabled, this will set the PR1 pending bit
    /// and fire the ISR.  The SWIER1 bit is cleared by hardware when PR1
    /// is cleared.
    static void software_trigger(std::uint8_t line) noexcept {
        reg(detail::kOfsSwier1) |= bit(line);
    }

    controller() = delete;
};

}  // namespace alloy::hal::exti::lite
