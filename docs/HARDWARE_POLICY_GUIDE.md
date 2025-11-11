# Hardware Policy Implementation Guide

**Version**: 1.0  
**Date**: 2025-11-11  
**Status**: Production-Ready

## Table of Contents

1. [Introduction](#introduction)
2. [What is a Hardware Policy?](#what-is-a-hardware-policy)
3. [Policy-Based Design Pattern](#policy-based-design-pattern)
4. [Creating a New Hardware Policy](#creating-a-new-hardware-policy)
5. [Metadata File Format](#metadata-file-format)
6. [Code Generation](#code-generation)
7. [Platform Integration](#platform-integration)
8. [Testing Your Policy](#testing-your-policy)
9. [Best Practices](#best-practices)
10. [Troubleshooting](#troubleshooting)

---

## Introduction

This guide explains how to implement hardware policies for the Alloy HAL using the **Policy-Based Design pattern**. Hardware policies provide platform-specific implementations that work with generic, platform-independent APIs.

### Why Hardware Policies?

✅ **Zero Overhead**: All methods are `static inline` → direct register access  
✅ **Platform Independence**: Same generic API works on all platforms  
✅ **Type Safety**: Compile-time validation of hardware operations  
✅ **Testability**: Mock hooks for unit testing without hardware  
✅ **Maintainability**: Platform details isolated in policies  

### Supported Platforms

- ✅ SAME70 (Atmel ARM Cortex-M7)
- ✅ STM32F4 (STMicro ARM Cortex-M4)
- ✅ STM32F1 (STMicro ARM Cortex-M3 - Blue Pill)

---

## What is a Hardware Policy?

A **hardware policy** is a C++ template class that provides platform-specific hardware access methods. It acts as a compile-time interface between generic APIs and hardware registers.

### Example: UART Hardware Policy

```cpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f4UartHardwarePolicy {
    using RegisterType = USART1_Registers;
    
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    
    // Hardware accessor
    static inline volatile RegisterType* hw() {
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    }
    
    // Policy methods
    static inline void reset() {
        hw()->CR1 &= ~(usart::cr1::TE::mask | usart::cr1::RE::mask);
    }
    
    static inline void set_baudrate(uint32_t baud) {
        uint32_t usartdiv = (PERIPH_CLOCK_HZ + (baud / 2)) / baud;
        hw()->BRR = usartdiv;
    }
    
    static inline bool is_tx_ready() const {
        return (hw()->SR & usart::sr::TXE::mask) != 0;
    }
    
    // ... more methods
};
```

### Key Characteristics

1. **Template Parameters**: Base address and clock frequency
2. **Static Methods**: No object instance needed
3. **Inline Methods**: Zero function call overhead
4. **Direct Register Access**: `hw()->REGISTER` pattern
5. **Const Correctness**: Read-only methods marked `const`

---

## Policy-Based Design Pattern

### Architecture Layers

```
┌─────────────────────────────────────┐
│  Application Code                   │
│  (Platform-independent)             │
└──────────────┬──────────────────────┘
               │ Uses type aliases
               ▼
┌─────────────────────────────────────┐
│  Platform Integration               │
│  platform/stm32f4/uart.hpp          │
│  Type Aliases: Usart1, Usart2, etc. │
└──────────────┬──────────────────────┘
               │ Combines
               ▼
┌─────────────────────────────────────┐
│  Generic API Layer                  │
│  api/uart_simple.hpp                │
│  api/uart_fluent.hpp                │
│  api/uart_expert.hpp                │
└──────────────┬──────────────────────┘
               │ Template Parameter
               │ <HardwarePolicy>
               ▼
┌─────────────────────────────────────┐
│  Hardware Policy Layer              │
│  vendors/st/stm32f4/                │
│  usart_hardware_policy.hpp          │
└──────────────┬──────────────────────┘
               │ Direct Access
               ▼
┌─────────────────────────────────────┐
│  Hardware Registers                 │
│  USART1, USART2, SPI1, etc.         │
└─────────────────────────────────────┘
```

### Generic API Example

```cpp
// Generic API accepts policy as template parameter
template <PeripheralId PeriphId, typename HardwarePolicy>
class Uart {
public:
    template <typename TxPin, typename RxPin>
    static constexpr auto quick_setup(BaudRate baudrate) {
        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>{
            PeriphId, baudrate
        };
    }
};

template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig {
    Result<void, ErrorCode> initialize() const {
        // Call policy methods - platform-specific behavior
        HardwarePolicy::reset();
        HardwarePolicy::configure_8n1();
        HardwarePolicy::set_baudrate(baudrate.value());
        HardwarePolicy::enable_tx();
        HardwarePolicy::enable_rx();
        return Ok();
    }
};
```

### Platform Integration Example

```cpp
// Platform integration combines generic API + hardware policy
namespace alloy::hal::stm32f4 {

// Hardware policy with platform-specific parameters
using Usart1Hardware = Stm32f4UartHardwarePolicy<
    0x40011000,  // USART1 base address
    84000000     // APB2 clock @ 84MHz
>;

// Type alias for convenience
using Usart1 = Uart<PeripheralId::USART1, Usart1Hardware>;

}  // namespace alloy::hal::stm32f4
```

### User Code Example

```cpp
#include "hal/uart.hpp"  // Platform dispatch header

using namespace alloy::hal::stm32f4;

int main() {
    // Platform-independent API, platform-specific behavior
    auto uart = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();
    
    const char* msg = "Hello World!\n";
    uart.write(reinterpret_cast<const uint8_t*>(msg), 13);
}
```

**Result**: Compiles to direct register access, zero overhead!

---

## Creating a New Hardware Policy

### Step 1: Create Metadata File

Create a JSON metadata file describing the peripheral:

**Location**: `tools/codegen/cli/generators/metadata/platform/<family>_<peripheral>.json`

**Example**: `stm32f4_uart.json`

```json
{
  "family": "stm32f4",
  "vendor": "st",
  "peripheral_name": "USART",
  "register_include": "hal/vendors/st/stm32f4/registers/usart1_registers.hpp",
  "bitfield_include": "hal/vendors/st/stm32f4/bitfields/usart1_bitfields.hpp",
  "register_namespace": "st::stm32f4::usart1",
  "namespace_alias": "usart",
  "register_type": "USART1_Registers",
  
  "policy_methods": {
    "reset": {
      "description": "Reset USART peripheral",
      "return_type": "void",
      "code": "hw()->CR1 &= ~(usart::cr1::TE::mask | usart::cr1::RE::mask);",
      "test_hook": "ALLOY_UART_TEST_HOOK_RESET"
    },
    
    "set_baudrate": {
      "description": "Set USART baud rate",
      "parameters": [
        {"name": "baud", "type": "uint32_t", "description": "Desired baud rate"}
      ],
      "return_type": "void",
      "code": "uint32_t usartdiv = PERIPH_CLOCK_HZ / baud; hw()->BRR = usartdiv;",
      "test_hook": "ALLOY_UART_TEST_HOOK_BAUDRATE"
    }
  }
}
```

### Step 2: Generate Hardware Policy

Run the hardware policy generator:

```bash
cd tools/codegen
python3 generate_hardware_policy.py --family stm32f4 --peripheral uart
```

**Output**: `src/hal/vendors/st/stm32f4/usart_hardware_policy.hpp`

### Step 3: Create Platform Integration

Create platform-specific type aliases:

**Location**: `src/hal/platform/<family>/<peripheral>.hpp`

**Example**: `src/hal/platform/stm32f4/uart.hpp`

```cpp
#pragma once

#include "hal/api/uart_simple.hpp"
#include "hal/api/uart_fluent.hpp"
#include "hal/api/uart_expert.hpp"
#include "hal/vendors/st/stm32f4/usart_hardware_policy.hpp"

namespace alloy::hal::stm32f4 {

// Hardware policies
using Usart1Hardware = Stm32f4UartHardwarePolicy<0x40011000, 84000000>;
using Usart2Hardware = Stm32f4UartHardwarePolicy<0x40004400, 42000000>;

// Simple API
using Usart1 = Uart<PeripheralId::USART1, Usart1Hardware>;
using Usart2 = Uart<PeripheralId::USART2, Usart2Hardware>;

// Fluent API
using Usart1Builder = UartBuilder<PeripheralId::USART1, Usart1Hardware>;
using Usart2Builder = UartBuilder<PeripheralId::USART2, Usart2Hardware>;

// Expert API
using Usart1ExpertConfig = UartExpertConfig<Usart1Hardware>;
using Usart2ExpertConfig = UartExpertConfig<Usart2Hardware>;

}  // namespace alloy::hal::stm32f4
```

### Step 4: Test Your Policy

Create unit tests with mock registers (see [Testing Your Policy](#testing-your-policy)).

---

## Metadata File Format

### Required Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `family` | string | MCU family | `"stm32f4"` |
| `vendor` | string | Vendor name | `"st"` |
| `peripheral_name` | string | Peripheral name | `"USART"` |
| `register_include` | string | Register header path | `"hal/vendors/.../usart1_registers.hpp"` |
| `bitfield_include` | string | Bitfield header path | `"hal/vendors/.../usart1_bitfields.hpp"` |
| `register_namespace` | string | C++ namespace for registers | `"st::stm32f4::usart1"` |
| `register_type` | string | Register struct name | `"USART1_Registers"` |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `namespace_alias` | string | Short alias for bitfield namespace |
| `template_params` | array | Template parameters for policy |
| `constants` | array | Compile-time constants |
| `instances` | array | Peripheral instances with base addresses |

### Policy Methods Section

```json
"policy_methods": {
  "description": "Hardware Policy methods",
  "peripheral_clock_hz": 84000000,
  "mock_hook_prefix": "ALLOY_UART_MOCK_HW",
  
  "method_name": {
    "description": "Method description",
    "return_type": "void | bool | uint8_t | ...",
    "const": true,  // Optional: for read-only methods
    "parameters": [  // Optional
      {"name": "param_name", "type": "uint32_t", "description": "..."}
    ],
    "code": "C++ code for method body",
    "test_hook": "MACRO_NAME_FOR_TESTING"  // Optional
  }
}
```

### Common Policy Methods

**Essential Methods** (all peripherals should have):
- `reset()` - Reset peripheral to default state
- `enable()` / `disable()` - Enable/disable peripheral
- `is_tx_ready()` / `is_rx_ready()` - Check status flags
- `write_byte()` / `read_byte()` - Data transfer

**UART-Specific**:
- `configure_8n1()` - Configure 8 data bits, no parity, 1 stop bit
- `set_baudrate(uint32_t baud)` - Set baud rate
- `enable_tx()` / `enable_rx()` - Enable transmitter/receiver

**SPI-Specific**:
- `configure_master(...)` - Configure as SPI master
- `select_chip(uint8_t cs)` - Select chip select

---

## Code Generation

### Generator Script

**Location**: `tools/codegen/generate_hardware_policy.py`

**Usage**:
```bash
# Generate single peripheral
python3 generate_hardware_policy.py --family stm32f4 --peripheral uart

# Generate all peripherals for a family
python3 generate_hardware_policy.py --family stm32f4 --all
```

### Template File

**Location**: `tools/codegen/templates/platform/uart_hardware_policy.hpp.j2`

The Jinja2 template generates C++ code from metadata:

```jinja2
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct {{ family.capitalize() }}UartHardwarePolicy {
    {% for method_name, method in policy_methods.items() %}
    static inline {{ method.return_type }} {{ method_name }}(
        {% for param in method.parameters %}
        {{ param.type }} {{ param.name }}
        {% endfor %}
    ) {% if method.const %}const{% endif %} {
        {{ method.code | indent(8) }}
    }
    {% endfor %}
};
```

### Generated Output Example

```cpp
/**
 * Auto-generated from: stm32f4/uart.json
 * Generated: 2025-11-11 07:08:59
 */

template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f4UartHardwarePolicy {
    using RegisterType = USART1_Registers;
    
    static inline void reset() {
        hw()->CR1 &= ~(usart::cr1::TE::mask | usart::cr1::RE::mask);
    }
    
    static inline void set_baudrate(uint32_t baud) {
        uint32_t usartdiv = (PERIPH_CLOCK_HZ + (baud / 2)) / baud;
        hw()->BRR = usartdiv;
    }
    
    // ... more methods
};
```

---

## Platform Integration

### Directory Structure

```
src/hal/
├── api/                        # Generic APIs (platform-independent)
│   ├── uart_simple.hpp
│   ├── uart_fluent.hpp
│   └── uart_expert.hpp
│
├── platform/                   # Platform integration (type aliases)
│   ├── stm32f4/
│   │   └── uart.hpp           # STM32F4 UART integration
│   ├── stm32f1/
│   │   └── uart.hpp           # STM32F1 UART integration
│   └── same70/
│       └── uart.hpp           # SAME70 UART integration
│
└── vendors/                    # Hardware policies (platform-specific)
    ├── st/stm32f4/
    │   └── usart_hardware_policy.hpp   # STM32F4 USART policy
    ├── st/stm32f1/
    │   └── usart_hardware_policy.hpp   # STM32F1 USART policy
    └── atmel/same70/
        └── uart_hardware_policy.hpp    # SAME70 UART policy
```

### Platform Dispatch Header

Top-level header that selects platform based on build configuration:

**Location**: `src/hal/uart.hpp`

```cpp
#pragma once

#if defined(ALLOY_PLATFORM_STM32F4)
    #include "platform/stm32f4/uart.hpp"
#elif defined(ALLOY_PLATFORM_STM32F1)
    #include "platform/stm32f1/uart.hpp"
#elif defined(ALLOY_PLATFORM_SAME70)
    #include "platform/same70/uart.hpp"
#else
    #error "No platform defined!"
#endif
```

**Usage in Application**:
```cpp
#include "hal/uart.hpp"  // Automatically selects correct platform

// Platform-specific namespace selected at compile time
using namespace alloy::hal::stm32f4;  // Or stm32f1, same70, etc.
```

---

## Testing Your Policy

### Mock Register System

Create mock registers for testing without hardware:

```cpp
struct MockUartRegisters {
    volatile uint32_t CR1{0};
    volatile uint32_t SR{0};
    volatile uint32_t DR{0};
    volatile uint32_t BRR{0};
    
    void reset_all() {
        CR1 = 0; SR = 0; DR = 0; BRR = 0;
    }
};

static MockUartRegisters g_mock_uart_regs;

extern "C" volatile USART1_Registers* mock_uart_hw() {
    return reinterpret_cast<volatile USART1_Registers*>(&g_mock_uart_regs);
}
```

### Test Policy with Mocks

```cpp
template <uint32_t PERIPH_CLOCK_HZ>
struct TestUartHardwarePolicy {
    using RegisterType = USART1_Registers;
    
    static inline volatile RegisterType* hw() {
        #ifdef TESTING
            return mock_uart_hw();  // Use mock in tests
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }
    
    // Same policy methods as production code
    static inline void reset() {
        hw()->CR1 &= ~(usart::cr1::TE::mask);
    }
};
```

### Unit Test Example

```cpp
TEST_CASE("UART Hardware Policy - reset()") {
    g_mock_uart_regs.reset_all();
    
    TestUartHardwarePolicy<84000000>::reset();
    
    // Verify register was written correctly
    REQUIRE((g_mock_uart_regs.CR1 & usart::cr1::TE::mask) == 0);
}

TEST_CASE("UART Hardware Policy - set_baudrate(115200)") {
    g_mock_uart_regs.reset_all();
    
    TestUartHardwarePolicy<84000000>::set_baudrate(115200);
    
    uint32_t expected_brr = 84000000 / 115200;
    REQUIRE(g_mock_uart_regs.BRR == expected_brr);
}
```

---

## Best Practices

### 1. Keep Methods Simple and Focused

✅ **Good**: One operation per method
```cpp
static inline void enable_tx() {
    hw()->CR1 |= usart::cr1::TE::mask;
}
```

❌ **Bad**: Multiple operations in one method
```cpp
static inline void configure_uart() {
    hw()->CR1 |= usart::cr1::TE::mask | usart::cr1::RE::mask | usart::cr1::UE::mask;
    hw()->BRR = 0x1234;
    // Too much in one method!
}
```

### 2. Use Bitfield Helpers

✅ **Good**: Use generated bitfield helpers
```cpp
hw()->CR1 |= usart::cr1::TE::mask;
```

❌ **Bad**: Magic numbers
```cpp
hw()->CR1 |= (1 << 3);  // What is bit 3?
```

### 3. Mark Read-Only Methods as const

✅ **Good**:
```cpp
static inline bool is_tx_ready() const {
    return (hw()->SR & usart::sr::TXE::mask) != 0;
}
```

### 4. Use Template Parameters for Compile-Time Configuration

✅ **Good**: Base address and clock as template parameters
```cpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct UartHardwarePolicy {
    static inline void set_baudrate(uint32_t baud) {
        uint32_t div = PERIPH_CLOCK_HZ / baud;  // Compile-time constant!
        hw()->BRR = div;
    }
};
```

### 5. Include Test Hooks

✅ **Good**: Test hooks for verification
```cpp
static inline void reset() {
    #ifdef ALLOY_UART_TEST_HOOK_RESET
        ALLOY_UART_TEST_HOOK_RESET();
    #endif
    
    hw()->CR1 = 0;
}
```

### 6. Document Register Operations

✅ **Good**: Comment complex register operations
```cpp
static inline void set_baudrate(uint32_t baud) {
    // Calculate USARTDIV = f_periph / baud
    // BRR = [Mantissa:12bits][Fraction:4bits]
    uint32_t usartdiv = (PERIPH_CLOCK_HZ + (baud / 2)) / baud;
    uint32_t mantissa = usartdiv >> 4;
    uint32_t fraction = usartdiv & 0xF;
    hw()->BRR = (mantissa << 4) | fraction;
}
```

---

## Troubleshooting

### Problem: "RegisterType not found"

**Cause**: Incorrect register include path in metadata

**Solution**: Verify paths in metadata file:
```json
"register_include": "hal/vendors/st/stm32f4/registers/usart1_registers.hpp",
"bitfield_include": "hal/vendors/st/stm32f4/bitfields/usart1_bitfields.hpp"
```

### Problem: "Bitfield mask not found"

**Cause**: Incorrect namespace alias

**Solution**: Check namespace alias matches bitfield namespace:
```json
"register_namespace": "st::stm32f4::usart1",
"namespace_alias": "usart"
```

Then use:
```cpp
hw()->CR1 |= usart::cr1::TE::mask;  // Correct
```

### Problem: "Static member function cannot have const qualifier"

**Cause**: Added `const` to method that modifies registers

**Solution**: Only mark read-only methods as const:
```json
"is_tx_ready": {
  "const": true,  // OK - read-only
  "code": "return (hw()->SR & usart::sr::TXE::mask) != 0;"
}

"enable_tx": {
  "const": false,  // REQUIRED - modifies registers
  "code": "hw()->CR1 |= usart::cr1::TE::mask;"
}
```

### Problem: "Compilation errors with template parameters"

**Cause**: Incorrect template parameter types

**Solution**: Ensure BASE_ADDR and clocks are `uint32_t`:
```json
"template_params": [
  {"name": "BASE_ADDR", "type": "uint32_t"},
  {"name": "PERIPH_CLOCK_HZ", "type": "uint32_t"}
]
```

---

## Summary

**Creating a hardware policy involves**:

1. ✅ Create metadata JSON file
2. ✅ Run code generator
3. ✅ Create platform integration file
4. ✅ Test with mock registers
5. ✅ Verify on real hardware

**Result**: Zero-overhead, platform-independent, type-safe hardware abstraction!

For more examples, see:
- `openspec/changes/modernize-peripheral-architecture/PHASE_8_SUMMARY.md`
- `openspec/changes/modernize-peripheral-architecture/PHASE_10_SUMMARY.md`
- `docs/PERIPHERAL_TYPE_ALIASES_GUIDE.md`
