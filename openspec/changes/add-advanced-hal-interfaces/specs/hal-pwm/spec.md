# Spec Delta: hal-pwm

**Capability**: `hal-pwm`
**Status**: NEW

## ADDED Requirements

### Requirement: Duty Cycle Control

The system SHALL provide a way to set PWM duty cycle as a percentage (0-100%) or absolute ticks.

**Rationale**: Core PWM functionality for controlling power/brightness.

#### Scenario: Set LED brightness to 50%
```cpp
auto result = pwm.set_duty_cycle(PwmChannel::Channel1, 50.0f);
assert(result.is_ok());
// LED is now at 50% brightness
```

### Requirement: Frequency Configuration

The system SHALL support configuring PWM frequency.

**Rationale**: Different applications need different frequencies (servo: 50Hz, LED: 1kHz+).

#### Scenario: Configure servo PWM at 50Hz
```cpp
PwmConfig config{
    .frequency_hz = 50,
    .resolution = PwmResolution::Bits16
};
auto result = pwm.configure(config);
assert(result.is_ok());
```

### Requirement: Channel Control

The system SHALL provide start() and stop() operations for individual PWM channels.

**Rationale**: Enable/disable PWM output without reconfiguration.

#### Scenario: Start PWM channel
```cpp
pwm.set_duty_cycle(PwmChannel::Channel1, 75.0f);
pwm.start(PwmChannel::Channel1);
// ... later
pwm.stop(PwmChannel::Channel1);
```

### Requirement: Polarity Control

The system SHALL support inverting PWM polarity.

**Rationale**: Some hardware requires inverted PWM signals.

#### Scenario: Set inverted polarity
```cpp
auto result = pwm.set_polarity(PwmChannel::Channel1, PwmPolarity::Inverted);
assert(result.is_ok());
```

### Requirement: Complementary Outputs

The system SHALL support complementary PWM outputs with dead-time insertion when available in hardware for motor control.

**Rationale**: Prevents shoot-through in H-bridge motor drivers.

#### Scenario: Configure complementary PWM with dead-time
```cpp
PwmComplementaryConfig config{
    .channel = PwmChannel::Channel1,
    .dead_time_ns = 100  // 100ns dead-time
};
auto result = pwm.configure_complementary(config);
assert(result.is_ok());
```

### Requirement: Resolution Options

The system SHALL support multiple PWM resolutions (8-bit, 10-bit, 12-bit, 16-bit).

**Rationale**: Trade-off between resolution and maximum frequency.

#### Scenario: High-frequency PWM with 8-bit resolution
```cpp
PwmConfig config{
    .frequency_hz = 100000,  // 100kHz
    .resolution = PwmResolution::Bits8
};
```

### Requirement: Synchronization

The system SHALL support synchronizing multiple PWM channels when available in hardware.

**Rationale**: Coordinated motor control, LED patterns.

#### Scenario: Synchronize 3 RGB LED channels
```cpp
std::array<PwmChannel, 3> channels = {
    PwmChannel::Channel1,  // Red
    PwmChannel::Channel2,  // Green
    PwmChannel::Channel3   // Blue
};
pwm.synchronize(channels);
pwm.start_synchronized();
```

## Non-Functional Requirements

### Perf-PWM-001: Update Latency
Duty cycle changes SHOULD take effect within one PWM period.

### Port-PWM-001: Platform Independence
The PWM interface MUST work across all supported architectures.
