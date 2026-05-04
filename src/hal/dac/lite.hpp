/// @file hal/dac/lite.hpp
/// Lightweight, direct-MMIO DAC driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
///
/// Supports:
///   StDac — all STM32 DAC peripherals (F0/G0/G4/H7/…)
///     kTemplate = "dac", kIpVersion starts with "dacif"
///     Covers dacif_v1_1_Cube (F4/G4) and dacif_v3_0_Cube (G0/H7).
///     Both versions share the same base register layout.
///
/// Register layout (STM32 DAC — all supported versions):
///   0x00  CR       — control register (channels 1 and 2)
///   0x04  SWTRIGR  — software trigger register
///   0x08  DHR12R1  — 12-bit right-aligned data for channel 1
///   0x0C  DHR12L1  — 12-bit left-aligned data for channel 1
///   0x10  DHR8R1   — 8-bit right-aligned data for channel 1
///   0x14  DHR12R2  — 12-bit right-aligned data for channel 2
///   0x18  DHR12L2  — 12-bit left-aligned data for channel 2
///   0x1C  DHR8R2   — 8-bit right-aligned data for channel 2
///   0x20  DHR12RD  — dual 12-bit right-aligned (both channels)
///   0x24  DHR12LD  — dual 12-bit left-aligned
///   0x28  DHR8RD   — dual 8-bit right-aligned
///   0x2C  DOR1     — output data register channel 1 (read-only)
///   0x30  DOR2     — output data register channel 2 (read-only)
///   0x34  SR       — status register (DMA underrun flags)
///
/// Channel index: 0 = channel 1 (bits 0–15 in CR), 1 = channel 2 (bits 16–31).
///
/// IMPORTANT: enable the peripheral clock before calling configure():
/// @code
///   dev::peripheral_on<dev::dac>();
/// @endcode
///
/// Typical usage (STM32G071 — channel 1 on PA4, 12-bit right-aligned):
/// @code
///   #include "hal/dac.hpp"
///   using Dac = alloy::hal::dac::lite::port<dev::dac>;
///
///   dev::peripheral_on<dev::dac>();
///   Dac::configure();               // no trigger, no noise/wave
///   Dac::enable_channel(0u);        // channel 1
///   Dac::write(0u, 2048u);          // ≈ Vdda/2
/// @endcode
#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::dac::lite {

// ============================================================================
// Detail
// ============================================================================

namespace detail {

// Register offsets
inline constexpr std::uintptr_t kOfsCr      = 0x00u;
inline constexpr std::uintptr_t kOfsSwtrigr  = 0x04u;
inline constexpr std::uintptr_t kOfsDhr12r1  = 0x08u;
inline constexpr std::uintptr_t kOfsDhr12l1  = 0x0Cu;
inline constexpr std::uintptr_t kOfsDhr8r1   = 0x10u;
inline constexpr std::uintptr_t kOfsDhr12r2  = 0x14u;
inline constexpr std::uintptr_t kOfsDhr12l2  = 0x18u;
inline constexpr std::uintptr_t kOfsDhr8r2   = 0x1Cu;
inline constexpr std::uintptr_t kOfsDhr12rd  = 0x20u;
inline constexpr std::uintptr_t kOfsDhr12ld  = 0x24u;
inline constexpr std::uintptr_t kOfsDhr8rd   = 0x28u;
inline constexpr std::uintptr_t kOfsDor1     = 0x2Cu;
inline constexpr std::uintptr_t kOfsDor2     = 0x30u;
inline constexpr std::uintptr_t kOfsSr       = 0x34u;

// CR bit positions (per channel).  Channel 2 fields are shifted +16 bits.
inline constexpr std::uint32_t kCrEnShift    = 0u;   ///< ENx — channel enable
inline constexpr std::uint32_t kCrTenShift   = 1u;   ///< TENx — trigger enable
inline constexpr std::uint32_t kCrTselShift  = 3u;   ///< TSELx[2:0] — trigger select
inline constexpr std::uint32_t kCrWaveShift  = 6u;   ///< WAVEx[1:0] — wave gen
inline constexpr std::uint32_t kCrMampShift  = 8u;   ///< MAMPx[3:0] — wave amplitude
inline constexpr std::uint32_t kCrDmaenShift = 12u;  ///< DMAENx — DMA enable
inline constexpr std::uint32_t kCrDmaudrie   = 13u;  ///< DMAUDRIEx — DMA underrun IE

// Channel stride in CR (channel 2 bits are at offset +16 vs channel 1)
inline constexpr std::uint32_t kCrChannelStride = 16u;

// SWTRIGR bit positions
inline constexpr std::uint32_t kSwtrigShift = 0u;  ///< SWTRIGx bit per channel

// SR: DMA underrun flags
inline constexpr std::uint32_t kSrDmaudr1 = 1u << 13;
inline constexpr std::uint32_t kSrDmaudr2 = 1u << 29;

[[nodiscard]] consteval auto is_dac(const char* tmpl, const char* ip) -> bool {
    return std::string_view{tmpl} == "dac" &&
           std::string_view{ip}.starts_with("dacif");
}

/// Byte offset of the 12-bit right-aligned data register for channel `ch`.
[[nodiscard]] constexpr auto dhr12r_offset(std::uint8_t ch) noexcept -> std::uintptr_t {
    return (ch == 0u) ? kOfsDhr12r1 : kOfsDhr12r2;
}
/// Byte offset of the 8-bit right-aligned data register for channel `ch`.
[[nodiscard]] constexpr auto dhr8r_offset(std::uint8_t ch) noexcept -> std::uintptr_t {
    return (ch == 0u) ? kOfsDhr8r1 : kOfsDhr8r2;
}
/// Byte offset of the output data register for channel `ch`.
[[nodiscard]] constexpr auto dor_offset(std::uint8_t ch) noexcept -> std::uintptr_t {
    return (ch == 0u) ? kOfsDor1 : kOfsDor2;
}

}  // namespace detail

