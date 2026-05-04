/// @file hal/lptim/lite.hpp
/// Lightweight, direct-MMIO LPTIM driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
///
/// Supports all STM32 LPTIM peripherals (G0 / G4 / H7 / L0 / L4 / WB / …):
///   kTemplate = "lptim"  (kIpVersion varies by family but layout is uniform)
///
/// Register layout (same across all families):
///   0x00  ISR    — CMPM(0) ARRM(1) EXTTRIG(2) CMPOK(3) ARROK(4) UP(5) DOWN(6)
///   0x04  ICR    — flag-clear register (write 1 to clear matching ISR bit)
///   0x08  IER    — interrupt-enable register
///   0x0C  CFGR   — CKSEL(0) CKPOL[2:1] CKFLT[4:3] TRGFLT[6:5] PRESC[11:9]
///                  TRIGSEL[14:13?] TRIGEN[16:15] TIMOUT(17) WAVE(18) WAVPOL(19)
///                  PRELOAD(20) COUNTMODE(21) ENC(22)
///   0x10  CR     — ENABLE(0) SNGSTRT(1) CNTSTRT(2) COUNTRST(3)
///   0x14  CMP    — 16-bit compare register (write after ENABLE=1, poll CMPOK)
///   0x18  ARR    — 16-bit auto-reload register (write after ENABLE=1, poll ARROK)
///   0x1C  CNT    — 16-bit counter (read-only)
///   0x24  CFGR2  — clock source selection on some families (G0/G4)
///
/// IMPORTANT: write order constraints from the hardware:
///   1. Write CFGR while ENABLE=0 (peripheral must be disabled).
///   2. Set ENABLE=1 (CR bit 0) before writing ARR or CMP.
///   3. After writing ARR, poll ISR.ARROK before starting.
///   4. After writing CMP, poll ISR.CMPOK before starting.
///
/// IMPORTANT: enable the peripheral clock before configure():
/// @code
///   dev::peripheral_on<dev::lptim1>();
/// @endcode
///
/// Typical usage — polled delay (internal clock, 32 MHz PCLK ÷ 128 ≈ 250 kHz):
/// @code
///   #include "hal/lptim/lite.hpp"
///   using Lpt = alloy::hal::lptim::lite::port<dev::lptim1>;
///
///   dev::peripheral_on<dev::lptim1>();
///   Lpt::configure({.prescaler = Prescaler::Div128});
///   Lpt::delay_ticks(250u);   // 250 × (1/250kHz) = 1 ms
/// @endcode
///
/// Continuous mode counter:
/// @code
///   Lpt::configure();          // default: Div1, preload on
///   Lpt::set_period(0xFFFFu);  // max ARR → count to 65535 then restart
///   Lpt::start_continuous();
///   ...
///   std::uint16_t v = Lpt::count();
/// @endcode
#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::lptim::lite {

// ============================================================================
// Detail
// ============================================================================

namespace detail {

inline constexpr std::uintptr_t kOfsIsr   = 0x00u;
inline constexpr std::uintptr_t kOfsIcr   = 0x04u;
inline constexpr std::uintptr_t kOfsIer   = 0x08u;
inline constexpr std::uintptr_t kOfsCfgr  = 0x0Cu;
inline constexpr std::uintptr_t kOfsCr    = 0x10u;
inline constexpr std::uintptr_t kOfsCmp   = 0x14u;
inline constexpr std::uintptr_t kOfsArr   = 0x18u;
inline constexpr std::uintptr_t kOfsCnt   = 0x1Cu;
inline constexpr std::uintptr_t kOfsCfgr2 = 0x24u;

// ISR / ICR bits
inline constexpr std::uint32_t kCmpm    = 1u << 0;  ///< Compare match
inline constexpr std::uint32_t kArrm    = 1u << 1;  ///< ARR match (overflow)
inline constexpr std::uint32_t kExtTrig = 1u << 2;  ///< External trigger
inline constexpr std::uint32_t kCmpok   = 1u << 3;  ///< CMP write complete
inline constexpr std::uint32_t kArrok   = 1u << 4;  ///< ARR write complete

// CR bits
inline constexpr std::uint32_t kCrEnable   = 1u << 0;  ///< Peripheral enable
inline constexpr std::uint32_t kCrSngstrt  = 1u << 1;  ///< Single-shot start
inline constexpr std::uint32_t kCrCntstrt  = 1u << 2;  ///< Continuous start
inline constexpr std::uint32_t kCrCountrst = 1u << 3;  ///< Counter reset

// CFGR bits
inline constexpr std::uint32_t kCfgrCksel    = 1u << 0;   ///< 1 = external clock
inline constexpr std::uint32_t kCfgrPrescShift = 9u;       ///< PRESC[11:9]
inline constexpr std::uint32_t kCfgrPrescMask  = 7u << 9;
inline constexpr std::uint32_t kCfgrWave       = 1u << 18; ///< Waveform output (WAVE=1: pin set at period reset, cleared on CMP match)
inline constexpr std::uint32_t kCfgrWavpol     = 1u << 19; ///< Waveform polarity (WAVPOL=1: inverted)
inline constexpr std::uint32_t kCfgrPreload    = 1u << 20; ///< Register preload (ARR/CMP buffered)

[[nodiscard]] consteval auto is_lptim(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "lptim";
}

}  // namespace detail

