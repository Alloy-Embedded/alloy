# Spec: Concept Layer for Peripheral Validation

## Overview

Define C++20 concepts for all HAL peripherals to replace SFINAE-based template validation. This provides clear, actionable error messages when peripheral configurations are invalid.

## ADDED Requirements

### Requirement: Core Peripheral Concepts
All peripheral types SHALL have corresponding C++20 concepts that validate their interface at compile-time.

**Rationale**: Concepts provide 10x better error messages than SFINAE template failures.

#### Scenario: GPIO Pin Concept Validation
```cpp
// Define concept
template<typename T>
concept GpioPin = requires(T pin) {
    { pin.set() } -> std::same_as<void>;
    { pin.clear() } -> std::same_as<void>;
    { pin.toggle() } -> std::same_as<void>;
    { pin.read() } -> std::convertible_to<bool>;
    { pin.setDirection(PinDirection{}) } -> std::same_as<void>;
    requires T::is_gpio_pin == true;
};

// Usage
template<GpioPin Pin>
void blink(Pin& pin) {
    pin.toggle();  // Guaranteed to exist
}

// Error message if invalid:
// error: 'MyType' does not satisfy concept 'GpioPin'
// note: 'MyType::set()' is not defined
```

**Success Criteria**:
- ✅ All 10 core peripherals have concept definitions
- ✅ Concept validation catches interface mismatches
- ✅ Error messages list missing methods/types

---

### Requirement: Signal Compatibility Concepts
Peripheral signals SHALL be validated for compatibility with pins and channels at compile-time.

**Rationale**: Prevents connecting incompatible pins (e.g., PA5 to USART1::TX when it only supports USART2::TX).

#### Scenario: Pin Supports UART Signal
```cpp
template<typename Pin, typename Signal>
concept SupportsSignal = requires {
    // Pin must have alternate function support
    requires Pin::template supports<Signal>();
    // Signal must be in pin's compatible signals
    requires contains(Pin::compatible_signals, Signal::id);
};

// Usage
template<GpioPin TxPin>
    requires SupportsSignal<TxPin, Usart1::TxSignal>
void configure_uart_tx(TxPin& pin) {
    pin.setAlternateFunction(Usart1::TxSignal::alternate_function);
}

// Error message:
// error: Pin PA5 does not support signal USART1::TX
// note: Compatible pins for USART1::TX: PA9, PA15, PB6
// note: PA5 supports: USART2::TX, SPI1::MOSI
```

**Success Criteria**:
- ✅ Compile-time validation of pin→signal compatibility
- ✅ Error messages list compatible alternatives
- ✅ Suggests correct peripheral if pin supports similar signal

#### Scenario: DMA Channel Compatibility
```cpp
template<typename DmaChannel, typename PeripheralSignal>
concept DmaCompatible = requires {
    // DMA channel must support this peripheral
    requires contains(DmaChannel::supported_peripherals, PeripheralSignal::peripheral_id);
    // No conflicting assignments
    requires !DmaChannel::is_allocated();
};

// Usage
template<typename DmaStream>
    requires DmaCompatible<DmaStream, Usart1::TxData>
void enable_dma(DmaStream& stream) {
    stream.configure(Usart1::tx_register_address());
}

// Error message:
// error: DMA Stream 7 does not support USART1 TX
// note: Compatible DMA streams for USART1 TX: Stream 4, Stream 7
// note: Stream 7 is already allocated to SPI1 TX
```

**Success Criteria**:
- ✅ DMA channel compatibility checked at compile-time
- ✅ Conflicts detected and reported with allocation info
- ✅ Suggests alternative free DMA channels

---

### Requirement: Configuration Validation Concepts
Peripheral configurations SHALL be validated for completeness and correctness at compile-time.

**Rationale**: Catches missing required fields (e.g., baudrate not set) at compile-time.

#### Scenario: UART Configuration Completeness
```cpp
template<typename Config>
concept ValidUsartConfig = requires(Config cfg) {
    { cfg.baudrate } -> std::convertible_to<uint32_t>;
    { cfg.word_length } -> std::convertible_to<uint8_t>;
    { cfg.parity } -> std::same_as<Parity>;
    { cfg.stop_bits } -> std::same_as<StopBits>;
    requires cfg.baudrate > 0;
    requires cfg.word_length >= 7 && cfg.word_length <= 9;
};

template<ValidUsartConfig Config>
void initialize_usart(Config cfg) {
    // All required fields guaranteed to exist and be valid
}

// Error message:
// error: UsartConfig does not satisfy ValidUsartConfig
// note: Missing field: 'baudrate'
// note: Field 'word_length' has invalid value: 10 (must be 7-9)
```

**Success Criteria**:
- ✅ All configuration structs have validation concepts
- ✅ Missing fields reported with field name
- ✅ Invalid values reported with acceptable ranges

---

## Success Criteria

1. **Comprehensive Coverage**: All 10 core peripherals have concepts defined
2. **Error Quality**: Error messages include specific field names and suggestions
3. **Compile Time**: Concept checks add < 10% to compile time
4. **Zero Runtime Cost**: No runtime overhead vs template version
5. **Adoption**: New code uses concepts, old code still compiles

## Testing Strategy

### Unit Tests
```cpp
// Test concept satisfaction
static_assert(GpioPin<GpioA9>);
static_assert(!GpioPin<int>);
static_assert(SupportsSignal<GpioA9, Usart1::TxSignal>);
static_assert(!SupportsSignal<GpioA5, Usart1::TxSignal>);
```

### Integration Tests
- Verify error messages by intentionally failing concept checks
- Compare compile time with/without concepts
- Measure binary size to ensure zero runtime cost

### Validation
- Manual review of error messages for clarity
- User testing with developers of varying experience
- Benchmark compile times on CI

## Dependencies

- C++20 compiler with full concepts support
- Enhanced SVD codegen to generate signal compatibility tables
- Updated HAL interfaces to expose required type traits

## Migration Path

1. **Phase 1**: Add concepts alongside existing templates (no breaking changes)
2. **Phase 2**: Update examples to use concepts
3. **Phase 3**: Deprecate old template-only APIs
4. **Phase 4**: (Optional) Remove deprecated APIs in major version

## Open Questions

- Should concepts be in a separate namespace or inline with peripherals?
- How to handle platform-specific signals (e.g., ESP32 vs STM32)?
- Should we provide concept matchers for testing (e.g., `concept_satisfied<GpioPin, int>()`)?
