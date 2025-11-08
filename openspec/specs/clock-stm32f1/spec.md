# clock-stm32f1 Specification

## Purpose
TBD - created by archiving change add-multi-vendor-clock-boards. Update Purpose after archive.
## Requirements
### Requirement: STM32F1 Clock Implementation

The system SHALL provide a complete clock configuration implementation for STM32F1 family (Cortex-M3, 72MHz max).

**Rationale**: STM32F103 is one of the most popular ARM Cortex-M3 MCUs, widely used in hobbyist and commercial projects.

#### Scenario: Configure 72MHz system clock from 8MHz HSE
```cpp
// Blue Pill board: 8MHz external crystal
using namespace alloy::hal::st::stm32f1;

SystemClock clock;
ClockConfig config{
    .source = ClockSource::ExternalCrystal,
    .crystal_frequency_hz = 8000000,  // 8MHz HSE
    .pll_multiplier = 9,               // 8MHz * 9 = 72MHz
    .ahb_divider = 1,                  // AHB = 72MHz
    .apb1_divider = 2,                 // APB1 = 36MHz (max 36MHz)
    .apb2_divider = 1                  // APB2 = 72MHz
};
auto result = clock.configure(config);
assert(result.is_ok());
assert(clock.get_frequency() == 72000000);
```
- **WHEN** configuring STM32F1 clock with HSE and PLL
- **THEN** system SHALL run at 72MHz
- **AND** flash latency SHALL be automatically set to 2 wait states
- **AND** bus frequencies SHALL respect maximum limits (APB1 ≤ 36MHz)

### Requirement: HSI Internal Oscillator Support

The system SHALL support HSI (8MHz internal RC oscillator) as clock source.

**Rationale**: Allows operation without external crystal, useful for prototyping and cost-sensitive designs.

#### Scenario: Use HSI for low-speed operation
```cpp
ClockConfig config{
    .source = ClockSource::InternalRC,
    .pll_multiplier = 0  // No PLL, run directly from HSI
};
auto result = clock.configure(config);
assert(result.is_ok());
assert(clock.get_frequency() == 8000000);  // HSI is 8MHz
```
- **WHEN** configuring with HSI and no PLL
- **THEN** system SHALL run at 8MHz
- **AND** no external components required

### Requirement: Peripheral Clock Control

The system SHALL provide functions to enable/disable peripheral clocks.

**Rationale**: Power management - disable unused peripherals.

#### Scenario: Enable GPIO and USART clocks
```cpp
clock.enable_peripheral(Peripheral::GpioC);  // For LED on PC13
clock.enable_peripheral(Peripheral::Uart1);  // For serial communication
```
- **WHEN** enabling peripheral clock
- **THEN** peripheral SHALL be powered and ready to use
- **AND** minimal power consumption when disabled

