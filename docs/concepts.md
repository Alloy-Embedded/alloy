# C++20 Concepts Reference {#concepts}

MicroCore uses **C++20 concepts** to define compile-time interfaces for hardware abstractions. This ensures type safety and provides clear error messages when types don't match requirements.

[TOC]

## What Are Concepts?

Concepts are **named requirements** for template parameters. They replace SFINAE and enable:

✅ **Clear interfaces** - Self-documenting code
✅ **Better errors** - Readable compiler messages
✅ **Type safety** - Catch errors at compile-time
✅ **Constraints** - Restrict template instantiation

## Core Concepts

### GpioPin

Defines the interface for a GPIO pin.

**Declaration:**
```cpp
template <typename T>
concept GpioPin = requires(T) {
    // Configuration
    { T::configure_output() } -> std::same_as<void>;
    { T::configure_input() } -> std::same_as<void>;
    { T::configure_pull_up() } -> std::same_as<void>;
    { T::configure_pull_down() } -> std::same_as<void>;

    // Operations
    { T::set_high() } -> std::same_as<void>;
    { T::set_low() } -> std::same_as<void>;
    { T::toggle() } -> std::same_as<void>;
    { T::read() } -> std::same_as<bool>;

    // Metadata
    { T::port } -> std::convertible_to<uint8_t>;
    { T::pin } -> std::convertible_to<uint8_t>;
};
```

**Usage:**
```cpp
// Generic function accepting any GpioPin
template <GpioPin Pin>
void blink() {
    Pin::configure_output();

    while (true) {
        Pin::set_high();
        delay_ms(500);
        Pin::set_low();
        delay_ms(500);
    }
}

// Usage
using Led = GpioPin<peripherals::GPIOA, 5>;
blink<Led>();  // ✓ Compiles - Led satisfies GpioPin
```

**Error Example:**
```cpp
struct NotAPin {};

blink<NotAPin>();  // ❌ Compiler error:
// "NotAPin does not satisfy GpioPin concept"
// - Missing configure_output()
// - Missing set_high()
// ...
```

**Implementing Types:**
- `ucore::hal::st::stm32f4::GpioHardwarePolicy<Port, Pin>`
- `ucore::hal::st::stm32f7::GpioHardwarePolicy<Port, Pin>`
- `ucore::hal::same70::GpioHardwarePolicy<Port, Pin>`
- `ucore::hal::host::HostGpioHardwarePolicy<Port, Pin>`

---

### SystemClock

Defines the interface for system clock configuration.

**Declaration:**
```cpp
template <typename T>
concept SystemClock = requires(T) {
    // Clock initialization
    { T::initialize() } -> std::same_as<Result<void>>;

    // Clock queries
    { T::get_system_clock_hz() } -> std::same_as<uint32_t>;
    { T::get_ahb_clock_hz() } -> std::same_as<uint32_t>;
    { T::get_apb1_clock_hz() } -> std::same_as<uint32_t>;
    { T::get_apb2_clock_hz() } -> std::same_as<uint32_t>;
};
```

**Usage:**
```cpp
template <SystemClock Clock>
void configure_peripherals() {
    auto result = Clock::initialize();
    if (!result.is_ok()) {
        // Handle clock initialization failure
        return;
    }

    uint32_t sys_freq = Clock::get_system_clock_hz();
    uint32_t apb1_freq = Clock::get_apb1_clock_hz();

    // Configure UART baud rate based on APB1 clock
    configure_uart_baud(apb1_freq, 115200);
}
```

**Implementing Types:**
- `ucore::hal::st::stm32f4::Clock`
- `ucore::hal::st::stm32f7::Clock`
- `ucore::hal::same70::Clock`

---

### UartPeripheral

Defines the interface for UART communication.

**Declaration:**
```cpp
template <typename T>
concept UartPeripheral = requires(T uart, const char* str, uint8_t byte) {
    // Initialization
    { uart.initialize() } -> std::same_as<Result<void>>;

    // Write operations
    { uart.write_byte(byte) } -> std::same_as<void>;
    { uart.write_string(str) } -> std::same_as<void>;

    // Read operations
    { uart.read_byte() } -> std::same_as<uint8_t>;
    { uart.available() } -> std::same_as<bool>;

    // Status
    { uart.is_initialized() } -> std::same_as<bool>;
};
```

**Usage:**
```cpp
template <UartPeripheral Uart>
void send_message(Uart& uart, const char* message) {
    if (!uart.is_initialized()) {
        auto result = uart.initialize();
        if (!result.is_ok()) {
            return;  // Initialization failed
        }
    }

    uart.write_string(message);
    uart.write_string("\r\n");
}
```

