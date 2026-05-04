/// @file hal/timer/lite.hpp
/// Lightweight, direct-MMIO general-purpose timer driver for alloy.device.v2.1.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
/// Works whenever `ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE` is true.
///
/// Supports:
///   - ST general-purpose / advanced timers (TIM1/2/3/…): kTemplate == "tim"
///     — same register layout across all STM32 families.
///
/// Two concept tiers
/// -----------------
/// `StTimer<P>`    — any TIM peripheral.  Time-base, delay, count methods.
/// `StPwmTimer<P>` — TIM peripheral with output-channel signals (ch1..ch4).
///                   Adds configure_pwm_ch / set_duty / enable_ch / disable_ch.
///
/// Register layout (all STM32 TIMx, both basic and general-purpose):
///   0x00 CR1      — CEN, UDIS, URS, OPM, DIR, ARPE, CKD
///   0x04 CR2      — CCDS, MMS, TI1S (general/advanced only)
///   0x08 SMCR     — slave mode (general/advanced only)
///   0x0C DIER     — update / CC interrupt enables
///   0x10 SR       — update flag (UIF) + CC flags
///   0x14 EGR      — UG (force-update) + CCxG
///   0x18 CCMR1   — output compare mode for CH1/CH2
///   0x1C CCMR2   — output compare mode for CH3/CH4
///   0x20 CCER     — output enable / polarity per channel
///   0x24 CNT      — counter (16-bit or 32-bit on TIM2/TIM5)
///   0x28 PSC      — prescaler (16-bit)
///   0x2C ARR      — auto-reload register (period)
///   0x30 RCR      — repetition counter (advanced only)
///   0x34 CCR1     — capture/compare register CH1
///   0x38 CCR2     — capture/compare register CH2
///   0x3C CCR3     — capture/compare register CH3
///   0x40 CCR4     — capture/compare register CH4
///   0x44 BDTR     — break/dead-time (advanced TIM1/TIM8 only)
///
/// IMPORTANT: enable the timer clock before calling `configure()`:
/// @code
///   namespace dev = alloy::device::traits;
///   dev::peripheral_on<dev::tim3>();
/// @endcode
///
/// Typical usage — time-base polling 1 kHz tick on TIM3 at 64 MHz:
/// @code
///   #include "hal/timer.hpp"
///   #include "hal/rcc.hpp"
///   #include "device/runtime.hpp"
///
///   namespace dev = alloy::device::traits;
///   using Tim3 = alloy::hal::timer::lite::port<dev::tim3>;
///
///   dev::peripheral_on<dev::tim3>();
///   Tim3::configure(63999u, 999u);   // PSC=63999, ARR=999 → 1 kHz at 64 MHz
///   Tim3::start();
///   while (!Tim3::update_pending()) { /* tick */ }
///   Tim3::clear_update();
/// @endcode
///
/// Typical usage — 50 Hz PWM on TIM3 CH1 at 64 MHz (period = 20 ms):
/// @code
///   dev::peripheral_on<dev::tim3>();
///   using Tim3 = alloy::hal::timer::lite::port<dev::tim3>;
///   using namespace alloy::hal::timer::lite;
///
///   Tim3::configure(63u, 19999u);            // PSC=63, ARR=19999 → 50 Hz
///   Tim3::configure_pwm_ch(Channel::Ch1);
///   Tim3::set_duty(Channel::Ch1, 1499u);     // ~7.5% duty = 1.5 ms (servo centre)
///   Tim3::enable_ch(Channel::Ch1);
///   Tim3::start();
/// @endcode
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::timer::lite {

// ============================================================================
// Detail — register layout
// ============================================================================

namespace detail {

/// STM32 TIM register map.
/// Basic timers (TIM6/TIM7) only use CR1, DIER, SR, EGR, CNT, PSC, ARR.
/// The remaining fields are reserved on basic timers — do not write them.
struct tim_regs {
    std::uint32_t cr1;    ///< 0x00
    std::uint32_t cr2;    ///< 0x04
    std::uint32_t smcr;   ///< 0x08
    std::uint32_t dier;   ///< 0x0C
    std::uint32_t sr;     ///< 0x10
    std::uint32_t egr;    ///< 0x14 (write-only)
    std::uint32_t ccmr1;  ///< 0x18
    std::uint32_t ccmr2;  ///< 0x1C
    std::uint32_t ccer;   ///< 0x20
    std::uint32_t cnt;    ///< 0x24
    std::uint32_t psc;    ///< 0x28
    std::uint32_t arr;    ///< 0x2C
    std::uint32_t rcr;    ///< 0x30 (advanced only)
    std::uint32_t ccr1;   ///< 0x34
    std::uint32_t ccr2;   ///< 0x38
    std::uint32_t ccr3;   ///< 0x3C
    std::uint32_t ccr4;   ///< 0x40
    std::uint32_t bdtr;   ///< 0x44 (advanced only)
};

// CR1 bits
inline constexpr std::uint32_t kCr1Cen  = 1u << 0;  ///< Counter enable
inline constexpr std::uint32_t kCr1Opm  = 1u << 3;  ///< One-pulse mode
inline constexpr std::uint32_t kCr1Arpe = 1u << 7;  ///< Auto-reload preload enable

// EGR bits (write-only — reading always returns 0)
inline constexpr std::uint32_t kEgrUg   = 1u << 0;  ///< Update generation (reinitialise)

// SR bits
inline constexpr std::uint32_t kSrUif   = 1u << 0;  ///< Update interrupt flag

// DIER bits
inline constexpr std::uint32_t kDierUie = 1u << 0;  ///< Update interrupt enable
inline constexpr std::uint32_t kDierCc1ie = 1u << 1;
inline constexpr std::uint32_t kDierCc2ie = 1u << 2;
inline constexpr std::uint32_t kDierCc3ie = 1u << 3;
inline constexpr std::uint32_t kDierCc4ie = 1u << 4;

// CCER: output enable bits per channel (step = 4 bits per channel)
inline constexpr std::uint32_t kCcerCc1e = 1u << 0;   ///< CH1 output enable
inline constexpr std::uint32_t kCcerCc2e = 1u << 4;   ///< CH2 output enable
inline constexpr std::uint32_t kCcerCc3e = 1u << 8;   ///< CH3 output enable
inline constexpr std::uint32_t kCcerCc4e = 1u << 12;  ///< CH4 output enable

// CCMR output compare mode bits
// CCMRx_OUTPUT layout (per two channels):
//   [1:0]   CCxS  — 00 = output
//   [2]     OCxFE — fast enable
//   [3]     OCxPE — preload enable
//   [6:4]   OCxM  — 000=frozen, 001=active, 010=inactive, 011=toggle,
//                   100=force-low, 101=force-high, 110=PWM1, 111=PWM2
// Second channel in the same register occupies [9:8], [10], [11], [14:12].

// PWM mode 1 = 0b110
inline constexpr std::uint32_t kPwmMode1 = 6u;

// CCMR1 masks for CH1 and CH2
inline constexpr std::uint32_t kCcmr1Ch1ModeMask = 0x7u << 4;
inline constexpr std::uint32_t kCcmr1Ch2ModeMask = 0x7u << 12;
inline constexpr std::uint32_t kCcmr1Ch1Oce      = 1u << 2;   ///< CH1 fast enable
inline constexpr std::uint32_t kCcmr1Ch1Ocpe     = 1u << 3;   ///< CH1 preload enable
inline constexpr std::uint32_t kCcmr1Ch2Ocpe     = 1u << 11;  ///< CH2 preload enable

// CCMR2 masks for CH3 and CH4
inline constexpr std::uint32_t kCcmr2Ch3ModeMask = 0x7u << 4;
inline constexpr std::uint32_t kCcmr2Ch4ModeMask = 0x7u << 12;
inline constexpr std::uint32_t kCcmr2Ch3Ocpe     = 1u << 3;   ///< CH3 preload enable
inline constexpr std::uint32_t kCcmr2Ch4Ocpe     = 1u << 11;  ///< CH4 preload enable

// BDTR — Main Output Enable (advanced timers only)
inline constexpr std::uint32_t kBdtrMoe = 1u << 15;

// SR capture/compare flags (CC1IF..CC4IF) — same layout as DIER CC bits
inline constexpr std::uint32_t kSrCc1if = 1u << 1;
inline constexpr std::uint32_t kSrCc2if = 1u << 2;
inline constexpr std::uint32_t kSrCc3if = 1u << 3;
inline constexpr std::uint32_t kSrCc4if = 1u << 4;

// SMCR — slave mode control / encoder mode (bits [2:0] = SMS)
inline constexpr std::uint32_t kSmcrSmsMask = 0x7u;   ///< SMS[2:0] — slave mode select

// CCMR input-capture CCxS field values (2 bits per channel in CCMRx)
// CC1S / CC3S at [1:0], CC2S / CC4S at [9:8] within the same register.
// 0b01 = TIx input (direct), 0b10 = cross-TIx input.
inline constexpr std::uint32_t kCcmrCcxsInput = 0x1u;   ///< CCxS = 01: input on TIx

// CCER polarity bit offset per channel (step 4: CCxP is bit 1 of each group)
inline constexpr std::uint32_t kCcerCcxPolarityShift = 1u;

[[nodiscard]] consteval auto is_st_timer(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "tim";
}

}  // namespace detail

// ============================================================================
// Enumerations
// ============================================================================

/// PWM / capture-compare channel selector.
enum class Channel : std::uint8_t { Ch1 = 0, Ch2 = 1, Ch3 = 2, Ch4 = 3 };

/// Encoder mode — controls which input edges increment/decrement the counter.
///
/// Maps directly to SMCR.SMS[2:0].
enum class EncoderMode : std::uint8_t {
    TI1  = 1,  ///< Count on TI1 edges only  (×2 mode)
    TI2  = 2,  ///< Count on TI2 edges only  (×2 mode)
    Both = 3,  ///< Count on both TI1 and TI2 edges (×4 mode)
};

// ============================================================================
// Concepts
// ============================================================================

/// Satisfied when P is any ST TIM peripheral.
template <typename P>
concept StTimer =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_st_timer(P::kTemplate);

/// Satisfied when P is a TIM peripheral that exposes output-channel signals
/// (ch1..ch4).  Basic timers (TIM6/TIM7) have no channel signals and
/// therefore do not satisfy this concept — PWM methods must not be called on
/// them.
template <typename P>
concept StPwmTimer =
    StTimer<P> &&
    requires {
        { P::kSignalCount } -> std::convertible_to<unsigned>;
    } &&
    (P::kSignalCount >= 1u);

// ============================================================================
// port<P> — zero-size type, all methods static
// ============================================================================

/// Direct-MMIO timer port.  P must satisfy StTimer.
///
/// PWM methods (configure_pwm_ch, set_duty, enable_ch, disable_ch,
/// enable_main_output) require StPwmTimer<P> and use a `requires` clause to
/// produce a clear diagnostic on misuse.
template <typename P>
    requires StTimer<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

   private:
    [[nodiscard]] static auto r() noexcept -> volatile detail::tim_regs& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile detail::tim_regs*>(kBase);
    }

    /// Return the CCER enable-bit for `ch`.
    [[nodiscard]] static constexpr auto ccer_mask(Channel ch) noexcept
        -> std::uint32_t
    {
        return std::uint32_t{1u} << (static_cast<unsigned>(ch) * 4u);
    }

    /// Write CCRx for the given channel.
    static void write_ccr(Channel ch, std::uint32_t value) noexcept {
        volatile std::uint32_t* ccr_base =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<volatile std::uint32_t*>(kBase + 0x34u);
        ccr_base[static_cast<unsigned>(ch)] = value;
    }

    /// Read CCRx for the given channel.
    [[nodiscard]] static auto read_ccr(Channel ch) noexcept -> std::uint32_t {
        const volatile std::uint32_t* ccr_base =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const volatile std::uint32_t*>(kBase + 0x34u);
        return ccr_base[static_cast<unsigned>(ch)];
    }

   public:
    // -----------------------------------------------------------------------
    // Time-base configuration (all timer types)
    // -----------------------------------------------------------------------

    /// Configure the time-base but do not start the counter.
    ///
    /// The effective period is `(psc + 1) * (arr + 1)` kernel-clock cycles.
    ///
    /// @param psc  Prescaler value (0 = divide by 1, 0xFFFF = divide by 65536).
    /// @param arr  Auto-reload register (0 = period of 1 tick).
    static void configure(std::uint16_t psc, std::uint32_t arr) noexcept {
        clock_on();
        r().cr1 = detail::kCr1Arpe;  // preload ARR; counter stopped
        r().psc = psc;
        r().arr = arr;
        // Force an update event so PSC/ARR are transferred to their shadow
        // registers immediately (otherwise they would take effect on the
        // next overflow).
        r().egr = detail::kEgrUg;
        r().sr  = 0u;               // clear UIF set by EGR.UG
    }

    // -----------------------------------------------------------------------
    // Counter control
    // -----------------------------------------------------------------------

    /// Start the counter (set CEN).
    static void start() noexcept { r().cr1 |= detail::kCr1Cen; }

    /// Stop the counter (clear CEN).
    static void stop() noexcept { r().cr1 &= ~detail::kCr1Cen; }

    /// Force a counter reinitialisation (clears CNT and copies shadow regs).
    static void reinit() noexcept {
        r().egr = detail::kEgrUg;
        r().sr  = 0u;  // clear resulting UIF
    }

    /// Read the current counter value.
    [[nodiscard]] static auto count() noexcept -> std::uint32_t {
        return r().cnt;
    }

    // -----------------------------------------------------------------------
    // Update-event (overflow) flag
    // -----------------------------------------------------------------------

    /// True when the counter has overflowed / the update event has fired.
    [[nodiscard]] static auto update_pending() noexcept -> bool {
        return (r().sr & detail::kSrUif) != 0u;
    }

    /// Clear the update-event flag (write 0 to UIF).
    static void clear_update() noexcept {
        r().sr = ~detail::kSrUif;  // RC_W0: write 0 to clear, 1 preserves
    }

    // -----------------------------------------------------------------------
    // Update-interrupt control
    // -----------------------------------------------------------------------

    /// Enable the update interrupt (UIE in DIER).
    static void enable_update_irq() noexcept {
        r().dier |= detail::kDierUie;
    }

    /// Disable the update interrupt.
    static void disable_update_irq() noexcept {
        r().dier &= ~detail::kDierUie;
    }

    // -----------------------------------------------------------------------
    // Blocking delay (one-pulse mode)
    // -----------------------------------------------------------------------

    /// Block for exactly `ticks` counter periods.
    ///
    /// Uses One-Pulse Mode so the counter stops automatically.
    /// The prescaler configured via `configure()` determines the tick period.
    /// Does not disturb PWM output channels.
    static void delay_ticks(std::uint32_t ticks) noexcept {
        r().cr1 &= ~detail::kCr1Cen;           // stop
        r().cnt  = 0u;
        r().arr  = ticks;
        r().egr  = detail::kEgrUg;              // reinit
        r().sr   = 0u;                          // clear UIF
        r().cr1 |= detail::kCr1Opm | detail::kCr1Cen;  // one-pulse + start
        while ((r().sr & detail::kSrUif) == 0u) { /* spin */ }
        r().sr   = 0u;
        r().cr1 &= ~detail::kCr1Opm;            // restore
    }

    // -----------------------------------------------------------------------
    // PWM configuration (requires StPwmTimer<P>)
    // -----------------------------------------------------------------------

    /// Configure one output channel in PWM Mode 1 (output active while CNT < CCRx).
    ///
    /// Call `enable_ch(ch)` and `start()` afterwards.
    /// For advanced timers (TIM1/TIM8) also call `enable_main_output()`.
    ///
    /// @param ch     Channel to configure (Ch1..Ch4).
    template <Channel Ch = Channel::Ch1>
    static void configure_pwm_ch()
        requires StPwmTimer<P>
    {
        r().cr1 &= ~detail::kCr1Cen;  // stop while reconfiguring

        constexpr auto ch = static_cast<unsigned>(Ch);

        if constexpr (ch == 0u || ch == 1u) {
            // CCMR1 covers CH1 (bits 6:4) and CH2 (bits 14:12)
            const std::uint32_t mode_shift = (ch == 0u) ? 4u : 12u;
            const std::uint32_t mode_mask  = 0x7u << mode_shift;
            const std::uint32_t ocpe_bit   = (ch == 0u)
                ? detail::kCcmr1Ch1Ocpe : detail::kCcmr1Ch2Ocpe;
            r().ccmr1 = (r().ccmr1 & ~mode_mask) |
                        (detail::kPwmMode1 << mode_shift) |
                        ocpe_bit;
        } else {
            // CCMR2 covers CH3 (bits 6:4) and CH4 (bits 14:12)
            const std::uint32_t mode_shift = (ch == 2u) ? 4u : 12u;
            const std::uint32_t mode_mask  = 0x7u << mode_shift;
            const std::uint32_t ocpe_bit   = (ch == 2u)
                ? detail::kCcmr2Ch3Ocpe : detail::kCcmr2Ch4Ocpe;
            r().ccmr2 = (r().ccmr2 & ~mode_mask) |
                        (detail::kPwmMode1 << mode_shift) |
                        ocpe_bit;
        }
    }

    /// Update the duty-cycle value for `ch` (CCRx ← `ccr`).
    ///
    /// `ccr` must be ≤ ARR.  Writing 0 produces 0 % duty;
    /// writing ARR+1 produces 100 % duty (pin always active).
    static void set_duty(Channel ch, std::uint32_t ccr)
        requires StPwmTimer<P>
    {
        write_ccr(ch, ccr);
    }

    /// Read the current duty-cycle value for `ch`.
    [[nodiscard]] static auto duty(Channel ch) noexcept -> std::uint32_t
        requires StPwmTimer<P>
    {
        return read_ccr(ch);
    }

    /// Enable the output pin for `ch` (set CCxE in CCER).
    static void enable_ch(Channel ch)
        requires StPwmTimer<P>
    {
        r().ccer |= ccer_mask(ch);
    }

    /// Disable the output pin for `ch` (clear CCxE in CCER).
    static void disable_ch(Channel ch)
        requires StPwmTimer<P>
    {
        r().ccer &= ~ccer_mask(ch);
    }

    /// Enable the main output (set MOE in BDTR).
    ///
    /// Required on advanced timers (TIM1 / TIM8) before PWM signals appear
    /// on the pins.  Calling this on basic or general-purpose timers writes
    /// to a reserved address — harmless but unnecessary.
    static void enable_main_output()
        requires StPwmTimer<P>
    {
        r().bdtr |= detail::kBdtrMoe;
    }

    /// Disable the main output (clear MOE).
    static void disable_main_output()
        requires StPwmTimer<P>
    {
        r().bdtr &= ~detail::kBdtrMoe;
    }

    // -----------------------------------------------------------------------
    // One-pulse mode
    // -----------------------------------------------------------------------

    /// Enable or disable one-pulse mode (CR1.OPM).
    ///
    /// When enabled, the counter stops (CEN cleared) automatically after the
    /// first update event.  Useful for generating a precisely timed single
    /// pulse: set ARR to the desired delay, start(), wait for update_pending().
    static void set_one_pulse(bool enable) noexcept {
        if (enable) {
            r().cr1 |= detail::kCr1Opm;
        } else {
            r().cr1 &= ~detail::kCr1Opm;
        }
    }

    // -----------------------------------------------------------------------
    // Capture/compare interrupt control
    // -----------------------------------------------------------------------

    /// Enable the CC interrupt for `ch` (sets CCxIE in DIER).
    static void enable_cc_irq(Channel ch) noexcept {
        // DIER: CC1IE=bit1, CC2IE=bit2, CC3IE=bit3, CC4IE=bit4
        r().dier |= 1u << (static_cast<std::uint32_t>(ch) + 1u);
    }

    /// Disable the CC interrupt for `ch` (clears CCxIE in DIER).
    static void disable_cc_irq(Channel ch) noexcept {
        r().dier &= ~(1u << (static_cast<std::uint32_t>(ch) + 1u));
    }

    /// True when the CC flag for `ch` is set in SR (CCxIF).
    [[nodiscard]] static auto cc_pending(Channel ch) noexcept -> bool {
        return (r().sr & (1u << (static_cast<std::uint32_t>(ch) + 1u))) != 0u;
    }

    /// Clear the CC flag for `ch` in SR (write 0 to CCxIF — RC_W0 register).
    static void clear_cc(Channel ch) noexcept {
        r().sr &= ~(1u << (static_cast<std::uint32_t>(ch) + 1u));
    }

    // -----------------------------------------------------------------------
    // Input capture
    // -----------------------------------------------------------------------

    /// Configure `ch` as an input capture on its direct TIx input.
    ///
    /// Sets CCxS = 01 (direct input) in the appropriate CCMR register,
    /// enables the capture via CCxE in CCER, and sets polarity (CCxP).
    /// The counter must be started with `start()` after configuring pins.
    ///
    /// @param ch        Channel to configure.
    /// @param inverted  true = capture on falling edge; false = rising edge.
    static void configure_input_capture(Channel ch,
                                        bool inverted = false) noexcept {
        const auto ch_idx = static_cast<std::uint32_t>(ch);

        // CCMRx: CCxS = 01 (direct TIx input), OCxM/OCxPE cleared.
        // Channel pairs: CCMR1 covers CH1(bits[1:0]) + CH2(bits[9:8]),
        //                CCMR2 covers CH3(bits[1:0]) + CH4(bits[9:8]).
        if (ch_idx < 2u) {
            const auto shift = ch_idx * 8u;
            r().ccmr1 = (r().ccmr1 & ~(0xFFu << shift)) |
                        (detail::kCcmrCcxsInput << shift);
        } else {
            const auto shift = (ch_idx - 2u) * 8u;
            r().ccmr2 = (r().ccmr2 & ~(0xFFu << shift)) |
                        (detail::kCcmrCcxsInput << shift);
        }

        // CCER: enable capture (CCxE=bit0 of the group) + polarity (CCxP=bit1).
        const auto ccer_shift = ch_idx * 4u;
        const std::uint32_t ccer_bits =
            0x1u | (inverted ? 0x2u : 0x0u);  // CCxE + CCxP
        r().ccer = (r().ccer & ~(0xFu << ccer_shift)) |
                   (ccer_bits << ccer_shift);
    }

    /// Read the captured counter value for `ch` (CCRx register).
    ///
    /// Reading CCRx also clears the CCxOF over-capture flag on most STM32
    /// families (check the RM for your device — generally safe to ignore).
    [[nodiscard]] static auto capture(Channel ch) noexcept -> std::uint32_t {
        return read_ccr(ch);
    }

    // -----------------------------------------------------------------------
    // Encoder mode
    // -----------------------------------------------------------------------

    /// Configure the timer as a quadrature encoder interface.
    ///
    /// Sets SMCR.SMS to the requested encoder mode, configures CH1 and CH2
    /// as direct TI inputs (CCMR1: CC1S=01, CC2S=01), enables both capture
    /// channels (CCER: CC1E + CC2E), and starts the counter.
    ///
    /// Typical use: shaft position / velocity measurement with incremental
    /// encoders.  Read `count()` for the current position; direction is
    /// reflected in CR1.DIR (bit 4 of CR1, set when counting down).
    ///
    /// @param mode  TI1, TI2 (×2), or Both (×4 — recommended for most encoders).
    static void configure_encoder(EncoderMode mode) noexcept {
        r().cr1 &= ~detail::kCr1Cen;   // stop while reconfiguring

        // Slave mode: SMS[2:0] = encoder mode
        r().smcr = (r().smcr & ~detail::kSmcrSmsMask) |
                   static_cast<std::uint32_t>(mode);

        // Both channels as direct TI inputs (CC1S=01, CC2S=01)
        r().ccmr1 = (detail::kCcmrCcxsInput << 0u) |   // CC1S
                    (detail::kCcmrCcxsInput << 8u);     // CC2S

        // Enable CH1 + CH2 captures (CC1E + CC2E, active-high polarity)
        r().ccer = (r().ccer & ~0xFFu) | 0x11u;

        // Zero the counter and start
        r().cnt = 0u;
        r().cr1 |= detail::kCr1Cen;
    }

    /// True when the counter is counting downward (CR1.DIR = 1).
    ///
    /// Meaningful only in encoder mode or centre-aligned PWM mode.
    [[nodiscard]] static auto counting_down() noexcept -> bool {
        return (r().cr1 & (1u << 4u)) != 0u;  // CR1.DIR
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    /// Advanced timers (TIM1/TIM8) expose multiple IRQ lines (BRK, UP, TRG, CC).
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

}  // namespace alloy::hal::timer::lite
