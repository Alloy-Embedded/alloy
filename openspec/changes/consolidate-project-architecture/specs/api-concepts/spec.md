# API Standardization with C++20 Concepts Specification

## ADDED Requirements

### Requirement: Platform API Concepts

All platform APIs SHALL be validated using C++20 concepts to ensure consistent interfaces across MCU families.

**Rationale**: Enables compile-time validation that platforms implement required APIs, preventing runtime errors and enabling generic programming.

#### Scenario: ClockPlatform concept defines interface

- **GIVEN** platform provides clock configuration
- **WHEN** ClockPlatform concept is checked
- **THEN** platform SHALL provide `system_clock_hz` constant
- **AND** platform SHALL provide `init()` function

```cpp
// src/hal/core/concepts.hpp

template <typename T>
concept ClockPlatform = requires {
    // Must have compile-time frequency constant
    { T::system_clock_hz } -> std::convertible_to<u32>;

    // Must have initialization function
    { T::init() } -> std::same_as<void>;
};

// Usage: Validate platform implementations
namespace stm32f4 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static constexpr u32 system_clock_hz = CLOCK_HZ;
        static void init() { /* ... */ }
    };
}

static_assert(ClockPlatform<stm32f4::Clock<84'000'000>>,
              "STM32F4 Clock must satisfy ClockPlatform concept");
```

#### Scenario: GpioPlatform concept defines interface

- **GIVEN** platform provides GPIO functionality
- **WHEN** GpioPlatform concept is checked
- **THEN** platform SHALL provide configure, set, clear, read methods

```cpp
template <typename T>
concept GpioPlatform = requires {
    // Port and pin configuration
    { T::template configure_output<GPIOA, 5>(OutputMode::PushPull, Speed::Low) };
    { T::template configure_input<GPIOA, 5>(InputMode::PullUp) };

    // Basic operations
    { T::template set<GPIOA, 5>() };
    { T::template clear<GPIOA, 5>() };
    { T::template toggle<GPIOA, 5>() };
    { T::template read<GPIOA, 5>() } -> std::same_as<bool>;
};

// Validate implementations
static_assert(GpioPlatform<stm32f4::GPIO>, "STM32F4 GPIO must satisfy GpioPlatform");
static_assert(GpioPlatform<stm32f7::GPIO>, "STM32F7 GPIO must satisfy GpioPlatform");
static_assert(GpioPlatform<same70::GPIO>, "SAME70 GPIO must satisfy GpioPlatform");
```

#### Scenario: UartPlatform concept with Result<T,E>

- **GIVEN** platform provides UART functionality
- **WHEN** UartPlatform concept is checked
- **THEN** platform SHALL return Result<T, Error> for all operations

```cpp
template <typename T>
concept UartPlatform = requires(T uart, const u8* data, u8* buffer, usize len) {
    // Init returns Result<void, Error>
    { T::init(115200) } -> std::same_as<Result<void, Error>>;

    // Write returns Result<void, Error>
    { T::write(data, len) } -> std::same_as<Result<void, Error>>;

    // Read returns Result<usize, Error> (number of bytes read)
    { T::read(buffer, len, 1000) } -> std::same_as<Result<usize, Error>>;
};
```

---

### Requirement: Consistent Platform APIs

All platforms SHALL implement identical API signatures for equivalent functionality.

**Rationale**: Enables writing truly portable code that works across all platforms without modification.

#### Scenario: Clock API consistency

- **GIVEN** different MCU families (STM32F4, STM32F7, SAME70)
- **WHEN** Clock APIs are examined
- **THEN** all SHALL have same interface
- **AND** all SHALL use template parameter for frequency
- **AND** all SHALL provide `init()` method

```cpp
// Before consolidation - INCONSISTENT

// STM32F4 - no template, hardcoded frequency
namespace stm32f4 {
    struct Clock {
        static constexpr u32 system_clock_hz = 84000000;
        static void init();  // ✓
    };
}

// STM32F7 - has template, different method name
namespace stm32f7 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static constexpr u32 frequency = CLOCK_HZ;  // ✗ Different name!
        static void configure();  // ✗ Different method name!
    };
}

// After consolidation - CONSISTENT

// All platforms follow same pattern
template <u32 CLOCK_HZ>
struct Clock {
    static constexpr u32 system_clock_hz = CLOCK_HZ;
    static void init();
};

static_assert(ClockPlatform<stm32f4::Clock<84'000'000>>);
static_assert(ClockPlatform<stm32f7::Clock<216'000'000>>);
static_assert(ClockPlatform<same70::Clock<300'000'000>>);
```

#### Scenario: GPIO API consistency

- **GIVEN** GPIO implementations across platforms
- **WHEN** APIs are standardized
- **THEN** all SHALL use same method names and signatures

```cpp
// Standardized GPIO API (all platforms)

template <typename HardwarePolicy>
struct GPIO {
    template <u32 PORT, u8 PIN>
    static void configure_output(OutputMode mode, Speed speed) {
        HardwarePolicy::template configure_output<PORT, PIN>(mode, speed);
    }

    template <u32 PORT, u8 PIN>
    static void set() {
        HardwarePolicy::template set_pin<PORT, PIN>();
    }

    template <u32 PORT, u8 PIN>
    static void clear() {
        HardwarePolicy::template clear_pin<PORT, PIN>();
    }

    template <u32 PORT, u8 PIN>
    static bool read() {
        return HardwarePolicy::template read_pin<PORT, PIN>();
    }
};

// All platforms provide same interface
using F4_GPIO = stm32f4::GPIO;
using F7_GPIO = stm32f7::GPIO;
using SAME70_GPIO = same70::GPIO;

static_assert(GpioPlatform<F4_GPIO>);
static_assert(GpioPlatform<F7_GPIO>);
static_assert(GpioPlatform<SAME70_GPIO>);
```