**Example:**
```cpp
using UartConfig = SimpleUartConfigTxOnly<
    board::uart::TxPin,
    board::uart::Policy
>;

UartConfig uart{
    board::uart::peripheral_id,
    BaudRate{115200},
    8, UartParity::NONE, 1
};

send_message(uart, "Hello, MicroCore!");
```

---

### SpiPeripheral

Defines the interface for SPI communication.

**Declaration:**
```cpp
template <typename T>
concept SpiPeripheral = requires(T spi, uint8_t byte, uint8_t* buffer, size_t len) {
    // Initialization
    { spi.initialize() } -> std::same_as<Result<void>>;

    // Transfer operations
    { spi.transfer_byte(byte) } -> std::same_as<uint8_t>;
    { spi.transfer(buffer, len) } -> std::same_as<void>;

    // Chip select (optional)
    { spi.select() } -> std::same_as<void>;
    { spi.deselect() } -> std::same_as<void>;
};
```

**Usage:**
```cpp
template <SpiPeripheral Spi>
uint8_t read_register(Spi& spi, uint8_t reg_addr) {
    spi.select();
    spi.transfer_byte(0x80 | reg_addr);  // Read command
    uint8_t value = spi.transfer_byte(0x00);  // Dummy byte
    spi.deselect();
    return value;
}
```

---

### I2cPeripheral

Defines the interface for I2C communication.

**Declaration:**
```cpp
template <typename T>
concept I2cPeripheral = requires(T i2c, uint8_t addr, uint8_t* buffer, size_t len) {
    // Initialization
    { i2c.initialize() } -> std::same_as<Result<void>>;

    // Write operations
    { i2c.write(addr, buffer, len) } -> std::same_as<Result<void>>;

    // Read operations
    { i2c.read(addr, buffer, len) } -> std::same_as<Result<void>>;

    // Combined operations
    { i2c.write_read(addr, buffer, len, buffer, len) } -> std::same_as<Result<void>>;
};
```

**Usage:**
```cpp
template <I2cPeripheral I2c>
Result<uint16_t> read_temperature(I2c& i2c, uint8_t sensor_addr) {
    uint8_t cmd = 0x00;  // Temperature register
    uint8_t data[2];

    auto result = i2c.write_read(sensor_addr, &cmd, 1, data, 2);
    if (!result.is_ok()) {
        return Result<uint16_t>::err(result.error());
    }

    uint16_t temp = (data[0] << 8) | data[1];
    return Result<uint16_t>::ok(temp);
}
```

---

## Advanced Concepts

### HardwarePolicy

Defines the interface for hardware-specific implementations using CRTP.

**Declaration:**
```cpp
template <typename T>
concept HardwarePolicy = requires {
    // Must have port and pin constants
    { T::port } -> std::convertible_to<uint32_t>;
    { T::pin } -> std::convertible_to<uint8_t>;

    // Must provide register operations
    typename T::RegisterType;

    // Must be stateless (all static methods)
    requires std::is_empty_v<T>;
};
```

**Usage:**
```cpp
template <HardwarePolicy Policy>
class GpioPin {
public:
    static void set_high() {
        Policy::set_high();
    }

    static void set_low() {
        Policy::set_low();
    }
};
```

---

### ClockSource

Defines different clock sources.

**Declaration:**
```cpp
template <typename T>
concept ClockSource = requires(T) {
    // Clock source must provide frequency
    { T::frequency_hz() } -> std::convertible_to<uint32_t>;

    // Clock source must be enableable
    { T::enable() } -> std::same_as<Result<void>>;
    { T::is_ready() } -> std::same_as<bool>;
};
```

**Usage:**
```cpp
template <ClockSource Source>
Result<void> configure_clock() {
    auto result = Source::enable();
    if (!result.is_ok()) {
        return result;
    }

    // Wait for clock to stabilize
    while (!Source::is_ready()) {
        // Busy wait
    }

    return Result<void>::ok();
}
```

---

## Concept Composition

Concepts can be combined to create more specific requirements:

```cpp
// Concept for output-capable GPIO
template <typename T>
concept OutputPin = GpioPin<T> && requires(T) {
    // Additional requirements for output
    { T::set_output_speed(uint8_t{}) } -> std::same_as<void>;
};

// Concept for interrupt-capable GPIO
template <typename T>
concept InterruptPin = GpioPin<T> && requires(T) {
    { T::enable_interrupt() } -> std::same_as<void>;
    { T::disable_interrupt() } -> std::same_as<void>;
};

// Usage
template <OutputPin Pin>
void fast_toggle() {
    Pin::set_output_speed(3);  // Maximum speed
    Pin::toggle();
}
```

