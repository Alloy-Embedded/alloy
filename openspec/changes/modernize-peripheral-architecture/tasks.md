# Tasks: Modernize Peripheral Architecture

## Phase 1: Foundation (Weeks 1-2)

### 1.1 Core Concept Definitions
- [ ] Create `src/hal/concepts.hpp` with base peripheral concepts
- [ ] Define `GpioPin` concept with required methods
- [ ] Define `UartPeripheral` concept
- [ ] Define `SpiPeripheral` concept
- [ ] Add compile tests for concept satisfaction
- [ ] Measure compile time impact (< 10% increase)

**Validation**: Static assertions pass, concepts detect invalid types

---

### 1.2 Signal Type Infrastructure
- [ ] Create `src/hal/signals.hpp` for signal type definitions
- [ ] Define `PeripheralSignal` base concept
- [ ] Define `GpioSignal` for alternate functions
- [ ] Define `DmaSignal` for DMA requests
- [ ] Add signal compatibility checking utilities

**Validation**: Signal types compile, compatibility checks work

---

### 1.3 consteval Validation Helpers
- [ ] Create `src/hal/validation.hpp` with consteval functions
- [ ] Implement `validate_pin_signal()` with custom error messages
- [ ] Implement `validate_dma_connection()`
- [ ] Add `format_error_message()` helper
- [ ] Test error message quality manually

**Validation**: Error messages are clear and actionable

---

## Phase 2: Signal Metadata Generation (Weeks 3-4)

### 2.1 Extend SVD Parser
- [ ] Add signal extraction to `tools/codegen/parsers/svd_parser.py`
- [ ] Parse `<signals>` tags from SVD files
- [ ] Extract pin alternate function mappings
- [ ] Build signal compatibility dictionary
- [ ] Add unit tests for parser

**Validation**: Parser extracts all USART1 signals correctly

---

### 2.2 Signal Table Generation
- [ ] Create `generate_signal_tables.py` template
- [ ] Generate per-peripheral signal structs
- [ ] Include pin IDs and AF numbers
- [ ] Generate reverse lookup (pin→signals)
- [ ] Validate generated code compiles

**Validation**: `Usart1Signals::Tx::pins` array is correct

---

### 2.3 DMA Compatibility Matrix
- [ ] Parse DMA request mappings from SVD
- [ ] Generate `DmaCompatibility` tables per stream
- [ ] Include request types (TX, RX, DATA)
- [ ] Add conflict detection data structures
- [ ] Test on SAME70, STM32F4, ESP32

**Validation**: All DMA streams have compatibility arrays

---

## Phase 3: GPIO Signal Routing (Weeks 5-6)

### 3.1 GPIO Pin Enhancements
- [ ] Add `supports<Signal>()` constexpr method to GpioPin
- [ ] Implement `compatible_signals` constexpr array
- [ ] Add `setAlternateFunction()` method
- [ ] Create `with_af()` builder method
- [ ] Update GpioPin template with signal support

**Validation**: `GpioA9::supports<Usart1::Tx>()` returns true

---

### 3.2 Pin→Signal Connection API
- [ ] Create `connect()` function for pin-signal pairs
- [ ] Implement compile-time compatibility checking
- [ ] Generate clear error messages with suggestions
- [ ] Add `PinSignalConnection` result type
- [ ] Test with valid and invalid combinations

**Validation**: Invalid connection shows helpful error with alternatives

---

### 3.3 Signal Routing Registry
- [ ] Create `SignalRegistry` to track connections
- [ ] Implement conflict detection (one pin, multiple signals)
- [ ] Add `is_allocated()` queries
- [ ] Support query by pin or by signal
- [ ] Test concurrent registrations

**Validation**: Registry detects PA9 used for both UART and SPI

---

## Phase 4: Multi-Level API Implementation (Weeks 7-8)

### 4.1 Level 1: Simple API
- [ ] Implement `Usart::quick_setup(tx, rx, baud)`
- [ ] Add pin validation inside quick_setup
- [ ] Set sensible defaults for all parameters
- [ ] Create simple API for SPI, I2C
- [ ] Write examples for beginners

**Validation**: One-liner UART setup works, shows clear errors

---

### 4.2 Level 2: Fluent API
- [ ] Create `UsartBuilder` class
- [ ] Implement `.pin()`, `.as_tx()`, `.as_rx()` methods
- [ ] Add `.baudrate()`, `.parity()`, etc.
- [ ] Implement `.initialize()` to apply config
- [ ] Chain validation across method calls

**Validation**: Fluent API reads naturally, validates incrementally

---

### 4.3 Level 3: Expert API
- [ ] Define `UsartConfig` struct with all parameters
- [ ] Implement `consteval is_valid()` method
- [ ] Add `error_message()` for detailed diagnostics
- [ ] Create `Usart::configure(config)` function
- [ ] Support both runtime and compile-time configs

**Validation**: Static assertion on config shows custom error message

---

## Phase 5: DMA Integration (Weeks 9-10)

