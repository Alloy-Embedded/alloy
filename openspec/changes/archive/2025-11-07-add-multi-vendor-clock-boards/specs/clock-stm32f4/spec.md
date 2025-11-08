# Spec Delta: clock-stm32f4

**Capability**: `clock-stm32f4`
**Status**: NEW

## ADDED Requirements

### Requirement: STM32F4 Clock Implementation

The system SHALL provide a complete clock configuration implementation for STM32F4 family (Cortex-M4F, 168MHz max).

**Rationale**: STM32F407 is high-performance MCU with FPU, used in demanding applications.

#### Scenario: Configure 168MHz system clock from 8MHz HSE
```cpp
// STM32F4 Discovery: 8MHz external crystal
using namespace alloy::hal::st::stm32f4;

SystemClock clock;
PllConfig pll{
    .input_source = ClockSource::ExternalCrystal,
    .input_frequency_hz = 8000000,
    .multiplier = 336,  // VCO = 8MHz * 336 = 2688MHz (invalid for direct use)
    .divider = 2,       // VCO / 2 = 1344MHz
    .p_divider = 2,     // System = 1344MHz / 2 = 672MHz... wait, that's wrong
    // Correct: VCO = 8MHz / M * N, then VCO / P = system
    // M = 8, N = 336, P = 2: VCO = 8MHz/8*336 = 336MHz, sys = 336/2 = 168MHz
};

ClockConfig config{
    .source = ClockSource::Pll,
    .crystal_frequency_hz = 8000000,
    .pll_multiplier = 336,
    .pll_divider = 8,    // M divider
    .ahb_divider = 1,    // AHB = 168MHz
    .apb1_divider = 4,   // APB1 = 42MHz (max 42MHz)
    .apb2_divider = 2    // APB2 = 84MHz (max 84MHz)
};
auto result = clock.configure(config);
assert(result.is_ok());
assert(clock.get_frequency() == 168000000);
```
- **WHEN** configuring STM32F4 clock with complex PLL
- **THEN** system SHALL run at 168MHz
- **AND** flash latency SHALL be 5 wait states @ 168MHz
- **AND** bus frequencies SHALL respect maximum limits

### Requirement: Over-Clocking Support

The system SHALL allow configuring frequencies above rated specifications with validation warnings.

**Rationale**: Some users over-clock STM32F4 to 180-200MHz for extra performance.

#### Scenario: Configure 180MHz (over-clocking)
```cpp
ClockConfig config{
    .target_frequency_hz = 180000000  // Above 168MHz spec
};
auto result = clock.configure(config);
// Should succeed but log warning
```
- **WHEN** requesting frequency above specification
- **THEN** configuration SHALL succeed with warning
- **AND** user assumes responsibility for stability
