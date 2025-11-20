# GPIO Template Generation Guide

**Version**: 1.0.0
**Date**: 2025-01-19
**Phase**: 2.2 - GPIO Template Implementation

---

## Overview

This guide explains how to use the GPIO template system to generate platform-specific GPIO hardware policies from metadata.

The GPIO template system supports two major GPIO architectural styles:
1. **STM32-style**: MODER/BSRR-based (STMicroelectronics)
2. **SAM-style**: PER/OER/SODR-based (Microchip/Atmel)

---

## Quick Start

### Generate GPIO Code for All Platforms

```bash
cd tools/codegen/generators
python3 gpio_generator.py --all
```

### Generate for Specific Platform

```bash
python3 gpio_generator.py stm32f4
python3 gpio_generator.py same70
```

### List Available Platforms

```bash
python3 gpio_generator.py --list
```

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    GPIO Template System                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐      ┌──────────────┐      ┌───────────┐ │
│  │ gpio.json    │──────▶│gpio_generator│──────▶│  gpio.hpp │ │
│  │ (metadata)   │      │    .py       │      │ (generated)│ │
│  └──────────────┘      └──────────────┘      └───────────┘ │
│         │                      │                      │       │
│         │                      │                      │       │
│   ┌─────▼────┐            ┌───▼────┐            ┌───▼────┐  │
│   │ JSON     │            │Jinja2  │            │Compile │  │
│   │Schema    │            │gpio.hpp│            │ Test   │  │
│   │Validator │            │  .j2   │            │        │  │
│   └──────────┘            └────────┘            └────────┘  │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## File Structure

```
tools/codegen/
├── templates/
│   └── platform/
│       └── gpio.hpp.j2              # GPIO template (558 lines)
│
├── metadata/
│   ├── schema/
│   │   └── peripheral.schema.json   # Validation schema
│   └── platforms/
│       ├── stm32f4/
│       │   ├── platform.json        # Platform metadata
│       │   └── gpio.json            # GPIO metadata (191 lines)
│       └── same70/
│           ├── platform.json
│           └── gpio.json            # GPIO metadata (370 lines)
│
└── generators/
    └── gpio_generator.py            # Generator script (463 lines)
```

### Generated Output

```
src/hal/vendors/
├── st/
│   └── stm32f4/
│       └── generated/
│           └── platform/
│               └── gpio.hpp         # STM32F4 GPIO hardware policy
└── microchip/
    └── same70/
        └── generated/
            └── platform/
                └── gpio.hpp         # SAME70 GPIO hardware policy
```

---

## GPIO Metadata Format

### Platform Metadata (`platform.json`)

```json
{
  "platform": {
    "name": "STM32F4",
    "version": "1.0.0"
  },
  "vendor": {
    "name": "STMicroelectronics",
    "namespace": "st"
  },
  "architecture": {
    "core": "Cortex-M4",
    "fpu": true
  },
  "memory": {
    "flash": { "start": "0x08000000", "size": "1024K" },
    "ram": { "start": "0x20000000", "size": "192K" }
  }
}
```

### GPIO Metadata (`gpio.json`)

#### STM32-Style GPIO

```json
{
  "peripheral": {
    "name": "GPIO",
    "type": "GPIO",
    "description": "General-purpose I/O"
  },
  "instances": [
    {
      "name": "GPIOA",
      "base_address": "0x40020000",
      "port_char": "A",
      "clock_enable": {
        "register": "RCC_AHB1ENR",
        "bit": 0
      }
    }
  ],
  "registers": {
    "MODER": {
      "offset": "0x00",
      "size": 32,
      "access": "RW",
      "description": "GPIO port mode register"
    },
    "BSRR": {
      "offset": "0x18",
      "size": 32,
      "access": "WO",
      "description": "GPIO port bit set/reset register"
    }
  },
  "gpio_specific": {
    "pins_per_port": 16,
    "alternate_functions": {
      "AF0": { "af_number": 0, "function": "System" },
      "AF7": { "af_number": 7, "function": "USART1/USART2/USART3" }
    }
  },
  "template_variables": {
    "style": "stm32",
    "register_type": "GPIO_TypeDef",
    "register_include": "hal/vendors/st/stm32f4/generated/registers/gpio_registers.hpp",
    "bitfield_include": "hal/vendors/st/stm32f4/generated/bitfields/gpio_bitfields.hpp"
  }
}
```

#### SAM-Style GPIO

