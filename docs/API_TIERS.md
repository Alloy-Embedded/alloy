# MicroCore API Tier System

MicroCore provides a sophisticated three-tier API system for hardware abstraction, built on the **CRTP (Curiously Recurring Template Pattern)** for zero-overhead code reuse.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│  User Code (Application Layer)                         │
│                                                         │
│  Choose your abstraction level:                        │
│  - Simple API  → Quick & easy                          │
│  - Fluent API  → Readable & chainable                  │
│  - Expert API  → Full control                          │
└─────────────────────────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────┐
│  API Tier Layer (src/hal/api/)                         │
│                                                         │
│  *_base.hpp   → CRTP base (shared code)                │
│  *_simple.hpp → Factory methods & presets              │
│  *_fluent.hpp → Builder pattern & chaining             │
│  *_expert.hpp → Full configuration control             │
│                                                         │
│  Peripherals: GPIO, UART, SPI, I2C, Timer, ADC, PWM    │
└─────────────────────────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────┐
│  Platform Layer (src/hal/platform/*/*)                 │
│                                                         │
│  Hardware policies (platform-specific)                 │
│  - stm32f4::GpioPinPolicy                              │
│  - stm32f7::GpioPinPolicy                              │
│  - same70::GpioPinPolicy                               │
└─────────────────────────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────┐
│  Hardware Registers (MCU-specific)                      │
└─────────────────────────────────────────────────────────┘
```

## CRTP Pattern: Zero-Overhead Code Reuse

Instead of traditional inheritance (virtual functions = runtime overhead), MicroCore uses **CRTP** for compile-time polymorphism:

```cpp
// Base class template
template <typename Derived>
class GpioBase {
    // Common methods delegate to derived via CRTP
    Result<void> on() {
        return static_cast<Derived*>(this)->on_impl();
    }

    Result<void> off() {
        return static_cast<Derived*>(this)->off_impl();
    }
};

// Derived classes inherit from GpioBase<Derived>
template <typename PinType>
class SimpleGpioPin : public GpioBase<SimpleGpioPin<PinType>> {
    friend GpioBase<SimpleGpioPin<PinType>>;

    // Implement required methods
    Result<void> on_impl() { /* ... */ }
    Result<void> off_impl() { /* ... */ }
};
```

**Benefits**:
- ✅ Zero runtime overhead (no vtable, no virtual calls)
- ✅ Code reuse across Simple/Fluent/Expert APIs
- ✅ Compile-time type checking via concepts
- ✅ Smaller binary size (shared implementation)

## Tier 1: Simple API

**Target audience**: Beginners, rapid prototyping, Arduino users

**Philosophy**: One-liner setup with factory methods for common use cases

### GPIO Simple API

Located in: `src/hal/api/gpio_simple.hpp`

```cpp
#include "hal/api/gpio_simple.hpp"

using namespace ucore::hal;

// Factory method: output pin (active-high LED)
auto led = Gpio::output<GpioPin<peripherals::PIOC, 8>>();
led.on();   // Turn LED on
led.off();  // Turn LED off
led.toggle();

// Factory method: input with pull-up (button)
auto button = Gpio::input_pullup<GpioPin<peripherals::PIOA, 11>>();
if (button.is_on().value()) {
    // Button pressed (active-low with pull-up)
}

// Factory method: active-low output
auto led_active_low = Gpio::output_active_low<GpioPin<peripherals::PIOC, 9>>();
led_active_low.on();  // Sets pin LOW physically

// Factory method: open-drain (I2C, 1-Wire)
auto sda = Gpio::open_drain<GpioPin<peripherals::PIOB, 7>>();
```

**Features**:
- ✅ Factory methods for common configurations
- ✅ Active-high/active-low abstraction (logical vs. physical state)
- ✅ No configuration boilerplate needed
- ✅ Result<T> error handling

### UART Simple API

Located in: `src/hal/api/uart_simple.hpp`

```cpp
#include "hal/api/uart_simple.hpp"

// Simple UART with default config (9600 baud, 8N1)
auto serial = UartSimple<Uart1>();

serial.write_str("Hello, World!\n");
serial.write_u32(12345);

if (serial.available()) {
    auto byte = serial.read_byte();
}
```

### SPI Simple API

Located in: `src/hal/api/spi_simple.hpp`

```cpp
#include "hal/api/spi_simple.hpp"

