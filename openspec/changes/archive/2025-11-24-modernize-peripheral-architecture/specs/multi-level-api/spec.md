# Spec: Multi-Level API Design

## Overview

Provide three API levels for different user expertise: Simple (beginners), Fluent (common), Expert (advanced).

## ADDED Requirements

### Requirement: Level 1 - Simple API
One-liner setup SHALL be provided for common configurations.

#### Scenario: Quick UART Setup
```cpp
// Single function call with minimal parameters
Usart1::quick_setup(GpioA9, GpioA10, 115200);

// Internally: validates pins, sets AF, configures UART
// Error: "Pin PA5 cannot be used for USART1 TX"
```

**Success Criteria**:
- ✅ One function call for basic setup
- ✅ Sensible defaults for all optional parameters
- ✅ Clear errors if pins incompatible

---

### Requirement: Level 2 - Fluent API
Builder pattern SHALL be provided for readable, maintainable code.

#### Scenario: UART with Custom Configuration
```cpp
Usart1::pin(GpioA9).as_tx()
      .pin(GpioA10).as_rx().with_pull(PullUp)
      .baudrate(115200)
      .parity(Parity::Even)
      .stop_bits(StopBits::Two)
      .enable_dma_tx<Dma1::Stream7>()
      .initialize();
```

**Success Criteria**:
- ✅ Method chaining for configuration
- ✅ Self-documenting (no need to look up parameter order)
- ✅ Validates each step

---

### Requirement: Level 3 - Expert API
Full control SHALL be provided via config structs with compile-time validation.

#### Scenario: Advanced UART Configuration
```cpp
constexpr UsartConfig cfg {
    .tx = GpioA9.with_af(AlternateFunction::AF7),
    .rx = GpioA10.with_pull(PullUp).with_af(AlternateFunction::AF7),
    .baudrate = 115200,
    .word_length = 8,
    .parity = Parity::Even,
    .stop_bits = StopBits::Two,
    .flow_control = FlowControl::RtsCts,
    .dma_tx = Dma1::Stream7,
    .dma_rx = Dma1::Stream5
};

static_assert(cfg.is_valid(), cfg.error_message());
Usart1::configure(cfg);
```

**Success Criteria**:
- ✅ All parameters explicit
- ✅ Compile-time validation with custom errors
- ✅ Zero runtime overhead

## Dependencies

- Concept layer for validation
- Signal routing for pin/DMA connections
