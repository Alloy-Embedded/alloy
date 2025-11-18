## ADDED Requirements

### Requirement: STM32F1 GPIO Implementation

The system SHALL provide GPIO implementation for STM32F1 family using CRL/CRH registers.

#### Scenario: GPIO configuration uses CRL/CRH
- **WHEN** configuring a GPIO pin on STM32F1
- **THEN** it SHALL use GPIOx->CRL for pins 0-7
- **AND** it SHALL use GPIOx->CRH for pins 8-15
- **AND** it SHALL NOT use MODER register (F4 style)

#### Scenario: GPIO port mapping
- **WHEN** creating GpioPin<PC13>
- **THEN** it SHALL map to GPIOC port, bit 13
- **AND** it SHALL enable GPIOC clock in RCC->APB2ENR

#### Scenario: GPIO operations
- **WHEN** calling set_high()
- **THEN** it SHALL use BSRR register (bit set)
- **WHEN** calling set_low()
- **THEN** it SHALL use BRR register (bit reset) or BSRR high bits

### Requirement: STM32F1 UART Implementation

The system SHALL provide UART implementation for STM32F1 using USART peripherals.

#### Scenario: USART initialization
- **WHEN** initializing USART1
- **THEN** it SHALL enable USART1 clock in RCC->APB2ENR
- **AND** it SHALL configure baud rate using BRR register
- **AND** it SHALL calculate BRR value based on APB2 clock frequency

#### Scenario: UART operations
- **WHEN** calling write_byte()
- **THEN** it SHALL wait for TXE flag
- **AND** it SHALL write to USART->DR register
- **WHEN** calling read_byte()
- **THEN** it SHALL check RXNE flag
- **AND** it SHALL read from USART->DR register

### Requirement: BluePill Board Definition

The system SHALL provide board definition for BluePill (STM32F103C8T6).

#### Scenario: Board configuration
- **WHEN** setting ALLOY_BOARD="bluepill"
- **THEN** ALLOY_MCU SHALL be "STM32F103C8T6"
- **AND** clock frequency SHALL be configurable up to 72MHz
- **AND** LED_PIN SHALL be PC13 (onboard LED)
- **AND** Flash size SHALL be 64KB (or 128KB for CB variant)
- **AND** RAM size SHALL be 20KB
