## MODIFIED Requirements

### Requirement: The selected device boundary consumes only the runtime contract

`alloy` SHALL consume the published device contract through `generated/runtime/**` as its only
supported generated C++ boundary.

#### Scenario: Selected config includes only runtime contract headers
- **WHEN** a board selects a published device from `alloy-devices`
- **THEN** the generated selected config includes the typed runtime contract headers
- **AND** it does not include legacy generated descriptor headers from `generated/devices/<device>/`
  for runtime behavior

#### Scenario: Runtime path does not depend on legacy generated tables
- **WHEN** GPIO/UART/SPI/I2C/DMA/timer/PWM/ADC/DAC/startup/system clock are compiled
- **THEN** they consume the typed runtime contract
- **AND** they do not require table-oriented generated C++ reflection headers