// Simple SPI master with preset config
auto spi = SpiSimple<Spi1>();

uint8_t tx_data[] = {0x01, 0x02, 0x03};
uint8_t rx_data[3];
spi.transfer(tx_data, rx_data, 3);
```

## Tier 2: Fluent API

**Target audience**: Intermediate users, production code, team development

**Philosophy**: Method chaining for readable, self-documenting configuration

### GPIO Fluent API

Located in: `src/hal/api/gpio_fluent.hpp`

```cpp
#include "hal/api/gpio_fluent.hpp"

using namespace ucore::hal;

// Fluent builder pattern
auto led = GpioFluent<GpioPin<peripherals::PIOC, 8>>()
    .as_output()          // Set direction
    .push_pull()          // Drive mode
    .active_high()        // Logical level
    .initial_state_off()  // Start state
    .build();             // Construct configured pin

led.on().unwrap();   // Result-based error handling
led.off().unwrap();

// Fluent chaining for inputs
auto button = GpioFluent<GpioPin<peripherals::PIOA, 11>>()
    .as_input()
    .pull_up()
    .active_low()  // Button press = LOW
    .build();

if (button.is_on().unwrap()) {
    // Button pressed
}
```

**Features**:
- ✅ Builder pattern with method chaining
- ✅ Incremental validation (compile-time checks)
- ✅ Self-documenting (reads like natural language)
- ✅ Explicit error handling with Result<T>

### UART Fluent API

Located in: `src/hal/api/uart_fluent.hpp`

```cpp
#include "hal/api/uart_fluent.hpp"

// Fluent UART configuration
auto serial = UartFluent<Uart1>()
    .baud_rate(115200)
    .data_bits(8)
    .stop_bits(1)
    .parity(Parity::None)
    .flow_control(FlowControl::None)
    .mode(UartMode::TxRx)
    .build()
    .unwrap();

serial.write_str("Configured!\n").unwrap();
```

### SPI Fluent API

Located in: `src/hal/api/spi_fluent.hpp`

```cpp
#include "hal/api/spi_fluent.hpp"

// Fluent SPI configuration
auto spi = SpiFluent<Spi1>()
    .mode(SpiMode::Mode0)          // CPOL=0, CPHA=0
    .bit_order(BitOrder::MsbFirst)
    .clock_speed(1000000)          // 1 MHz
    .data_size(8)                  // 8-bit transfers
    .build()
    .unwrap();

spi.transfer(tx_buffer, rx_buffer, 10).unwrap();
```

## Tier 3: Expert API

**Target audience**: Advanced users, library developers, performance-critical code

**Philosophy**: Full control over all configuration parameters with validation

### GPIO Expert API

Located in: `src/hal/api/gpio_expert.hpp`

```cpp
#include "hal/api/gpio_expert.hpp"

using namespace ucore::hal;

// Expert configuration with all parameters exposed
GpioExpertConfig config = {
    .direction = PinDirection::Output,
    .pull = PinPull::None,
    .drive = PinDrive::PushPull,
    .active_high = true,
    .initial_state_on = false,

    // Advanced platform-specific features
    .drive_strength = 3,        // Maximum (0-3)
    .slew_rate_fast = true,     // Fast switching
    .input_filter_enable = false,
    .filter_clock_div = 0
};

// Compile-time validation
static_assert(config.is_valid(), config.error_message());

// Create expert GPIO pin
auto led = GpioExpert<GpioPin<peripherals::PIOC, 8>>(config);
led.on().unwrap();

// Factory methods for common expert configs
auto led2 = GpioExpert<GpioPin<peripherals::PIOC, 9>>(
    GpioExpertConfig::high_speed_output()
);

auto led3 = GpioExpert<GpioPin<peripherals::PIOC, 10>>(
    GpioExpertConfig::low_power_output()
);
```

**Features**:
- ✅ Complete control over all parameters
- ✅ Platform-specific advanced features (drive strength, slew rate, filters)
- ✅ Compile-time validation with `static_assert`
- ✅ Factory methods for common expert patterns
- ✅ Struct-based configuration (aggregate initialization)

### UART Expert API

Located in: `src/hal/api/uart_expert.hpp`

```cpp
#include "hal/api/uart_expert.hpp"