```json
{
  "peripheral": {
    "name": "PIO",
    "type": "GPIO",
    "description": "Parallel I/O Controller"
  },
  "instances": [
    {
      "name": "PIOA",
      "base_address": "0x400E0E00",
      "port_char": "A",
      "irq_number": 10,
      "clock_enable": {
        "register": "PMC_PCER0",
        "bit": 10
      }
    }
  ],
  "registers": {
    "PER": { "offset": "0x00", "size": 32, "access": "WO" },
    "OER": { "offset": "0x10", "size": 32, "access": "WO" },
    "SODR": { "offset": "0x30", "size": 32, "access": "WO" },
    "CODR": { "offset": "0x34", "size": 32, "access": "WO" },
    "PDSR": { "offset": "0x3C", "size": 32, "access": "RO" }
  },
  "gpio_specific": {
    "pins_per_port": 32,
    "peripheral_select": {
      "A": { "abcdsr1": 0, "abcdsr2": 0 },
      "B": { "abcdsr1": 1, "abcdsr2": 0 }
    }
  },
  "template_variables": {
    "style": "sam",
    "register_type": "Pio",
    "register_include": "hal/vendors/microchip/same70/generated/registers/pio_registers.hpp"
  }
}
```

---

## Generated Code Structure

### STM32-Style GPIO Hardware Policy

```cpp
template <uint32_t BASE_ADDR, char PORT_CHAR>
struct STM32F4GpioHardwarePolicy {
    using RegisterType = GPIO_TypeDef;

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr char port_char = PORT_CHAR;
    static constexpr uint32_t pins_per_port = 16;

    // Mode constants
    static constexpr uint32_t MODE_INPUT = 0;
    static constexpr uint32_t MODE_OUTPUT = 1;
    static constexpr uint32_t MODE_AF = 2;
    static constexpr uint32_t MODE_ANALOG = 3;

    // Pin configuration
    static inline void set_mode(u8 pin, u32 mode);
    static inline void set_output_type(u8 pin, u32 otype);
    static inline void set_output_speed(u8 pin, u32 speed);
    static inline void set_pull(u8 pin, u32 pupd);
    static inline void set_alternate_function(u8 pin, u8 af);

    // Pin I/O
    static inline void set(u8 pin);
    static inline void clear(u8 pin);
    static inline void toggle(u8 pin);
    static inline bool read(u8 pin);
    static inline void write(u8 pin, bool value);

    // Port-wide operations
    static inline u16 read_port();
    static inline void write_port(u16 value);
    static inline void set_mask(u16 mask);
    static inline void clear_mask(u16 mask);
};

// Type aliases for each port
using GPIOAHardware = STM32F4GpioHardwarePolicy<0x40020000, 'A'>;
using GPIOBHardware = STM32F4GpioHardwarePolicy<0x40020400, 'B'>;
// ... (GPIOC-GPIOH)
```

### SAM-Style GPIO Hardware Policy

```cpp
template <uint32_t BASE_ADDR, char PORT_CHAR>
struct SAME70GpioHardwarePolicy {
    using RegisterType = Pio;

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr char port_char = PORT_CHAR;
    static constexpr uint32_t pins_per_port = 32;

    // Pin configuration
    static inline void enable_pio(u8 pin);
    static inline void disable_pio(u8 pin);
    static inline void set_output(u8 pin);
    static inline void set_input(u8 pin);

    // Pin I/O
    static inline void set(u8 pin);
    static inline void clear(u8 pin);
    static inline void toggle(u8 pin);
    static inline bool read(u8 pin);
    static inline void write(u8 pin, bool value);

    // Port-wide operations
    static inline u32 read_port();
    static inline void set_mask(u32 mask);
    static inline void clear_mask(u32 mask);
};

// Type aliases for each port
using PIOAHardware = SAME70GpioHardwarePolicy<0x400E0E00, 'A'>;
using PIOBHardware = SAME70GpioHardwarePolicy<0x400E1000, 'B'>;
// ... (PIOC-PIOE)
```

---

## Usage Examples

### Example 1: Using with Generic GPIO API

```cpp
#include "hal/api/gpio_simple.hpp"
#include "hal/vendors/st/stm32f4/generated/platform/gpio.hpp"

using namespace alloy::hal;
using namespace alloy::hal::st::stm32f4;

// Create GPIO pin with hardware policy
auto led = Gpio::output<GPIOAHardware, 5>();  // PA5

int main() {
    led.on();      // Turn on LED
    led.off();     // Turn off LED
    led.toggle();  // Toggle LED
}
```