// ============================================================================
// Concept
// ============================================================================

/// Satisfied by all STM32 DAC peripherals (dacif_v1_1_Cube / dacif_v3_0_Cube).
template <typename P>
concept StDac =
    device::PeripheralSpec<P> &&
    requires {
        { P::kTemplate  } -> std::convertible_to<const char*>;
        { P::kIpVersion } -> std::convertible_to<const char*>;
    } &&
    detail::is_dac(P::kTemplate, P::kIpVersion);

// ============================================================================
// Trigger source
// ============================================================================

/// Hardware trigger source for DAC conversions.
///
/// The numeric values match the TSEL field encoding on most STM32 families.
/// Consult your device's reference manual for family-specific assignments.
enum class TriggerSource : std::uint8_t {
    None   = 0,  ///< No trigger — output updated immediately on DHR write
    Tim6   = 0,  ///< TIM6 TRGO (typical default on G0/G4/H7; check RM)
    Tim8   = 1,
    Tim7   = 2,
    Tim5   = 3,
    Tim2   = 4,
    Tim4   = 5,
    Exti9  = 6,  ///< External trigger on EXTI line 9
    Software = 7,  ///< Software trigger via SWTRIGR
};

// ============================================================================
// port<P>
// ============================================================================

/// Direct-MMIO STM32 DAC port.  P must satisfy StDac.
///
/// Channel index: 0 = DAC channel 1 (PA4 on G071/G431),
///                1 = DAC channel 2 (PA5 on devices that have two channels).
template <typename P>
    requires StDac<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

   private:
    [[nodiscard]] static auto reg(std::uintptr_t offset) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + offset);
    }

    /// CR bit shift for a given channel (0 or 1).
    [[nodiscard]] static constexpr auto ch_shift(std::uint8_t ch) noexcept -> std::uint32_t {
        return (ch == 0u) ? 0u : detail::kCrChannelStride;
    }

   public:
    // -----------------------------------------------------------------------
    // Channel configuration
    // -----------------------------------------------------------------------

    /// Configure a channel: trigger source, wave generation (disabled by default).
    ///
    /// Does not enable the channel — call `enable_channel()` after configuring.
    ///
    /// @param ch   Channel index: 0 = CH1, 1 = CH2.
    /// @param src  Trigger source.  `TriggerSource::None` → software output
    ///             (DHR write drives the output directly, no trigger needed).
    static void configure_channel(std::uint8_t ch,
                                  TriggerSource src = TriggerSource::None) noexcept {
        const auto shift = ch_shift(ch);
        std::uint32_t cr = reg(detail::kOfsCr);

        // Clear channel fields (EN, TEN, TSEL, WAVE, MAMP, DMAEN, DMAUDRIE)
        constexpr std::uint32_t kChMask = 0x3FFFu;
        cr &= ~(kChMask << shift);

        if (src != TriggerSource::None) {
            cr |= (1u << (detail::kCrTenShift  + shift));  // TEN = 1
            cr |= (static_cast<std::uint32_t>(src) << (detail::kCrTselShift + shift));
        }
        reg(detail::kOfsCr) = cr;
    }

    /// Enable channel `ch` output (sets ENx bit in CR).
    static void enable_channel(std::uint8_t ch) noexcept {
        reg(detail::kOfsCr) |= (1u << (detail::kCrEnShift + ch_shift(ch)));
    }

    /// Disable channel `ch` output.
    static void disable_channel(std::uint8_t ch) noexcept {
        reg(detail::kOfsCr) &= ~(1u << (detail::kCrEnShift + ch_shift(ch)));
    }

    /// Shorthand: configure (no trigger) + enable channel `ch`.
    static void configure() noexcept {
        clock_on();
        configure_channel(0u);
        configure_channel(1u);
    }

    // -----------------------------------------------------------------------
    // Output data
    // -----------------------------------------------------------------------

    /// Write a 12-bit value (0–4095) to channel `ch` (right-aligned).
    ///
    /// If no hardware trigger is configured the output updates immediately.
    /// If a trigger is configured the value is latched on the next trigger.
    ///
    /// @param ch     Channel index: 0 = CH1, 1 = CH2.
    /// @param value  12-bit DAC code (0 = 0 V, 4095 ≈ Vdda).
    static void write(std::uint8_t ch, std::uint16_t value) noexcept {
        reg(detail::dhr12r_offset(ch)) = static_cast<std::uint32_t>(value) & 0xFFFu;
    }

    /// Write an 8-bit value (0–255) to channel `ch` (right-aligned).
    static void write_8bit(std::uint8_t ch, std::uint8_t value) noexcept {
        reg(detail::dhr8r_offset(ch)) = static_cast<std::uint32_t>(value);
    }

    /// Write both channels simultaneously (12-bit right-aligned, dual mode).
    ///
    /// @param ch1_value  12-bit value for channel 1 (bits 11:0).
    /// @param ch2_value  12-bit value for channel 2 (bits 27:16).
    static void write_dual(std::uint16_t ch1_value, std::uint16_t ch2_value) noexcept {
        reg(detail::kOfsDhr12rd) =
            (static_cast<std::uint32_t>(ch2_value) << 16u) |
            (static_cast<std::uint32_t>(ch1_value) & 0xFFFu);
    }

    // -----------------------------------------------------------------------
    // Software trigger
    // -----------------------------------------------------------------------

    /// Generate a software trigger on channel `ch`.
    ///
    /// Only valid when the channel is configured with `TriggerSource::Software`.
    static void software_trigger(std::uint8_t ch) noexcept {
        reg(detail::kOfsSwtrigr) = 1u << (detail::kSwtrigShift + ch);
    }

    // -----------------------------------------------------------------------
    // Status / read-back
    // -----------------------------------------------------------------------

    /// Read the current DAC output register for channel `ch` (read-only).
    [[nodiscard]] static auto output(std::uint8_t ch) noexcept -> std::uint16_t {
        return static_cast<std::uint16_t>(
            reg(detail::dor_offset(ch)) & 0xFFFu);
    }

    /// True when the DMA underrun flag for channel `ch` is set.
    [[nodiscard]] static auto dma_underrun(std::uint8_t ch) noexcept -> bool {
        const std::uint32_t mask =
            (ch == 0u) ? detail::kSrDmaudr1 : detail::kSrDmaudr2;
        return (reg(detail::kOfsSr) & mask) != 0u;
    }

    /// Clear the DMA underrun flag for channel `ch`.
    static void clear_dma_underrun(std::uint8_t ch) noexcept {
        const std::uint32_t mask =
            (ch == 0u) ? detail::kSrDmaudr1 : detail::kSrDmaudr2;
        reg(detail::kOfsSr) = mask;  // w1c
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
    ///
    /// Uses the typed `P::kRccEnable = { addr, mask }` emitted by alloy-codegen v0.4+.
    /// No-op when the peripheral has no kRccEnable field.
    static void clock_on() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) |= P::kRccEnable.mask;
        }
        if constexpr (requires { P::kRccReset; }) {
            // Assert then release reset so the peripheral starts from a known state.
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

    port() = delete;
};

}  // namespace alloy::hal::dac::lite