### 5.1 DMA Connection Types
- [ ] Create `DmaConnection<Signal, Stream>` template
- [ ] Implement compatibility checking
- [ ] Add `.is_compatible()` constexpr method
- [ ] Define `.conflicts_with<Other>()` checker
- [ ] Generate clear error messages

**Validation**: Incompatible DMA connection caught at compile-time

---

### 5.2 DMA Channel Registry
- [ ] Create `DmaRegistry` to track allocations
- [ ] Implement `allocate_stream()` compile-time function
- [ ] Add `is_stream_available()` query
- [ ] Support multiple DMA controllers
- [ ] Test allocation conflicts

**Validation**: Double allocation detected with helpful error

---

### 5.3 Type-Safe DMA Configuration
- [ ] Add `transfer()` method to DmaConnection
- [ ] Validate source/destination types
- [ ] Auto-set peripheral register addresses
- [ ] Support Circular, Normal, DoubleBuffer modes
- [ ] Integrate with UART/SPI/ADC

**Validation**: UART DMA transfer works, wrong types rejected

---

## Phase 6: Peripheral Migration (Weeks 11-12)

### 6.1 UART Complete Implementation
- [ ] Implement all 3 API levels for UART
- [ ] Add GPIO signal routing
- [ ] Integrate DMA TX/RX
- [ ] Create comprehensive examples
- [ ] Benchmark vs old implementation (binary size, compile time)

**Validation**: Full UART example works, zero overhead confirmed

---

### 6.2 SPI Implementation
- [ ] Adapt API pattern for SPI
- [ ] Handle MOSI/MISO/SCK/NSS signals
- [ ] Support master and slave modes
- [ ] Add DMA integration
- [ ] Create SPI + DMA example

**Validation**: SPI with DMA transfers data correctly

---

### 6.3 I2C Implementation
- [ ] Implement I2C multi-level API
- [ ] Handle SDA/SCL signals
- [ ] Support 7-bit and 10-bit addressing
- [ ] Add DMA for large transfers
- [ ] Create I2C EEPROM example

**Validation**: I2C reads/writes with DMA

---

### 6.4 ADC Implementation
- [ ] Implement ADC multi-level API
- [ ] Handle multiple channels
- [ ] Support Timer triggers
- [ ] Integrate DMA for continuous sampling
- [ ] Create ADC + Timer + DMA example

**Validation**: ADC triggered by timer, data via DMA

---

## Phase 7: Documentation & Migration (Weeks 13-14)

### 7.1 Migration Guide
- [ ] Write comparison: old vs new API
- [ ] Document each API level with examples
- [ ] Create step-by-step migration guide
- [ ] List breaking changes (if any)
- [ ] Provide error message decoder

**Validation**: Guide covers all common migration scenarios

---

### 7.2 Comprehensive Examples
- [ ] Create "Hello World" for each API level
- [ ] Add complex examples (UART + DMA)
- [ ] Show cross-peripheral examples (Timer→ADC→DMA)
- [ ] Include troubleshooting section
- [ ] Add performance benchmarks

**Validation**: Examples compile and run on all targets

---

### 7.3 Best Practices Documentation
- [ ] Document when to use each API level
- [ ] Explain concept error messages
- [ ] Provide performance guidelines
- [ ] List common pitfalls and solutions
- [ ] Add FAQ section

**Validation**: Documentation reviewed and approved

---

## Phase 8: Validation & Performance (Week 15)

### 8.1 Comprehensive Testing
- [ ] Unit tests for all concepts
- [ ] Integration tests for signal routing
- [ ] DMA allocation conflict tests
- [ ] Cross-peripheral interaction tests
- [ ] Platform-specific tests (SAME70, STM32, ESP32)

**Validation**: All tests pass on all platforms

---

### 8.2 Performance Benchmarking
- [ ] Measure binary size vs old implementation
- [ ] Measure compile time for each API level
- [ ] Profile runtime performance (should be identical)
- [ ] Compare error message quality
- [ ] Document results

**Validation**: Zero runtime overhead confirmed, compile time acceptable

---

### 8.3 Final Review
- [ ] Code review with team
- [ ] Review error message quality
- [ ] Validate documentation completeness
- [ ] Check backward compatibility
- [ ] Get approval for merge

**Validation**: All reviewers approve

---

## Parallel Work Opportunities

These tasks can be done in parallel:
- **Parser work** (2.1-2.3) parallel to **API design** (4.1-4.3)
- **DMA Registry** (5.2) parallel to **Peripheral Migration** (6.2-6.4)
- **Documentation** (7.1-7.3) can start once Phase 6 begins

## Dependencies

```
Phase 1 (Foundation)
  ↓
Phase 2 (Codegen) ←→ Phase 3 (GPIO)
  ↓                    ↓
Phase 4 (API) ←→ Phase 5 (DMA)
  ↓
Phase 6 (Peripherals)
  ↓
Phase 7 (Docs) ←→ Phase 8 (Validation)
```

## Success Metrics

- [ ] Zero runtime overhead vs old implementation
- [ ] < 15% compile time increase
- [ ] Error messages < 10 lines (vs 50+ currently)
- [ ] 80% adoption in new code
- [ ] No regression in existing examples