// ============================================================================
// Concept
// ============================================================================

/// Satisfied by any STM32 LPTIM peripheral (all families).
template <typename P>
concept StLptim =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_lptim(P::kTemplate);

// ============================================================================
// Enumerations
// ============================================================================

/// LPTIM internal clock prescaler (CFGR.PRESC).
///
/// Effective tick rate = source_clock / prescaler_div.
enum class Prescaler : std::uint8_t {
    Div1   = 0,
    Div2   = 1,
    Div4   = 2,
    Div8   = 3,
    Div16  = 4,
    Div32  = 5,
    Div64  = 6,
    Div128 = 7,
};

// ============================================================================
// Configuration
// ============================================================================

struct Config {
    /// Clock prescaler (default: Div1 — full PCLK speed).
    Prescaler prescaler{Prescaler::Div1};

    /// Enable register preloading (CFGR.PRELOAD).
    ///
    /// When true, writes to ARR and CMP are buffered and only take effect at
    /// the next ARR/CMP match — avoids glitches during live updates.
    /// When false, writes take effect immediately (suitable for one-shot setup).
    bool preload{true};

    /// Enable waveform output on the LPTIM_OUT pin (CFGR.WAVE = 1).
    ///
    /// When true, the output pin is set at every period reset and cleared on
    /// the CMP match — producing a PWM waveform.  Duty cycle = CMP / ARR.
    /// Requires: GPIO pin configured for the LPTIM_OUT alternate function.
    bool pwm_output{false};

    /// Invert the waveform polarity (CFGR.WAVPOL = 1).
    ///
    /// When true the pin is cleared at period reset and set on CMP match
    /// (duty cycle is then (ARR − CMP) / ARR).
    bool pwm_inverted{false};
};

// ============================================================================
// port<P>
// ============================================================================

