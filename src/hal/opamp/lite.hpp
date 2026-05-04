/// @file hal/opamp/lite.hpp
/// STM32 OPAMP (Operational Amplifier) — alloy.device.v2.1 lite driver.
///
/// Address-template variant — no codegen format required.  Each OPAMP unit
/// has three registers (CSR, OTR, HSOTR) at its base address.
///
/// Available on: STM32G4 (6 OPAMPs), STM32G0 (2 OPAMPs, limited feature set),
/// STM32L4 (2 OPAMPs), STM32H7 (2 OPAMPs).
///
/// @code
///   #include "hal/opamp.hpp"
///
///   // OPAMP1 on STM32G4 at 0x40007800 — follower mode
///   using Op1 = alloy::hal::opamp::lite::engine<0x40007800u>;
///
///   Op1::configure_follower();          // output = INP1
///   Op1::enable();
///
///   // PGA x4 mode with INP1 input
///   Op1::configure_pga(alloy::hal::opamp::lite::PgaGain::x4);
///   Op1::enable();
/// @endcode
///
/// Base addresses (STM32G4, APB1 + 0x7800, stride 0x10):
///   OPAMP1 : 0x40007800
///   OPAMP2 : 0x40007810
///   OPAMP3 : 0x40007820
///   OPAMP4 : 0x40007830
///   OPAMP5 : 0x40007840
///   OPAMP6 : 0x40007850
///
/// Base addresses (STM32L4, same base, only 2 units):
///   OPAMP1 : 0x40007800
///   OPAMP2 : 0x40007810
///
/// Register layout per OPAMP unit:
///   +0x00  CSR    — control and status
///   +0x04  OTR    — offset trimming (normal speed)
///   +0x08  HSOTR  — offset trimming (high speed)
///
/// CSR bit layout:
///   bit  0       OPAEN      — enable
///   bit  1       OPALPM     — low power mode
///   bits 3:2     OPAMODE    — operating mode (00=standalone, 10=PGA, 11=follower)
///   bits 6:4     PGA_GAIN   — gain in PGA mode (see PgaGain enum)
///   bits 9:8     VM_SEL     — inverting input (00=INM1, 01=INM2)
///   bits 11:10   VP_SEL     — non-inverting input (00=INP1, 01=INP2, 10=DAC)
///   bit  12      USERTRIM   — user offset trimming enable
///   bit  13      OPAHSM     — high speed mode
///   bit  14      OPAINTOEN  — internal output to ADC
///   bit  31      CALOUT     — calibration output (read-only)
#pragma once

#include <cstdint>

