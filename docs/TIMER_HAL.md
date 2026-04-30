# Timer / PWM HAL

Covers `alloy::hal::timer::handle<P>` — advanced features added by `close-timer-pwm-hal-gaps`.

## Basic usage (existing)

```cpp
#include "hal/timer.hpp"

auto tim = alloy::hal::timer::open<PeripheralId::TIM1>(
    alloy::hal::timer::Config{
        .period            = 999u,          // ARR = 999 → period = (PSC+1)*(ARR+1)/clk
        .peripheral_clock_hz = 64'000'000u,
        .apply_period      = true,
        .start_immediately = false,
    });

tim.configure();                            // enable clock, apply period
tim.set_frequency(20'000u);                 // auto-compute PSC+ARR, ±2% tolerance
tim.start();                                // CEN = 1
```

## Dead-time insertion

Dead-time is required for complementary half-bridge outputs (BLDC, buck/boost) to prevent
shoot-through. It is configured via **BDTR.DTG** on STM32 advanced timers (TIM1, TIM8,
TIM16, TIM17).

### ns_to_dtg — compile-time helper

```cpp
// Convert nanoseconds to DTG[7:0] encoding for a given timer clock.
// Implements all four STM32 ranges:
//   0b0xxxxxxx  — DT = tDTS * DTG[6:0]           (0 … 127 ticks)
//   0b10xxxxxx  — DT = tDTS * (64+n) * 2          (128 … 254 ticks)
//   0b110xxxxx  — DT = tDTS * (32+n) * 8          (256 … 504 ticks)
//   0b111xxxxx  — DT = tDTS * (32+n) * 16         (512 … 1008 ticks)
// Returns 0xFF if ns exceeds maximum encodable dead-time.

constexpr auto dtg = alloy::hal::timer::handle<PeripheralId::TIM1>
                         ::ns_to_dtg(500u, 64'000'000u);
static_assert(dtg != 0xFFu, "dead-time too large");
```

### configure_dead_time

```cpp
const auto r = tim.configure_dead_time(alloy::hal::timer::DeadTimeConfig{
    .dtg             = dtg,    // raw DTG encoding from ns_to_dtg()
    .enable_moe      = true,   // assert BDTR.MOE after configuring DTG
    .auto_output_enable = false,  // AOE: auto re-enable after break (usually false)
});
```

`configure_dead_time` returns `core::Err(NotSupported)` when `kDtgField` or `kMoeField`
are not present — i.e., on general-purpose timers that lack BDTR.

### enable_main_output / disable_main_output

```cpp
tim.enable_main_output();   // BDTR.MOE = 1 — required before PWM appears on pins
tim.disable_main_output();  // BDTR.MOE = 0 — outputs go to idle state
```

## Break input

The break input (BKIN pin or comparator output) forces outputs to their idle state when
asserted. On STM32, this clears **BDTR.MOE**.

```cpp
// High-level configuration
const auto r = tim.configure_break(alloy::hal::timer::BreakConfig{
    .enabled     = true,   // BDTR.BKE = 1
    .active_high = true,   // BDTR.BKP: false = active-low (default for BKIN)
    .auto_rearm  = false,  // BDTR.AOE: true = auto re-enable MOE after break clears
});

// Fine-grained control
tim.enable_break_input(true);
tim.set_break_polarity(alloy::hal::timer::Polarity::Active);  // active-high

// Runtime state
if (tim.break_active()) {
    // BDTR.BIF is set — break event occurred
    // After fault is cleared:
    tim.clear_break_flag();
    tim.enable_main_output();  // re-enable outputs
}
```

## Complementary outputs

Complementary outputs (OCxN channels) must be enabled per-channel **after** dead-time
is configured.

```cpp
// Enable CH1 + CH1N, CH2 + CH2N, CH3 + CH3N
for (std::uint8_t ch = 0u; ch < 3u; ++ch) {
    tim.enable_channel_output(ch, true);
    tim.enable_complementary_output(ch, true);
}

// Set complementary polarity independently
tim.set_complementary_polarity(0u, alloy::hal::timer::Polarity::Active);   // CC1NP=0
tim.set_complementary_polarity(0u, alloy::hal::timer::Polarity::Inverted); // CC1NP=1
```

`set_complementary_polarity` returns `NotSupported` when `kComplementaryOutputPolarityField`
is absent (i.e., CH4 which has no complementary output, or general-purpose timers).

## Encoder mode

```cpp
tim.set_encoder_mode(alloy::hal::timer::EncoderMode::BothChannels); // SMCR.SMS = 0b011
tim.start();

// Read position
const auto count = tim.get_count();  // CNT register
```

## Center-aligned PWM

```cpp
tim.set_center_aligned(alloy::hal::timer::CenterAligned::Mode3);  // CR1.CMS = 0b11
```

Center-aligned mode doubles effective switching frequency at the same ARR.

## One-pulse mode

```cpp
tim.set_one_pulse(true);   // CR1.OPM = 1 — timer stops after one period
tim.start();
```

## 3-phase BLDC example (nucleo_g071rb)

```cpp
#include "hal/timer.hpp"

namespace {
constexpr std::uint32_t kClkHz   = 64'000'000u;
constexpr std::uint32_t kPwmHz   = 20'000u;
constexpr std::uint32_t kDeadNs  = 500u;   // 500 ns dead-time

constexpr auto kDtg = alloy::hal::timer::handle<PeripheralId::TIM1>
                          ::ns_to_dtg(kDeadNs, kClkHz);
static_assert(kDtg != 0xFFu, "dead-time exceeds maximum");
}  // namespace

void init_bldc_tim1() {
    auto tim = alloy::hal::timer::open<PeripheralId::TIM1>(
        alloy::hal::timer::Config{.peripheral_clock_hz = kClkHz});

    tim.configure();
    tim.set_frequency(kPwmHz);
    tim.set_auto_reload_preload(true);
    tim.set_period_preload(true);

    // PWM mode 1 on all three channels
    for (std::uint8_t ch = 0u; ch < 3u; ++ch) {
        tim.set_channel_mode(ch, alloy::hal::timer::CaptureCompareMode::Pwm1);
        tim.set_compare_value(ch, 0u);
        tim.enable_channel_output(ch, true);
        tim.enable_complementary_output(ch, true);
    }

    // Dead-time + break input
    tim.configure_dead_time({.dtg = kDtg, .enable_moe = false});
    tim.configure_break({.enabled = true, .active_high = false});  // BKIN active-low

    // Enable outputs — PWM appears on pins
    tim.enable_main_output();
    tim.start();
}
```

## Vendor extension points

`kDtgField`, `kBkeField`, `kBkpField`, `kMoeField`, `kAoeField`, `kBreakFlagField`
are published via the **alloy-devices** bootstrap patches (`stm32g0-timer.yaml`,
`stm32f4-timer.yaml`). To add support for a new advanced timer:

1. Add `add_semantic` ops for the peripheral in the target's timer patch file.
2. Run the codegen regen pipeline to produce the updated `driver_semantics/timer.hpp`.
3. The HAL methods automatically become functional — no HAL source changes required.

See `docs/PORTING_NEW_PLATFORM.md#timer--advanced-features` for the full field list.