/// Direct-MMIO LPTIM driver.  P must satisfy StLptim.
///
/// Write-order rules enforced by this driver:
///   - configure() disables the peripheral before writing CFGR.
///   - set_period() and set_compare() wait for ARROK/CMPOK.
///   - start_continuous() / start_single() require prior enable().
template <typename P>
    requires StLptim<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

   private:
    [[nodiscard]] static auto reg(std::uintptr_t offset) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + offset);
    }

   public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Configure the LPTIM (writes CFGR — peripheral must be disabled).
    ///
    /// Disables the peripheral if it is currently running before writing.
    /// After configure(), call enable() then set_period() before starting.
    static void configure(const Config& cfg = {}) noexcept {
        clock_on();
        // Must be disabled before writing CFGR.
        reg(detail::kOfsCr) = 0u;

        std::uint32_t cfgr = 0u;
        cfgr |= (static_cast<std::uint32_t>(cfg.prescaler) << detail::kCfgrPrescShift);
        if (cfg.preload)       { cfgr |= detail::kCfgrPreload; }
        if (cfg.pwm_output)    { cfgr |= detail::kCfgrWave; }
        if (cfg.pwm_inverted)  { cfgr |= detail::kCfgrWavpol; }
        reg(detail::kOfsCfgr) = cfgr;
    }

    /// Enable the peripheral (CR.ENABLE = 1).
    ///
    /// Must be called before writing ARR or CMP.
    static void enable() noexcept {
        reg(detail::kOfsCr) |= detail::kCrEnable;
    }

    /// Disable the peripheral (CR.ENABLE = 0).
    static void disable() noexcept {
        reg(detail::kOfsCr) = 0u;
        // Clear all pending ISR flags so the next configure() starts clean.
        reg(detail::kOfsIcr) =
            detail::kCmpm | detail::kArrm | detail::kExtTrig |
            detail::kCmpok | detail::kArrok;
    }

    // -----------------------------------------------------------------------
    // ARR / CMP writes (require ENABLE = 1)
    // -----------------------------------------------------------------------

    /// Write the auto-reload register and wait for ARROK.
    ///
    /// Defines the counting period: counter counts 0 → ARR, then restarts.
    /// Must be written while ENABLE = 1.
    ///
    /// @param arr  16-bit reload value.  Must be > CMP.
    static void set_period(std::uint16_t arr) noexcept {
        // Clear ARROK before writing so we can reliably detect the new ack.
        reg(detail::kOfsIcr) = detail::kArrok;
        reg(detail::kOfsArr) = static_cast<std::uint32_t>(arr);
        while ((reg(detail::kOfsIsr) & detail::kArrok) == 0u) { /* spin */ }
    }

    /// Write the compare register and wait for CMPOK.
    ///
    /// When the counter matches CMP, the CMPM flag is set (and the waveform
    /// output toggles if configured).  Must be written while ENABLE = 1.
    ///
    /// @param cmp  16-bit compare value.  Must be < ARR.
    static void set_compare(std::uint16_t cmp) noexcept {
        reg(detail::kOfsIcr) = detail::kCmpok;
        reg(detail::kOfsCmp) = static_cast<std::uint32_t>(cmp);
        while ((reg(detail::kOfsIsr) & detail::kCmpok) == 0u) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // Start / stop
    // -----------------------------------------------------------------------

    /// Start the counter in continuous mode (counts repeatedly 0 → ARR).
    ///
    /// Requires: enable() and set_period() called first.
    static void start_continuous() noexcept {
        reg(detail::kOfsCr) |= detail::kCrCntstrt;
    }

    /// Start the counter in single-shot mode (stops after one ARR match).
    ///
    /// Requires: enable() and set_period() called first.
    static void start_single() noexcept {
        reg(detail::kOfsCr) |= detail::kCrSngstrt;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// Read the current 16-bit counter value.
    [[nodiscard]] static auto count() noexcept -> std::uint16_t {
        // CNT may be updated asynchronously; read twice and take if identical.
        std::uint32_t a = reg(detail::kOfsCnt);
        std::uint32_t b = reg(detail::kOfsCnt);
        while (a != b) { a = b; b = reg(detail::kOfsCnt); }
        return static_cast<std::uint16_t>(a & 0xFFFFu);
    }

    /// True when the compare-match flag is set (ISR.CMPM).
    [[nodiscard]] static auto compare_match() noexcept -> bool {
        return (reg(detail::kOfsIsr) & detail::kCmpm) != 0u;
    }

    /// True when the ARR-match (overflow) flag is set (ISR.ARRM).
    [[nodiscard]] static auto overflow() noexcept -> bool {
        return (reg(detail::kOfsIsr) & detail::kArrm) != 0u;
    }

    /// Clear CMPM and ARRM flags in ISR.
    static void clear_flags() noexcept {
        reg(detail::kOfsIcr) = detail::kCmpm | detail::kArrm;
    }

    // -----------------------------------------------------------------------
    // Convenience: blocking delay
    // -----------------------------------------------------------------------

    /// Block for `ticks` counter periods using single-shot mode.
    ///
    /// Configures LPTIM with the given prescaler, starts a single-shot
    /// count, and waits for ARRM.  Leaves the peripheral disabled afterward.
    ///
    /// @param ticks  Number of ticks to wait (1 tick = 1 / (clock / prescaler)).
    static void delay_ticks(std::uint32_t ticks) noexcept {
        // The maximum single ARR is 0xFFFF (65535 ticks).
        // For longer delays, iterate.
        while (ticks > 0u) {
            const auto chunk = static_cast<std::uint16_t>(
                ticks > 0xFFFFu ? 0xFFFFu : ticks);

            disable();
            // CFGR already written by configure() — no need to re-write.
            enable();
            set_period(chunk);
            clear_flags();
            start_single();

            while (!overflow()) { /* spin */ }

            ticks -= static_cast<std::uint32_t>(chunk);
        }
        disable();
    }

    // -----------------------------------------------------------------------
    // Interrupt enables
    // -----------------------------------------------------------------------

    /// Enable the ARR-match (overflow) interrupt.
    static void enable_overflow_irq() noexcept {
        reg(detail::kOfsIer) |= detail::kArrm;
    }

    /// Enable the compare-match interrupt.
    static void enable_compare_irq() noexcept {
        reg(detail::kOfsIer) |= detail::kCmpm;
    }

    /// Disable all LPTIM interrupts.
    static void disable_irqs() noexcept {
        reg(detail::kOfsIer) = 0u;
    }

    // -----------------------------------------------------------------------
    // Convenience: PWM output
    // -----------------------------------------------------------------------

    /// Configure and start a continuous PWM signal on the LPTIM_OUT pin.
    ///
    /// The output is set at every period reload and cleared on the compare
    /// match.  With `inverted = false`:
    ///   duty_cycle = compare / period    (0.0 … 1.0)
    ///
    /// @param psc      Clock prescaler (sets the tick rate).
    /// @param period   ARR value — counter counts 0 → period, then reloads.
    /// @param compare  CMP value — output goes low when counter == compare.
    ///                 Must be < period; 0 → always low, period+1 → always high.
    /// @param inverted true = inverted polarity (WAVPOL=1).
    static void configure_pwm(Prescaler    psc,
                               std::uint16_t period,
                               std::uint16_t compare,
                               bool          inverted = false) noexcept {
        configure({
            .prescaler    = psc,
            .preload      = true,
            .pwm_output   = true,
            .pwm_inverted = inverted,
        });
        enable();
        set_period(period);
        set_compare(compare);
        clear_flags();
        start_continuous();
    }

    /// Update the duty cycle of a running PWM without stopping the counter.
    ///
    /// Writes the new compare value and waits for CMPOK.
    /// Safe to call while the counter is running (PRELOAD buffers the write).
    static void set_pwm_compare(std::uint16_t compare) noexcept {
        set_compare(compare);
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

    port() = delete;
};

}  // namespace alloy::hal::lptim::lite
