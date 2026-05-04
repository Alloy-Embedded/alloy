/// @file hal/adc/lite.hpp
/// Lightweight, direct-MMIO ADC driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
/// Works whenever `ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE` is true.
///
/// Supports two STM32 ADC IP families:
///
///   StSimpleAdc (kIpVersion starts with "aditf4"):
///     - STM32F0: aditf4_v1_1_Cube  (F030, F072, …)
///     - STM32G0: aditf4_v3_0_G0_Cube  (G030, G031, G071, …)
///     - Register layout: ISR/IER/CR/CFGR1/CFGR2/SMPR/TR/CHSELR/DR
///     - Single SMPR register (sample time applies to all channels)
///     - Channel selection via CHSELR bitmask
///     - Calibration: CR.ADCAL → wait → CR.ADEN → wait ADRDY
///
///   StModernAdc (kIpVersion contains "aditf5"):
///     - STM32G4: G4_aditf5_90_v1_0_Cube  (G431, G474, G491, …)
///     - STM32H7: aditf5_v3_0_Cube  (H742, H743, H750, …)
///     - STM32L4 / STM32WB with aditf5 variant
///     - Register layout: ISR/IER/CR/CFGR/CFGR2/SMPR1/SMPR2/SQR1–4/DR
///     - Per-channel sample time in SMPR1/SMPR2
///     - Channel selection via SQR1.SQ1 (single-conversion regular sequence)
///     - Init sequence: DEEPPWD → ADVREGEN → wait tVREG → ADCAL → ADEN
///
/// NOT supported (different register layout):
///   - STM32F1 / F2 / F4: kIpVersion = "aditf2_v…" (SR/CR1/CR2 layout)
///
/// IMPORTANT: caller must enable the peripheral clock before configure():
/// @code
///   namespace dev = alloy::device::selected::device_traits;
///   dev::peripheral_on<dev::adc1>();   // clk + rst
/// @endcode
///
/// Typical usage (STM32G071, single-channel polling):
/// @code
///   #include "hal/adc.hpp"
///   #include "device/runtime.hpp"
///
///   namespace dev = alloy::device::selected::device_traits;
///   using Adc = alloy::hal::adc::lite::port<dev::adc>;
///
///   dev::peripheral_on<dev::adc>();
///   Adc::configure();                         // calibrate + enable
///   std::uint16_t v = Adc::convert(0u);       // sample channel 0 (IN0 / PA0)
/// @endcode
///
/// Modern ADC (STM32G4 — note vreg_wait_cycles for 170 MHz):
/// @code
///   using Adc1 = alloy::hal::adc::lite::port<dev::adc1>;
///   dev::peripheral_on<dev::adc1>();
///   Adc1::configure({.vreg_wait_cycles = 3500u});  // ~20 µs @ 170 MHz
///   std::uint16_t v = Adc1::convert(1u);           // channel 1
/// @endcode
#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::adc::lite {

// ============================================================================
// Detail — register offsets and bit constants
// ============================================================================