### Example 2: Direct Hardware Access

```cpp
#include "hal/vendors/st/stm32f4/generated/platform/gpio.hpp"

using namespace alloy::hal::st::stm32f4;

int main() {
    // Configure PA5 as output
    GPIOAHardware::set_mode(5, GPIOAHardware::MODE_OUTPUT);
    GPIOAHardware::set_output_type(5, GPIOAHardware::OTYPE_PUSH_PULL);
    GPIOAHardware::set_output_speed(5, GPIOAHardware::OSPEED_MEDIUM);

    // Set PA5 high
    GPIOAHardware::set(5);

    // Read PA6 state
    bool button_pressed = GPIOAHardware::read(6);

    // Port-wide operations
    GPIOAHardware::write_port(0xFFFF);  // Set all pins high
    GPIOAHardware::clear_mask(0x00FF);  // Clear pins 0-7
}
```

### Example 3: Alternate Function Configuration

```cpp
// Configure PA9 as USART1 TX (AF7)
GPIOAHardware::set_mode(9, GPIOAHardware::MODE_AF);
GPIOAHardware::set_alternate_function(9, 7);
GPIOAHardware::set_output_type(9, GPIOAHardware::OTYPE_PUSH_PULL);
GPIOAHardware::set_output_speed(9, GPIOAHardware::OSPEED_HIGH);

// Configure PA10 as USART1 RX (AF7)
GPIOAHardware::set_mode(10, GPIOAHardware::MODE_AF);
GPIOAHardware::set_alternate_function(10, 7);
GPIOAHardware::set_pull(10, GPIOAHardware::PUPD_PULL_UP);
```

---

## Key Differences: STM32 vs SAM GPIO

| Feature | STM32 (MODER/BSRR) | SAM (PER/OER/SODR) |
|---------|-------------------|-------------------|
| **Pins per port** | 16 | 32 |
| **Mode configuration** | MODER register (2 bits per pin) | PER/PDR registers (enable/disable) |
| **Output enable** | Part of MODER | Separate OER/ODR registers |
| **Set pin high** | BSRR (lower 16 bits) | SODR (write-only) |
| **Set pin low** | BSRR (upper 16 bits) | CODR (write-only) |
| **Read input** | IDR | PDSR |
| **Atomicity** | BSRR is atomic | SODR/CODR are atomic |
| **Alternate functions** | AFRL/AFRH (4 bits per pin) | ABCDSR1/ABCDSR2 (2 bits per pin) |
| **Pull-up/down** | PUPDR (2 bits per pin) | PUER/PUDR + PPDER/PPDDR |
| **Output type** | OTYPER (1 bit per pin) | MDER/MDDR (multi-driver) |
| **Speed control** | OSPEEDR (2 bits per pin) | Not available |

---

## Adding a New Platform

### Step 1: Create Platform Metadata

Create `tools/codegen/metadata/platforms/<platform>/platform.json`:

```json
{
  "platform": { "name": "...", "version": "1.0.0" },
  "vendor": { "name": "...", "namespace": "..." },
  "architecture": { "core": "...", "fpu": true },
  "memory": {
    "flash": { "start": "...", "size": "..." },
    "ram": { "start": "...", "size": "..." }
  }
}
```

### Step 2: Create GPIO Metadata

Create `tools/codegen/metadata/platforms/<platform>/gpio.json`:

1. **Identify GPIO style**: STM32 or SAM?
2. **List all instances**: GPIOA, GPIOB, etc.
3. **Define registers**: Offsets, sizes, access types
4. **Add capabilities**: modes, features
5. **Set template_variables**:
   - `style`: "stm32" or "sam"
   - `register_type`: RegisterType name
   - `register_include`: Path to register definitions

### Step 3: Generate GPIO Code

```bash
python3 tools/codegen/generators/gpio_generator.py <platform>
```

### Step 4: Test Generated Code

```bash
arm-none-eabi-g++ -std=c++23 -I src -fsyntax-only \
    src/hal/vendors/<vendor>/<platform>/generated/platform/gpio.hpp
```

### Step 5: Create Compile Test

Create `tests/compile_tests/test_gpio_template_<platform>.cpp` and verify:
- Compile-time constants are correct
- All methods compile
- Zero-overhead (no vtables)
- Empty Base Optimization (`sizeof(Policy) == 1`)

---

## Validation and Testing

### Schema Validation

The generator automatically validates metadata against JSON schemas:

```python
jsonschema.validate(gpio_metadata, peripheral_schema)
```

