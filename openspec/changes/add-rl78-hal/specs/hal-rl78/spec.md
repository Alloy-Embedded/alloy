## ADDED Requirements

### Requirement: RL78 Toolchain Support

The system SHALL support Renesas RL78 toolchain (GNURL78 or CC-RL) for building RL78 targets.

#### Scenario: RL78 toolchain configured
- **WHEN** setting ALLOY_BOARD to "cf_rl78"
- **THEN** CMake SHALL use `cmake/toolchains/rl78-gcc.cmake`
- **AND** it SHALL set appropriate compiler flags for RL78 architecture

#### Scenario: RL78 linker script
- **WHEN** building for RL78
- **THEN** linker script SHALL define Flash and RAM regions for RL78
- **AND** it SHALL place interrupt vectors at correct address (0x0000)

### Requirement: RL78 GPIO Implementation

The system SHALL provide GPIO implementation for Renesas RL78 using port-based control.

#### Scenario: Port-based GPIO mapping
- **WHEN** creating GpioPin<10>
- **THEN** it SHALL map to Port 1, bit 2 (or appropriate mapping)
- **AND** it SHALL use port registers (P, PM, PU) for control

#### Scenario: GPIO operations work
- **WHEN** calling set_high() on RL78 GPIO
- **THEN** it SHALL set the appropriate bit in port register
- **AND** it SHALL satisfy GpioPin concept

#### Scenario: Pull-up configuration
- **WHEN** configuring GPIO as InputPullUp
- **THEN** it SHALL set PU (pull-up) register bit
- **AND** it SHALL set PM (port mode) register for input

### Requirement: RL78 UART Implementation

The system SHALL provide UART implementation for Renesas RL78 using SAU (Serial Array Unit).

#### Scenario: SAU initialization
- **WHEN** configuring UART
- **THEN** it SHALL initialize SAU0 or SAU1
- **AND** it SHALL set baud rate using BRG register
- **AND** it SHALL configure data format (8N1 default)

#### Scenario: UART read/write
- **WHEN** calling write_byte()
- **THEN** it SHALL use SAU transmit register
- **WHEN** calling read_byte()
- **THEN** it SHALL use SAU receive register
- **AND** it SHALL check UART errors (framing, parity, overrun)

#### Scenario: UART concept compliance
- **WHEN** static_assert(UartDevice<rl78::Uart>) is evaluated
- **THEN** it SHALL compile without errors

### Requirement: CF_RL78 Board Definition

The system SHALL provide board definition for CF_RL78 (Renesas starter kit).

#### Scenario: Board configuration
- **WHEN** setting ALLOY_BOARD="cf_rl78"
- **THEN** CMake SHALL load `cmake/boards/cf_rl78.cmake`
- **AND** it SHALL set ALLOY_MCU to appropriate RL78 variant
- **AND** it SHALL define LED_PIN and UART pins

#### Scenario: Memory configuration
- **WHEN** building for CF_RL78
- **THEN** Flash size SHALL match the specific RL78 variant (e.g., 64KB)
- **AND** RAM size SHALL be correctly configured (e.g., 4KB)