namespace detail {

// ---------------------------------------------------------------------------
// Simple ADC (aditf4 — F0 / G0) register offsets
// ---------------------------------------------------------------------------

inline constexpr std::uintptr_t kSimOfsIsr    = 0x00u;   ///< ISR
inline constexpr std::uintptr_t kSimOfsIer    = 0x04u;   ///< IER
inline constexpr std::uintptr_t kSimOfsCr     = 0x08u;   ///< CR
inline constexpr std::uintptr_t kSimOfsCfgr1  = 0x0Cu;   ///< CFGR1
inline constexpr std::uintptr_t kSimOfsCfgr2  = 0x10u;   ///< CFGR2
inline constexpr std::uintptr_t kSimOfsSmpr   = 0x14u;   ///< SMPR (all channels)
inline constexpr std::uintptr_t kSimOfsTr     = 0x20u;   ///< TR (threshold)
inline constexpr std::uintptr_t kSimOfsChselr = 0x28u;   ///< CHSELR (channel select)
inline constexpr std::uintptr_t kSimOfsDr     = 0x40u;   ///< DR (data)

// CR bits (simple ADC)
inline constexpr std::uint32_t kSimCrAden    = 1u << 0;  ///< ADC enable
inline constexpr std::uint32_t kSimCrAddis   = 1u << 1;  ///< ADC disable
inline constexpr std::uint32_t kSimCrAdstart = 1u << 2;  ///< Start regular conversion
inline constexpr std::uint32_t kSimCrAdstop  = 1u << 4;  ///< Stop regular conversion
inline constexpr std::uint32_t kSimCrAdcal   = 1u << 31; ///< ADC calibration

// ISR bits (simple ADC)
inline constexpr std::uint32_t kSimIsrAdrdy  = 1u << 0;  ///< ADC ready
inline constexpr std::uint32_t kSimIsrEoc    = 1u << 2;  ///< End of conversion
inline constexpr std::uint32_t kSimIsrEos    = 1u << 3;  ///< End of sequence
inline constexpr std::uint32_t kSimIsrOvr    = 1u << 4;  ///< Overrun

// IER bits (simple ADC) — same bit positions as the corresponding ISR flags
inline constexpr std::uint32_t kSimIerEocie  = 1u << 2;  ///< EOC interrupt enable
inline constexpr std::uint32_t kSimIerEosie  = 1u << 3;  ///< EOS interrupt enable
inline constexpr std::uint32_t kSimIerOvrie  = 1u << 4;  ///< OVR interrupt enable

// CFGR1 bits (simple ADC)
inline constexpr std::uint32_t kSimCfgr1ResMask  = 3u << 3;  ///< RES[4:3]
inline constexpr std::uint32_t kSimCfgr1ResShift = 3u;
inline constexpr std::uint32_t kSimCfgr1Align    = 1u << 5;  ///< Left-align
inline constexpr std::uint32_t kSimCfgr1Cont     = 1u << 13; ///< Continuous mode
inline constexpr std::uint32_t kSimCfgr1OvrMod   = 1u << 12; ///< Overrun mode (replace)

// ---------------------------------------------------------------------------
// Modern ADC (aditf5 — G4 / H7) register offsets
// ---------------------------------------------------------------------------

inline constexpr std::uintptr_t kModOfsIsr   = 0x00u;   ///< ISR
inline constexpr std::uintptr_t kModOfsIer   = 0x04u;   ///< IER
inline constexpr std::uintptr_t kModOfsCr    = 0x08u;   ///< CR
inline constexpr std::uintptr_t kModOfsCfgr  = 0x0Cu;   ///< CFGR
inline constexpr std::uintptr_t kModOfsCfgr2 = 0x10u;   ///< CFGR2
inline constexpr std::uintptr_t kModOfsSmpr1 = 0x14u;   ///< SMPR1 (channels 0–9)
inline constexpr std::uintptr_t kModOfsSmpr2 = 0x18u;   ///< SMPR2 (channels 10–18)
inline constexpr std::uintptr_t kModOfsSqr1  = 0x30u;   ///< SQR1 (sequence register 1)
inline constexpr std::uintptr_t kModOfsDr    = 0x40u;   ///< DR (data)

// CR bits (modern ADC — superset of simple)
inline constexpr std::uint32_t kModCrAden     = 1u << 0;  ///< ADC enable
inline constexpr std::uint32_t kModCrAddis    = 1u << 1;  ///< ADC disable
inline constexpr std::uint32_t kModCrAdstart  = 1u << 2;  ///< Start regular conversion
inline constexpr std::uint32_t kModCrAdstop   = 1u << 4;  ///< Stop regular conversion
inline constexpr std::uint32_t kModCrAdvregen = 1u << 28; ///< Voltage regulator enable
inline constexpr std::uint32_t kModCrDeeppwd  = 1u << 29; ///< Deep-power-down mode
inline constexpr std::uint32_t kModCrAdcal    = 1u << 31; ///< ADC calibration (single-ended)

// ISR bits (modern ADC — same layout as simple)
inline constexpr std::uint32_t kModIsrAdrdy   = 1u << 0;
inline constexpr std::uint32_t kModIsrEoc     = 1u << 2;
inline constexpr std::uint32_t kModIsrEos     = 1u << 3;
inline constexpr std::uint32_t kModIsrOvr     = 1u << 4;

// IER bits (modern ADC) — same bit positions as the corresponding ISR flags
inline constexpr std::uint32_t kModIerEocie  = 1u << 2;  ///< EOC interrupt enable
inline constexpr std::uint32_t kModIerEosie  = 1u << 3;  ///< EOS interrupt enable
inline constexpr std::uint32_t kModIerOvrie  = 1u << 4;  ///< OVR interrupt enable

// CFGR bits (modern ADC)
inline constexpr std::uint32_t kModCfgrResMask  = 3u << 3;  ///< RES[4:3]
inline constexpr std::uint32_t kModCfgrResShift = 3u;
inline constexpr std::uint32_t kModCfgrAlign    = 1u << 5;  ///< Left-align
inline constexpr std::uint32_t kModCfgrCont     = 1u << 13; ///< Continuous mode
inline constexpr std::uint32_t kModCfgrOvrMod   = 1u << 12; ///< Overrun mode (replace)

// SQR1 fields (modern ADC — single conversion)
inline constexpr std::uint32_t kModSqr1LMask   = 0xFu;      ///< L[3:0] = seq length - 1
inline constexpr std::uint32_t kModSqr1Sq1Shift = 6u;        ///< SQ1[12:6] = first channel
inline constexpr std::uint32_t kModSqr1Sq1Mask  = 0x1Fu;     ///< 5-bit channel number

// SMPR1/2 encoding: 3 bits per channel, channels 0–9 in SMPR1, 10–18 in SMPR2.
// SMP field values:
//   0: 2.5 cycles  1: 6.5 cycles  2: 12.5 cycles  3: 24.5 cycles
//   4: 47.5 cycles  5: 92.5 cycles  6: 247.5 cycles  7: 640.5 cycles
inline constexpr std::uint32_t kModSmprSmpWidth = 3u;  ///< bits per channel in SMPR1/2

// ---------------------------------------------------------------------------
// Family discriminators
// ---------------------------------------------------------------------------

[[nodiscard]] consteval auto is_simple_adc(const char* tmpl, const char* ip) -> bool {
    return std::string_view{tmpl} == "adc" &&
           std::string_view{ip}.starts_with("aditf4");
}

[[nodiscard]] consteval auto is_modern_adc(const char* tmpl, const char* ip) -> bool {
    return std::string_view{tmpl} == "adc" &&
           std::string_view{ip}.find("aditf5") != std::string_view::npos;
}

}  // namespace detail