If validation fails, you'll see specific error messages:
```
❌ Validation error: 'name' is a required property
❌ Validation error: None is not of type 'number'
```

### Compile Testing

Every generated file should be tested for:

1. **Syntax correctness**: Compiles without errors
2. **Zero-overhead**: `sizeof(HardwarePolicy) == 1`
3. **No vtables**: No `_ZTV` symbols in assembly
4. **Constexpr constants**: All constants are compile-time
5. **Static inline methods**: All methods are inline

### Example Test

```cpp
// Compile-time assertions
static_assert(GPIOAHardware::base_address == 0x40020000);
static_assert(GPIOAHardware::port_char == 'A');
static_assert(GPIOAHardware::pins_per_port == 16);
static_assert(sizeof(GPIOAHardware) == 1);  // EBO
```

---

## Template Customization

### Jinja2 Template Structure

The template `templates/platform/gpio.hpp.j2` contains:

1. **Common header** (`{% include 'common/header.j2' %}`)
2. **Includes** (core, registers, bitfields)
3. **Namespace declaration**
4. **Conditional compilation** (`{% if gpio.style == 'stm32' %}`)
5. **Hardware policy definition**
6. **Type aliases** (`{% for instance in gpio.instances %}`)
7. **Usage example** (in comments)

### Key Template Variables

- `{{ platform.name }}` - Platform name (e.g., "STM32F4")
- `{{ vendor.name }}` - Vendor name (e.g., "STMicroelectronics")
- `{{ gpio.style }}` - GPIO style ("stm32" or "sam")
- `{{ gpio.instances }}` - List of GPIO instances
- `{{ gpio.pins_per_port }}` - Pins per port (16 or 32)
- `{{ gpio.register_type }}` - Register structure type
- `{{ platform_namespace }}` - Full namespace path

### Adding New GPIO Style

To support a new GPIO architecture style:

1. Add new conditional block in template:
   ```jinja2
   {% elif gpio.style == 'new_style' %}
   // New-style GPIO implementation
   {% endif %}
   ```

2. Define required methods for the style
3. Update schema to include new style in `enum`
4. Document differences in this guide

---

## Troubleshooting

### Issue: "Template not found"

**Error**: `TemplateNotFound: platform/gpio.hpp.j2`

**Solution**: Ensure you're running the generator from the correct directory:
```bash
cd tools/codegen/generators
python3 gpio_generator.py <platform>
```

### Issue: "Validation error"

**Error**: `❌ Validation error: None is not of type 'number'`

**Solution**: Check your GPIO metadata against the schema. Common issues:
- Missing required fields
- Wrong data types (string vs number)
- `null` values where numbers are required

### Issue: "Generated code doesn't compile"

**Solution**:
1. Check that `register_include` path is correct
2. Verify register type name matches actual struct
3. Ensure all register offsets are correct
4. Test with mock registers first

### Issue: "sizeof(Policy) != 1"

**Problem**: Policy class is not empty (breaks EBO)

**Solution**: Ensure policy has:
- No data members
- Only static methods
- Only static constexpr constants

---

## Performance Characteristics

### Zero-Overhead Abstraction

Generated GPIO policies are **zero-overhead**:

- **No vtables**: All methods are non-virtual
- **No runtime data**: All data is static constexpr
- **Inline expansion**: All methods are static inline
- **Compile-time evaluation**: Constants resolved at compile time
- **Empty Base Optimization**: `sizeof(Policy) == 1`

### Assembly Verification

```bash
# Generate assembly
arm-none-eabi-g++ -std=c++23 -I src -O2 -S test.cpp -o test.s

# Check for vtables (should find none)
grep "_ZTV" test.s
```

### Code Size Comparison

| Implementation | Code Size | Binary Size |
|---------------|-----------|-------------|
| Direct register access | Baseline | Baseline |
| Generated GPIO policy | +0 bytes | +0 bytes |
| Traditional HAL | +2-4 KB | +5-10 KB |

---

## References

- **Template Architecture**: See `TEMPLATE_ARCHITECTURE.md`
- **Template Conventions**: See `TEMPLATE_CONVENTIONS.md`
- **Peripheral Schema**: `tools/codegen/metadata/schema/peripheral.schema.json`
- **Platform Schema**: `tools/codegen/metadata/schema/platform.schema.json`
- **Jinja2 Documentation**: https://jinja.palletsprojects.com/

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-01-19 | Initial GPIO template guide |

---

**Next Steps**: Implement UART, SPI, and I2C templates using the same pattern.
