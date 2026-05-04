/// @file hal/systick/lite.hpp
/// Lightweight Cortex-M SysTick driver — zero external dependencies.
///
/// Provides two operating modes:
///
///   Polling mode (no ISR needed):
///     Uses SysTick COUNTFLAG (CTRL bit 16) to count elapsed ticks without
///     any interrupt.  Accurate to one SysTick period.  Suitable for startup
///     delays, simple blocking waits, and systems that don't run an RTOS.
///
///   Interrupt mode:
///     Enables the SysTick exception (TICKINT).  Call `increment()` from
///     SysTick_Handler to maintain the tick counter.  Provides `millis()`,
///     `ticks()`, and `delay_ms()` based on the accumulated count.
///
/// Register map (Cortex-M SysTick at 0xE000E010):
///   0x00  CTRL  — ENABLE(0) TICKINT(1) CLKSOURCE(2) COUNTFLAG(16, rc)
///   0x04  LOAD  — 24-bit reload value (counter restarts from this)
///   0x08  VAL   — current counter value (write any value to clear to 0)
///   0x0C  CALIB — calibration (TENMS[23:0], SKEW(30), NOREF(31))
///
/// Clock source (CTRL.CLKSOURCE):
///   1 = processor clock (HCLK / FCLK) — default in this driver
///   0 = implementation-defined reference clock (AHB/8 on CM4/CM7)
///
/// Template parameter `ClockHz` must equal the clock feeding the SysTick.
/// When CLKSOURCE=1, this is the CPU/AHB clock.
///
/// Typical usage (polling delay, 16 MHz G071):
/// @code
///   #include "hal/systick.hpp"
///   using Tick = alloy::hal::systick::lite::timer<16'000'000u>;
///
///   Tick::delay_ms(500u);   // blocking 500 ms, no ISR needed
/// @endcode
///
/// Interrupt mode (1 ms tick):
/// @code
///   Tick::configure_interrupt(1000u);  // 1 kHz tick (1 ms period)
///   // SysTick_Handler calls Tick::increment();
///   ...
///   Tick::delay_ms(250u);   // waits 250 ticks (250 ms)
///   auto t = Tick::millis();
/// @endcode
#pragma once

#include <cstdint>

namespace alloy::hal::systick::lite {

namespace detail {

inline constexpr std::uintptr_t kBase = 0xE000E010u;

inline constexpr std::uintptr_t kOfsCtrl  = 0x00u;
inline constexpr std::uintptr_t kOfsLoad  = 0x04u;
inline constexpr std::uintptr_t kOfsVal   = 0x08u;
inline constexpr std::uintptr_t kOfsCalib = 0x0Cu;

inline constexpr std::uint32_t kCtrlEnable      = 1u << 0;
inline constexpr std::uint32_t kCtrlTickint      = 1u << 1;
inline constexpr std::uint32_t kCtrlClksource    = 1u << 2;  ///< 1 = processor clock
inline constexpr std::uint32_t kCtrlCountflag    = 1u << 16; ///< Set when counter reaches 0 (rc)

[[nodiscard]] inline auto reg(std::uintptr_t offset) noexcept
    -> volatile std::uint32_t& {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<volatile std::uint32_t*>(kBase + offset);
}

}  // namespace detail

// ============================================================================
// timer<ClockHz>
// ============================================================================

/// Cortex-M SysTick driver parameterised by the clock frequency feeding it.
///
/// `ClockHz` must equal the processor clock (HCLK) when using the default
/// CLKSOURCE=1 setting.
///
/// All static; instantiate with `using Tick = timer<16'000'000u>;`.
template <std::uint32_t ClockHz>
class timer {
   public:
    static constexpr std::uint32_t kClockHz = ClockHz;

    // -----------------------------------------------------------------------
    // Polling mode — no ISR needed
    // -----------------------------------------------------------------------

    /// Blocking delay using COUNTFLAG polling.
    ///
    /// Configures SysTick for `ms` × 1 ms periods.  Polls COUNTFLAG once per
    /// millisecond.  Does NOT enable the SysTick interrupt (TICKINT=0).
    ///
    /// Precision: ± 1 ms (one SysTick period).
    ///
    /// @note Disables and reconfigures SysTick; do not call while SysTick
    ///       interrupt mode is active (it will disrupt the tick counter).
    ///       Use `delay_ms()` (interrupt-mode version) if TICKINT is already on.
    static void delay_ms_poll(std::uint32_t ms) noexcept {
        constexpr std::uint32_t kReload = (ClockHz / 1000u) - 1u;
        static_assert(kReload <= 0x00FF'FFFFu,
                      "SysTick reload must fit in 24 bits; reduce ClockHz or increase tick period.");

        // Disable SysTick, set reload for 1 ms, clear current value.
        detail::reg(detail::kOfsCtrl) = 0u;
        detail::reg(detail::kOfsLoad) = kReload;
        detail::reg(detail::kOfsVal)  = 0u;
        // Enable with processor clock, no interrupt.
        detail::reg(detail::kOfsCtrl) =
            detail::kCtrlEnable | detail::kCtrlClksource;

        for (std::uint32_t i = 0u; i < ms; i = i + 1u) {
            // COUNTFLAG is set (and cleared on read of CTRL) when VAL hits 0.
            while ((detail::reg(detail::kOfsCtrl) & detail::kCtrlCountflag) == 0u) { }
        }

        detail::reg(detail::kOfsCtrl) = 0u;  // stop SysTick
    }