// ============================================================================
// Concepts
// ============================================================================

/// Satisfied by STM32 F0 / G0 simplified ADC peripherals.
/// Register layout: ISR/IER/CR/CFGR1/CFGR2/SMPR/CHSELR/DR.
/// Channel selection via CHSELR bitmask.
template <typename P>
concept StSimpleAdc =
    device::PeripheralSpec<P> &&
    requires {
        { P::kTemplate  } -> std::convertible_to<const char*>;
        { P::kIpVersion } -> std::convertible_to<const char*>;
    } &&
    detail::is_simple_adc(P::kTemplate, P::kIpVersion);

/// Satisfied by STM32 G4 / H7 full ADC peripherals.
/// Register layout: ISR/IER/CR/CFGR/CFGR2/SMPR1/SMPR2/SQR1–4/DR.
/// Channel selection via SQR1.SQ1 + L.
template <typename P>
concept StModernAdc =
    device::PeripheralSpec<P> &&
    requires {
        { P::kTemplate  } -> std::convertible_to<const char*>;
        { P::kIpVersion } -> std::convertible_to<const char*>;
    } &&
    detail::is_modern_adc(P::kTemplate, P::kIpVersion);

// ============================================================================
// Resolution and configuration
// ============================================================================

/// ADC resolution.  Not all resolutions are available on all families.
///
/// - Simple ADC (F0/G0): 12 / 10 / 8 / 6 bit via CFGR1.RES[4:3] = 0–3.
/// - Modern ADC (G4/H7): 12 / 10 / 8 / 6 bit via CFGR.RES[4:3] = 0–3.
enum class Resolution : std::uint8_t {
    Bits12 = 0,  ///< 12-bit (full resolution)
    Bits10 = 1,  ///< 10-bit
    Bits8  = 2,  ///< 8-bit
    Bits6  = 3,  ///< 6-bit (fastest conversion)
};