// Expert UART configuration
UartExpertConfig config = {
    .baud_rate = 921600,         // High-speed
    .data_bits = 8,
    .stop_bits = 1,
    .parity = Parity::Even,
    .flow_control = FlowControl::RtsCts,

    // Advanced features
    .oversampling = 8,           // Reduce noise
    .one_bit_sampling = true,    // Higher speed
    .receiver_timeout_enable = true,
    .timeout_value = 100,

    // DMA configuration
    .tx_dma_enable = true,
    .rx_dma_enable = true,
    .tx_dma_priority = DmaPriority::High,
    .rx_dma_priority = DmaPriority::VeryHigh
};

static_assert(config.is_valid(), config.error_message());

auto serial = UartExpert<Uart1>(config);
```

### SPI Expert API

Located in: `src/hal/api/spi_expert.hpp`

```cpp
#include "hal/api/spi_expert.hpp"

// Expert SPI configuration
SpiExpertConfig config = {
    .mode = SpiMode::Mode3,
    .bit_order = BitOrder::LsbFirst,
    .clock_speed = 10000000,     // 10 MHz
    .data_size = 16,             // 16-bit transfers

    // Advanced features
    .nss_mode = NssMode::Hardware,
    .nss_pulse = true,
    .ti_mode = false,
    .crc_enable = true,
    .crc_polynomial = 0x1021,

    // DMA
    .tx_dma_enable = true,
    .rx_dma_enable = true
};

auto spi = SpiExpert<Spi1>(config);
```

## Tier Comparison

| Feature | Simple | Fluent | Expert |
|---------|--------|--------|--------|
| **Ease of use** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Explicitness** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Control** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Lines of code** | 1-2 | 3-8 | 5-20 |
| **Factory methods** | ✅ Yes | ❌ No | ✅ Yes (expert presets) |
| **Method chaining** | ❌ No | ✅ Yes | ❌ No |
| **Configuration style** | Factory | Builder | Struct |
| **Validation** | Runtime | Compile-time | Compile-time |
| **Platform features** | Basic only | Basic + some | All features |
| **Error handling** | Result<T> | Result<T> | Result<T> |
| **Use case** | Prototyping | Production | Performance-critical |

## Available Peripherals

All peripherals follow the same tier pattern:

| Peripheral | Simple | Fluent | Expert | Base (CRTP) |
|------------|--------|--------|--------|-------------|
| **GPIO** | ✅ | ✅ | ✅ | ✅ |
| **UART** | ✅ | ✅ | ✅ | ✅ |
| **SPI** | ✅ | ✅ | ✅ | ✅ |
| **I2C** | ✅ | ✅ | ✅ | ❌ |
| **Timer** | ✅ | ✅ | ✅ | ❌ |
| **ADC** | ✅ | ❌ | ✅ | ❌ |
| **PWM** | ✅ | ❌ | ✅ | ❌ |
| **Watchdog** | ✅ | ❌ | ✅ | ❌ |
| **Interrupt** | ✅ | ❌ | ✅ | ❌ |

**Note**: Peripherals without Fluent API don't benefit from builder pattern (simple config only).

## When to Use Each Tier

```
┌─────────────────────────────────────────────────┐
│ Are you prototyping or learning embedded?      │
│              YES → Use Simple API               │
└─────────────────────────────────────────────────┘
                      │ NO
                      ▼
┌─────────────────────────────────────────────────┐
│ Do you need explicit config & self-documenting │
│ code for team development?                     │
│              YES → Use Fluent API               │
└─────────────────────────────────────────────────┘
                      │ NO
                      ▼
┌─────────────────────────────────────────────────┐
│ Do you need platform-specific features, full   │
│ control, or performance-critical tuning?       │
│              YES → Use Expert API               │
└─────────────────────────────────────────────────┘
```

### Simple API - Use When:
- ✅ Rapid prototyping
- ✅ Learning embedded development
- ✅ Simple applications with default config
- ✅ Arduino-style projects
- ✅ You want minimal code

**Example**: LED blink, button reading, basic UART debug

### Fluent API - Use When:
- ✅ Production code requiring maintainability
- ✅ Team development (self-documenting)
- ✅ Multiple configuration options needed
- ✅ You want readable, chainable code
- ✅ Explicit error handling with Result<T>

**Example**: Industrial control, data logging, protocol implementations

### Expert API - Use When:
- ✅ Performance-critical code paths
- ✅ Platform-specific features needed (drive strength, DMA, etc.)
- ✅ Fine-grained control over hardware
- ✅ Real-time systems
- ✅ Power-optimized applications

**Example**: High-speed communications, motor control, sensor fusion

## Mixing Tiers

You can freely mix tiers in the same application:

```cpp
// Simple API for status LED
auto status_led = Gpio::output<GpioPin<PIOC, 8>>();

