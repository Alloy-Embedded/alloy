# Code Generation Guide

This guide explains the Alloy HAL code generation system - how we automatically generate register access code from CMSIS-SVD files.

## Table of Contents

1. [Overview](#overview)
2. [Code Generation Pipeline](#code-generation-pipeline)
3. [Prerequisites](#prerequisites)
4. [SVD File Format](#svd-file-format)
5. [Generator Architecture](#generator-architecture)
6. [Step-by-Step Guide](#step-by-step-guide)
7. [Generated File Structure](#generated-file-structure)
8. [Customizing Generation](#customizing-generation)
9. [Validation and Testing](#validation-and-testing)
10. [Troubleshooting](#troubleshooting)
11. [Best Practices](#best-practices)

---

## Overview

### What is Code Generation?

Alloy uses **automated code generation** to create register access code from CMSIS-SVD (System View Description) files. This eliminates thousands of lines of error-prone manual register definitions.

### Benefits

- **Accuracy**: Generated directly from vendor-provided SVD files
- **Consistency**: Uniform structure across all platforms
- **Type Safety**: Strong typing for register access
- **Zero Overhead**: Compile-time masks and offsets
- **Maintainability**: Easy to update when vendors release new SVD files

### What Gets Generated?

For each MCU platform, we generate:

1. **Register Definitions** (`*_registers.hpp`) - Memory-mapped peripheral structures
2. **Bitfield Definitions** (`*_bitfields.hpp`) - Register field access with masks/offsets
3. **Hardware Policies** (`*_hardware_policy.hpp`) - Low-level register manipulation

### Time Investment

- **One-time setup**: 30 minutes (install tools)
- **Per platform**: 15-30 minutes (run generator + review)

---

## Code Generation Pipeline

```
┌─────────────────────┐
│   Vendor SVD File   │  (e.g., STM32G0B1.svd)
│  (XML Description)  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   SVD Parser        │  (Python: svd_parser.py)
│  (Extract metadata) │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Code Generator     │  (Python + Jinja2 templates)
│  (Apply templates)  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Generated Code     │  (*.hpp files marked .generated)
│  (C++ headers)      │
└─────────────────────┘
```

### Pipeline Stages

1. **SVD Parsing**: Extract peripheral, register, and field metadata from XML
2. **Data Normalization**: Clean and validate extracted data
3. **Template Application**: Apply Jinja2 templates to generate C++ code
4. **File Output**: Write generated files with `.generated` markers
5. **Validation**: Compile and test generated code

---

## Prerequisites

### Required Tools

```bash
# Python 3.8+
python3 --version

# Python packages
pip3 install cmsis-svd jinja2 pyyaml

# Build tools (for validation)
arm-none-eabi-gcc --version
cmake --version
```

### Required Files

1. **CMSIS-SVD File**: Download from vendor or [cmsis-svd repository](https://github.com/posborne/cmsis-svd)
   - Example: `STM32G0B1.svd`

2. **Generator Scripts**: Located in `tools/codegen/`
   ```
   tools/codegen/
   ├── svd_parser.py       # SVD parsing logic
   ├── generator.py        # Main generator script
   ├── templates/          # Jinja2 templates
   │   ├── registers.hpp.j2
   │   ├── bitfields.hpp.j2
   │   └── hardware_policy.hpp.j2
   └── config/             # Platform-specific configs
       └── stm32g0.yaml
   ```

3. **Platform Configuration**: YAML file with platform-specific settings

---

## SVD File Format

### What is CMSIS-SVD?

CMSIS-SVD (System View Description) is an XML-based format from ARM that describes:
- CPU architecture
- Memory maps
- Peripherals and registers
- Register fields with bit positions

### Example SVD Structure

```xml
<device>
  <name>STM32G0B1xx</name>
  <peripherals>
    <peripheral>
      <name>GPIOA</name>
      <baseAddress>0x50000000</baseAddress>
      <registers>
        <register>
          <name>MODER</name>
          <addressOffset>0x0</addressOffset>
          <fields>
            <field>
              <name>MODE0</name>
              <bitOffset>0</bitOffset>
              <bitWidth>2</bitWidth>
            </field>
          </fields>
        </register>
      </registers>
    </peripheral>
  </peripherals>
</device>
```

### Key SVD Elements

| Element | Description | Example |
|---------|-------------|---------|
| `<device>` | Top-level device description | `STM32G0B1xx` |
| `<peripheral>` | Hardware peripheral (GPIO, UART, etc.) | `GPIOA` |
| `<baseAddress>` | Peripheral base address in memory | `0x50000000` |
| `<register>` | Individual register | `MODER` |
| `<addressOffset>` | Offset from base address | `0x0` |
| `<field>` | Bit field within register | `MODE0` |
| `<bitOffset>` | Starting bit position | `0` |
| `<bitWidth>` | Number of bits | `2` |

---

## Generator Architecture

### Directory Structure

```
tools/codegen/
├── svd_parser.py           # Core SVD parsing logic
├── generator.py            # Main generator orchestrator
├── templates/              # Jinja2 templates
│   ├── registers.hpp.j2    # Register definitions template
│   ├── bitfields.hpp.j2    # Bitfield definitions template
│   └── hardware_policy.hpp.j2  # Hardware policy template
├── config/                 # Platform-specific configurations
│   ├── stm32f4.yaml
│   ├── stm32f7.yaml
│   ├── stm32g0.yaml
│   └── same70.yaml
└── utils/                  # Helper utilities
    ├── naming.py           # Name sanitization
    └── validators.py       # Generated code validation
```

### Component Responsibilities

#### 1. SVD Parser (`svd_parser.py`)

Parses SVD XML and extracts structured data:

```python
class SvdParser:
    def parse(self, svd_file: Path) -> DeviceInfo:
        """Parse SVD file and return device metadata"""
        device = self._parse_xml(svd_file)
        return DeviceInfo(
            name=device.name,
            peripherals=self._extract_peripherals(device),
            registers=self._extract_registers(device),
            fields=self._extract_fields(device)
        )
```

#### 2. Code Generator (`generator.py`)

Applies templates to generate C++ code:

```python
class CodeGenerator:
    def __init__(self, config: PlatformConfig):
        self.config = config
        self.env = jinja2.Environment(
            loader=jinja2.FileSystemLoader('templates/')
        )

    def generate_registers(self, peripheral: Peripheral) -> str:
        """Generate register definitions for a peripheral"""
        template = self.env.get_template('registers.hpp.j2')
        return template.render(peripheral=peripheral)
```

#### 3. Templates (Jinja2)

Define the structure of generated C++ code:

```jinja2
{# registers.hpp.j2 #}
namespace {{ namespace }}::{{ peripheral_name | lower }} {

// {{ peripheral_name }} Registers
struct {{ peripheral_name }}_Registers {
    {% for register in registers %}
    volatile uint32_t {{ register.name }};  // Offset: {{ register.offset }}
    {% endfor %}
};

inline auto {{ peripheral_name }}() -> {{ peripheral_name }}_Registers* {
    return reinterpret_cast<{{ peripheral_name }}_Registers*>({{ base_address }});
}

} // namespace
```

---

## Step-by-Step Guide

### Step 1: Obtain SVD File

Download the CMSIS-SVD file for your target MCU:

```bash
# Option 1: Download from vendor website (ST, Microchip, etc.)
# Example: https://www.st.com/en/microcontrollers-microprocessors/stm32g0-series.html

# Option 2: Clone cmsis-svd repository
git clone https://github.com/posborne/cmsis-svd.git
cd cmsis-svd/data/STMicro/

# Copy SVD to project
cp STM32G0B1.svd /path/to/alloy/tools/codegen/svd/
```

### Step 2: Create Platform Configuration

Create a YAML config file defining platform-specific settings:

```yaml
# tools/codegen/config/stm32g0.yaml
platform:
  name: stm32g0
  vendor: st
  family: stm32g0

svd:
  file: svd/STM32G0B1.svd

output:
  base_path: src/hal/vendors/st/stm32g0/generated

namespaces:
  - alloy
  - hal
  - st
  - stm32g0

peripherals:
  # List of peripherals to generate (or 'all')
  - GPIOA
  - GPIOB
  - GPIOC
  - GPIOD
  - GPIOE
  - GPIOF
  - RCC
  - FLASH
  - USART1
  - USART2
  - SPI1
  - SPI2
  - I2C1
  - I2C2
  - TIM1
  - TIM2
  - ADC
  - DMA1

naming:
  # Naming conventions
  register_suffix: _Registers
  bitfield_suffix: _Bitfields

generation:
  # What to generate
  registers: true
  bitfields: true
  hardware_policies: true

options:
  # Code generation options
  add_comments: true
  add_safety_checks: true
  use_constexpr: true
```

### Step 3: Run Code Generator

Execute the generator with your configuration:

```bash
cd tools/codegen

# Generate code for STM32G0
python3 generator.py --config config/stm32g0.yaml

# Output:
# ✓ Parsing SVD file: svd/STM32G0B1.svd
# ✓ Extracted 45 peripherals
# ✓ Generating registers for GPIOA... done
# ✓ Generating bitfields for GPIOA... done
# ✓ Generating registers for RCC... done
# ✓ Generating bitfields for RCC... done
# ...
# ✓ Generated 90 files in src/hal/vendors/st/stm32g0/generated/
```

### Step 4: Review Generated Code

Check the generated files:

```bash
# View generated structure
tree src/hal/vendors/st/stm32g0/generated/

# Example output:
# src/hal/vendors/st/stm32g0/generated/
# ├── registers/
# │   ├── gpioa_registers.hpp
# │   ├── rcc_registers.hpp
# │   ├── flash_registers.hpp
# │   └── ...
# └── bitfields/
#     ├── gpioa_bitfields.hpp
#     ├── rcc_bitfields.hpp
#     ├── flash_bitfields.hpp
#     └── ...
```

### Step 5: Verify Generated Code

Inspect a generated register file:

```cpp
// src/hal/vendors/st/stm32g0/generated/registers/gpioa_registers.hpp
// AUTO-GENERATED - DO NOT EDIT
// Generated from: STM32G0B1.svd
// Generated on: 2025-01-15

#pragma once
#include <cstdint>

namespace alloy::hal::st::stm32g0::gpioa {

// GPIOA Register Block
struct GPIOA_Registers {
    volatile uint32_t MODER;    // Offset: 0x00 - GPIO port mode register
    volatile uint32_t OTYPER;   // Offset: 0x04 - GPIO port output type register
    volatile uint32_t OSPEEDR;  // Offset: 0x08 - GPIO port output speed register
    volatile uint32_t PUPDR;    // Offset: 0x0C - GPIO port pull-up/pull-down register
    volatile uint32_t IDR;      // Offset: 0x10 - GPIO port input data register
    volatile uint32_t ODR;      // Offset: 0x14 - GPIO port output data register
    volatile uint32_t BSRR;     // Offset: 0x18 - GPIO port bit set/reset register
    volatile uint32_t LCKR;     // Offset: 0x1C - GPIO port configuration lock register
    volatile uint32_t AFRL;     // Offset: 0x20 - GPIO alternate function low register
    volatile uint32_t AFRH;     // Offset: 0x24 - GPIO alternate function high register
    volatile uint32_t BRR;      // Offset: 0x28 - GPIO port bit reset register
};

// Base address
static constexpr uint32_t BASE = 0x50000000;

// Accessor function
inline auto GPIOA() -> GPIOA_Registers* {
    return reinterpret_cast<GPIOA_Registers*>(BASE);
}

} // namespace alloy::hal::st::stm32g0::gpioa
```

Inspect a generated bitfield file:

```cpp
// src/hal/vendors/st/stm32g0/generated/bitfields/gpioa_bitfields.hpp
// AUTO-GENERATED - DO NOT EDIT
// Generated from: STM32G0B1.svd

#pragma once
#include <cstdint>

namespace alloy::hal::st::stm32g0::gpioa {

// MODER Register Bitfields
namespace moder {
    // MODE0: Port x configuration bits (y = 0..15)
    struct MODE0 {
        static constexpr uint32_t offset = 0;
        static constexpr uint32_t width = 2;
        static constexpr uint32_t mask = 0x3 << offset;

        static uint32_t read(uint32_t reg) {
            return (reg & mask) >> offset;
        }

        static uint32_t write(uint32_t reg, uint32_t value) {
            return (reg & ~mask) | ((value << offset) & mask);
        }
    };

    // MODE1: Port x configuration bits (y = 0..15)
    struct MODE1 {
        static constexpr uint32_t offset = 2;
        static constexpr uint32_t width = 2;
        static constexpr uint32_t mask = 0x3 << offset;

        static uint32_t read(uint32_t reg) {
            return (reg & mask) >> offset;
        }

        static uint32_t write(uint32_t reg, uint32_t value) {
            return (reg & ~mask) | ((value << offset) & mask);
        }
    };

    // ... MODE2-MODE15 ...
}

// BSRR Register Bitfields
namespace bsrr {
    // BS0: Port x set bit y (y= 0..15)
    struct BS0 {
        static constexpr uint32_t offset = 0;
        static constexpr uint32_t width = 1;
        static constexpr uint32_t mask = 0x1 << offset;
    };

    // ... BS1-BS15 ...

    // BR0: Port x reset bit y (y = 0..15)
    struct BR0 {
        static constexpr uint32_t offset = 16;
        static constexpr uint32_t width = 1;
        static constexpr uint32_t mask = 0x1 << offset;
    };

    // ... BR1-BR15 ...
}

} // namespace alloy::hal::st::stm32g0::gpioa
```

### Step 6: Build and Test

Verify the generated code compiles:

```bash
# Configure build with platform
cmake -B build-test \
      -DALLOY_PLATFORM=stm32g0 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

# Build
cmake --build build-test

# Should compile without errors
```

### Step 7: Create Hardware Policy

Write a hardware policy using the generated code (this is manual):

```cpp
// src/hal/vendors/st/stm32g0/gpio_hardware_policy.hpp
#pragma once

#include "hal/vendors/st/stm32g0/generated/registers/gpioa_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/gpioa_bitfields.hpp"
// ... include other GPIO ports ...

namespace alloy::hal::st::stm32g0 {

template <uint32_t PORT_BASE, uint32_t CLOCK_FREQ>
class Stm32g0GPIOHardwarePolicy {
public:
    // Set output HIGH
    static void set_output(uint32_t pin_mask) {
        auto gpio = get_gpio_port(PORT_BASE);
        gpio->BSRR = pin_mask;  // Set bits
    }

    // Set output LOW
    static void clear_output(uint32_t pin_mask) {
        auto gpio = get_gpio_port(PORT_BASE);
        gpio->BSRR = (pin_mask << 16);  // Reset bits
    }

    // Toggle output
    static void toggle_output(uint32_t pin_mask) {
        auto gpio = get_gpio_port(PORT_BASE);
        gpio->ODR ^= pin_mask;
    }

    // Read input
    static bool read_input(uint32_t pin_mask) {
        auto gpio = get_gpio_port(PORT_BASE);
        return (gpio->IDR & pin_mask) != 0;
    }

    // Set mode to output
    static void set_mode_output(uint8_t pin_num) {
        auto gpio = get_gpio_port(PORT_BASE);
        uint32_t moder = gpio->MODER;
        moder &= ~(0x3 << (pin_num * 2));  // Clear mode bits
        moder |= (0x1 << (pin_num * 2));   // Set to output (01)
        gpio->MODER = moder;
    }

    // ... more methods ...

private:
    static auto get_gpio_port(uint32_t base) {
        if (base == gpioa::BASE) return gpioa::GPIOA();
        if (base == gpiob::BASE) return gpiob::GPIOB();
        // ... other ports ...
        return gpioa::GPIOA();  // Default
    }
};

} // namespace
```

---

## Generated File Structure

### Complete Platform Structure

```
src/hal/vendors/st/stm32g0/
├── generated/                      # All generated code
│   ├── .generated                  # Marker file (DO NOT EDIT)
│   ├── registers/                  # Register definitions
│   │   ├── gpioa_registers.hpp
│   │   ├── gpiob_registers.hpp
│   │   ├── rcc_registers.hpp
│   │   ├── flash_registers.hpp
│   │   ├── usart1_registers.hpp
│   │   └── ...
│   └── bitfields/                  # Bitfield definitions
│       ├── gpioa_bitfields.hpp
│       ├── gpiob_bitfields.hpp
│       ├── rcc_bitfields.hpp
│       ├── flash_bitfields.hpp
│       ├── usart1_bitfields.hpp
│       └── ...
├── gpio_hardware_policy.hpp        # Manual: GPIO policy
├── gpio.hpp                        # Manual: GPIO API
├── clock_platform.hpp              # Manual: Clock API
├── systick_platform.hpp            # Manual: SysTick API
└── stm32g0b1/                      # Manual: Device-specific
    ├── peripherals.hpp
    └── linker.ld
```

### File Naming Conventions

| Type | Pattern | Example |
|------|---------|---------|
| Register file | `<peripheral>_registers.hpp` | `gpioa_registers.hpp` |
| Bitfield file | `<peripheral>_bitfields.hpp` | `gpioa_bitfields.hpp` |
| Namespace | `<peripheral>` (lowercase) | `gpioa`, `rcc` |
| Struct name | `<PERIPHERAL>_Registers` | `GPIOA_Registers` |

### .generated Marker

The `.generated` file marks a directory as containing auto-generated code:

```
# .generated
# This directory contains auto-generated code.
# DO NOT EDIT FILES IN THIS DIRECTORY.
#
# Generated from: STM32G0B1.svd
# Generated on: 2025-01-15
# Generator version: 1.0
```

Purpose:
- Warns developers not to manually edit
- Documents generation source
- Used by tooling to detect generated code

---

## Customizing Generation

### Modifying Templates

Templates are located in `tools/codegen/templates/`. You can customize the generated code format.

#### Example: Add More Documentation

Edit `registers.hpp.j2`:

```jinja2
/**
 * @file {{ peripheral_name | lower }}_registers.hpp
 * @brief {{ peripheral_name }} Register Definitions
 *
 * AUTO-GENERATED - DO NOT EDIT
 * Generated from: {{ svd_file }}
 * Generated on: {{ generation_date }}
 *
 * @note This file provides memory-mapped access to {{ peripheral_name }} registers.
 */

#pragma once
#include <cstdint>

namespace {{ namespace }}::{{ peripheral_name | lower }} {

/**
 * @brief {{ peripheral_name }} Register Block
 * @details Base Address: {{ base_address }}
 */
struct {{ peripheral_name }}_Registers {
    {% for register in registers %}
    /**
     * @brief {{ register.description }}
     * @offset {{ register.offset }}
     * @reset {{ register.reset_value }}
     */
    volatile uint32_t {{ register.name }};
    {% endfor %}
};

// ... rest of template ...
```

#### Example: Add Safety Checks

Edit `bitfields.hpp.j2` to add runtime assertions:

```jinja2
static uint32_t write(uint32_t reg, uint32_t value) {
    #ifdef ALLOY_BITFIELD_CHECKS
    // Verify value fits in field width
    if (value >= (1u << width)) {
        // Handle error - value too large for field
        return reg;  // Return unchanged
    }
    #endif
    return (reg & ~mask) | ((value << offset) & mask);
}
```

### Adding Custom Filters

Add custom Jinja2 filters in `generator.py`:

```python
def to_hex(value: int) -> str:
    """Format integer as hex string"""
    return f"0x{value:08X}"

def to_camel_case(name: str) -> str:
    """Convert snake_case to CamelCase"""
    return ''.join(word.capitalize() for word in name.split('_'))

# Register filters
env.filters['hex'] = to_hex
env.filters['camel'] = to_camel_case
```

Use in templates:

```jinja2
static constexpr uint32_t BASE = {{ base_address | hex }};
struct {{ peripheral_name | camel }}_Registers { /* ... */ };
```

### Platform-Specific Overrides

Override default behavior in platform config:

```yaml
# config/stm32g0.yaml
overrides:
  # Custom base addresses (if SVD is incorrect)
  GPIOA:
    base_address: 0x50000000

  # Skip certain peripherals
  skip_peripherals:
    - USB_OTG_FS  # Not available on this variant

  # Custom register types
  register_types:
    GPIOA:
      BSRR: write_only  # Mark as write-only
      IDR: read_only    # Mark as read-only
```

---

## Validation and Testing

### Automated Validation

The generator includes validation steps:

1. **SVD Schema Validation**: Verify SVD file is valid XML
2. **Address Overlap Detection**: Ensure registers don't overlap
3. **Bitfield Bounds Checking**: Verify fields fit within registers
4. **Name Collision Detection**: Detect duplicate names

Run validation:

```bash
python3 generator.py --config config/stm32g0.yaml --validate-only

# Output:
# ✓ SVD schema validation passed
# ✓ No address overlaps detected
# ✓ All bitfields within register bounds
# ✓ No name collisions found
# ✓ Validation complete - 0 errors, 2 warnings
#
# Warnings:
#   - GPIOA.MODER: Missing reset value
#   - RCC.CR: Field HSIRDY is read-only but no access attribute
```

### Compilation Testing

Test that generated code compiles:

```bash
# Create minimal test
cat > test_generated.cpp << 'EOF'
#include "hal/vendors/st/stm32g0/generated/registers/gpioa_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/gpioa_bitfields.hpp"

int main() {
    using namespace alloy::hal::st::stm32g0;

    // Test register access
    auto gpio = gpioa::GPIOA();
    gpio->MODER = 0x12345678;

    // Test bitfield access
    uint32_t mode = gpioa::moder::MODE0::read(gpio->MODER);
    gpio->MODER = gpioa::moder::MODE0::write(gpio->MODER, 1);

    return 0;
}
EOF

# Compile
arm-none-eabi-g++ -c test_generated.cpp \
    -I src \
    -std=c++20 \
    -mcpu=cortex-m0plus \
    -mthumb \
    -Wall -Wextra -Werror

# Should compile without errors
```

### Runtime Testing

Create a test that runs on hardware:

```cpp
// test/hardware/test_gpio_generated.cpp
#include "unity.h"
#include "hal/vendors/st/stm32g0/gpio.hpp"

void test_gpio_set_clear() {
    using namespace alloy::hal::st::stm32g0;
    using LedPin = GpioPin<gpioa::BASE, 5>;

    LedPin led;
    led.setDirection(PinDirection::Output);

    // Test set
    led.set();
    TEST_ASSERT_TRUE(led.read().unwrap());

    // Test clear
    led.clear();
    TEST_ASSERT_FALSE(led.read().unwrap());

    // Test toggle
    led.toggle();
    TEST_ASSERT_TRUE(led.read().unwrap());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_set_clear);
    return UNITY_END();
}
```

---

## Troubleshooting

### Problem: Generator Crashes on SVD Parse

**Symptoms**:
```
Error: Failed to parse SVD file
XML parse error at line 1234
```

**Solutions**:
1. Validate SVD file XML syntax:
   ```bash
   xmllint --schema svd.xsd STM32G0B1.svd
   ```

2. Check SVD file encoding (must be UTF-8):
   ```bash
   file -I STM32G0B1.svd
   # Should show: charset=utf-8
   ```

3. Try different SVD source (vendor vs. cmsis-svd repo)

### Problem: Missing Peripherals in Output

**Symptoms**:
```
Warning: Peripheral 'USART3' not found in SVD
Skipping USART3...
```

**Solutions**:
1. Check SVD file contains the peripheral:
   ```bash
   grep -i "USART3" STM32G0B1.svd
   ```

2. Verify peripheral name spelling in config YAML

3. Some variants don't have all peripherals - check datasheet

### Problem: Bitfield Widths Incorrect

**Symptoms**:
```cpp
// Generated with wrong width
struct MODE0 {
    static constexpr uint32_t width = 1;  // Should be 2!
};
```

**Solutions**:
1. Check SVD `<bitWidth>` element:
   ```xml
   <field>
       <name>MODE0</name>
       <bitOffset>0</bitOffset>
       <bitWidth>2</bitWidth>  <!-- Should be 2 -->
   </field>
   ```

2. Add override in platform config:
   ```yaml
   overrides:
     GPIOA:
       MODER:
         MODE0:
           width: 2
   ```

3. Report issue to SVD vendor if incorrect

### Problem: Compilation Errors with Generated Code

**Symptoms**:
```
error: 'GPIOA_Registers' does not name a type
error: 'gpioa' has not been declared
```

**Solutions**:
1. Check include paths in CMakeLists.txt:
   ```cmake
   target_include_directories(my_target PRIVATE
       ${PROJECT_SOURCE_DIR}/src
       ${PROJECT_SOURCE_DIR}/src/hal/vendors/st/stm32g0/generated
   )
   ```

2. Verify namespace structure:
   ```cpp
   // Correct:
   using namespace alloy::hal::st::stm32g0::gpioa;

   // Incorrect:
   using namespace gpioa;  // Missing parent namespaces
   ```

3. Regenerate code if templates changed

### Problem: Name Collisions

**Symptoms**:
```
error: redefinition of 'struct CR'
note: previous definition here
```

**Solutions**:
1. SVD has duplicate names - add uniquification in config:
   ```yaml
   naming:
     uniquify_registers: true
     uniquify_fields: true
   ```

2. Manually rename in override:
   ```yaml
   overrides:
     RCC:
       CR:
         rename: CR_CONTROL
   ```

---

## Best Practices

### 1. Version Control for SVD Files

Keep SVD files under version control:

```
tools/codegen/svd/
├── STM32F4xx.svd
├── STM32F7xx.svd
├── STM32G0B1.svd
└── README.md  # Document SVD versions and sources
```

Document SVD provenance:

```markdown
# SVD Files

## STM32G0B1.svd
- **Source**: ST Microelectronics official website
- **Version**: 1.7
- **Date**: 2023-11-15
- **URL**: https://www.st.com/resource/en/svd/stm32g0_svd.zip
- **SHA256**: a1b2c3d4...
```

### 2. Regeneration Workflow

Create a script to regenerate all platforms:

```bash
#!/bin/bash
# tools/codegen/regenerate_all.sh

set -e

echo "Regenerating all platforms..."

for config in config/*.yaml; do
    platform=$(basename "$config" .yaml)
    echo "Generating $platform..."
    python3 generator.py --config "$config"
done

echo "Done! Remember to rebuild and test."
```

Run after updating SVD files or templates:

```bash
./tools/codegen/regenerate_all.sh
```

### 3. Review Generated Code

Always review generated code before committing:

```bash
# Generate
python3 generator.py --config config/stm32g0.yaml

# Review changes
git diff src/hal/vendors/st/stm32g0/generated/

# Look for:
# - Unexpected register addresses
# - Missing peripherals
# - Incorrect bitfield widths
# - Namespace issues
```

### 4. Separate Manual and Generated Code

**Never mix manual edits with generated code:**

- Generated: `src/hal/vendors/st/stm32g0/generated/`
- Manual: `src/hal/vendors/st/stm32g0/*.hpp`

If you need to customize, create a wrapper:

```cpp
// Manual file: gpio_custom.hpp
#include "generated/registers/gpioa_registers.hpp"

namespace alloy::hal::st::stm32g0 {

// Custom extensions
namespace gpioa {
    // Add helper functions
    inline void set_pin_fast(uint8_t pin) {
        GPIOA()->BSRR = (1u << pin);
    }
}

} // namespace
```

### 5. Document Generation Process

Add generation metadata to platform README:

```markdown
# STM32G0 Platform

## Code Generation

Generated code in `generated/` was created using:

- **SVD File**: STM32G0B1.svd (v1.7)
- **Generator**: Alloy codegen v1.0
- **Generated**: 2025-01-15
- **Command**: `python3 generator.py --config config/stm32g0.yaml`

To regenerate:
```bash
cd tools/codegen
./regenerate_all.sh
```

### 6. Test After Generation

Always compile and test after generation:

```bash
# Regenerate
./tools/codegen/regenerate_all.sh

# Clean build
rm -rf build-*

# Build all platforms
./scripts/build_all_platforms.sh

# Run unit tests
./scripts/run_unit_tests.sh

# If all pass, commit
git add src/hal/vendors/*/generated/
git commit -m "Regenerate platform code from updated SVD files"
```

### 7. Use Stable SVD Versions

Prefer stable, released SVD files over bleeding-edge versions:

- Vendor official releases (recommended)
- Tagged releases from cmsis-svd repo
- Avoid development branches

Document any patches or fixes:

```yaml
# config/stm32g0.yaml
svd:
  file: svd/STM32G0B1.svd
  patches:
    - fix_gpioa_base_address.patch
    - add_missing_usart3.patch
```

---

## Summary

The Alloy code generation system:

1. **Parses** CMSIS-SVD files using Python
2. **Extracts** peripheral, register, and bitfield metadata
3. **Applies** Jinja2 templates to generate C++ headers
4. **Outputs** type-safe register and bitfield definitions
5. **Validates** generated code for correctness

### Key Files

| File | Purpose |
|------|---------|
| `tools/codegen/generator.py` | Main generator script |
| `tools/codegen/svd_parser.py` | SVD parsing logic |
| `tools/codegen/templates/*.j2` | Code generation templates |
| `tools/codegen/config/*.yaml` | Platform configurations |
| `src/hal/vendors/*/generated/` | Generated code output |

### Workflow Summary

```
1. Obtain SVD file
2. Create platform config YAML
3. Run generator
4. Review generated code
5. Write hardware policies (manual)
6. Build and test
7. Commit
```

### Next Steps

- **For new platforms**: Follow [PORTING_NEW_PLATFORM.md](PORTING_NEW_PLATFORM.md)
- **For new boards**: Follow [PORTING_NEW_BOARD.md](PORTING_NEW_BOARD.md)
- **For architecture**: Read [ARCHITECTURE.md](ARCHITECTURE.md)

---

**Note**: This guide assumes familiarity with C++, embedded systems, and basic Python. For questions or issues, see the troubleshooting section or open an issue on the project repository.
