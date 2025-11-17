# Alloy HAL API Reference

**Version**: 1.0.0
**Date**: 2025-01-17
**Status**: Complete

---

## Table of Contents

1. [Overview](#overview)
2. [Core Types](#core-types)
3. [Result Type](#result-type)
4. [GPIO API](#gpio-api)
5. [Clock API](#clock-api)
6. [UART API](#uart-api)
7. [SPI API](#spi-api)
8. [I2C API](#i2c-api)
9. [Board Interface](#board-interface)
10. [Concept Requirements](#concept-requirements)
11. [Error Handling](#error-handling)
12. [Platform-Specific APIs](#platform-specific-apis)

---

## Overview

The Alloy Hardware Abstraction Layer (HAL) provides a modern C++20 interface for embedded systems programming with:

- **Zero-cost abstractions**: Template-based design with no runtime overhead
- **Type safety**: Compile-time validation using C++20 concepts
- **Error handling**: `Result<T, E>` type for robust error management (no exceptions)
- **Cross-platform**: Unified API across STM32, SAME70, and other platforms

### Design Principles

1. **Policy-based design**: Configuration via template parameters
2. **Compile-time resolution**: All addresses and masks computed at compile-time
3. **No virtual functions**: Zero vtable overhead
4. **Explicit error handling**: All operations return `Result<T, ErrorCode>`

---

## Core Types

### Fundamental Types

```cpp
namespace alloy::core {
    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    using i8  = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
}
```

### HAL Configuration Types

```cpp
namespace alloy::hal {
    // GPIO direction
    enum class PinDirection {
        Input,
        Output
    };

    // GPIO pull resistor configuration
    enum class PinPull {
        None,
        PullUp,
        PullDown  // Platform-dependent
    };

    // GPIO drive mode
    enum class PinDrive {
        PushPull,
        OpenDrain
    };

    // GPIO speed (platform-specific)
    enum class PinSpeed {
        Low,
        Medium,
        High,
        VeryHigh
    };
}
```

---

## Result Type

The `Result<T, E>` type provides Rust-inspired error handling without exceptions.

### Type Definition

```cpp
namespace alloy::core {
    template <typename T, typename E>
    class Result {
    public:
        // Constructors
        static Result Ok(T value);
        static Result Err(E error);

        // State checking
        bool is_ok() const;
        bool is_err() const;
        explicit operator bool() const;  // Returns is_ok()

        // Value access
        T& value();
        const T& value() const;
        E& error();
        const E& error() const;

        // Functional operations
        T unwrap();  // Panics if error
        T unwrap_or(T default_value);
        template<typename F> auto map(F func) -> Result<U, E>;
        template<typename F> auto and_then(F func) -> Result<U, E>;
    };

    // Convenience functions
    template<typename T, typename E>
    Result<T, E> Ok(T value);

    template<typename T, typename E>
    Result<T, E> Err(E error);
}
```

### Usage Examples

```cpp
#include "core/result.hpp"
#include "core/error_code.hpp"

using namespace alloy::core;

// Creating results
Result<int, ErrorCode> success = Ok(42);
Result<int, ErrorCode> failure = Err(ErrorCode::InvalidParameter);

// Checking state
if (success.is_ok()) {
    int value = success.value();  // Access value
}

// Pattern matching style
auto result = some_operation();
if (result) {  // Implicit bool conversion
    process(result.value());
} else {
    handle_error(result.error());
}

// Unwrapping (use with caution - panics on error)
int value = success.unwrap();

// Unwrapping with default
int value = result.unwrap_or(0);

// Chaining operations
auto final_result = operation1()
    .and_then([](auto val) { return operation2(val); })
    .and_then([](auto val) { return operation3(val); });
```

### Error Codes

```cpp
enum class ErrorCode {
    Success = 0,
    InvalidParameter,
    NotInitialized,
    HardwareError,
    Timeout,
    NotSupported,
    Busy,
    OutOfRange,
    // ... platform-specific codes
};
```

---

## GPIO API

GPIO pins are template-based with compile-time port/pin resolution.

### Class Definition

```cpp
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Configuration
    Result<void, ErrorCode> setDirection(PinDirection direction);
    Result<void, ErrorCode> setDrive(PinDrive drive);
    Result<void, ErrorCode> setPull(PinPull pull);
    Result<void, ErrorCode> setSpeed(PinSpeed speed);  // STM32 only

    // State manipulation
    Result<void, ErrorCode> set();      // Set HIGH
    Result<void, ErrorCode> clear();    // Set LOW
    Result<void, ErrorCode> toggle();   // Toggle state
    Result<void, ErrorCode> write(bool value);

    // State reading
    Result<bool, ErrorCode> read() const;
    Result<bool, ErrorCode> isOutput() const;
};
```

### STM32 GPIO Example

```cpp
#include "hal/vendors/st/stm32f4/gpio.hpp"

using namespace alloy::hal::stm32f4;

// Define LED pin (GPIOA pin 5)
using LedPin = GpioPin<GPIOA_BASE, 5>;

void setup_led() {
    auto led = LedPin{};

    // Configure as output
    auto result = led.setDirection(PinDirection::Output);
    if (!result) {
        // Handle error
        return;
    }

    // Set drive mode
    led.setDrive(PinDrive::PushPull);
    led.setSpeed(PinSpeed::Medium);
    led.setPull(PinPull::None);
}

void blink_led() {
    auto led = LedPin{};

    led.set();     // Turn on
    delay(500);
    led.clear();   // Turn off
    delay(500);
    led.toggle();  // Toggle
}

void read_button() {
    // Define button pin (GPIOC pin 13)
    using ButtonPin = GpioPin<GPIOC_BASE, 13>;

    auto button = ButtonPin{};
    button.setDirection(PinDirection::Input);
    button.setPull(PinPull::PullUp);

    auto state = button.read();
    if (state.is_ok()) {
        if (state.value()) {
            // Button pressed
        }
    }
}
```

### SAME70 GPIO Example

```cpp
#include "hal/vendors/arm/same70/gpio.hpp"

using namespace alloy::hal::same70;

// Define LED pin (PIOC pin 8)
using LedGreen = GpioPin<PIOC_BASE, 8>;

void setup() {
    auto led = LedGreen{};

    led.setDirection(PinDirection::Output);
    led.setDrive(PinDrive::PushPull);
    // Note: SAME70 only supports PullUp, not PullDown
    led.setPull(PinPull::PullUp);
}
```

### GPIO Concept Requirements

For a type to satisfy the `GpioPin` concept:

```cpp
concept GpioPin = requires(T pin, const T const_pin) {
    // State manipulation
    { pin.set() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.clear() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.toggle() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.write(bool{}) } -> std::same_as<Result<void, ErrorCode>>;

    // State reading
    { const_pin.read() } -> std::same_as<Result<bool, ErrorCode>>;
    { const_pin.isOutput() } -> std::same_as<Result<bool, ErrorCode>>;

    // Configuration
    { pin.setDirection(PinDirection{}) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setDrive(PinDrive{}) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setPull(PinPull{}) } -> std::same_as<Result<void, ErrorCode>>;

    // Metadata
    requires requires { T::port_base; };
    requires requires { T::pin_number; };
};
```

---

## Clock API

System clock configuration and peripheral clock management.

### STM32F4 Clock API

```cpp
class Stm32f4Clock {
public:
    // System initialization
    static Result<void, ErrorCode> initialize();
    static Result<void, ErrorCode> initialize(const ClockConfig& config);

    // Peripheral clock enable
    static Result<void, ErrorCode> enable_gpio_clocks();
    static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_id);
    static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_id);
    static Result<void, ErrorCode> enable_i2c_clock(uint32_t i2c_id);

    // Query functions
    static uint32_t getSystemClockFrequency();
    static uint32_t getAHBFrequency();
    static uint32_t getAPB1Frequency();
    static uint32_t getAPB2Frequency();
    static bool isInitialized();
};
```

### Clock Configuration

```cpp
struct ClockConfig {
    ClockSource source;          // HSI, HSE, or PLL
    uint32_t crystal_freq_hz;    // External crystal frequency
    PllConfig pll;               // PLL multiplier/divider
    AhbPrescaler ahb_prescaler;  // AHB bus divider
    ApbPrescaler apb1_prescaler; // APB1 bus divider
    ApbPrescaler apb2_prescaler; // APB2 bus divider
};

// Predefined configurations
constexpr ClockConfig CLOCK_CONFIG_84MHZ;   // STM32F4 max
constexpr ClockConfig CLOCK_CONFIG_168MHZ;  // STM32F4 high performance
constexpr ClockConfig CLOCK_CONFIG_16MHZ;   // HSI only (safe mode)
```

### STM32 Clock Example

```cpp
#include "hal/vendors/st/stm32f4/clock_platform.hpp"

using namespace alloy::hal::stm32f4;

void setup_clocks() {
    // Use high-performance preset (168 MHz)
    auto result = Stm32f4Clock::initialize(CLOCK_CONFIG_168MHZ);
    if (!result) {
        // Fall back to HSI
        Stm32f4Clock::initialize(CLOCK_CONFIG_16MHZ);
    }

    // Enable peripheral clocks
    Stm32f4Clock::enable_gpio_clocks();      // Enable all GPIO ports
    Stm32f4Clock::enable_uart_clock(1);      // Enable USART1
    Stm32f4Clock::enable_spi_clock(1);       // Enable SPI1

    // Query frequencies
    uint32_t sysclk = Stm32f4Clock::getSystemClockFrequency();
    uint32_t ahb = Stm32f4Clock::getAHBFrequency();
}
```

### SAME70 Clock API

```cpp
class Clock {
public:
    // System initialization
    static Result<void, ErrorCode> initialize(const ClockConfig& config);

    // Peripheral clock enable
    static Result<void, ErrorCode> enable_gpio_clocks();
    static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_id);
    static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_id);
    static Result<void, ErrorCode> enable_i2c_clock(uint32_t i2c_id);

    // Low-level peripheral enable
    static Result<void, ErrorCode> enablePeripheralClock(uint8_t peripheral_id);
    static Result<void, ErrorCode> disablePeripheralClock(uint8_t peripheral_id);

    // Query functions
    static uint32_t getMasterClockFrequency();
    static bool isInitialized();
    static const ClockConfig& getConfig();
};
```

### SAME70 Clock Example

```cpp
#include "hal/vendors/arm/same70/clock.hpp"

using namespace alloy::hal::same70;

void setup_clocks() {
    // Use 150 MHz configuration (12 MHz crystal -> 300 MHz PLL -> 150 MHz MCK)
    auto result = Clock::initialize(CLOCK_CONFIG_150MHZ);
    if (!result) {
        // Fall back to safe 12 MHz RC
        Clock::initialize(CLOCK_CONFIG_12MHZ_RC);
    }

    // Enable peripheral clocks
    Clock::enable_gpio_clocks();   // Enable PIOA-PIOE
    Clock::enable_uart_clock(0);   // Enable UART0
    Clock::enable_uart_clock(100); // Enable USART0
    Clock::enable_spi_clock(0);    // Enable SPI0
    Clock::enable_i2c_clock(0);    // Enable TWIHS0 (I2C)
}
```

### Clock Concept Requirements

```cpp
concept ClockPlatform = requires {
    // System initialization
    { T::initialize() } -> std::same_as<Result<void, ErrorCode>>;

    // Peripheral clock enable
    { T::enable_gpio_clocks() } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_uart_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_spi_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_i2c_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
};
```

---

## UART API

Universal Asynchronous Receiver/Transmitter communication.

### UART Class

```cpp
template <typename HardwarePolicy>
class Uart {
public:
    // Configuration
    struct Config {
        uint32_t baudrate;
        WordLength word_length;
        StopBits stop_bits;
        Parity parity;
        FlowControl flow_control;
    };

    // Initialization
    static Result<void, ErrorCode> initialize(const Config& config);

    // Transmit
    static Result<void, ErrorCode> write(uint8_t data);
    static Result<void, ErrorCode> write(const uint8_t* data, size_t length);
    static Result<void, ErrorCode> write_string(const char* str);

    // Receive
    static Result<uint8_t, ErrorCode> read();
    static Result<size_t, ErrorCode> read(uint8_t* buffer, size_t max_length);

    // Status
    static bool is_tx_ready();
    static bool is_rx_ready();
    static bool has_error();
};
```

### UART Configuration Types

```cpp
enum class WordLength { Bits7, Bits8, Bits9 };
enum class StopBits { Bits1, Bits1_5, Bits2 };
enum class Parity { None, Even, Odd };
enum class FlowControl { None, RTS, CTS, RTS_CTS };
```

### UART Example

```cpp
#include "hal/uart.hpp"
#include "hal/vendors/st/stm32f4/uart_hardware_policy.hpp"

using namespace alloy::hal;
using namespace alloy::hal::stm32f4;

// Define UART with STM32F4 policy
using Console = Uart<Stm32f4UartHardwarePolicy<USART2_BASE>>;

void setup_console() {
    Console::Config config = {
        .baudrate = 115200,
        .word_length = WordLength::Bits8,
        .stop_bits = StopBits::Bits1,
        .parity = Parity::None,
        .flow_control = FlowControl::None
    };

    auto result = Console::initialize(config);
    if (!result) {
        // Handle initialization error
    }
}

void send_message() {
    Console::write_string("Hello, World!\r\n");
}

void read_data() {
    uint8_t buffer[64];
    auto result = Console::read(buffer, sizeof(buffer));
    if (result.is_ok()) {
        size_t bytes_read = result.value();
        // Process buffer
    }
}

void echo_console() {
    while (true) {
        if (Console::is_rx_ready()) {
            auto byte = Console::read();
            if (byte.is_ok()) {
                Console::write(byte.value());
            }
        }
    }
}
```

---

## SPI API

Serial Peripheral Interface communication.

### SPI Class

```cpp
template <typename HardwarePolicy>
class Spi {
public:
    struct Config {
        SpiMode mode;           // CPOL/CPHA
        uint32_t frequency_hz;
        BitOrder bit_order;
        DataSize data_size;
    };

    // Initialization
    static Result<void, ErrorCode> initialize(const Config& config);

    // Transfer
    static Result<uint8_t, ErrorCode> transfer(uint8_t data);
    static Result<void, ErrorCode> transfer(const uint8_t* tx_data,
                                           uint8_t* rx_data,
                                           size_t length);

    // Transmit only
    static Result<void, ErrorCode> write(uint8_t data);
    static Result<void, ErrorCode> write(const uint8_t* data, size_t length);

    // Receive only
    static Result<uint8_t, ErrorCode> read();
    static Result<void, ErrorCode> read(uint8_t* buffer, size_t length);

    // Status
    static bool is_busy();
};
```

### SPI Configuration Types

```cpp
enum class SpiMode {
    Mode0,  // CPOL=0, CPHA=0
    Mode1,  // CPOL=0, CPHA=1
    Mode2,  // CPOL=1, CPHA=0
    Mode3   // CPOL=1, CPHA=1
};

enum class BitOrder { MSB_First, LSB_First };
enum class DataSize { Bits8, Bits16 };
```

### SPI Example

```cpp
#include "hal/spi.hpp"
#include "hal/vendors/st/stm32f4/spi_hardware_policy.hpp"

using namespace alloy::hal;
using namespace alloy::hal::stm32f4;

// Define SPI1
using Spi1 = Spi<Stm32f4SpiHardwarePolicy<SPI1_BASE>>;

void setup_spi() {
    Spi1::Config config = {
        .mode = SpiMode::Mode0,
        .frequency_hz = 1000000,  // 1 MHz
        .bit_order = BitOrder::MSB_First,
        .data_size = DataSize::Bits8
    };

    Spi1::initialize(config);
}

uint8_t read_register(uint8_t address) {
    // Select device (chip select LOW)
    cs_pin.clear();

    // Send address
    Spi1::write(address | 0x80);  // Read bit

    // Read data
    auto result = Spi1::read();

    // Deselect device
    cs_pin.set();

    return result.unwrap_or(0);
}

void write_register(uint8_t address, uint8_t value) {
    cs_pin.clear();
    Spi1::write(address & 0x7F);  // Write bit
    Spi1::write(value);
    cs_pin.set();
}
```

---

## I2C API

Inter-Integrated Circuit communication.

### I2C Class

```cpp
template <typename HardwarePolicy>
class I2c {
public:
    struct Config {
        uint32_t frequency_hz;
        AddressingMode addressing_mode;
        bool enable_general_call;
    };

    // Initialization
    static Result<void, ErrorCode> initialize(const Config& config);

    // Master transmit
    static Result<void, ErrorCode> write(uint8_t device_address,
                                        const uint8_t* data,
                                        size_t length);

    // Master receive
    static Result<void, ErrorCode> read(uint8_t device_address,
                                       uint8_t* buffer,
                                       size_t length);

    // Write then read (common pattern)
    static Result<void, ErrorCode> write_read(uint8_t device_address,
                                             const uint8_t* tx_data,
                                             size_t tx_length,
                                             uint8_t* rx_buffer,
                                             size_t rx_length);

    // Status
    static bool is_busy();
    static bool has_error();
};
```

### I2C Configuration Types

```cpp
enum class AddressingMode {
    SevenBit,   // 7-bit addressing
    TenBit      // 10-bit addressing
};

// Standard speeds
constexpr uint32_t I2C_STANDARD_MODE = 100000;   // 100 kHz
constexpr uint32_t I2C_FAST_MODE = 400000;       // 400 kHz
constexpr uint32_t I2C_FAST_MODE_PLUS = 1000000; // 1 MHz
```

### I2C Example

```cpp
#include "hal/i2c.hpp"
#include "hal/vendors/st/stm32f4/i2c_hardware_policy.hpp"

using namespace alloy::hal;
using namespace alloy::hal::stm32f4;

// Define I2C1
using I2c1 = I2c<Stm32f4I2cHardwarePolicy<I2C1_BASE>>;

constexpr uint8_t SENSOR_ADDRESS = 0x68;

void setup_i2c() {
    I2c1::Config config = {
        .frequency_hz = I2C_FAST_MODE,  // 400 kHz
        .addressing_mode = AddressingMode::SevenBit,
        .enable_general_call = false
    };

    I2c1::initialize(config);
}

uint8_t read_sensor_register(uint8_t reg_addr) {
    uint8_t data;

    auto result = I2c1::write_read(
        SENSOR_ADDRESS,
        &reg_addr, 1,
        &data, 1
    );

    if (result.is_ok()) {
        return data;
    }
    return 0;
}

void write_sensor_register(uint8_t reg_addr, uint8_t value) {
    uint8_t data[2] = { reg_addr, value };
    I2c1::write(SENSOR_ADDRESS, data, 2);
}

void read_sensor_burst(uint8_t start_reg, uint8_t* buffer, size_t length) {
    I2c1::write_read(
        SENSOR_ADDRESS,
        &start_reg, 1,
        buffer, length
    );
}
```

---

## Board Interface

Each board provides a standardized interface via `board_config.hpp` and `board.hpp`.

### Board Configuration Structure

```cpp
// boards/{board_name}/board_config.hpp

namespace board {

// Type aliases to platform types
using ClockPlatform = alloy::hal::stm32f4::Stm32f4Clock;
using ClockConfig = alloy::hal::stm32f4::ClockConfig;

// Clock configuration
struct BoardClockConfig {
    static constexpr ClockConfig config = CLOCK_CONFIG_84MHZ;
};

// LED configuration
struct LedConfig {
    using led_green = alloy::hal::stm32f4::GpioPin<GPIOA_BASE, 5>;
    static constexpr bool led_green_active_high = true;
};

// Button configuration
struct ButtonConfig {
    using button0 = alloy::hal::stm32f4::GpioPin<GPIOC_BASE, 13>;
    static constexpr bool button0_active_low = true;
};

// UART console configuration
struct UartConsoleConfig {
    using tx_pin = alloy::hal::stm32f4::GpioPin<GPIOA_BASE, 2>;
    using rx_pin = alloy::hal::stm32f4::GpioPin<GPIOA_BASE, 3>;
    static constexpr uint32_t baudrate = 115200;
    static constexpr uint32_t uart_base = USART2_BASE;
};

} // namespace board
```

### Board API

```cpp
// boards/{board_name}/board.hpp

namespace board {

class Board {
public:
    // Initialization
    static Result<void, ErrorCode> initialize();

    // LED control
    static void led_on();
    static void led_off();
    static void led_toggle();

    // Button
    static bool button_pressed();

    // Console
    static void console_write(const char* str);
    static void console_write(const uint8_t* data, size_t length);
};

// SysTick for delays
class BoardSysTick {
public:
    static void init();
    static void delay_ms(uint32_t ms);
    static uint32_t get_tick();
    static void increment_tick();  // Called from SysTick_Handler
};

} // namespace board
```

### Board Usage Example

```cpp
#include "boards/nucleo_f401re/board.hpp"

int main() {
    // Initialize board
    auto result = board::Board::initialize();
    if (!result) {
        // Handle error
        while(1);
    }

    // Initialize timing
    board::BoardSysTick::init();

    // Blink LED
    while (true) {
        board::Board::led_toggle();
        board::BoardSysTick::delay_ms(500);

        // Check button
        if (board::Board::button_pressed()) {
            board::Board::console_write("Button pressed!\r\n");
        }
    }
}
```

### Cross-Platform Application

```cpp
// Works on ANY board that implements the board interface!

#include "board.hpp"  // Automatically selected based on ALLOY_BOARD

void blink_forever() {
    board::Board::initialize().unwrap();
    board::BoardSysTick::init();

    while (true) {
        board::Board::led_toggle();
        board::BoardSysTick::delay_ms(1000);
    }
}
```

---

## Concept Requirements

C++20 concepts provide compile-time validation of API compliance.

### ClockPlatform Concept

```cpp
namespace alloy::hal::concepts {

template <typename T>
concept ClockPlatform = requires {
    // System initialization
    { T::initialize() } -> std::same_as<Result<void, ErrorCode>>;

    // Peripheral clock enable functions
    { T::enable_gpio_clocks() } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_uart_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_spi_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_i2c_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
};

} // namespace alloy::hal::concepts
```

**Purpose**: Ensures all clock implementations provide consistent peripheral enable API.

**Platforms**: STM32F4, STM32F7, STM32G0, SAME70 all satisfy this concept.

### GpioPin Concept

```cpp
template <typename T>
concept GpioPin = requires(T pin, const T const_pin,
                           PinDirection direction, PinDrive drive,
                           PinPull pull, bool value) {
    // State manipulation methods
    { pin.set() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.clear() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.toggle() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.write(value) } -> std::same_as<Result<void, ErrorCode>>;

    // State reading
    { const_pin.read() } -> std::same_as<Result<bool, ErrorCode>>;
    { const_pin.isOutput() } -> std::same_as<Result<bool, ErrorCode>>;

    // Configuration methods
    { pin.setDirection(direction) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setDrive(drive) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setPull(pull) } -> std::same_as<Result<void, ErrorCode>>;

    // Compile-time metadata
    requires requires { T::port_base; };
    requires requires { T::pin_number; };
};
```

**Purpose**: Ensures GPIO pin types provide complete interface for digital I/O.

**Platforms**: STM32F4, STM32F7, STM32G0, SAME70 all satisfy this concept.

### Using Concepts for Validation

```cpp
#include "hal/core/concepts.hpp"

// Static assertion in platform code
template <uint32_t BASE, uint8_t PIN>
class GpioPin {
    // ... implementation ...
};

// Validate at compile time
static_assert(alloy::hal::concepts::GpioPin<GpioPin<GPIOA_BASE, 5>>,
              "GpioPin must satisfy GpioPin concept");

// Generic function using concept
template <alloy::hal::concepts::GpioPin Pin>
void blink(Pin& pin, uint32_t delay_ms) {
    pin.set();
    delay(delay_ms);
    pin.clear();
    delay(delay_ms);
}
```

---

## Error Handling

Alloy uses explicit error handling via the `Result<T, E>` type - **no exceptions**.

### Error Code Enumeration

```cpp
enum class ErrorCode {
    Success = 0,

    // Parameter errors
    InvalidParameter,
    OutOfRange,

    // State errors
    NotInitialized,
    AlreadyInitialized,
    Busy,

    // Hardware errors
    HardwareError,
    Timeout,
    NotSupported,

    // Communication errors
    AckFailure,
    BusError,
    ArbitrationLost,

    // Platform-specific errors
    // (defined in platform headers)
};
```

### Error Handling Patterns

#### Pattern 1: Check and Handle

```cpp
auto result = gpio_pin.set();
if (!result) {
    // Handle error
    ErrorCode error = result.error();
    switch (error) {
        case ErrorCode::NotInitialized:
            initialize_gpio();
            break;
        case ErrorCode::HardwareError:
            reset_hardware();
            break;
        default:
            log_error(error);
    }
}
```

#### Pattern 2: Unwrap or Default

```cpp
// Get value or use default if error
uint8_t data = uart.read().unwrap_or(0x00);

// Chain with default
bool button_state = button.read()
    .unwrap_or(false);
```

#### Pattern 3: Propagate Errors

```cpp
Result<void, ErrorCode> initialize_system() {
    auto clock_result = Clock::initialize();
    if (!clock_result) {
        return Err(clock_result.error());
    }

    auto gpio_result = setup_gpio();
    if (!gpio_result) {
        return Err(gpio_result.error());
    }

    return Ok();
}
```

#### Pattern 4: Functional Chaining

```cpp
auto process_sensor_data() -> Result<int, ErrorCode> {
    return i2c.read(SENSOR_ADDR)
        .and_then([](uint8_t raw) {
            return validate_data(raw);
        })
        .and_then([](uint8_t valid) {
            return convert_to_celsius(valid);
        })
        .map([](int celsius) {
            return celsius * 9 / 5 + 32;  // Convert to Fahrenheit
        });
}
```

---

## Platform-Specific APIs

### STM32F4 Platform

```cpp
namespace alloy::hal::stm32f4 {

// GPIO
using GpioPin<PORT_BASE, PIN_NUM>;

// Clock
class Stm32f4Clock;

// Peripherals
constexpr uintptr_t GPIOA_BASE = 0x40020000;
constexpr uintptr_t GPIOB_BASE = 0x40020400;
constexpr uintptr_t GPIOC_BASE = 0x40020800;
// ... more peripherals

} // namespace alloy::hal::stm32f4
```

**Documentation**: See `docs/platforms/STM32F4.md`

### STM32F7 Platform

```cpp
namespace alloy::hal::stm32f7 {

// GPIO
using GpioPin<PORT_BASE, PIN_NUM>;

// Clock
class Stm32f7Clock;

// Peripherals (similar to F4)

} // namespace alloy::hal::stm32f7
```

**Documentation**: See `docs/platforms/STM32F7.md`

### STM32G0 Platform

```cpp
namespace alloy::hal::stm32g0 {

// GPIO
using GpioPin<PORT_BASE, PIN_NUM>;

// Clock
class Stm32g0Clock;

} // namespace alloy::hal::stm32g0
```

**Documentation**: See `docs/platforms/STM32G0.md`

### SAME70 Platform (Atmel/Microchip)

```cpp
namespace alloy::hal::same70 {

// GPIO (uses PIO - Parallel I/O)
using GpioPin<PORT_BASE, PIN_NUM>;

// Clock (PMC - Power Management Controller)
class Clock;

// Peripherals
constexpr uintptr_t PIOA_BASE = 0x400E0E00;
constexpr uintptr_t PIOB_BASE = 0x400E1000;
constexpr uintptr_t PIOC_BASE = 0x400E1200;
// ... more peripherals

// Peripheral IDs for clock enable
namespace id {
    constexpr uint8_t PIOA = 10;
    constexpr uint8_t UART0 = 7;
    constexpr uint8_t TWIHS0 = 19;  // I2C
    // ... more IDs
}

} // namespace alloy::hal::same70
```

**Documentation**: See `docs/platforms/SAME70.md`

---

## Best Practices

### 1. Always Check Result Values

```cpp
// BAD - ignoring errors
gpio_pin.set();

// GOOD - explicit error handling
auto result = gpio_pin.set();
if (!result) {
    handle_error(result.error());
}
```

### 2. Use Type Aliases for Pins

```cpp
// Define once
using LedRed = GpioPin<GPIOA_BASE, 5>;
using LedGreen = GpioPin<GPIOA_BASE, 6>;

// Use throughout code
void init() {
    auto led_red = LedRed{};
    auto led_green = LedGreen{};

    led_red.setDirection(PinDirection::Output);
    led_green.setDirection(PinDirection::Output);
}
```

### 3. Leverage Board Abstraction

```cpp
// Write portable code using board interface
#include "board.hpp"

void application_main() {
    board::Board::initialize().unwrap();

    while (true) {
        board::Board::led_toggle();
        board::BoardSysTick::delay_ms(100);
    }
}
```

### 4. Use Concepts for Generic Code

```cpp
template <alloy::hal::concepts::GpioPin Pin>
class LedController {
public:
    void blink(uint32_t period_ms) {
        pin_.set();
        delay(period_ms / 2);
        pin_.clear();
        delay(period_ms / 2);
    }

private:
    Pin pin_;
};
```

### 5. Initialize in Correct Order

```cpp
int main() {
    // 1. Clock first
    Clock::initialize().unwrap();

    // 2. Enable peripheral clocks
    Clock::enable_gpio_clocks().unwrap();
    Clock::enable_uart_clock(1).unwrap();

    // 3. Configure peripherals
    setup_gpio();
    setup_uart();

    // 4. Enable interrupts (if needed)
    enable_interrupts();

    // 5. Start application
    application_main();
}
```

---

## Quick Reference

### Common Operations

| Operation | Code |
|-----------|------|
| Set GPIO high | `pin.set()` |
| Set GPIO low | `pin.clear()` |
| Toggle GPIO | `pin.toggle()` |
| Read GPIO | `bool val = pin.read().unwrap()` |
| Configure as output | `pin.setDirection(PinDirection::Output)` |
| Send UART byte | `uart.write(0x42)` |
| Read UART byte | `uint8_t data = uart.read().unwrap()` |
| I2C write | `i2c.write(addr, data, len)` |
| SPI transfer | `uint8_t rx = spi.transfer(tx).unwrap()` |
| Delay | `board::BoardSysTick::delay_ms(100)` |

### Platform Namespaces

| Platform | Namespace |
|----------|-----------|
| STM32F4 | `alloy::hal::stm32f4` |
| STM32F7 | `alloy::hal::stm32f7` |
| STM32G0 | `alloy::hal::stm32g0` |
| SAME70 | `alloy::hal::same70` |

### Supported Boards

| Board | Platform | MCU |
|-------|----------|-----|
| nucleo_f401re | STM32F4 | STM32F401RE |
| nucleo_f722ze | STM32F7 | STM32F722ZE |
| nucleo_g071rb | STM32G0 | STM32G071RB |
| nucleo_g0b1re | STM32G0 | STM32G0B1RE |
| same70_xplained | SAME70 | ATSAME70Q21B |

---

## See Also

- [Architecture Documentation](ARCHITECTURE.md)
- [Board Porting Guide](PORTING_NEW_BOARD.md)
- [Platform Porting Guide](PORTING_NEW_PLATFORM.md)
- [Code Generation Guide](CODE_GENERATION.md)
- [Building Guide](BUILDING.md)
- [Testing Guide](TESTING.md)

---

**© 2025 Alloy HAL Project**
**License**: MIT