// Fluent API for complex UART
auto serial = UartFluent<Uart1>()
    .baud_rate(115200)
    .flow_control(FlowControl::RtsCts)
    .build()
    .unwrap();

// Expert API for high-speed SPI
SpiExpertConfig spi_config = {
    .mode = SpiMode::Mode0,
    .clock_speed = 10000000,
    .tx_dma_enable = true,
    .rx_dma_enable = true
};
auto spi = SpiExpert<Spi1>(spi_config);
```

**Rule of thumb**: Use the simplest tier that meets your requirements.

## Implementation Details

### CRTP Base Classes

All tier APIs inherit from a `*_base.hpp` CRTP base:

```cpp
// gpio_base.hpp
template <typename Derived>
class GpioBase {
protected:
    Derived& impl() { return static_cast<Derived&>(*this); }
    const Derived& impl() const { return static_cast<const Derived&>(*this); }

public:
    // Common interface delegates to derived
    Result<void> on() { return impl().on_impl(); }
    Result<void> off() { return impl().off_impl(); }
    Result<void> toggle() { return impl().toggle_impl(); }
    Result<bool> is_on() const { return impl().is_on_impl(); }

    // ... many more shared methods
};

// gpio_simple.hpp
template <typename PinType>
class SimpleGpioPin : public GpioBase<SimpleGpioPin<PinType>> {
    friend GpioBase<SimpleGpioPin<PinType>>;

    // Only implement required methods - base provides the rest!
    Result<void> on_impl() { /* ... */ }
    Result<void> off_impl() { /* ... */ }
};
```

### Compile-Time Validation

Expert tier uses concepts for compile-time validation:

```cpp
template <typename T>
concept GpioImplementation = requires(T gpio) {
    { gpio.on_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.off_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.read_impl() } -> std::same_as<Result<bool, ErrorCode>>;
    // ...
};

// Compile error if interface incomplete
static_assert(GpioImplementation<SimpleGpioPin<GpioPin<PIOC, 8>>>);
```

## Performance

All three tiers compile to **identical machine code** with optimization:

```cpp
// Simple
auto led = Gpio::output<GpioPin<PIOC, 8>>();
led.on();

// Fluent
auto led = GpioFluent<GpioPin<PIOC, 8>>().as_output().build();
led.on();

// Expert
auto led = GpioExpert<GpioPin<PIOC, 8>>(GpioExpertConfig::standard_output());
led.on();

// ALL compile to the same assembly:
// STR [PIOC_SODR], #(1 << 8)  ; Single instruction!
```

**Proof**: Compile with `-O2` and compare assembly output - they're identical.

## Related Documentation

- [CRTP Pattern Guide](architecture/CRTP_PATTERN.md) - Deep dive into CRTP implementation
- [GPIO API Reference](api/gpio.md) - Complete GPIO API documentation
- [UART API Reference](api/uart.md) - Complete UART API documentation
- [Porting Guide](porting_platform.md) - Adding new platforms
- [Examples](../examples/) - Code examples for each tier

## FAQs

**Q: Does Simple tier make my code slower?**
A: No. All tiers compile to identical machine code with optimization enabled.

**Q: Can I mix tiers for different peripherals?**
A: Yes! Use Simple for LED, Fluent for UART, Expert for SPI - all in the same app.

**Q: Which tier does MicroCore recommend?**
A: Start with Simple, move to Fluent for production, use Expert for performance-critical paths.

**Q: What's the benefit of CRTP over virtual functions?**
A: Zero runtime overhead - no vtable, no virtual calls, smaller binary, faster execution.

**Q: Can I access platform-specific features from Simple/Fluent?**
A: Limited. Simple/Fluent expose common features. For platform-specific, use Expert tier.

**Q: How do I migrate between tiers?**
A: Easy - same underlying implementation. Just change the factory/builder/config approach.
