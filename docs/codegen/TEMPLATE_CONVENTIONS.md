# Template System Conventions

**Version**: 1.0.0
**Date**: 2025-01-19
**Status**: Phase 2.1 - Template Architecture Design

---

## Overview

This document defines the conventions and best practices for the Alloy HAL template system. The template system uses Jinja2 to generate platform-specific code from metadata.

---

## Table of Contents

1. [Directory Structure](#directory-structure)
2. [Naming Conventions](#naming-conventions)
3. [Template Variable Schema](#template-variable-schema)
4. [Template Hierarchy](#template-hierarchy)
5. [Metadata Organization](#metadata-organization)
6. [Best Practices](#best-practices)
7. [Examples](#examples)

---

## Directory Structure

```
tools/codegen/
├── templates/              # Jinja2 templates
│   ├── common/            # Common templates (headers, footers)
│   ├── platform/          # Platform-specific templates
│   │   ├── gpio.hpp.j2
│   │   ├── uart.hpp.j2
│   │   ├── spi.hpp.j2
│   │   └── i2c.hpp.j2
│   ├── peripheral/        # Peripheral templates
│   ├── startup/           # Startup code templates
│   ├── linker/            # Linker script templates
│   └── registers/         # Register definition templates
│
├── metadata/              # Metadata files
│   ├── schema/            # JSON schemas
│   │   ├── platform.schema.json
│   │   ├── peripheral.schema.json
│   │   └── board.schema.json
│   ├── platforms/         # Platform metadata
│   │   ├── stm32f4/
│   │   │   ├── platform.json
│   │   │   ├── gpio.json
│   │   │   └── uart.json
│   │   └── same70/
│   │       ├── platform.json
│   │       └── gpio.json
│   └── boards/            # Board metadata
│       ├── nucleo_f446re.json
│       └── arduino_due.json
│
└── generators/            # Template rendering scripts
    ├── platform_generator.py
    ├── peripheral_generator.py
    └── board_generator.py
```

---

## Naming Conventions

### Template Files

- **Extension**: `.j2` or `.jinja2`
- **Format**: `<type>_<name>.<ext>.j2`
- **Examples**:
  - `gpio.hpp.j2` - GPIO header template
  - `uart_hardware_policy.hpp.j2` - UART hardware policy
  - `cortex_m_startup.cpp.j2` - Startup code
  - `cortex_m.ld.j2` - Linker script

### Metadata Files

- **Extension**: `.json`
- **Format**: `<category>.json` or `<instance>.json`
- **Examples**:
  - `platform.json` - Platform metadata
  - `gpio.json` - GPIO peripheral metadata
  - `nucleo_f446re.json` - Board metadata

### Generated Files

- **Format**: Match original filename without `.j2` extension
- **Examples**:
  - `gpio.hpp.j2` → `gpio.hpp`
  - `uart_hardware_policy.hpp.j2` → `uart_hardware_policy.hpp`

### Variable Names

#### Jinja2 Variables

- **snake_case** for all variables
- **Descriptive names** (avoid abbreviations)

```jinja2
# Good
{{ peripheral_name }}
{{ base_address }}
{{ register_type }}

# Bad
{{ pName }}
{{ baseAddr }}
{{ regType }}
```

#### C++ Generated Code

- **PascalCase** for classes/structs
- **snake_case** for functions/variables
- **SCREAMING_SNAKE_CASE** for macros/constants

```cpp
// Good
class GpioPin { };
void set_output();
static constexpr uint32_t BASE_ADDRESS = 0x40020000;

// Bad
class gpioPin { };
void SetOutput();
static constexpr uint32_t base_address = 0x40020000;
```

---

## Template Variable Schema

### Platform Variables

Available in all platform templates:

```jinja2
{{ platform.name }}          # e.g., "STM32F4"
{{ platform.version }}       # e.g., "1.0.0"
{{ vendor.name }}            # e.g., "STMicroelectronics"
{{ vendor.namespace }}       # e.g., "stm32"
{{ family.name }}            # e.g., "STM32F4"
{{ family.series }}          # e.g., "F4"
{{ mcu.name }}               # e.g., "STM32F407VGT6"
{{ mcu.part_number }}        # Full part number
{{ architecture.core }}      # e.g., "Cortex-M4"
{{ architecture.fpu }}       # true/false
{{ architecture.frequency_max.value }}  # e.g., 168
{{ architecture.frequency_max.unit }}   # e.g., "MHz"
{{ memory.flash.start }}     # e.g., "0x08000000"
{{ memory.flash.size }}      # e.g., "1024K"
{{ memory.ram.start }}       # e.g., "0x20000000"
{{ memory.ram.size }}        # e.g., "192K"
```

### Peripheral Variables

Available in peripheral templates:

```jinja2
{{ peripheral.name }}        # e.g., "GPIO", "USART"
{{ peripheral.type }}        # e.g., "GPIO", "UART"
{{ instances }}              # Array of peripheral instances
{{ instances[0].name }}      # e.g., "USART1"
{{ instances[0].base_address }}  # e.g., "0x40011000"
{{ instances[0].irq_number }}    # IRQ number
{{ registers }}              # Register definitions
{{ registers.CR1.offset }}   # Register offset
{{ registers.CR1.size }}     # Register size in bits
{{ registers.CR1.fields }}   # Bitfield definitions
```

### Board Variables

Available in board templates:

```jinja2
{{ board.name }}             # e.g., "Nucleo-F446RE"
{{ board.manufacturer }}     # e.g., "STMicroelectronics"
{{ mcu.platform }}           # References platform.json
{{ pins.gpio.LED1.port }}    # e.g., "GPIOA"
{{ pins.gpio.LED1.pin }}     # e.g., 5
{{ peripherals.uart[0].instance }}  # e.g., "USART2"
{{ peripherals.uart[0].tx_pin }}    # e.g., "PA2"
{{ components.oscillators.hse.frequency_hz }}  # e.g., 8000000
```

### Template-Specific Variables

Additional variables for specific templates:

```jinja2
# Code generation metadata
{{ generation_date }}        # ISO 8601 date
{{ generator_name }}         # Script name
{{ generator_version }}      # Generator version

# Include paths
{{ register_include }}       # Path to registers
{{ bitfield_include }}       # Path to bitfields

# Namespaces
{{ platform_namespace }}     # e.g., "alloy::hal::stm32f4"
{{ vendor_namespace }}       # e.g., "stm32"
```

---

## Template Hierarchy

### Base Templates

Located in `templates/common/`:
- `header.j2` - File header with copyright and metadata
- `namespace_begin.j2` - Namespace opening
- `namespace_end.j2` - Namespace closing

### Platform Templates

Located in `templates/platform/`:
- `gpio.hpp.j2` - GPIO peripheral
- `uart.hpp.j2` - UART peripheral
- `spi.hpp.j2` - SPI peripheral
- `i2c.hpp.j2` - I2C peripheral
- `adc.hpp.j2` - ADC peripheral
- `timer.hpp.j2` - Timer peripheral
- `dma.hpp.j2` - DMA controller

### Startup Templates

Located in `templates/startup/`:
- `cortex_m_startup.cpp.j2` - Startup code
- `vector_table.cpp.j2` - Interrupt vector table

### Linker Templates

Located in `templates/linker/`:
- `cortex_m.ld.j2` - Linker script

---

## Metadata Organization

### Hierarchy

```
Platform (MCU/SoC)
  ├── Peripherals (GPIO, UART, SPI, etc.)
  └── Capabilities (FPU, MPU, etc.)

Board
  ├── MCU (references Platform)
  ├── Pin Assignments
  ├── Peripheral Configurations
  └── Components (oscillators, sensors, etc.)
```

### Schema Validation

All metadata files MUST validate against their respective JSON schemas:
- `platform.schema.json` - Platform metadata
- `peripheral.schema.json` - Peripheral metadata
- `board.schema.json` - Board metadata

### Required Fields

#### Platform Metadata

```json
{
  "platform": { "name": "...", "version": "..." },
  "vendor": { "name": "...", "namespace": "..." },
  "family": { "name": "...", "series": "..." },
  "mcu": { "name": "...", "part_number": "..." },
  "architecture": { "core": "...", "endianness": "..." },
  "memory": {
    "flash": { "start": "...", "size": "..." },
    "ram": { "start": "...", "size": "..." }
  },
  "peripherals": { }
}
```

#### Peripheral Metadata

```json
{
  "peripheral": { "name": "...", "type": "..." },
  "instances": [
    { "name": "...", "base_address": "..." }
  ],
  "registers": { }
}
```

#### Board Metadata

```json
{
  "board": { "name": "...", "version": "..." },
  "mcu": { "platform": "...", "part_number": "..." },
  "pins": { }
}
```

---

## Best Practices

### Template Design

1. **Keep templates focused**: One peripheral/feature per template
2. **Use includes**: Share common code via `{% include %}`
3. **Add comments**: Explain complex logic
4. **Validate inputs**: Check for required variables
5. **Handle optionals**: Use `{% if var %}` for optional features

### Metadata Design

1. **Complete documentation**: Every field should have clear purpose
2. **Consistent naming**: Follow naming conventions strictly
3. **Validate early**: Use JSON schema validation
4. **Version metadata**: Track changes with version fields
5. **Cross-reference**: Use references instead of duplication

### Code Generation

1. **Idempotent**: Re-running should produce same output
2. **Deterministic**: Same input = same output
3. **Readable**: Generated code should be human-readable
4. **Documented**: Add generation metadata in headers
5. **Compile-tested**: Validate generated code compiles

### Performance

1. **Compile-time**: Prefer `constexpr` and `static inline`
2. **Zero-overhead**: No virtual functions in generated code
3. **Type-safe**: Use templates and concepts
4. **Memory-efficient**: Avoid runtime allocations

---

## Examples

### Example 1: Platform Template

```jinja2
{# gpio.hpp.j2 #}
{% include 'common/header.j2' %}

#pragma once

#include "core/types.hpp"
#include "{{ register_include }}"

namespace {{ platform_namespace }} {

/**
 * @brief {{ peripheral.name }} for {{ platform.name }}
 */
class Gpio {
public:
    {% for instance in instances %}
    /// {{ instance.name }} base address
    static constexpr uint32_t {{ instance.name }}_BASE = {{ instance.base_address }};
    {% endfor %}
};

}  // namespace {{ platform_namespace }}
```

### Example 2: Platform Metadata

```json
{
  "platform": {
    "name": "STM32F4",
    "version": "1.0.0"
  },
  "vendor": {
    "name": "STMicroelectronics",
    "namespace": "stm32"
  },
  "family": {
    "name": "STM32F4",
    "series": "F4"
  },
  "mcu": {
    "name": "STM32F407VGT6",
    "part_number": "STM32F407VGT6"
  },
  "architecture": {
    "core": "Cortex-M4",
    "endianness": "little",
    "fpu": true,
    "fpu_type": "FPv4-SP"
  },
  "memory": {
    "flash": {
      "start": "0x08000000",
      "size": "1024K",
      "size_bytes": 1048576
    },
    "ram": {
      "start": "0x20000000",
      "size": "192K",
      "size_bytes": 196608
    }
  }
}
```

### Example 3: Peripheral Metadata

```json
{
  "peripheral": {
    "name": "GPIO",
    "type": "GPIO"
  },
  "instances": [
    {
      "name": "GPIOA",
      "base_address": "0x40020000",
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
      "reset_value": "0x00000000"
    }
  }
}
```

---

## Validation Workflow

1. **Write metadata** following schema
2. **Validate JSON** against schema:
   ```bash
   jsonschema -i platform.json platform.schema.json
   ```
3. **Generate code** from template:
   ```bash
   python generators/platform_generator.py
   ```
4. **Compile generated code**:
   ```bash
   arm-none-eabi-g++ -c generated/gpio.hpp
   ```
5. **Review output** for correctness

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-01-19 | Initial template conventions document |

---

**Next Steps**: See `GPIO_TEMPLATE_GUIDE.md` for GPIO-specific template details.