---

### Requirement: Concept Validation in Board Code

Board code SHALL use static_assert to validate platform types satisfy required concepts.

**Rationale**: Catches configuration errors at compile-time with clear error messages.

#### Scenario: Board config validates platform types

- **GIVEN** board_config.hpp defines platform types
- **WHEN** file is compiled
- **THEN** static_assert SHALL validate each type satisfies its concept

```cpp
// boards/nucleo_f401re/board_config.hpp

namespace board {

using ClockPlatform = stm32f4::Clock<84'000'000>;
using GpioPlatform = stm32f4::GPIO;
using UartPlatform = stm32f4::UART;

// Validate all platform types
static_assert(ClockPlatform<ClockPlatform>,
              "ClockPlatform must satisfy ClockPlatform concept");

static_assert(GpioPlatform<GpioPlatform>,
              "GpioPlatform must satisfy GpioPlatform concept");

static_assert(UartPlatform<UartPlatform>,
              "UartPlatform must satisfy UartPlatform concept");

} // namespace board
```

#### Scenario: Concept violation shows clear error

- **GIVEN** platform type missing required method
- **WHEN** code is compiled
- **THEN** static_assert SHALL fail
- **AND** error SHALL indicate which requirement is violated

```cpp
// Intentionally broken Clock (missing init())
template <u32 CLOCK_HZ>
struct BrokenClock {
    static constexpr u32 system_clock_hz = CLOCK_HZ;
    // Missing: static void init();
};

static_assert(ClockPlatform<BrokenClock<84'000'000>>);
// Error: static assertion failed: ClockPlatform<BrokenClock<84'000'000>>
// Note: concept 'ClockPlatform' was not satisfied
// Note: required expression '{ T::init() } -> std::same_as<void>' is invalid
```

---

### Requirement: Generic Programming with Concepts

HAL APIs SHALL enable generic programming using concept-constrained templates.

**Rationale**: Allows writing hardware-agnostic code that works with any conforming platform.

#### Scenario: Generic GPIO blink function

- **GIVEN** generic function using GpioPlatform concept
- **WHEN** function is instantiated with any GPIO platform
- **THEN** code SHALL compile and work correctly

```cpp
// Generic blink function - works with ANY GpioPlatform
template <GpioPlatform GPIO, u32 PORT, u8 PIN>
void blink_led(u32 delay_ms) {
    GPIO::template configure_output<PORT, PIN>(OutputMode::PushPull, Speed::Low);

    while (true) {
        GPIO::template toggle<PORT, PIN>();
        delay_ms_function(delay_ms);
    }
}

// Works with ANY platform satisfying GpioPlatform
blink_led<stm32f4::GPIO, GPIOA, 5>(500);  // STM32F4
blink_led<stm32f7::GPIO, GPIOB, 0>(500);  // STM32F7
blink_led<same70::GPIO, PIOA, 10>(500);   // SAME70
```

#### Scenario: Generic UART logger

- **GIVEN** generic UART logging function
- **WHEN** instantiated with any UartPlatform
- **THEN** function SHALL work identically across platforms

```cpp
template <UartPlatform UART>
class Logger {
public:
    static Result<void, Error> log(const char* message) {
        return UART::write(reinterpret_cast<const u8*>(message),
                          strlen(message));
    }

    static Result<void, Error> log_hex(u32 value) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "0x%08X\n", value);
        return log(buffer);
    }
};

// Use with any UART platform
using F4_Logger = Logger<stm32f4::UART>;
using F7_Logger = Logger<stm32f7::UART>;
using SAME70_Logger = Logger<same70::UART>;

// All work identically
F4_Logger::log("Hello from STM32F4");
F7_Logger::log("Hello from STM32F7");
SAME70_Logger::log("Hello from SAME70");
```

---

## ADDED Requirements

### Requirement: Concept Documentation

All concepts SHALL be documented with requirements and usage examples.

**Rationale**: Developers need to understand what concepts require when implementing new platforms.

#### Scenario: Concept reference documentation

- **GIVEN** developer implementing new platform
- **WHEN** concept documentation is consulted
- **THEN** documentation SHALL list all required members
- **AND** documentation SHALL provide implementation example

```cpp
/**
 * @concept ClockPlatform
 * @brief Platform clock configuration interface
 *
 * Requirements:
 * - static constexpr u32 system_clock_hz: System clock frequency in Hz
 * - static void init(): Initialize clock system to configured frequency
 *
 * Example implementation:
 * @code
 * template <u32 CLOCK_HZ>
 * struct Clock {
 *     static constexpr u32 system_clock_hz = CLOCK_HZ;
 *
 *     static void init() {
 *         // Configure PLL, enable oscillator, etc.
 *     }
 * };
 *
 * static_assert(ClockPlatform<Clock<84'000'000>>);
 * @endcode
 */
template <typename T>
concept ClockPlatform = requires {
    { T::system_clock_hz } -> std::convertible_to<u32>;
    { T::init() } -> std::same_as<void>;
};
```
