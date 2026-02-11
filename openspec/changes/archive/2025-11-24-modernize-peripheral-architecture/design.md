# Design: Modernize Peripheral Architecture

> **⚠️ IMPORTANT**: This project uses **EXCLUSIVELY Policy-Based Design** for hardware abstraction.
> See [ARCHITECTURE.md](ARCHITECTURE.md) for the comprehensive rationale and decision.
> No other techniques (traits, CRTP, inheritance) will be used.

## Architecture Overview

### Current State (Template-Heavy)

```
┌─────────────────────────────────────────┐
│  User Code                              │
│  Usart1::initialize(config);            │
│  // Template errors if wrong!           │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  Platform Layer (src/hal/platform/)     │
│  - Template metaprogramming (SFINAE)    │
│  - 50+ line error messages               │
│  - No signal routing validation          │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  Vendor Layer (src/hal/vendors/)        │
│  - Auto-generated registers             │
│  - No metadata about connections         │
└─────────────────────────────────────────┘
```

### Target State (Concept-Based with Signals)

```
┌─────────────────────────────────────────────────────────┐
│  Level 1: Simple API (Beginners)                        │
│  Usart1::quick_setup(tx_pin, rx_pin, 115200);           │
└──────────────┬──────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────┐
│  Level 2: Fluent API (Common Use)                       │
│  Usart1::pin(A9).as_tx()                                 │
│        .pin(A10).as_rx()                                 │
│        .baudrate(115200)                                 │
│        .initialize();                                    │
└──────────────┬──────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────┐
│  Level 3: Expert API (Advanced Configuration)            │
│  constexpr UsartConfig cfg {                             │
│      .tx = GpioA9.with_af(AlternateFunction7),          │
│      .rx = GpioA10.with_pull(PullUp),                   │
│      .dma_tx = Dma1::Stream7                             │
│  };                                                      │
│  static_assert(cfg.is_valid());                          │
│  Usart1::configure(cfg);                                 │
└──────────────┬──────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────┐
│  Concept Layer (C++20)                                   │
│  - Clear error messages with suggestions                 │
│  - Compile-time validation                               │
│  - Signal compatibility checking                         │
└──────────────┬──────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────┐
│  Signal Routing Layer (NEW)                              │
│  - Pin→Peripheral signal mapping                         │
│  - DMA channel allocation                                │
│  - Compile-time conflict detection                       │
└──────────────┬──────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────┐
│  Generic API Layer (Platform-Agnostic)                   │
│  - template <typename HardwarePolicy>                    │
│  - Business logic & validation                           │
│  - NO platform-specific code                             │
│  - Result<T, ErrorCode> error handling                   │
└──────────────┬──────────────────────────────────────────┘
               │ uses
               ▼
┌─────────────────────────────────────────────────────────┐
│  Hardware Policy Layer (NEW - Platform-Specific)         │
│  - Auto-generated from JSON metadata                     │
│  - Static inline methods (zero overhead)                 │
│  - Direct register access                                │
│  - Mock hooks for testing                                │
└──────────────┬──────────────────────────────────────────┘
               │ accesses
               ▼
┌─────────────────────────────────────────────────────────┐
│  Vendor Layer (Enhanced with Metadata)                   │
│  - Auto-generated registers (existing)                   │
│  - Auto-generated bitfields (existing)                   │
│  - NEW: Signal routing tables from SVD                   │
│  - NEW: IRQ tables from SVD                              │
│  - NEW: DMA channel compatibility matrices               │
└─────────────────────────────────────────────────────────┘
```

## Key Design Decisions

### 0. Policy-Based Design for Hardware Abstraction ✅ CANONICAL

**Decision**: Use **ONLY** policy-based design for connecting generic APIs to platform-specific hardware.

**Rationale**:
- Zero runtime overhead (all methods static inline)
- Clear separation of concerns (generic logic vs hardware access)
- Testable (can inject mock policies)
- Maintainable (auto-generated from JSON metadata)

**Implementation Pattern**:
```cpp
// Generic API (platform-agnostic)
template <PeripheralId Id, typename HardwarePolicy>
class UartImpl {
public:
    Result<void, ErrorCode> initialize() {
        HardwarePolicy::reset();        // Uses policy
        HardwarePolicy::set_baudrate(); // Uses policy
        HardwarePolicy::enable();       // Uses policy
        return Ok();
    }
};

// Hardware Policy (platform-specific)
template <uint32_t BASE_ADDR, uint32_t CLOCK_HZ>
struct Same70UartHardwarePolicy {
    static inline void reset() {
        hw()->CR = uart::cr::RSTRX::mask;  // Direct register access
    }

    static inline void set_baudrate(uint32_t baud) {
        hw()->BRGR = CLOCK_HZ / (16 * baud);
    }

    static inline void enable() {
        hw()->CR = uart::cr::TXEN::mask | uart::cr::RXEN::mask;
    }

    static inline volatile UART0_Registers* hw() {
        return reinterpret_cast<volatile UART0_Registers*>(BASE_ADDR);
    }
};

// Platform-specific aliases
using Uart0 = UartImpl<PeripheralId::USART0, Same70UartHardwarePolicy<0x400E0800, 150000000>>;
```

