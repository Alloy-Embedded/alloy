/// @file hal/comp/lite.hpp
/// STM32 Analog Comparator (COMP) — alloy.device.v2.1 lite driver.
///
/// Address-template variant — no codegen format required.  Each comparator
/// unit has exactly one CSR register at its base address.
///
/// @code
///   #include "hal/comp.hpp"
///
///   // COMP1 on STM32G4 at 0x40010200, rising threshold = VREFINT
///   using Comp1 = alloy::hal::comp::lite::engine<0x40010200u>;
///
///   Comp1::configure(
///       alloy::hal::comp::lite::InvertingInput::VRef,
///       alloy::hal::comp::lite::Hysteresis::Low);
///   Comp1::enable();
///
///   if (Comp1::output()) { /* INP > VREFINT */ }
/// @endcode
///
/// Base addresses (STM32G4 — each unit is 4 bytes apart on APB2):
///   COMP1 : 0x40010200
///   COMP2 : 0x40010204
///   COMP3 : 0x40010208
///   COMP4 : 0x4001020C
///   COMP5 : 0x40010210
///   COMP6 : 0x40010214
///   COMP7 : 0x40010218
///
/// Base addresses (STM32G0 / L4 — same layout):
///   COMP1 : 0x40010200
///   COMP2 : 0x40010204
///
/// CSR register layout (one register per comparator unit):
///   bit  0      EN        — comparator enable
///   bits 4:2    INMSEL    — inverting input selection (see InvertingInput)
///   bit  8      INPSEL    — non-inverting input (0=INP1, 1=INP2)
///   bit  15     POL       — output polarity (0=normal, 1=inverted)
///   bits 17:16  HYST      — hysteresis (see Hysteresis)
///   bits 20:18  BLANKSEL  — blanking source (3-bit, device-specific)
///   bit  30     VALUE     — comparator output (read-only)
///   bit  31     LOCK      — CSR write-protect (set-only; clears on reset)
#pragma once

#include <cstdint>

namespace alloy::hal::comp::lite {

namespace detail {

// CSR bit positions
inline constexpr std::uint32_t kCsrEn           = 1u << 0;
inline constexpr std::uint32_t kCsrInmselShift  = 2u;
inline constexpr std::uint32_t kCsrInmselMask   = 0x7u << 2;
inline constexpr std::uint32_t kCsrInpselShift  = 8u;
inline constexpr std::uint32_t kCsrInpselMask   = 1u << 8;
inline constexpr std::uint32_t kCsrPol          = 1u << 15;
inline constexpr std::uint32_t kCsrHystShift    = 16u;
inline constexpr std::uint32_t kCsrHystMask     = 0x3u << 16;
inline constexpr std::uint32_t kCsrBlanksShift  = 18u;
inline constexpr std::uint32_t kCsrBlanksMask   = 0x7u << 18;
inline constexpr std::uint32_t kCsrValue        = 1u << 30;
inline constexpr std::uint32_t kCsrLock         = 1u << 31;

}  // namespace detail

// ----------------------------------------------------------------------------
// Enumerations
// ----------------------------------------------------------------------------

/// Inverting (-) input selection (INMSEL[2:0] in CSR).
enum class InvertingInput : std::uint8_t {
    VRef1_4 = 0,  ///< 1/4 × VREFINT
    VRef1_2 = 1,  ///< 1/2 × VREFINT
    VRef3_4 = 2,  ///< 3/4 × VREFINT
    VRef    = 3,  ///< VREFINT (1.2 V nominal)
    DAC1Ch1 = 4,  ///< DAC1 channel 1 output
    DAC1Ch2 = 5,  ///< DAC1 channel 2 output
    GPIO1   = 6,  ///< INM1 GPIO pin (comparator-specific; see pinout table)
    GPIO2   = 7,  ///< INM2 GPIO pin
};

/// Non-inverting (+) input selection (INPSEL bit in CSR).
enum class NonInvertingInput : std::uint8_t {
    INP1 = 0,  ///< INP1 GPIO pin (default)
    INP2 = 1,  ///< INP2 GPIO pin (not available on all comparators)
};

/// Input hysteresis (HYST[1:0] in CSR).
enum class Hysteresis : std::uint8_t {
    None   = 0,  ///< No hysteresis (0 mV)
    Low    = 1,  ///< Low hysteresis (~10 mV)
    Medium = 2,  ///< Medium hysteresis (~20 mV)
    High   = 3,  ///< High hysteresis (~40 mV)
};

// ----------------------------------------------------------------------------
// engine<CompBase>
// ----------------------------------------------------------------------------

/// STM32 analog comparator — address-template variant.
///
/// `CompBase` is the base address of the specific comparator unit
/// (e.g. 0x40010200 for COMP1 on G4).  The CSR register is at `CompBase+0`.
template <std::uintptr_t CompBase>
class engine {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 COMP (analog comparator) — G4/H7 (single CSR per unit)
   //
   // CSR bit encoding (INMSEL, INPSEL, POL, HYST, BLANKSEL, VALUE, LOCK)
   // is STM32-specific.  Do NOT use for SAME70 ACC or nRF52 COMP.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/comp/lite.hpp: COMP layout is STM32-only. "
        "Use a vendor-specific comparator driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto csr() noexcept -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(CompBase);
    }

