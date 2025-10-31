# Spec Delta: clock-rp2040

**Capability**: `clock-rp2040`
**Status**: NEW

## ADDED Requirements

### Requirement: RP2040 Clock Implementation

The system SHALL provide a complete clock configuration implementation for RP2040 (Cortex-M0+ dual-core, 133MHz).

**Rationale**: Raspberry Pi Pico is extremely popular, unique dual-core Cortex-M0+ design.

#### Scenario: Configure 125MHz system clock from 12MHz crystal
```cpp
// Raspberry Pi Pico: 12MHz external crystal
using namespace alloy::hal::raspberry::rp2040;

SystemClock clock;
ClockConfig config{
    .source = ClockSource::ExternalCrystal,
    .crystal_frequency_hz = 12000000,
    .target_frequency_hz = 125000000
};
auto result = clock.configure(config);
assert(result.is_ok());
assert(clock.get_frequency() == 125000000);
```
- **WHEN** configuring RP2040 with PLL_SYS
- **THEN** both cores SHALL run at 125MHz
- **AND** PLL SHALL generate 125MHz from 12MHz XOSC
- **AND** peripheral clock SHALL be derived correctly

### Requirement: Dual-Core Clock Synchronization

The system SHALL ensure both cores receive synchronized clock signals.

**Rationale**: RP2040 is dual-core; both cores must run at same frequency.

#### Scenario: Both cores at same frequency
```cpp
clock.set_frequency(133000000);
// Core 0 and Core 1 both at 133MHz
assert(clock.get_frequency() == clock.get_core1_frequency());
```
- **WHEN** setting system frequency
- **THEN** both cores SHALL run at same speed
- **AND** no clock domain crossing issues

### Requirement: USB Clock Configuration

The system SHALL configure USB PLL to provide 48MHz for USB peripheral.

**Rationale**: RP2040 has separate PLL for USB, must be 48MHz exactly.

#### Scenario: Enable USB with 48MHz clock
```cpp
auto usb_freq = clock.get_usb_frequency();
assert(usb_freq == 48000000);  // USB requires exactly 48MHz
```
- **WHEN** USB is used
- **THEN** PLL_USB SHALL generate exactly 48MHz
- **AND** independent from system clock changes