**Why NOT other techniques:**
- ❌ **Traits**: Requires specialization for each peripheral instance, less flexible
- ❌ **CRTP**: Inheritance-based, more complex, harder to test
- ❌ **Virtual Functions**: Runtime overhead (vtable), violates zero-cost principle
- ❌ **Template Specialization**: Code duplication, hard to maintain

**See [ARCHITECTURE.md](ARCHITECTURE.md) for complete rationale.**

---

### 1. Three-Level API Hierarchy

**Rationale**: Different users have different needs
- **Beginners**: Want minimal code, don't care about details
- **Common users**: Want readable, maintainable code
- **Experts**: Need full control and optimization

**Implementation**:
```cpp
// Level 1: One-liner
template<GpioPin TxPin, GpioPin RxPin>
static void quick_setup(TxPin tx, RxPin rx, uint32_t baudrate);

// Level 2: Fluent builder
class UsartBuilder {
    UsartBuilder& pin(GpioPin auto p);
    UsartBuilder& as_tx();
    UsartBuilder& baudrate(uint32_t baud);
    void initialize();
};

// Level 3: Config struct
struct UsartConfig {
    GpioPin auto tx;
    GpioPin auto rx;
    uint32_t baudrate;
    consteval bool is_valid() const;
};
```

### 2. C++20 Concepts for Error Messages

**Current Problem**:
```
error: no matching function for call to 'Usart1::initialize'
note: candidate template ignored: substitution failure [with ...]
... (48 more lines of template instantiation stack)
```

**With Concepts**:
```
error: Pin PA5 does not support USART1::Tx signal
note: Compatible pins for USART1::Tx: PA9, PA15, PB6
note: Did you mean USART2? PA5 supports USART2::Tx
```

**Implementation**:
```cpp
template<typename T>
concept GpioPin = requires(T pin) {
    { pin.set() } -> std::same_as<void>;
    { pin.read() } -> std::convertible_to<bool>;
    requires T::is_gpio_pin;
};

template<GpioPin Pin, typename Signal>
concept SupportsSignal = requires {
    // Check if pin's alternate functions include this signal
    requires Pin::template supports<Signal>();
};
```

### 3. Signal Routing System

**Purpose**: Explicit peripheral-to-peripheral connections

**Signals**:
- **GPIO Signals**: UART_TX, UART_RX, SPI_MOSI, SPI_MISO, I2C_SDA, I2C_SCL
- **DMA Signals**: ADC_DATA, UART_TX_DATA, UART_RX_DATA, SPI_TX_DATA
- **Trigger Signals**: TIMER_TRIGGER, ADC_TRIGGER, PWM_TRIGGER

**Example**:
```cpp
// GPIO to UART connection
constexpr auto tx_connection = connect(GpioA9, Usart1::Tx);
static_assert(tx_connection.is_valid(), tx_connection.error_message());

// DMA to UART connection
constexpr auto dma_connection = connect(Dma1::Stream7, Usart1::TxData);
static_assert(dma_connection.is_valid());
```

### 4. Compile-Time Validation with consteval

**Purpose**: Catch errors before runtime with custom messages

**Example**:
```cpp
struct PinSignalConnection {
    GpioPin auto pin;
    PeripheralSignal auto signal;

    consteval PinSignalConnection(auto p, auto s) : pin(p), signal(s) {
        if (!pin.supports(signal)) {
            // C++23: Custom compile error
            throw "Pin does not support this signal!";
        }
    }

    consteval const char* error_message() const {
        if (!pin.supports(signal)) {
            return format("Pin {} cannot connect to {}\n"
                         "Compatible pins: {}",
                         pin.name(), signal.name(),
                         signal.compatible_pins());
        }
        return "";
    }
};
```

### 5. Auto-Generated Signal Metadata

**From SVD**: Extract pin alternate functions

**Input** (SVD XML):
```xml
<peripheral>
  <name>USART1</name>
  <signals>
    <signal>
      <name>TX</name>
      <pins>
        <pin>PA9</pin>
        <pin>PA15</pin>
        <pin>PB6</pin>
      </pins>
      <alternate-function>AF7</alternate-function>
    </signal>
  </signals>
</peripheral>
```