/// Configuration passed to `port::configure()`.
struct Config {
    /// Run the self-calibration sequence before enabling the ADC.
    ///
    /// Calibration compensates for offset errors due to manufacturing variation.
    /// Should be done at power-up with stable Vdda.  Takes ~1 µs @ 16 MHz for
    /// simple ADC; ~160 ADCCLK cycles for modern ADC.
    bool calibrate{true};

    /// ADC conversion resolution (default: 12-bit).
    Resolution resolution{Resolution::Bits12};

    /// Default sample time for all channels.
    ///
    /// For simple ADC (F0/G0): SMPR.SMP field, 0–7 (2.5 – 239.5 cycles).
    ///   Default 7 (239.5 cycles) — safe for all Vdda, high-impedance sources.
    /// For modern ADC (G4/H7): applied to SMPR1/2 for all channels, 0–7
    ///   (2.5 – 640.5 cycles).  Default 7 (640.5 cycles).
    std::uint8_t sample_time{7u};

    /// Voltage-regulator startup wait (modern ADC only).
    ///
    /// After ADVREGEN=1, the H7/G4 voltage regulator requires tADCVREG_STUP
    /// (~20 µs) before calibration can start.  Set to ≥ 20 µs × SYSCLK_Hz.
    ///
    /// Examples:
    ///   G4 @ 170 MHz → 3500 cycles
    ///   H7 @ 400 MHz → 8000 cycles
    ///   H7 @ 550 MHz → 11000 cycles
    std::uint32_t vreg_wait_cycles{5000u};
};

// ============================================================================
// port<P> — direct-MMIO ADC driver
// ============================================================================