namespace alloy::hal::opamp::lite {

namespace detail {

// CSR bit positions
inline constexpr std::uint32_t kCsrOpaen       = 1u << 0;
inline constexpr std::uint32_t kCsrOpalpm      = 1u << 1;
inline constexpr std::uint32_t kCsrOpamodeShift = 2u;
inline constexpr std::uint32_t kCsrOpamodeMask  = 0x3u << 2;
inline constexpr std::uint32_t kCsrPgaGainShift = 4u;
inline constexpr std::uint32_t kCsrPgaGainMask  = 0x7u << 4;
inline constexpr std::uint32_t kCsrVmSelShift   = 8u;
inline constexpr std::uint32_t kCsrVmSelMask    = 0x3u << 8;
inline constexpr std::uint32_t kCsrVpSelShift   = 10u;
inline constexpr std::uint32_t kCsrVpSelMask    = 0x3u << 10;
inline constexpr std::uint32_t kCsrUsertrim     = 1u << 12;
inline constexpr std::uint32_t kCsrOpahsm       = 1u << 13;
inline constexpr std::uint32_t kCsrOpaintoen    = 1u << 14;
inline constexpr std::uint32_t kCsrCalout       = 1u << 31;

// OPAMODE values (bits 3:2)
inline constexpr std::uint32_t kModeStandalone  = 0x0u;  ///< Open loop
inline constexpr std::uint32_t kModePga         = 0x2u;  ///< PGA (programmable gain)
inline constexpr std::uint32_t kModeFollower    = 0x3u;  ///< Voltage follower

// OTR / HSOTR bit positions
inline constexpr std::uint32_t kOtrTrimNShift   = 0u;   ///< TRIMOFFSETN[4:0]
inline constexpr std::uint32_t kOtrTrimNMask    = 0x1Fu;
inline constexpr std::uint32_t kOtrTrimPShift   = 8u;   ///< TRIMOFFSETP[4:0]
inline constexpr std::uint32_t kOtrTrimPMask    = 0x1Fu << 8;

// Register offsets within one OPAMP unit
inline constexpr std::uintptr_t kOfsCsr   = 0x00u;
inline constexpr std::uintptr_t kOfsOtr   = 0x04u;
inline constexpr std::uintptr_t kOfsHsotr = 0x08u;

}  // namespace detail

// ----------------------------------------------------------------------------
// Enumerations
// ----------------------------------------------------------------------------

/// PGA gain selection (PGA_GAIN[2:0] in CSR).
///
/// The "WithFilter" variants connect an internal RC low-pass filter between
/// the PGA output and the OPAMP inverting input for anti-aliasing.  The
/// filter capacitor pin (OPAMP_INM) must be left unconnected in this mode.
enum class PgaGain : std::uint8_t {
    x2          = 0,  ///<  ×2
    x4          = 1,  ///<  ×4
    x8          = 2,  ///<  ×8
    x16         = 3,  ///< ×16
    x2_filtered = 4,  ///<  ×2 with internal low-pass filter
    x4_filtered = 5,  ///<  ×4 with internal low-pass filter
    x8_filtered = 6,  ///<  ×8 with internal low-pass filter
    x16_filtered= 7,  ///< ×16 with internal low-pass filter
};

/// Inverting (−) input selection (VM_SEL[1:0] in CSR).
enum class InvertingInput : std::uint8_t {
    INM1 = 0,  ///< INM1 GPIO pin
    INM2 = 1,  ///< INM2 GPIO pin
};

/// Non-inverting (+) input selection (VP_SEL[1:0] in CSR).
enum class NonInvertingInput : std::uint8_t {
    INP1  = 0,  ///< INP1 GPIO pin (default)
    INP2  = 1,  ///< INP2 GPIO pin
    DAC1  = 2,  ///< Internal DAC output 1 (device-specific mapping)
    DAC2  = 3,  ///< Internal DAC output 2 (device-specific mapping)
};

// ----------------------------------------------------------------------------
// engine<OpampBase>
// ----------------------------------------------------------------------------

/// STM32 OPAMP — address-template variant.
///
/// `OpampBase` is the base address of the specific OPAMP unit.
template <std::uintptr_t OpampBase>
class engine {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 OPAMP — G4/H7 (CSR + OTR + HSOTR per unit)
   //
   // Mode encoding (OPAMODE, VP_SEL, VM_SEL, PGA_GAIN) is STM32-specific.
   // Do NOT use for generic op-amp ICs or SAME70 ACC.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/opamp/lite.hpp: OPAMP layout is STM32-only. "
        "Use a vendor-specific analog driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(OpampBase + ofs);
    }
    [[nodiscard]] static auto csr()   noexcept -> volatile std::uint32_t& {
        return reg(detail::kOfsCsr);
    }
    [[nodiscard]] static auto otr()   noexcept -> volatile std::uint32_t& {
        return reg(detail::kOfsOtr);
    }
    [[nodiscard]] static auto hsotr() noexcept -> volatile std::uint32_t& {
        return reg(detail::kOfsHsotr);
    }

   public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Enable the OPAMP (CSR.OPAEN = 1).
    ///
    /// Call after `configure_*()`.  The GPIO pins used as inputs must already
    /// be configured in analog mode.  Allow a short settling time (~µs) after
    /// enabling before relying on the output.
    static void enable() noexcept {
        csr() |= detail::kCsrOpaen;
    }

    /// Disable the OPAMP (CSR.OPAEN = 0).
    static void disable() noexcept {
        csr() &= ~detail::kCsrOpaen;
    }

    // -----------------------------------------------------------------------
    // Mode configuration
    // -----------------------------------------------------------------------

    /// Configure as a unity-gain voltage follower.
    ///
    /// Output = INP1 (or the selected non-inverting input).
    /// The inverting input is internally connected to the output — no external
    /// feedback network required.  Best for buffering high-impedance sources.
    ///
    /// @param inp  Non-inverting input selection.
    static void configure_follower(
        NonInvertingInput inp = NonInvertingInput::INP1) noexcept {
        std::uint32_t val = csr() & ~(detail::kCsrOpamodeMask |
                                       detail::kCsrVpSelMask   |
                                       detail::kCsrPgaGainMask);
        val |= (detail::kModeFollower << detail::kCsrOpamodeShift);
        val |= (static_cast<std::uint32_t>(inp) << detail::kCsrVpSelShift);
        csr() = val;
    }

    /// Configure as a PGA (Programmable Gain Amplifier).
    ///
    /// Output = gain × INP.  The inverting input feedback is internal.
    /// For filtered variants (`PgaGain::x*_filtered`) the INM pin must be
    /// left floating to connect to the internal RC filter.
    ///
    /// @param gain  Gain selection (×2 to ×16, optionally with filter).
    /// @param inp   Non-inverting input.
    static void configure_pga(
        PgaGain           gain = PgaGain::x2,
        NonInvertingInput inp  = NonInvertingInput::INP1) noexcept {
        std::uint32_t val = csr() & ~(detail::kCsrOpamodeMask |
                                       detail::kCsrVpSelMask   |
                                       detail::kCsrPgaGainMask);
        val |= (detail::kModePga << detail::kCsrOpamodeShift);
        val |= (static_cast<std::uint32_t>(gain) << detail::kCsrPgaGainShift);
        val |= (static_cast<std::uint32_t>(inp)  << detail::kCsrVpSelShift);
        csr() = val;
    }