   public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Configure the comparator (does not enable — call `enable()` after).
    ///
    /// @param inv    Inverting (-) input source.
    /// @param hyst   Hysteresis level.
    /// @param noninv Non-inverting (+) input (INP1 or INP2).
    /// @param invert true → invert the comparator output (POL=1).
    static void configure(InvertingInput      inv,
                          Hysteresis          hyst   = Hysteresis::None,
                          NonInvertingInput   noninv = NonInvertingInput::INP1,
                          bool                invert = false) noexcept {
        std::uint32_t val = csr();
        // Clear all configurable bits first
        val &= ~(detail::kCsrInmselMask |
                 detail::kCsrInpselMask |
                 detail::kCsrPol        |
                 detail::kCsrHystMask);

        val |= (static_cast<std::uint32_t>(inv)    << detail::kCsrInmselShift);
        val |= (static_cast<std::uint32_t>(noninv) << detail::kCsrInpselShift);
        val |= (static_cast<std::uint32_t>(hyst)   << detail::kCsrHystShift);
        if (invert) { val |= detail::kCsrPol; }

        csr() = val;
    }

    /// Enable the comparator (CSR.EN = 1).
    ///
    /// The GPIO pins used as inputs must already be configured in analog mode.
    /// After enabling, a short settling time (a few µs) is needed before the
    /// output is valid.
    static void enable() noexcept {
        csr() |= detail::kCsrEn;
    }

    /// Disable the comparator (CSR.EN = 0, lowest power).
    static void disable() noexcept {
        csr() &= ~detail::kCsrEn;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// Read the current comparator output level (CSR.VALUE, bit 30).
    ///
    /// Returns true when INP > INM (before polarity inversion).
    /// If POL=1 (inverted output), the hardware bit is already inverted —
    /// this method always returns the physical output level seen on the pin.
    [[nodiscard]] static auto output() noexcept -> bool {
        return (csr() & detail::kCsrValue) != 0u;
    }

    // -----------------------------------------------------------------------
    // Output polarity
    // -----------------------------------------------------------------------

    /// Set output polarity (POL bit).
    ///
    /// When `inverted=true`, the output and VALUE bit are inverted
    /// relative to the comparator result (INP > INM → output low).
    static void set_inverted_output(bool inverted) noexcept {
        if (inverted) { csr() |=  detail::kCsrPol; }
        else          { csr() &= ~detail::kCsrPol; }
    }

    // -----------------------------------------------------------------------
    // Blanking (PWM motor control)
    // -----------------------------------------------------------------------

    /// Set the blanking source for the comparator output (BLANKSEL[2:0]).
    ///
    /// Blanking suppresses the comparator output during PWM switching
    /// noise windows.  The encoding is device-specific — refer to the
    /// reference manual blanking source table for your MCU.
    ///
    /// @param source  3-bit blanking source code (0 = blanking disabled).
    static void set_blanking_source(std::uint8_t source) noexcept {
        csr() = (csr() & ~detail::kCsrBlanksMask) |
                ((static_cast<std::uint32_t>(source) & 0x7u) << detail::kCsrBlanksShift);
    }

    // -----------------------------------------------------------------------
    // Lock
    // -----------------------------------------------------------------------

    /// Lock the CSR register (write-protect until next reset).
    ///
    /// Once locked, no further writes to CSR take effect until the MCU
    /// is reset.  Typically called after final production configuration.
    static void lock() noexcept {
        csr() |= detail::kCsrLock;
    }

    /// True when CSR is write-locked.
    [[nodiscard]] static auto is_locked() noexcept -> bool {
        return (csr() & detail::kCsrLock) != 0u;
    }

    engine() = delete;
};

}  // namespace alloy::hal::comp::lite
