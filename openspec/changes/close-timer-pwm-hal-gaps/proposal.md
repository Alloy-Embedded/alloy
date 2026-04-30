# Proposal: Close Timer/PWM HAL Gaps

## Status
`open` — advanced timer features (dead-time, break input, carrier modulation,
encoder) absent from IR and HAL.

## Problem

The Timer/PWM HAL covers basic compare-match output and input capture, but
advanced timer features required by motor control, power conversion, and
communication protocols are absent:

| Feature                       | Status  | Blocker                            |
|-------------------------------|---------|------------------------------------|
| Dead-time insertion (DTG)     | absent  | BDTR register not in IR            |
| Break input (BKIN)            | absent  | BDTR BKE/BKP fields not in IR      |
| Fault protection (MOE)        | absent  | BDTR MOE field not in IR           |
| Complementary outputs (OCxN)  | absent  | CCER CCxNE fields not in IR        |
| Carrier modulation (OCxM=111) | absent  | TIM_CCMR OCxM=0b111 not mapped     |
| Encoder mode                  | absent  | SMCR SMS field not in IR           |
| Center-aligned PWM (CMS)      | absent  | CR1 CMS field not in IR            |
| One-pulse mode                | absent  | CR1 OPM field not in IR            |
| External clock source         | absent  | SMCR ECE field not in IR           |
| DMA burst transfer            | absent  | DCR/DMAR registers not in IR       |

Without dead-time and break input, alloy cannot be used for 3-phase motor
inverters or switch-mode power supplies — high-value embedded applications.

## Proposed Solution

### IR additions

Fields to add for STM32 advanced timers (TIM1, TIM8):

```cpp
// BDTR register fields:
kDtgField         // [7:0]  dead-time generator
kBkeField         // [12]   break enable
kBkpField         // [13]   break polarity
kAoeField         // [14]   automatic output enable
kMoeField         // [15]   main output enable

// CCER register fields (complementary outputs):
kCc1neField       // [2]    CC1N output enable
kCc2neField       // [6]    CC2N output enable
kCc3neField       // [10]   CC3N output enable
kCc1npField       // [3]    CC1N polarity

// SMCR register fields (encoder / external clock):
kSmsField         // [2:0]  slave mode selection (encoder modes: 001, 010, 011)
kEceField         // [14]   external clock enable

// CR1 register fields:
kCmsField         // [6:5]  center-aligned mode
kOpmField         // [3]    one-pulse mode

// DCR/DMAR (DMA burst):
kDbaField         // DCR [4:0]  DMA base address
kDblField         // DCR [12:8] DMA burst length
kDmarReg          // DMAR register
```

### Dead-time API

```cpp
// src/hal/timer/timer_handle.hpp (extensions)

struct DeadTimeConfig {
    uint8_t  dtg;           // raw DTG[7:0] field value (vendor-specific encoding)
    bool     complementary_outputs;  // enable OCxN channels
};

auto configure_dead_time(const DeadTimeConfig& cfg)
    -> core::Result<void, core::ErrorCode>
{
    if constexpr (!semantic_traits::kDtgField.valid ||
                  !semantic_traits::kMoeField.valid)
        return core::Err(core::ErrorCode::NotSupported);
    // write BDTR DTG + enable MOE + configure CCxNE bits
}

/// Helper: convert nanoseconds to DTG register value for the current timer clock.
static constexpr auto ns_to_dtg(uint32_t ns, uint32_t timer_clk_hz) -> uint8_t;
```

### Break input API

```cpp
struct BreakConfig {
    bool    enabled;
    bool    active_high;    // false = break on low
    bool    auto_rearm;     // AOE: re-enable outputs after break clears
};

auto configure_break(const BreakConfig& cfg)
    -> core::Result<void, core::ErrorCode>;

/// Force-enable main output (after break). MOE must be set before PWM restarts.
auto enable_main_output() -> core::Result<void, core::ErrorCode>;
```

### Encoder mode

```cpp
enum class EncoderMode : uint8_t {
    ti1_only   = 0b001,  // count on TI1 edges
    ti2_only   = 0b010,  // count on TI2 edges
    both       = 0b011,  // count on both edges (4× resolution)
};

auto configure_encoder(EncoderMode mode)
    -> core::Result<void, core::ErrorCode>;

auto read_encoder_count() -> core::Result<uint32_t, core::ErrorCode>;
```

### Center-aligned PWM

```cpp
enum class CenterAlignedMode : uint8_t {
    edge    = 0b00,
    center1 = 0b01,   // interrupt on down-count
    center2 = 0b10,   // interrupt on up-count
    center3 = 0b11,   // interrupt on both
};

auto set_center_aligned(CenterAlignedMode mode)
    -> core::Result<void, core::ErrorCode>;
```

### One-pulse mode

```cpp
auto enable_one_pulse_mode() -> core::Result<void, core::ErrorCode>;
```

## Impact

- Enables 3-phase motor inverter firmware (BLDC, PMSM) using alloy.
- Enables SMPS (buck/boost) with complementary outputs and dead-time.
- Enables quadrature encoder reading for position feedback.
- Enables carrier modulation for IR remote control transmitters.
