# clock-samd21 Specification

## Purpose
TBD - created by archiving change add-multi-vendor-clock-boards. Update Purpose after archive.
## Requirements
### Requirement: SAMD21 Clock Implementation

The system SHALL provide a complete clock configuration implementation for ATSAMD21 family (Cortex-M0+, 48MHz max).

**Rationale**: ATSAMD21 is used in Arduino Zero/MKR boards, different clock architecture (GCLK system).

#### Scenario: Configure 48MHz system clock from 32kHz crystal
```cpp
// Arduino Zero: 32kHz external crystal + DFLL48M
using namespace alloy::hal::microchip::samd21;

SystemClock clock;
ClockConfig config{
    .source = ClockSource::Dfll48m,  // Digital FLL
    .reference_source = ClockSource::ExternalCrystal,  // 32kHz XOSC32K
    .crystal_frequency_hz = 32768,
    .target_frequency_hz = 48000000
};
auto result = clock.configure(config);
assert(result.is_ok());
assert(clock.get_frequency() == 48000000);
```
- **WHEN** configuring SAMD21 with DFLL48M
- **THEN** system SHALL run at 48MHz from 32kHz reference
- **AND** DFLL SHALL lock to external 32kHz crystal
- **AND** zero wait states for flash @ 48MHz

### Requirement: Generic Clock Generators

The system SHALL support configuring multiple generic clock generators (GCLK).

**Rationale**: SAMD21 has flexible GCLK system where peripherals can use different clock sources.

#### Scenario: Configure GCLK for peripheral
```cpp
// GCLK0 = 48MHz for CPU
// GCLK1 = 8MHz for slow peripherals
clock.configure_generic_clock(0, 48000000);
clock.configure_generic_clock(1, 8000000);
```
- **WHEN** configuring generic clocks
- **THEN** each GCLK SHALL run at specified frequency
- **AND** peripherals can select which GCLK to use