    /// Blocking delay using COUNTFLAG polling — microsecond granularity.
    ///
    /// Loads SysTick once and waits for COUNTFLAG; one iteration = `us` µs.
    /// Minimum granularity: 1 µs (ClockHz must be ≥ 1 MHz).
    ///
    /// @note Same caveat as `delay_ms_poll()` — reconfigures SysTick.
    static void delay_us_poll(std::uint32_t us) noexcept {
        const std::uint32_t reload = (ClockHz / 1'000'000u) * us - 1u;

        detail::reg(detail::kOfsCtrl) = 0u;
        detail::reg(detail::kOfsLoad) = reload & 0x00FF'FFFFu;
        detail::reg(detail::kOfsVal)  = 0u;
        detail::reg(detail::kOfsCtrl) =
            detail::kCtrlEnable | detail::kCtrlClksource;

        while ((detail::reg(detail::kOfsCtrl) & detail::kCtrlCountflag) == 0u) { }

        detail::reg(detail::kOfsCtrl) = 0u;
    }

    // -----------------------------------------------------------------------
    // Interrupt mode — call increment() from SysTick_Handler
    // -----------------------------------------------------------------------

    /// Configure SysTick for interrupt-driven tick at `tick_hz` Hz.
    ///
    /// Enables the SysTick interrupt (TICKINT=1).  The caller must:
    ///   1. Implement `SysTick_Handler` (or the board's equivalent) and call
    ///      `timer<ClockHz>::increment()` from it.
    ///   2. Enable interrupts globally (CPSIE i or `__enable_irq()`).
    ///
    /// Typical: `configure_interrupt(1000u)` for a 1 ms / 1 kHz tick.
    ///
    /// @param tick_hz  Desired tick frequency in Hz (e.g. 1000 = 1 ms period).
    static void configure_interrupt(std::uint32_t tick_hz = 1000u) noexcept {
        const std::uint32_t reload = (ClockHz / tick_hz) - 1u;
        tick_period_us_ = 1'000'000u / tick_hz;

        detail::reg(detail::kOfsCtrl) = 0u;
        detail::reg(detail::kOfsLoad) = reload & 0x00FF'FFFFu;
        detail::reg(detail::kOfsVal)  = 0u;
        detail::reg(detail::kOfsCtrl) =
            detail::kCtrlEnable | detail::kCtrlTickint | detail::kCtrlClksource;
    }

    /// Increment the tick counter.  Call this from `SysTick_Handler`.
    static void increment() noexcept {
        tick_count_ = tick_count_ + 1u;
    }

    /// Current tick count (raw, wraps at 2³²).
    [[nodiscard]] static auto ticks() noexcept -> std::uint32_t {
        return tick_count_;
    }

    /// Milliseconds elapsed since `configure_interrupt()` (wraps at ~49 days
    /// for a 1 kHz tick; use `ticks()` and compute manually for longer spans).
    [[nodiscard]] static auto millis() noexcept -> std::uint32_t {
        return tick_count_ * (tick_period_us_ / 1000u);
    }

    /// Microseconds elapsed since `configure_interrupt()`.
    [[nodiscard]] static auto micros() noexcept -> std::uint64_t {
        return static_cast<std::uint64_t>(tick_count_) *
               static_cast<std::uint64_t>(tick_period_us_);
    }

    /// Blocking delay using the interrupt-mode tick counter.
    ///
    /// Requires that `configure_interrupt()` has been called and ISR is
    /// running.  Waits for `ms` ticks to elapse (one tick = one SysTick period).
    static void delay_ms(std::uint32_t ms) noexcept {
        const std::uint32_t end = tick_count_ + ms;
        while ((end - tick_count_) != 0u) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // Low-level access
    // -----------------------------------------------------------------------

    /// Disable SysTick entirely (clears CTRL).
    static void disable() noexcept {
        detail::reg(detail::kOfsCtrl) = 0u;
    }

    /// Current value of the SysTick counter (24-bit, counts down).
    [[nodiscard]] static auto current_count() noexcept -> std::uint32_t {
        return detail::reg(detail::kOfsVal) & 0x00FF'FFFFu;
    }

   private:
    static inline volatile std::uint32_t tick_count_    = 0u;
    static inline volatile std::uint32_t tick_period_us_ = 1000u;
};

}  // namespace alloy::hal::systick::lite
