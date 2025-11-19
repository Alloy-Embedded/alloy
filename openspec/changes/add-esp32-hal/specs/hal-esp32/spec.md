## ADDED Requirements

### Requirement: ESP32 GPIO Implementation

The system SHALL provide GPIO implementation for ESP32 using GPIO matrix.

#### Scenario: GPIO matrix configuration
- **WHEN** configuring GPIO on ESP32
- **THEN** it SHALL use GPIO matrix registers
- **AND** it SHALL handle GPIO 0-33 (general purpose)
- **AND** it SHALL handle GPIO 34-39 (input only)

#### Scenario: GPIO operations
- **WHEN** calling set_high()
- **THEN** it SHALL use GPIO_OUT_W1TS_REG (write-1-to-set)
- **WHEN** calling set_low()
- **THEN** it SHALL use GPIO_OUT_W1TC_REG (write-1-to-clear)
- **WHEN** calling read()
- **THEN** it SHALL use GPIO_IN_REG

#### Scenario: GPIO pull resistors
- **WHEN** configuring InputPullUp
- **THEN** it SHALL set pull-up resistor via PIN_PULLUP_EN
- **WHEN** configuring InputPullDown
- **THEN** it SHALL set pull-down resistor via PIN_PULLDOWN_EN

### Requirement: ESP32 UART Implementation

The system SHALL provide UART implementation for ESP32.

#### Scenario: UART initialization
- **WHEN** initializing UART
- **THEN** it SHALL configure UART clock divider for baud rate
- **AND** it SHALL configure UART FIFO
- **AND** it SHALL enable UART transmitter and receiver

#### Scenario: UART FIFO operations
- **WHEN** calling write_byte()
- **THEN** it SHALL check TX FIFO full status
- **AND** it SHALL write to UART_FIFO_REG
- **WHEN** calling read_byte()
- **THEN** it SHALL check RX FIFO count
- **AND** it SHALL read from UART_FIFO_REG

### Requirement: ESP32 Board Definition

The system SHALL provide board definition for ESP32-DevKitC.

#### Scenario: Board configuration
- **WHEN** setting ALLOY_BOARD="esp32_devkit"
- **THEN** ALLOY_MCU SHALL be "ESP32"
- **AND** clock frequency SHALL be up to 240MHz
- **AND** LED_PIN SHALL be GPIO2
- **AND** Flash size SHALL be configurable (4MB typical)
- **AND** RAM size SHALL reflect ESP32 SRAM (520KB)

### Requirement: Runtime Environment Configuration

The system SHALL support both FreeRTOS and bare-metal modes for ESP32.

#### Scenario: FreeRTOS mode enabled
- **WHEN** ALLOY_USE_FREERTOS is set to ON
- **THEN** main() SHALL be wrapped in a FreeRTOS task
- **AND** FreeRTOS scheduler SHALL be started before main() executes
- **AND** FreeRTOS heap SHALL be configured for ESP32

#### Scenario: Bare-metal mode attempted
- **WHEN** ALLOY_USE_FREERTOS is set to OFF
- **THEN** CMake configuration SHALL warn that bare-metal may be limited on ESP32
- **AND** main() SHALL run directly if feasible
- **AND** WiFi/Bluetooth features SHALL be disabled in bare-metal mode
