# Spec Delta: clock-esp32

**Capability**: `clock-esp32`
**Status**: NEW

## ADDED Requirements

### Requirement: ESP32 Clock Implementation

The system SHALL provide a complete clock configuration implementation for ESP32 (Xtensa LX6 dual-core, 240MHz max).

**Rationale**: ESP32 is extremely popular for IoT applications with WiFi/Bluetooth, different architecture (Xtensa vs ARM).

#### Scenario: Configure 160MHz CPU clock from 40MHz XTAL
```cpp
// ESP32 DevKit: 40MHz external crystal
using namespace alloy::hal::espressif::esp32;

SystemClock clock;
ClockConfig config{
    .source = ClockSource::ExternalCrystal,
    .crystal_frequency_hz = 40000000,  // 40MHz XTAL
    .target_frequency_hz = 160000000   // 160MHz CPU
};
auto result = clock.configure(config);
assert(result.is_ok());
assert(clock.get_frequency() == 160000000);
assert(clock.get_apb_frequency() == 80000000);  // APB = 80MHz
```
- **WHEN** configuring ESP32 clock with external crystal
- **THEN** CPU SHALL run at 160MHz
- **AND** APB frequency SHALL be 80MHz
- **AND** both cores SHALL run at same frequency

### Requirement: Dynamic Frequency Scaling

The system SHALL support runtime frequency changes for power management.

**Rationale**: ESP32 applications often need to scale frequency based on workload (WiFi active vs sleep).

#### Scenario: Switch between 240MHz and 80MHz
```cpp
// High performance mode
clock.set_frequency(240000000);  // WiFi active
// ... do work ...

// Power saving mode
clock.set_frequency(80000000);   // WiFi idle
```
- **WHEN** changing frequency at runtime
- **THEN** transition SHALL be safe without glitches
- **AND** peripheral clocks SHALL adjust automatically

### Requirement: RTC Clock Support

The system SHALL provide access to RTC 8MHz clock for low-power modes.

**Rationale**: ESP32 has separate RTC domain for ultra-low-power operation.

#### Scenario: Configure RTC clock
```cpp
auto rtc_freq = clock.get_rtc_frequency();
assert(rtc_freq == 8000000);  // RTC = 8MHz
```
- **WHEN** querying RTC clock
- **THEN** frequency SHALL be 8MHz
- **AND** RTC SHALL remain active in deep sleep