    /// Configure in standalone (open-loop / external feedback) mode.
    ///
    /// All inputs are disconnected from internal paths — connect feedback
    /// network externally.  Useful as a high-speed comparator or for
    /// custom closed-loop circuits.
    ///
    /// @param inp  Non-inverting input.
    /// @param inv  Inverting input.
    static void configure_standalone(
        NonInvertingInput inp = NonInvertingInput::INP1,
        InvertingInput    inv = InvertingInput::INM1) noexcept {
        std::uint32_t val = csr() & ~(detail::kCsrOpamodeMask |
                                       detail::kCsrVpSelMask   |
                                       detail::kCsrVmSelMask   |
                                       detail::kCsrPgaGainMask);
        val |= (detail::kModeStandalone << detail::kCsrOpamodeShift);
        val |= (static_cast<std::uint32_t>(inp) << detail::kCsrVpSelShift);
        val |= (static_cast<std::uint32_t>(inv) << detail::kCsrVmSelShift);
        csr() = val;
    }

    // -----------------------------------------------------------------------
    // Speed / power
    // -----------------------------------------------------------------------

    /// Enable high speed mode (OPAHSM=1, higher bandwidth, higher current).
    static void enable_high_speed() noexcept {
        csr() |= detail::kCsrOpahsm;
    }

    /// Disable high speed mode (normal speed, lower current).
    static void disable_high_speed() noexcept {
        csr() &= ~detail::kCsrOpahsm;
    }

    /// Enable low power mode (OPALPM=1, reduced bandwidth and current).
    static void enable_low_power() noexcept {
        csr() |= detail::kCsrOpalpm;
    }

    /// Disable low power mode.
    static void disable_low_power() noexcept {
        csr() &= ~detail::kCsrOpalpm;
    }

    // -----------------------------------------------------------------------
    // Internal output to ADC
    // -----------------------------------------------------------------------

    /// Route the OPAMP output directly to the ADC (OPAINTOEN=1).
    ///
    /// When enabled, the OPAMP output is disconnected from the GPIO output
    /// pin and connected to an internal ADC input channel.  Refer to the
    /// device reference manual for which ADC channel maps to this OPAMP.
    static void enable_internal_output() noexcept {
        csr() |= detail::kCsrOpaintoen;
    }

    /// Disconnect the internal ADC path (output returns to GPIO pin).
    static void disable_internal_output() noexcept {
        csr() &= ~detail::kCsrOpaintoen;
    }

    // -----------------------------------------------------------------------
    // Offset trimming
    // -----------------------------------------------------------------------

    /// Apply user offset trim values for normal-speed operation.
    ///
    /// The factory-programmed trim values are used by default.  Use
    /// `use_factory_trimming()` to revert.
    ///
    /// @param trim_n  NMOS trimming code (0–31).
    /// @param trim_p  PMOS trimming code (0–31).
    static void set_user_trimming(std::uint8_t trim_n,
                                   std::uint8_t trim_p) noexcept {
        otr() = (static_cast<std::uint32_t>(trim_p & 0x1Fu) << detail::kOtrTrimPShift) |
                (static_cast<std::uint32_t>(trim_n & 0x1Fu) << detail::kOtrTrimNShift);
        csr() |= detail::kCsrUsertrim;
    }

    /// Apply user offset trim values for high-speed operation.
    static void set_hs_user_trimming(std::uint8_t trim_n,
                                      std::uint8_t trim_p) noexcept {
        hsotr() = (static_cast<std::uint32_t>(trim_p & 0x1Fu) << detail::kOtrTrimPShift) |
                  (static_cast<std::uint32_t>(trim_n & 0x1Fu) << detail::kOtrTrimNShift);
        csr() |= detail::kCsrUsertrim;
    }

    /// Revert to factory offset trimming (USERTRIM=0).
    static void use_factory_trimming() noexcept {
        csr() &= ~detail::kCsrUsertrim;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when the OPAMP calibration output is high (CALOUT=1).
    ///
    /// Only meaningful during trimming calibration sequences (USERTRIM=1,
    /// special calibration input routing).  Not normally used in production.
    [[nodiscard]] static auto cal_output() noexcept -> bool {
        return (csr() & detail::kCsrCalout) != 0u;
    }

    engine() = delete;
};

}  // namespace alloy::hal::opamp::lite
