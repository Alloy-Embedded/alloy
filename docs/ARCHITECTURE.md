# Alloy Framework Architecture

**Version:** 1.0
**Last Updated:** 2025-11-15
**Status:** Complete through Phase 6

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Principles](#architecture-principles)
3. [Directory Structure](#directory-structure)
4. [Layer Architecture](#layer-architecture)
5. [C++20 Concepts System](#c20-concepts-system)
6. [Policy-Based Design](#policy-based-design)
7. [Code Generation Pipeline](#code-generation-pipeline)
8. [Build System](#build-system)
9. [Platform Support](#platform-support)
10. [Error Handling](#error-handling)

---

## Overview

MicroCore is a modern C++20 framework for bare-metal embedded systems designed around three core pillars:

1. **Zero Runtime Overhead** - All abstraction happens at compile-time
2. **Type Safety** - C++20 concepts validate interfaces at compile-time
3. **Vendor Independence** - Consistent APIs across all MCU vendors

### Key Design Decisions

- **No RTOS Dependency** - Pure bare-metal by design (RTOS optional)
- **Policy-Based Design** - Hardware access through compile-time policies
- **Auto-Generated Code** - Registers, startup code, and boilerplate from SVD files
- **Result<T, E> Pattern** - Rust-inspired error handling
- **Concepts-First** - All platform implementations validated by C++20 concepts

---

## Architecture Principles

### 1. Compile-Time Everything

```cpp
// Example: GPIO pin configuration resolved at compile-time
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Single instruction at runtime
    void set() { *BSRR = pin_mask; }
};
```

**Benefits:**
- Zero runtime configuration overhead
- Compiler can inline everything
- Impossible to configure at runtime = safer
- Binary size equivalent to hand-written assembly

### 2. Type Safety Through Concepts

```cpp
// Platform implementations MUST satisfy concepts
template <typename T>
concept ClockPlatform = requires {
    { T::initialize() } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_gpio_clocks() } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_uart_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
};

// Compile-time validation
static_assert(ClockPlatform<Stm32f4Clock<Config>>,
              "Implementation must satisfy ClockPlatform concept");
```

**Benefits:**
- Compilation fails if API contract is broken
- Clear error messages ("missing method X")
- Self-documenting code
- Prevents regressions

### 3. Layered Abstraction

```
┌─────────────────────────────────────┐
│     Application Layer               │
│  (User code, examples, apps)        │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│      Board Layer                    │
│  (Board-specific pin mappings)      │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│   HAL Platform Layer                │
│  (GPIO, Clock, UART - per vendor)   │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│   Hardware Policy Layer             │
│  (Direct register access, auto-gen) │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│    Generated Layer                  │
│  (Registers, bitfields from SVD)    │
└─────────────────────────────────────┘
```

---

## Directory Structure

```
alloy/
├── src/
│   ├── core/                       # Core utilities (Result, types, concepts)
│   │   ├── error.hpp               # Error codes and ErrorCode enum
│   │   ├── result.hpp              # Result<T, E> implementation
│   │   ├── types.hpp               # Standard types (u8, u16, u32, etc.)
│   │   └── concepts.hpp            # Core concepts
│   │
│   ├── hal/
│   │   ├── core/
│   │   │   └── concepts.hpp        # HAL-specific concepts (ClockPlatform, GpioPin, etc.)
│   │   │
│   │   ├── types.hpp               # HAL types (PinDirection, PinPull, PinDrive)
│   │   │
│   │   └── vendors/                # Vendor-specific implementations
│   │       ├── st/                 # STMicroelectronics
│   │       │   ├── stm32f4/
│   │       │   │   ├── generated/  # Auto-generated from SVD
│   │       │   │   │   ├── registers/    # Register structs
│   │       │   │   │   └── bitfields/    # Bitfield definitions
│   │       │   │   │
│   │       │   │   ├── gpio_hardware_policy.hpp    # GPIO register access
│   │       │   │   ├── uart_hardware_policy.hpp    # UART register access
│   │       │   │   ├── spi_hardware_policy.hpp     # SPI register access
│   │       │   │   │
│   │       │   │   ├── gpio.hpp            # GPIO platform implementation
│   │       │   │   ├── clock_platform.hpp  # Clock platform implementation
│   │       │   │   ├── systick_platform.hpp
│   │       │   │   │
│   │       │   │   └── stm32f401/          # MCU-specific
│   │       │   │       ├── peripherals.hpp # Peripheral addresses
│   │       │   │       └── startup.cpp     # Startup code
│   │       │   │
│   │       │   ├── stm32f7/        # STM32F7 family (same structure)
│   │       │   └── stm32g0/        # STM32G0 family (same structure)
│   │       │
│   │       └── arm/                # ARM (Microchip/Atmel)
│   │           └── same70/         # SAME70 family (same structure)
│   │
│   └── rtos/                       # Optional RTOS support
│       └── freertos/
│
├── boards/                         # Board support packages
│   ├── nucleo_f401re/
│   │   ├── board.hpp               # Board API (pins, clocks)
│   │   ├── board.cpp               # Board initialization
│   │   └── linker.ld               # Linker script
│   │
│   ├── nucleo_f722ze/
│   ├── nucleo_g0b1re/
│   └── same70_xplained/
│
├── examples/                       # Example applications
│   ├── blink/
│   └── rtos/
│
├── tools/
│   └── codegen/                    # Code generator
│       ├── generators/
│       │   ├── registers.py        # Generate register structs
│       │   ├── bitfields.py        # Generate bitfield masks
│       │   ├── startup.py          # Generate startup code
│       │   └── hardware_policy.py  # Generate hardware policies
│       │
│       └── templates/              # Jinja2 templates
│
└── cmake/                          # Build system
    ├── platforms/                  # Platform definitions
    │   ├── stm32f4.cmake
    │   ├── stm32f7.cmake
    │   └── stm32g0.cmake
    │
    └── toolchains/
        └── arm-none-eabi.cmake
```

### Directory Organization Principles

1. **`src/hal/vendors/{vendor}/{family}/`** - All vendor code here
   - No `platform/` directory (consolidated)
   - Hand-written code at family level
   - Generated code in `generated/` subdirectory

2. **`.generated` marker files** - Indicate auto-generated directories
   - Prevents accidental manual edits
   - Clear separation of concerns

3. **Hardware policies** - At family level (e.g., `stm32f4/gpio_hardware_policy.hpp`)
   - Auto-generated from SVD
   - Direct register access
   - Template-based, zero overhead

4. **Platform implementations** - At family level (e.g., `stm32f4/gpio.hpp`)
   - Hand-written, uses hardware policies
   - Satisfies HAL concepts
   - Validated with `static_assert`

---

## Layer Architecture

### Layer 1: Generated Register Layer

**Location:** `src/hal/vendors/{vendor}/{family}/generated/`

**Purpose:** Direct hardware access with type-safe bitfield operations

**Example:**
```cpp
namespace alloy::hal::st::stm32f4::gpioa {

// Auto-generated register structure
struct GPIOA_Registers {
    volatile uint32_t MODER;   // Mode register
    volatile uint32_t OTYPER;  // Output type register
    volatile uint32_t OSPEEDR; // Output speed register
    volatile uint32_t PUPDR;   // Pull-up/pull-down register
    volatile uint32_t IDR;     // Input data register
    volatile uint32_t ODR;     // Output data register
    volatile uint32_t BSRR;    // Bit set/reset register
    // ... more registers
};

// Bitfield definitions
namespace moder {
    template<uint8_t Pos, uint8_t Width>
    using BitField = /* ... */;

    using MODER0 = BitField<0, 2>;   // Pin 0 mode
    using MODER1 = BitField<2, 2>;   // Pin 1 mode
    // ... 16 pins
}

} // namespace alloy::hal::st::stm32f4::gpioa
```

**Generation:** From CMSIS-SVD XML files via Python code generator

### Layer 2: Hardware Policy Layer

**Location:** `src/hal/vendors/{vendor}/{family}/*_hardware_policy.hpp`

**Purpose:** Provide low-level register manipulation with zero overhead

**Example:**
```cpp
template <uint32_t GPIO_BASE, uint32_t PERIPHERAL_CLOCK_HZ>
struct Stm32f4GPIOHardwarePolicy {

    // Set pin output HIGH (single instruction)
    static void set_output(uint32_t pin_mask) {
        reinterpret_cast<volatile uint32_t*>(GPIO_BASE + BSRR_OFFSET)[0] = pin_mask;
    }

    // Configure pin mode (2 bits per pin)
    static void set_mode_output(uint8_t pin_num) {
        auto* moder = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + MODER_OFFSET);
        *moder = (*moder & ~(0x3 << (pin_num * 2))) | (0x1 << (pin_num * 2));
    }

    // All operations are static, inline, constexpr where possible
};
```

**Key Features:**
- Template-based (port address, clock frequency)
- All methods are `static` (no object state)
- Designed to inline to single instructions
- No runtime overhead

### Layer 3: Platform Implementation Layer

**Location:** `src/hal/vendors/{vendor}/{family}/gpio.hpp`, `clock_platform.hpp`, etc.

**Purpose:** User-facing API that satisfies HAL concepts

**Example:**
```cpp
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
public:
    using HwPolicy = Stm32f4GPIOHardwarePolicy<PORT_BASE, 84000000>;

    // Satisfies GpioPin concept
    Result<void, ErrorCode> set() {
        HwPolicy::set_output(pin_mask);
        return Ok();
    }

    Result<bool, ErrorCode> read() const {
        return Ok(HwPolicy::read_input(pin_mask));
    }

    Result<void, ErrorCode> setDirection(PinDirection direction) {
        if (direction == PinDirection::Output) {
            HwPolicy::set_mode_output(PIN_NUM);
        } else {
            HwPolicy::set_mode_input(PIN_NUM);
        }
        return Ok();
    }

    // ... more methods
};

// Compile-time validation
static_assert(alloy::hal::concepts::GpioPin<GpioPin<0x40020000, 5>>,
              "GpioPin must satisfy GpioPin concept");
```

**Validated by:** C++20 concepts with `static_assert`

### Layer 4: Board Layer

**Location:** `boards/{board_name}/board.hpp`

**Purpose:** Board-specific pin mappings and initialization

**Example:**
```cpp
namespace alloy::board {

using namespace alloy::hal::st::stm32f4;

// Board-specific type aliases
using LedGreen = GpioPin<peripherals::GPIOA, 5>;
using Button   = GpioPin<peripherals::GPIOC, 13>;

// Board initialization
void init() {
    // Initialize system clock (84 MHz for STM32F401)
    BoardClock::initialize();
    BoardClock::enable_gpio_clocks();

    // Configure LED
    LedGreen led;
    led.setDirection(PinDirection::Output);
}

} // namespace alloy::board
```

### Layer 5: Application Layer

**Location:** `examples/`, user applications

**Purpose:** User code using board API

**Example:**
```cpp
#include "boards/nucleo_f401re/board.hpp"

int main() {
    alloy::board::init();

    alloy::board::LedGreen led;

    while (true) {
        led.toggle();
        alloy::board::delay_ms(500);
    }
}
```

---

## C++20 Concepts System

### Concept Hierarchy

```
Core Concepts (src/core/concepts.hpp)
    ↓
HAL Concepts (src/hal/core/concepts.hpp)
    ├── ClockPlatform
    ├── GpioPin
    ├── UartPeripheral
    ├── SpiPeripheral
    ├── TimerPeripheral
    ├── AdcPeripheral
    └── PwmPeripheral
```

### ClockPlatform Concept

```cpp
template <typename T>
concept ClockPlatform = requires {
    // System clock initialization
    { T::initialize() } -> std::same_as<Result<void, ErrorCode>>;

    // Peripheral clock enable functions
    { T::enable_gpio_clocks() } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_uart_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_spi_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_i2c_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
};
```

**Purpose:**
- Enforce consistent clock APIs across all platforms
- Compile-time validation (fails if method missing)
- Self-documenting interface requirements

**Validation:**
```cpp
// In stm32f4/clock_platform.hpp
static_assert(alloy::hal::concepts::ClockPlatform<Stm32f4Clock<ExampleConfig>>,
              "Stm32f4Clock must satisfy ClockPlatform concept - missing required methods");
```

**Error Message** (if concept not satisfied):
```
error: static assertion failed: "Stm32f4Clock must satisfy ClockPlatform concept - missing required methods"
note: constraints not satisfied because 'enable_uart_clock' is missing
```

### GpioPin Concept

```cpp
template <typename T>
concept GpioPin = requires(T pin, const T const_pin, PinDirection direction,
                            PinDrive drive, PinPull pull, bool value) {
    // State manipulation
    { pin.set() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.clear() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.toggle() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.write(value) } -> std::same_as<Result<void, ErrorCode>>;

    // State reading
    { const_pin.read() } -> std::same_as<Result<bool, ErrorCode>>;
    { const_pin.isOutput() } -> std::same_as<Result<bool, ErrorCode>>;

    // Configuration
    { pin.setDirection(direction) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setDrive(drive) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setPull(pull) } -> std::same_as<Result<void, ErrorCode>>;

    // Compile-time metadata
    requires requires { T::port_base; };
    requires requires { T::pin_number; };
    requires requires { T::pin_mask; };
};
```

**All platforms validated:**
- STM32F4 ✅
- STM32F7 ✅
- STM32G0 ✅
- SAME70 (in progress)

---

## Policy-Based Design

### What is Policy-Based Design?

A design pattern where behavior is defined through template parameters ("policies") instead of inheritance.

**Traditional OOP:**
```cpp
class GpioBase {
    virtual void set() = 0;  // Runtime polymorphism
};

class Stm32Gpio : public GpioBase {
    void set() override { /* ... */ }  // Virtual dispatch
};
```

**Policy-Based Design:**
```cpp
template <typename HardwarePolicy>
class Gpio {
    void set() { HardwarePolicy::set_output(pin_mask); }  // Static dispatch
};
```

### Benefits

1. **Zero Runtime Overhead** - No vtables, no virtual dispatch
2. **Compile-Time Optimization** - Compiler can inline everything
3. **Type Safety** - Policies validated at compile-time
4. **Testability** - Easy to mock by swapping policy
5. **Binary Size** - Only code you use is compiled in

### Hardware Policy Pattern in MicroCore

```cpp
// 1. Define hardware policy (auto-generated)
template <uint32_t GPIO_BASE, uint32_t CLOCK_HZ>
struct Stm32f4GPIOHardwarePolicy {
    static void set_output(uint32_t mask) {
        *BSRR = mask;
    }
};

// 2. Use policy in platform implementation
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
    using HwPolicy = Stm32f4GPIOHardwarePolicy<PORT_BASE, CLOCK_HZ>;

    void set() {
        HwPolicy::set_output(pin_mask);  // Inlines to single instruction
    }
};

// 3. Instantiate with specific port and pin
using LedPin = GpioPin<0x40020000, 5>;  // GPIOA, pin 5
```

**Resulting Assembly:**
```asm
; led.set() compiles to:
movw r0, #0x0018    ; BSRR offset + pin mask
movt r0, #0x4002    ; GPIO base address
str  r0, [r0]       ; Single store instruction
```

---

## Code Generation Pipeline

### Overview

```
┌──────────────┐
│  CMSIS-SVD   │  (Vendor-provided XML)
│   XML File   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  SVD Parser  │  (Python: tools/codegen/svd_parser.py)
│              │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ JSON Database│  (Normalized MCU data)
│              │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Generator   │  (Python: tools/codegen/generator.py)
│  + Templates │  (Jinja2)
└──────┬───────┘
       │
       ├─────► Register Structs    (.hpp)
       ├─────► Bitfield Definitions (.hpp)
       ├─────► Hardware Policies   (.hpp)
       ├─────► Startup Code        (.cpp)
       └─────► Linker Scripts      (.ld)
```

### Generated Code

**Input:** `STM32F401.svd` (CMSIS-SVD XML)

**Output:**
```
src/hal/vendors/st/stm32f4/generated/
├── registers/
│   ├── gpioa_registers.hpp
│   ├── rcc_registers.hpp
│   └── ... (100+ files)
│
├── bitfields/
│   ├── gpioa_bitfields.hpp
│   ├── rcc_bitfields.hpp
│   └── ... (100+ files)
│
└── .generated  (marker file)

src/hal/vendors/st/stm32f4/
├── gpio_hardware_policy.hpp  (uses generated registers)
├── uart_hardware_policy.hpp
└── spi_hardware_policy.hpp

src/hal/vendors/st/stm32f4/stm32f401/
└── startup.cpp  (vector table, init code)
```

### Code Generation Commands

```bash
# Generate everything for all platforms
cd tools/codegen
python3 codegen.py generate --all

# Generate only registers for STM32F4
python3 codegen.py generate --registers --vendor st --family stm32f4

# Validate generated code
python3 codegen.py validate

# Clean generated files (dry-run first)
python3 codegen.py clean --dry-run
```

---

## Build System

### CMake Architecture

```
CMakeLists.txt (root)
    ├── cmake/platforms/stm32f4.cmake
    ├── cmake/platforms/stm32f7.cmake
    ├── cmake/platforms/stm32g0.cmake
    ├── cmake/toolchains/arm-none-eabi.cmake
    └── boards/{board}/CMakeLists.txt
```

### Platform Selection

```cmake
# User specifies board
cmake -B build -DALLOY_BOARD=nucleo_f401re

# CMake auto-detects platform
board_to_platform(nucleo_f401re → stm32f4)

# Load platform configuration
include(cmake/platforms/stm32f4.cmake)
```

### Platform Configuration File

**Example:** `cmake/platforms/stm32f4.cmake`

```cmake
# Define platform
set(ALLOY_PLATFORM "stm32f4")

# Platform directory
set(ALLOY_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32f4")

# Collect platform sources (GLOB within family directory)
file(GLOB ALLOY_PLATFORM_HEADERS
    "${ALLOY_PLATFORM_DIR}/*.hpp"
    "${ALLOY_PLATFORM_DIR}/generated/**/*.hpp"
)

# CPU configuration
set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# Linker configuration
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/${ALLOY_BOARD}/linker.ld")
```

### Build Targets

```bash
# Build all examples
cmake --build build

# Build specific target
cmake --build build --target blink

# Flash to hardware
cmake --build build --target flash
```

---

## Platform Support

### Currently Supported Platforms

| Platform | Vendor | Core | Frequency | Status |
|----------|--------|------|-----------|--------|
| STM32F4 | STMicroelectronics | Cortex-M4F | 84-168 MHz | ✅ Complete |
| STM32F7 | STMicroelectronics | Cortex-M7 | 180-216 MHz | ✅ Complete |
| STM32G0 | STMicroelectronics | Cortex-M0+ | 64 MHz | ✅ Complete |
| SAME70 | Microchip (Atmel) | Cortex-M7 | 300 MHz | 🔄 In Progress |

### Supported Boards

| Board | Platform | MCU | Status |
|-------|----------|-----|--------|
| nucleo_f401re | STM32F4 | STM32F401RE | ✅ |
| nucleo_f722ze | STM32F7 | STM32F722ZE | ✅ |
| nucleo_g0b1re | STM32G0 | STM32G0B1RE | ✅ |
| same70_xplained | SAME70 | ATSAME70Q21 | 🔄 |

### Feature Matrix

| Feature | STM32F4 | STM32F7 | STM32G0 | SAME70 |
|---------|---------|---------|---------|--------|
| Clock | ✅ | ✅ | ✅ | 🔄 |
| GPIO | ✅ | ✅ | ✅ | 🔄 |
| UART | 🔄 | 🔄 | 🔄 | 🔄 |
| SPI | 🔄 | 🔄 | 🔄 | 🔄 |
| I2C | 🔄 | 🔄 | 🔄 | 🔄 |
| Concepts | ✅ | ✅ | ✅ | ⏳ |

✅ = Complete | 🔄 = In Progress | ⏳ = Planned

---

## Error Handling

### Result<T, E> Pattern

MicroCore uses Rust-inspired `Result<T, E>` for all fallible operations:

```cpp
template <typename T, typename E>
class Result {
public:
    // Success case
    static Result Ok(T value);

    // Error case
    static Result Err(E error);

    // Check status
    bool is_ok() const;
    bool is_err() const;

    // Extract value (panics if error)
    T unwrap();

    // Extract value or default
    T unwrap_or(T default_value);

    // Map transformations
    template<typename F>
    auto map(F func) -> Result</* ... */, E>;
};
```

### Usage Example

```cpp
Result<bool, ErrorCode> read_pin() {
    if (hardware_error) {
        return Err(ErrorCode::HardwareFault);
    }
    return Ok(pin_state);
}

// Use result
auto result = read_pin();
if (result.is_ok()) {
    bool state = result.unwrap();
} else {
    ErrorCode error = result.unwrap_err();
    handle_error(error);
}

// Or use monadic style
auto result = read_pin()
    .map([](bool state) { return !state; })  // Invert
    .unwrap_or(false);  // Default to false on error
```

### Error Codes

```cpp
enum class ErrorCode {
    None,
    Timeout,
    HardwareFault,
    InvalidParameter,
    NotInitialized,
    Busy,
    // ... more
};
```

---

## Conclusion

MicroCore's architecture achieves:

1. **✅ Zero Overhead** - All abstraction at compile-time
2. **✅ Type Safety** - C++20 concepts validate everything
3. **✅ Vendor Independence** - Same API across all MCUs
4. **✅ Testability** - Policy-based design enables mocking
5. **✅ Maintainability** - Auto-generated boilerplate

This architecture enables **bare-metal embedded development with the safety of high-level languages and the performance of assembly**.

---

**Next:** See [PORTING_NEW_BOARD.md](PORTING_NEW_BOARD.md) to add support for your board.
