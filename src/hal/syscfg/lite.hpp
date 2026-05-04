/// @file hal/syscfg/lite.hpp
/// SYSCFG (System Configuration Controller) — alloy.device.v2.1 lite driver.
///
/// Thin wrapper around the SYSCFG EXTICR registers that route GPIO port pins
/// to EXTI lines.  Must be called before enabling EXTI on GPIO-sourced lines.
///
/// The SYSCFG peripheral clock is usually gated on by default; for parts where
/// it requires explicit enable, call `dev::peripheral_on<dev::syscfg>()` first.
///
/// Address-template variant — no codegen format required:
/// @code
///   #include "hal/syscfg.hpp"
///   using Syscfg = alloy::hal::syscfg::lite::controller<0x40010000u>;
///
///   // Route PB7 to EXTI7:
///   Syscfg::connect_exti(7, alloy::hal::syscfg::lite::GpioPort::PB);
/// @endcode
///
/// Common SYSCFG base addresses:
///   STM32F0 / F3 / G0 / G4 / L4 / WB : 0x40010000  (APB2)
///   STM32H7                            : 0x58000400
///   STM32U5                            : 0x46000400
///
/// EXTICR register layout (offsets from SYSCFG base):
///   0x08  EXTICR1  — EXTI0–3   port select (4 bits per line)
///   0x0C  EXTICR2  — EXTI4–7
///   0x10  EXTICR3  — EXTI8–11
///   0x14  EXTICR4  — EXTI12–15
///
/// Each 4-bit field: 0=PA, 1=PB, 2=PC, 3=PD, 4=PE, 5=PF, 6=PG, 7=PH, …
/// Only lines 0–15 are GPIO-routable; higher lines are fixed to internal sources.
#pragma once

#include <cstdint>

namespace alloy::hal::syscfg::lite {

namespace detail {

/// Offset of EXTICR1 from the SYSCFG base address.
inline constexpr std::uintptr_t kExticrBase = 0x08u;

}  // namespace detail

// ----------------------------------------------------------------------------
// GPIO port enumeration
// ----------------------------------------------------------------------------

/// GPIO port index used to select which port drives an EXTI line.
enum class GpioPort : std::uint8_t {
    PA =  0,  ///< GPIOA
    PB =  1,  ///< GPIOB
    PC =  2,  ///< GPIOC
    PD =  3,  ///< GPIOD
    PE =  4,  ///< GPIOE
    PF =  5,  ///< GPIOF
    PG =  6,  ///< GPIOG
    PH =  7,  ///< GPIOH
    PI =  8,  ///< GPIOI (H7/U5)
    PJ =  9,  ///< GPIOJ (H7/U5)
    PK = 10,  ///< GPIOK (H7/U5)
};

// ----------------------------------------------------------------------------
// controller<SyscfgBase>
// ----------------------------------------------------------------------------

/// SYSCFG controller — address-template variant.
///
/// `SyscfgBase` is the SYSCFG peripheral base address (see file header).
template <std::uintptr_t SyscfgBase>
class controller {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 SYSCFG (system configuration controller) — G0/G4/L4/WB/H7
   //
   // EXTICR encoding (4 bits per line, 4 lines per register) is STM32-specific.
   // Do NOT use for SAME70 PIO multiplexer or nRF52 pin-select registers.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/syscfg/lite.hpp: SYSCFG layout is STM32-only. "
        "Use a vendor-specific pin-mux driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(SyscfgBase + ofs);
    }

   public:
    // -----------------------------------------------------------------------
    // EXTI source routing
    // -----------------------------------------------------------------------

    /// Route a GPIO port to the EXTI line matching the pin number.
    ///
    /// EXTI line == GPIO pin number: PA3 and PB3 both map to EXTI3 —
    /// use this function to select which port actually drives that line.
    ///
    /// @param line  EXTI line / pin number (0–15).
    /// @param port  GPIO port to connect to the line.
    static void connect_exti(std::uint8_t line, GpioPort port) noexcept {
        // EXTICR[n] covers lines [4n … 4n+3], 4 bits per line.
        const std::uint32_t reg_index = static_cast<std::uint32_t>(line) / 4u;
        const std::uint32_t bit_pos   = (static_cast<std::uint32_t>(line) % 4u) * 4u;
        const std::uintptr_t ofs      = detail::kExticrBase + reg_index * 4u;

        reg(ofs) = (reg(ofs) & ~(0xFu << bit_pos)) |
                   (static_cast<std::uint32_t>(port) << bit_pos);
    }

    /// Read back the GPIO port currently routed to EXTI `line` (0–15).
    [[nodiscard]] static auto get_exti_source(std::uint8_t line) noexcept -> GpioPort {
        const std::uint32_t reg_index = static_cast<std::uint32_t>(line) / 4u;
        const std::uint32_t bit_pos   = (static_cast<std::uint32_t>(line) % 4u) * 4u;
        const std::uintptr_t ofs      = detail::kExticrBase + reg_index * 4u;

        return static_cast<GpioPort>((reg(ofs) >> bit_pos) & 0xFu);
    }

    controller() = delete;
};

}  // namespace alloy::hal::syscfg::lite