---

## Concept-Based Function Overloading

Use concepts for cleaner overload resolution:

```cpp
// Traditional SFINAE (C++17)
template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void configure(T value) {
    // Handle integer values
}

// Modern concepts (C++20)
template <std::integral T>
void configure(T value) {
    // Handle integer values - much clearer!
}

// Overload for GPIO pins
template <GpioPin Pin>
void configure(Pin pin) {
    Pin::configure_output();
}
```

---

## Compile-Time Validation

Concepts provide **clear error messages**:

### Good Error (With Concepts)

```cpp
template <GpioPin Pin>
void blink() { /* ... */ }

struct NotAPin {};
blink<NotAPin>();
```

Compiler error:
```
error: 'NotAPin' does not satisfy concept 'GpioPin'
  required expression 'T::configure_output()' would be ill-formed
  required expression 'T::set_high()' would be ill-formed
  required expression 'T::port' would be ill-formed
```

### Bad Error (Without Concepts)

```cpp
template <typename Pin>
void blink() {
    Pin::configure_output();  // Error happens here
}
```

Compiler error:
```
error: 'NotAPin' has no member named 'configure_output'
  in instantiation of function template specialization 'blink<NotAPin>'
  candidate template ignored: substitution failure [with T = NotAPin]
  ... 50 lines of template error messages ...
```

---

## Best Practices

### 1. Use Concepts for Public APIs

```cpp
// ✅ Good - Clear interface
template <GpioPin Pin>
void initialize_led() {
    Pin::configure_output();
}

// ❌ Bad - Unclear requirements
template <typename Pin>
void initialize_led() {
    Pin::configure_output();
}
```

### 2. Combine with auto

```cpp
// Concepts work with auto
GpioPin auto led = GpioPin<peripherals::GPIOA, 5>{};
```

### 3. Use Subsumption

More specific concepts automatically satisfy more general ones:

```cpp
template <GpioPin Pin>
void configure(Pin pin) {
    // General implementation
}

template <OutputPin Pin>  // More specific
void configure(Pin pin) {
    // Optimized for output pins
    Pin::set_output_speed(3);
}
```

### 4. Document Concept Requirements

Always document what a concept requires in comments:

```cpp
/**
 * @concept UartPeripheral
 * @brief Defines requirements for UART peripherals
 *
 * Required operations:
 * - initialize() -> Result<void>
 * - write_byte(uint8_t) -> void
 * - read_byte() -> uint8_t
 * - available() -> bool
 *
 * @see SimpleUartConfigTxOnly
 * @see SimpleUartConfigBidirectional
 */
template <typename T>
concept UartPeripheral = /* ... */;
```

---

## Testing Concepts

Verify types satisfy concepts at compile-time:

```cpp
// Static assertion - fails at compile time if false
static_assert(GpioPin<GpioHardwarePolicy<GPIOA, 5>>);
static_assert(SystemClock<Clock>);
static_assert(UartPeripheral<SimpleUartConfigTxOnly<TxPin, Policy>>);

// Negative tests
static_assert(!GpioPin<int>);  // int is not a GpioPin
```

---

## Common Patterns

### Pattern 1: Concept + CRTP

```cpp
template <HardwarePolicy Policy>
class Peripheral {
    static void operation() {
        Policy::low_level_operation();
    }
};
```

### Pattern 2: Concept Forwarding

```cpp
template <GpioPin Pin>
class Led {
    template <GpioPin OtherPin>
    static void sync_with(OtherPin& other) {
        if (Pin::read() != OtherPin::read()) {
            OtherPin::toggle();
        }
    }
};
```

### Pattern 3: Concept Constraints on Member Functions

```cpp
class Device {
    template <GpioPin Pin>
    void attach_led(Pin pin) {
        // Only accepts types satisfying GpioPin
    }
};
```

---

## Performance

Concepts have **zero runtime cost**:

- Evaluated at **compile-time**
- No code generation
- Same assembly as unconstrained templates
- Better optimization (compiler knows constraints)

---

## Migration from C++17

### Before (SFINAE)

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
void process(T value) { /* ... */ }
```

### After (Concepts)

```cpp
template <std::integral T>
void process(T value) { /* ... */ }
```

---

## See Also

- [C++20 Concepts (cppreference)](https://en.cppreference.com/w/cpp/language/constraints)
- @ref getting_started - Using concepts in practice
- @ref adding_board - Creating concept-satisfying types
- Source code: `src/hal/concepts/`

---

**Next**: @ref porting_platform - Implementing concepts for new platforms
