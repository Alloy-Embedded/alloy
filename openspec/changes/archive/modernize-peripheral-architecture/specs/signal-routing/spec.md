# Spec: Signal Routing System

## Overview

Implement explicit signal routing between peripherals (GPIO→UART, ADC→DMA, Timer→ADC) with compile-time validation.

## ADDED Requirements

### Requirement: Signal Type Definitions
All peripheral signals SHALL be strongly typed for compile-time checking.

#### Scenario: GPIO Alternate Function Signals
```cpp
// Generated from SVD
struct Usart1TxSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::TX;
    static constexpr AlternateFunction af = AlternateFunction::AF7;
    static constexpr std::array compatible_pins = {PinId::PA9, PinId::PA15, PinId::PB6};
};

// Type-safe connection
auto connection = connect(GpioA9, Usart1TxSignal{});
static_assert(connection.is_valid());
```

**Success Criteria**:
- ✅ All peripheral signals defined as types
- ✅ Compatible pins listed in signal definition
- ✅ Alternate function number included

---

### Requirement: Compile-Time Signal Routing
Signal connections SHALL be validated and configured at compile-time.

#### Scenario: UART Pin Connection
```cpp
// Fluent API with signal routing
Usart1::pin(GpioA9).as_tx()  // Checks: A9 supports USART1::TX?
      .pin(GpioA10).as_rx()  // Checks: A10 supports USART1::RX?
      .initialize();

// Config struct with validation
constexpr UsartPinConfig pins {
    .tx = GpioA9,
    .rx = GpioA10
};
static_assert(pins.validate_signals<Usart1>());
```

**Success Criteria**:
- ✅ Pin compatibility checked at compile-time
- ✅ Alternate function automatically set
- ✅ Clear error if incompatible pin used

---

### Requirement: DMA Signal Routing
DMA channels SHALL be type-safely connected to peripheral data streams.

#### Scenario: UART DMA TX Connection
```cpp
// Type-safe DMA connection
using UartTxDma = DmaConnection<Usart1::TxData, Dma1::Stream7>;
static_assert(UartTxDma::is_compatible());

// Configure
UartTxDma::configure({
    .memory_address = tx_buffer,
    .transfer_size = buffer_size,
    .mode = DmaMode::Circular
});
```

**Success Criteria**:
- ✅ DMA stream compatibility validated
- ✅ Peripheral data address automatically set
- ✅ Conflicts detected (stream already used)

## Dependencies

- Concept layer for type validation
- SVD codegen for signal metadata
- Platform HAL for low-level configuration
