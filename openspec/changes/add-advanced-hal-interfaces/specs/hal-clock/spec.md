# Spec Delta: hal-clock

**Capability**: `hal-clock`
**Status**: NEW

## ADDED Requirements

### Requirement: System Clock Configuration

The system SHALL provide a way to configure the main system clock source and frequency.

**Rationale**: MCU speed affects all peripheral timing and power consumption.

#### Scenario: Configure system clock to 72MHz using external crystal
```cpp
ClockConfig config{
    .source = ClockSource::ExternalCrystal,
    .crystal_frequency_hz = 8000000,  // 8MHz crystal
    .pll_multiplier = 9,               // 8MHz * 9 = 72MHz
    .ahb_divider = 1,
    .apb1_divider = 2,
    .apb2_divider = 1
};
auto result = system_clock.configure(config);
assert(result.is_ok());
assert(system_clock.get_frequency() == 72000000);
```

### Requirement: Clock Source Selection

The system SHALL support selecting between internal RC oscillator and external crystal.

**Rationale**: Different accuracy/power requirements.

#### Scenario: Use internal oscillator for low-power mode
```cpp
ClockConfig config{
    .source = ClockSource::InternalRC,
    .target_frequency_hz = 16000000  // 16MHz internal
};
auto result = system_clock.configure(config);
```

### Requirement: PLL Configuration

The system SHALL support PLL configuration for frequencies not achievable with direct oscillator.

**Rationale**: Most MCUs need PLL to reach maximum speed.

#### Scenario: Configure PLL for maximum speed
```cpp
PllConfig pll{
    .input_source = ClockSource::ExternalCrystal,
    .input_frequency_hz = 8000000,
    .multiplier = 9,
    .divider = 1
};
auto result = system_clock.configure_pll(pll);
assert(result.is_ok());
```

### Requirement: Frequency Query

The system SHALL provide functions to query current clock frequencies (system, AHB, APB1, APB2).

**Rationale**: Peripherals need to know their clock speed for timing calculations.

#### Scenario: Calculate UART baud rate divisor
```cpp
uint32_t apb1_freq = system_clock.get_apb1_frequency();
uint32_t baud_divisor = apb1_freq / (16 * 115200);
```

### Requirement: Peripheral Clock Enable

The system SHALL provide functions to enable/disable peripheral clocks.

**Rationale**: Power saving by disabling unused peripherals.

#### Scenario: Enable UART2 clock
```cpp
auto result = system_clock.enable_peripheral(Peripheral::Uart2);
assert(result.is_ok());
// ... use UART2 ...
system_clock.disable_peripheral(Peripheral::Uart2);
```

### Requirement: Flash Latency Adjustment

The system SHALL automatically adjust flash wait states when changing system frequency.

**Rationale**: Prevents crashes from insufficient flash latency at high speeds.

#### Scenario: Safe transition to high speed
```cpp
// Flash latency automatically adjusted
auto result = system_clock.set_frequency(72000000);
assert(result.is_ok());
// Flash latency is now 2 wait states (required for 72MHz)
```

### Requirement: Clock Switching Safety

The system SHALL ensure clock source is stable before switching.

**Rationale**: Switching to unstable clock causes system crash.

#### Scenario: Wait for PLL lock before switching
```cpp
auto result = system_clock.configure(config);
// Implementation waits for PLL lock flag before switching
assert(result.is_ok());
```

### Requirement: Peripheral Frequency Query

The system SHALL provide functions to query specific peripheral clock frequencies.

**Rationale**: Some peripherals need to know their exact clock for timing.

#### Scenario: Configure timer based on peripheral clock
```cpp
uint32_t timer_clock = system_clock.get_peripheral_frequency(Peripheral::Timer2);
uint32_t prescaler = timer_clock / 1000000;  // 1MHz timer base
```

### Requirement: Clock Configuration Validation

The system SHALL validate clock configurations and return errors for invalid settings.

**Rationale**: Prevents configuring impossible frequencies or violating MCU limits.

#### Scenario: Reject frequency above MCU maximum
```cpp
ClockConfig config{
    .target_frequency_hz = 200000000  // Above STM32F1 max (72MHz)
};
auto result = system_clock.configure(config);
assert(result.is_error());
assert(result.error() == ErrorCode::ClockInvalidFrequency);
```

### Requirement: Runtime Frequency Changes

The system SHALL support changing system frequency at runtime for dynamic power management.

**Rationale**: Scale frequency based on workload to save power.

#### Scenario: Reduce frequency during idle
```cpp
// High performance mode
system_clock.set_frequency(72000000);
// ... do work ...

// Low power mode
system_clock.set_frequency(8000000);
// ... idle ...
```

## Non-Functional Requirements

### Safe-CLOCK-001: Atomic Configuration
Clock configuration changes MUST be atomic - either fully applied or fully rolled back on error.

### Safe-CLOCK-002: No Intermediate Invalid States
The system MUST NOT enter invalid clock states during configuration transitions.

### Port-CLOCK-001: Vendor Abstraction
The clock interface SHOULD hide vendor-specific PLL/divider details behind a simple frequency-based API.

### Perf-CLOCK-001: Configuration Time
Clock configuration SHOULD complete in less than 10ms (PLL lock time).
