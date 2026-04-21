## ADDED Requirements

### Requirement: Foundational Peripheral Classes Shall Have Finished Public HAL Paths

The public runtime SHALL expose finished primary APIs for foundational peripheral classes:
UART, SPI, I2C, DMA, timer, PWM, ADC, DAC, RTC, CAN, and watchdog.

#### Scenario: User configures a foundational peripheral
- **WHEN** a user configures one of the foundational peripheral classes on a supported board
- **THEN** they use the same primary public HAL style used by the rest of the runtime
- **AND** they do not need a family-private helper or handwritten board-local glue as the normal path

### Requirement: DMA-Capable Peripheral APIs Shall Expose DMA As A First-Class Option

The public HAL SHALL expose DMA-backed operation as a first-class option for peripheral classes
whose generated semantics support DMA.

#### Scenario: User opens SPI with DMA
- **WHEN** the selected SPI peripheral publishes DMA capability in the runtime contract
- **THEN** the public HAL can enable DMA through its normal configuration path
- **AND** the runtime resolves the DMA route through typed runtime descriptors instead of handwritten tables

### Requirement: Shared Bus Use Shall Be A First-Class Public Story

SPI and I2C public APIs SHALL support shared-bus use through explicit runtime-owned mechanisms.

#### Scenario: Two devices share one SPI controller
- **WHEN** two logical devices share one SPI controller
- **THEN** the public runtime provides an explicit shared-bus path with ownership and configuration rules
- **AND** the user does not need to build ad hoc locking or bus reconfiguration outside the runtime model