**Generated Output**:
```cpp
namespace generated {
    struct Usart1TxSignal {
        static constexpr std::array compatible_pins = {
            PinId::PA9, PinId::PA15, PinId::PB6
        };
        static constexpr AlternateFunction af = AlternateFunction::AF7;
        static constexpr const char* name = "USART1::TX";
    };
}
```

### 6. DMA Channel Type-Checking

**Purpose**: Prevent DMA channel conflicts at compile-time

**Channel Registry**:
```cpp
template<typename Peripheral>
struct DmaChannelRegistry {
    // Maps peripheral to compatible DMA channels
    static constexpr auto channels = /* generated from SVD */;
};

// Usage
using UartTxDma = DmaConnection<Usart1::Tx, Dma1::Stream7>;
static_assert(UartTxDma::is_valid());
static_assert(!UartTxDma::conflicts_with<SpiTxDma>());
```

## Data Flow Diagrams

### Signal Connection Flow

```
User Code: Usart1::pin(A9).as_tx()
                  │
                  ▼
         ┌─────────────────┐
         │ Concept Check   │
         │ Is A9 GpioPin?  │
         └────────┬─────────┘
                  │ Yes
                  ▼
         ┌─────────────────┐
         │ Signal Lookup   │
         │ A9 supports TX? │
         └────────┬─────────┘
                  │ Yes: AF7
                  ▼
         ┌─────────────────┐
         │ Configure GPIO  │
         │ Set AF7 mode    │
         └────────┬─────────┘
                  │
                  ▼
         ┌─────────────────┐
         │ Register Signal │
         │ A9 → USART1::TX │
         └─────────────────┘
```

### DMA Connection Flow

```
User Code: Usart1::enable_dma_tx<Dma1::Stream7>()
                         │
                         ▼
              ┌──────────────────┐
              │ Check Channel    │
              │ Available?       │
              └────────┬──────────┘
                       │ Yes
                       ▼
              ┌──────────────────┐
              │ Check Compatible │
              │ USART1 + Stream7?│
              └────────┬──────────┘
                       │ Yes
                       ▼
              ┌──────────────────┐
              │ Register DMA     │
              │ Stream7 → USART1 │
              └────────┬──────────┘
                       │
                       ▼
              ┌──────────────────┐
              │ Configure HW     │
              │ DMA registers    │
              └──────────────────┘
```

## Implementation Phases

### Phase 1: Foundation (Weeks 1-2)
- Add concept definitions for all peripherals
- Create signal metadata structures
- Implement consteval validation helpers

### Phase 2: GPIO Integration (Weeks 3-4)
- Generate pin alternate function tables from SVD
- Implement GPIO signal routing
- Add compile-time pin validation

### Phase 3: UART Example (Weeks 5-6)
- Implement 3-level API for UART
- Add GPIO→UART signal connections
- Create comprehensive examples

### Phase 4: DMA Integration (Weeks 7-8)
- Generate DMA channel compatibility tables
- Implement type-safe DMA connections
- Add UART DMA example

### Phase 5: Expand to Other Peripherals (Weeks 9-12)
- Migrate SPI, I2C, ADC, Timer, PWM
- Add cross-peripheral examples (Timer→ADC, ADC→DMA)
- Performance benchmarking

### Phase 6: Documentation & Migration (Weeks 13-14)
- Write comprehensive migration guide
- Create comparison examples (old vs new API)
- Document best practices

## Trade-offs

### Compile Time vs Error Quality
- **Trade-off**: Concepts add ~10-15% compile time
- **Justification**: 10x better error messages worth the cost
- **Mitigation**: Use concepts only at API boundary, not internally

### Complexity vs Flexibility
- **Trade-off**: Three API levels means more code to maintain
- **Justification**: Serves beginners to experts equally well
- **Mitigation**: Levels build on each other, shared implementation

### Generics vs Performance
- **Trade-off**: Generic code might be less optimized
- **Justification**: Templates still compile to same assembly
- **Mitigation**: Benchmark every peripheral, maintain zero-overhead

## Success Metrics

| Metric | Current | Target | Validation |
|--------|---------|--------|------------|
| Error message lines | 50+ | < 10 | Manual review |
| Compile time | 100% | < 115% | CI benchmarks |
| Binary size | 100% | 100% | Size comparison |
| Lines per config | 10-15 | 5-8 | Example analysis |
| API adoption | 0% | 80% | Usage tracking |

## Open Questions

1. **Q**: Should we support runtime pin remapping?
   **A**: No, keep compile-time only for now (revisit in Phase 6 if requested)

2. **Q**: How to handle conflicting DMA assignments?
   **A**: Compile-time registry tracks allocations, fails if conflict detected

3. **Q**: Backward compatibility with existing code?
   **A**: New API coexists with old, migration is optional and gradual

4. **Q**: Performance impact of concepts?
   **A**: Zero runtime impact (all resolved at compile-time), minor compile time increase