/// Direct-MMIO ADC master port.  P must satisfy StSimpleAdc or StModernAdc.
///
/// All conversions are polled (blocking).  No DMA, no interrupts.
/// Single-channel regular conversion only (use the legacy HAL for injected
/// channels, scan sequences, or DMA scan).
///
/// For optimal accuracy:
///   - Enable the ADC clock before calling configure().
///   - With G4/H7, set vreg_wait_cycles to ≥ 20 µs × core_clock_Hz.
///   - With any ADC, calibrate once after power-up before the first conversion.
template <typename P>
    requires (StSimpleAdc<P> || StModernAdc<P>)
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

    /// True for simplified ADC (F0/G0); false for modern (G4/H7).
    static constexpr bool kIsSimple = StSimpleAdc<P>;

   private:
    // -----------------------------------------------------------------------
    // Register access helpers
    // -----------------------------------------------------------------------

    [[nodiscard]] static auto reg(std::uintptr_t offset) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + offset);
    }

    // Aliases for ISR, IER, and CR (same-named registers in both families)
    [[nodiscard]] static auto isr() noexcept -> volatile std::uint32_t& {
        if constexpr (kIsSimple) { return reg(detail::kSimOfsIsr); }
        else                     { return reg(detail::kModOfsIsr); }
    }
    [[nodiscard]] static auto ier() noexcept -> volatile std::uint32_t& {
        if constexpr (kIsSimple) { return reg(detail::kSimOfsIer); }
        else                     { return reg(detail::kModOfsIer); }
    }
    [[nodiscard]] static auto cr() noexcept -> volatile std::uint32_t& {
        if constexpr (kIsSimple) { return reg(detail::kSimOfsCr); }
        else                     { return reg(detail::kModOfsCr); }
    }

    // -----------------------------------------------------------------------
    // Init helpers — simple ADC (F0/G0)
    // -----------------------------------------------------------------------

    static void init_simple(const Config& cfg) noexcept {
        // Ensure ADC is disabled before calibrating (ADEN must be 0 for ADCAL).
        if ((cr() & detail::kSimCrAden) != 0u) {
            cr() |= detail::kSimCrAddis;
            while ((cr() & detail::kSimCrAden) != 0u) { /* spin */ }
        }
        // Clear any pending ADRDY flag.
        isr() = detail::kSimIsrAdrdy;

        if (cfg.calibrate) {
            cr() |= detail::kSimCrAdcal;
            while ((cr() & detail::kSimCrAdcal) != 0u) { /* spin */ }
        }

        // Apply resolution and sample time before enabling.
        const auto res = static_cast<std::uint32_t>(cfg.resolution);
        reg(detail::kSimOfsCfgr1) =
            (reg(detail::kSimOfsCfgr1) & ~detail::kSimCfgr1ResMask) |
            ((res << detail::kSimCfgr1ResShift) & detail::kSimCfgr1ResMask) |
            detail::kSimCfgr1OvrMod;  // overrun replace mode

        reg(detail::kSimOfsSmpr) = cfg.sample_time & 0x7u;

        // Enable ADC.
        cr() |= detail::kSimCrAden;
        while ((isr() & detail::kSimIsrAdrdy) == 0u) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // Init helpers — modern ADC (G4/H7)
    // -----------------------------------------------------------------------

    static void init_modern(const Config& cfg) noexcept {
        // Step 1: Exit deep-power-down (DEEPPWD must be 0 before ADVREGEN=1).
        if ((cr() & detail::kModCrDeeppwd) != 0u) {
            cr() &= ~detail::kModCrDeeppwd;
        }
        // Ensure ADC is disabled (ADVREGEN cannot be set while ADEN=1).
        if ((cr() & detail::kModCrAden) != 0u) {
            cr() |= detail::kModCrAddis;
            while ((cr() & detail::kModCrAden) != 0u) { /* spin */ }
        }

        // Step 2: Enable internal voltage regulator.
        cr() |= detail::kModCrAdvregen;

        // Step 3: Wait tADCVREG_STUP (~20 µs, specified in busy-loop cycles).
        // The volatile write on each iteration prevents the compiler from
        // optimising away the delay loop; (void) suppresses set-but-not-used.
        {
            volatile std::uint32_t delay_sink = 0u;
            for (std::uint32_t i = 0u; i < cfg.vreg_wait_cycles; i = i + 1u) {
                delay_sink = i;
            }
            static_cast<void>(delay_sink);
        }

        // Step 4: Calibrate (single-ended, ADCALDIF=0).
        if (cfg.calibrate) {
            cr() |= detail::kModCrAdcal;
            while ((cr() & detail::kModCrAdcal) != 0u) { /* spin */ }
        }

        // Step 5: Apply resolution in CFGR.
        const auto res = static_cast<std::uint32_t>(cfg.resolution);
        reg(detail::kModOfsCfgr) =
            (reg(detail::kModOfsCfgr) & ~detail::kModCfgrResMask) |
            ((res << detail::kModCfgrResShift) & detail::kModCfgrResMask) |
            detail::kModCfgrOvrMod;

        // Step 6: Apply sample time to all channels in SMPR1/SMPR2.
        // SMPR1 covers channels 0–9 (3 bits each = 30 bits).
        // SMPR2 covers channels 10–18 (3 bits each = 27 bits).
        const auto smp = static_cast<std::uint32_t>(cfg.sample_time & 0x7u);
        std::uint32_t smpr1_val = 0u;
        std::uint32_t smpr2_val = 0u;
        for (std::uint32_t ch = 0u; ch < 10u; ++ch) {
            smpr1_val |= (smp << (ch * detail::kModSmprSmpWidth));
        }
        for (std::uint32_t ch = 0u; ch < 9u; ++ch) {
            smpr2_val |= (smp << (ch * detail::kModSmprSmpWidth));
        }
        reg(detail::kModOfsSmpr1) = smpr1_val;
        reg(detail::kModOfsSmpr2) = smpr2_val;

        // Step 7: Enable ADC — wait for ADRDY.
        isr() = detail::kModIsrAdrdy;  // clear stale ADRDY
        cr() |= detail::kModCrAden;
        while ((isr() & detail::kModIsrAdrdy) == 0u) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // Conversion helpers — simple ADC
    // -----------------------------------------------------------------------

    static void select_channel_simple(std::uint8_t channel) noexcept {
        // CHSELR: bit N selects channel N.  Clear all, set only this channel.
        reg(detail::kSimOfsChselr) = 1u << channel;
    }

    // -----------------------------------------------------------------------
    // Conversion helpers — modern ADC
    // -----------------------------------------------------------------------

    static void select_channel_modern(std::uint8_t channel) noexcept {
        // SQR1: L=0 (1 conversion), SQ1=channel.
        reg(detail::kModOfsSqr1) =
            (static_cast<std::uint32_t>(channel) << detail::kModSqr1Sq1Shift) &
            (detail::kModSqr1Sq1Mask << detail::kModSqr1Sq1Shift);
    }

   public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Initialise the ADC: calibrate (optional), set resolution + sample time,
    /// and enable.
    ///
    /// For the modern ADC (G4/H7) the internal voltage regulator is started and
    /// the busy-loop waits `cfg.vreg_wait_cycles` before calibration.  Set this
    /// to match your system clock — see Config::vreg_wait_cycles.
    ///
    /// @param cfg  Configuration (defaults are safe for all families).
    static void configure(const Config& cfg = {}) noexcept {
        clock_on();
        if constexpr (kIsSimple) {
            init_simple(cfg);
        } else {
            init_modern(cfg);
        }
    }

    /// Disable the ADC (clear ADEN).
    static void disable() noexcept {
        if constexpr (kIsSimple) {
            if ((cr() & detail::kSimCrAden) != 0u) {
                cr() |= detail::kSimCrAddis;
                while ((cr() & detail::kSimCrAden) != 0u) { /* spin */ }
            }
        } else {
            if ((cr() & detail::kModCrAden) != 0u) {
                cr() |= detail::kModCrAddis;
                while ((cr() & detail::kModCrAden) != 0u) { /* spin */ }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Single-channel polling conversion
    // -----------------------------------------------------------------------

    /// Start a single regular conversion on `channel` and return the result.
    ///
    /// Blocking: polls ISR.EOC until the conversion is complete.
    ///
    /// @param channel  ADC channel number (e.g. 0 for IN0/PA0 on G071).
    /// @returns        Raw ADC result (right-aligned, 12–6 bit per resolution).
    [[nodiscard]] static auto convert(std::uint8_t channel) noexcept
        -> std::uint16_t {
        start_conversion(channel);
        while (!eoc()) { /* spin */ }
        return read_dr();
    }

    // -----------------------------------------------------------------------
    // Non-blocking conversion primitives
    // -----------------------------------------------------------------------

    /// Select `channel` and trigger a single regular conversion (ADSTART=1).
    ///
    /// Does not wait for completion.  Poll `eoc()` or use an interrupt, then
    /// call `read_dr()` to get the result.
    static void start_conversion(std::uint8_t channel) noexcept {
        // Clear any pending EOC/EOS flags before starting.
        if constexpr (kIsSimple) {
            isr() = detail::kSimIsrEoc | detail::kSimIsrEos | detail::kSimIsrOvr;
            select_channel_simple(channel);
            cr() |= detail::kSimCrAdstart;
        } else {
            isr() = detail::kModIsrEoc | detail::kModIsrEos | detail::kModIsrOvr;
            select_channel_modern(channel);
            cr() |= detail::kModCrAdstart;
        }
    }

    /// True when the most recent conversion is complete (ISR.EOC = 1).
    [[nodiscard]] static auto eoc() noexcept -> bool {
        if constexpr (kIsSimple) {
            return (isr() & detail::kSimIsrEoc) != 0u;
        } else {
            return (isr() & detail::kModIsrEoc) != 0u;
        }
    }

    /// True when the overrun flag is set (ISR.OVR = 1).
    ///
    /// An overrun occurs when a new conversion completes before the previous
    /// result has been read.  Clear by calling `clear_flags()`.
    [[nodiscard]] static auto overrun() noexcept -> bool {
        if constexpr (kIsSimple) {
            return (isr() & detail::kSimIsrOvr) != 0u;
        } else {
            return (isr() & detail::kModIsrOvr) != 0u;
        }
    }

    /// Clear EOC, EOS, and OVR flags in ISR.
    static void clear_flags() noexcept {
        if constexpr (kIsSimple) {
            isr() = detail::kSimIsrEoc | detail::kSimIsrEos | detail::kSimIsrOvr;
        } else {
            isr() = detail::kModIsrEoc | detail::kModIsrEos | detail::kModIsrOvr;
        }
    }

    /// Read the conversion result from DR (clears EOC in most ADC configurations).
    ///
    /// Call only after `eoc()` returns true, or after `convert()` returns.
    [[nodiscard]] static auto read_dr() noexcept -> std::uint16_t {
        if constexpr (kIsSimple) {
            return static_cast<std::uint16_t>(
                reg(detail::kSimOfsDr) & 0xFFFFu);
        } else {
            return static_cast<std::uint16_t>(
                reg(detail::kModOfsDr) & 0xFFFFu);
        }
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when the ADC is ready (ISR.ADRDY = 1, set after ADEN=1).
    [[nodiscard]] static auto ready() noexcept -> bool {
        if constexpr (kIsSimple) {
            return (isr() & detail::kSimIsrAdrdy) != 0u;
        } else {
            return (isr() & detail::kModIsrAdrdy) != 0u;
        }
    }

    /// True when the end-of-sequence flag is set (ISR.EOS = 1).
    ///
    /// Set after all channels in the regular sequence have been converted.
    /// In single-channel mode, EOS fires at the same time as EOC.
    [[nodiscard]] static auto eos() noexcept -> bool {
        if constexpr (kIsSimple) {
            return (isr() & detail::kSimIsrEos) != 0u;
        } else {
            return (isr() & detail::kModIsrEos) != 0u;
        }
    }

    // -----------------------------------------------------------------------
    // Continuous conversion mode
    // -----------------------------------------------------------------------

    /// Start continuous regular conversions on `channel`.
    ///
    /// Sets CFGR1/CFGR.CONT = 1 then starts ADSTART.  The ADC re-triggers
    /// itself immediately after each conversion — poll `eoc()` or enable the
    /// EOC interrupt.  Call `stop_conversions()` to halt.
    static void start_continuous(std::uint8_t channel) noexcept {
        clear_flags();
        if constexpr (kIsSimple) {
            select_channel_simple(channel);
            reg(detail::kSimOfsCfgr1) |= detail::kSimCfgr1Cont;
            cr() |= detail::kSimCrAdstart;
        } else {
            select_channel_modern(channel);
            reg(detail::kModOfsCfgr) |= detail::kModCfgrCont;
            cr() |= detail::kModCrAdstart;
        }
    }

    /// Stop any ongoing conversion (sets ADSTP, waits for it to clear).
    ///
    /// After stop, CONT is cleared automatically when the in-progress
    /// conversion finishes and the ADC halts.  Call `clear_flags()` before
    /// starting a new sequence.
    static void stop_conversions() noexcept {
        if constexpr (kIsSimple) {
            cr() |= detail::kSimCrAdstop;
            while ((cr() & detail::kSimCrAdstop) != 0u) { /* spin */ }
            reg(detail::kSimOfsCfgr1) &= ~detail::kSimCfgr1Cont;
        } else {
            cr() |= detail::kModCrAdstop;
            while ((cr() & detail::kModCrAdstop) != 0u) { /* spin */ }
            reg(detail::kModOfsCfgr) &= ~detail::kModCfgrCont;
        }
    }

    // -----------------------------------------------------------------------
    // Interrupt control
    // -----------------------------------------------------------------------

    /// Enable the EOC (end-of-conversion) interrupt.
    static void enable_eoc_irq() noexcept {
        if constexpr (kIsSimple) {
            ier() |= detail::kSimIerEocie;
        } else {
            ier() |= detail::kModIerEocie;
        }
    }

    /// Enable the EOS (end-of-sequence) interrupt.
    static void enable_eos_irq() noexcept {
        if constexpr (kIsSimple) {
            ier() |= detail::kSimIerEosie;
        } else {
            ier() |= detail::kModIerEosie;
        }
    }

    /// Enable the OVR (overrun) interrupt.
    static void enable_ovr_irq() noexcept {
        if constexpr (kIsSimple) {
            ier() |= detail::kSimIerOvrie;
        } else {
            ier() |= detail::kModIerOvrie;
        }
    }

    /// Disable all ADC interrupt sources (EOC, EOS, OVR).
    static void disable_irqs() noexcept {
        if constexpr (kIsSimple) {
            ier() &= ~(detail::kSimIerEocie | detail::kSimIerEosie | detail::kSimIerOvrie);
        } else {
            ier() &= ~(detail::kModIerEocie | detail::kModIerEosie | detail::kModIerOvrie);
        }
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

}  // namespace alloy::hal::adc::lite
