# Template Authoring Guide

## Overview

This guide explains how to create and maintain Jinja2 templates for the Alloy code generation system. Templates transform metadata JSON into C++ HAL code, register definitions, startup files, and linker scripts.

## Table of Contents

1. [Template Basics](#template-basics)
2. [Available Context](#available-context)
3. [Custom Filters](#custom-filters)
4. [Template Types](#template-types)
5. [Best Practices](#best-practices)
6. [Examples](#examples)
7. [Testing](#testing)

---

## Template Basics

### Directory Structure

```
tools/codegen/templates/
├── registers/
│   └── register_struct.hpp.j2
├── bitfields/
│   └── bitfield_enum.hpp.j2
├── platform/
│   ├── gpio.hpp.j2
│   ├── uart.hpp.j2
│   ├── spi.hpp.j2
│   └── i2c.hpp.j2
├── startup/
│   └── startup.cpp.j2
└── linker/
    └── cortex_m.ld.j2
```

### File Naming Convention

- **Extension:** `.j2` for Jinja2 templates
- **Output extension:** Match intended output (`.hpp.j2` → `.hpp`, `.cpp.j2` → `.cpp`)
- **Naming:** Descriptive of purpose (e.g., `gpio.hpp.j2`, `startup.cpp.j2`)

### Jinja2 Syntax

```jinja2
{# Comments - won't appear in output #}

{{ variable }}                    {# Variable substitution #}
{% if condition %}...{% endif %}  {# Conditionals #}
{% for item in list %}...{% endfor %}  {# Loops #}
{{ value | filter }}              {# Filters #}
```

---

## Available Context

Templates receive a context dictionary with the following structure:

### Top-Level Variables

| Variable | Type | Description |
|----------|------|-------------|
| `vendor` | string | Vendor name (e.g., "Atmel", "ST") |
| `family` | string | Family name (e.g., "SAME70", "STM32F4") |
| `core` | string | ARM core (e.g., "Cortex-M7") |
| `mcu` | string | MCU name (e.g., "SAME70Q21") |
| `peripheral` | string | Peripheral name (e.g., "gpio", "uart") |
| `generation_date` | string | Timestamp (YYYY-MM-DD HH:MM:SS) |

### Nested Objects

```python
{
    # Platform configuration
    'platform': {
        'gpio': {...},
        'uart': {...}
    },

    # Peripheral definitions
    'peripherals': {
        'PIO': {
            'registers': [...],
            'bitfields': {...}
        }
    },

    # Memory layout
    'memory_layout': {
        'flash': {...},
        'ram': {...}
    },

    # Startup configuration
    'startup': {
        'vectors': [...],
        'init': {...}
    },

    # Architecture details
    'architecture': {
        'core': 'Cortex-M7',
        'fpu': 'double',
        ...
    }
}
```

### Backward Compatibility

For compatibility with older templates:

```jinja2
{{ metadata.vendor }}      {# Also available #}
{{ metadata.family }}
{{ metadata.mcu_name }}
```

---

## Custom Filters

The template engine provides custom filters for code generation:

### `sanitize`

Sanitizes identifiers for C++ code:

```jinja2
{{ "GPIO-A" | sanitize }}        {# GPIO_A #}
{{ "123_invalid" | sanitize }}   {# _123_invalid #}
```

### `format_hex`

Formats numbers as hexadecimal:

```jinja2
{{ 0x400E0E00 | format_hex }}           {# 0x400E0E00 #}
{{ 255 | format_hex(width=2) }}         {# 0xFF #}
{{ "0x1000" | format_hex }}             {# 0x1000 (preserves if string) #}
```

### `cpp_type`

Converts types to standard C++ types:

```jinja2
{{ "int" | cpp_type }}           {# int32_t #}
{{ "uint" | cpp_type }}          {# uint32_t #}
{{ "uint32_t" | cpp_type }}      {# uint32_t (no change) #}
```

### `to_pascal_case`

Converts to PascalCase:

```jinja2
{{ "gpio_control" | to_pascal_case }}    {# GpioControl #}
{{ "GPIO_CONTROL" | to_pascal_case }}    {# GpioControl #}
```

### `to_upper_snake`

Converts to UPPER_SNAKE_CASE:

```jinja2
{{ "gpioControl" | to_upper_snake }}     {# GPIO_CONTROL #}
{{ "GpioControl" | to_upper_snake }}     {# GPIO_CONTROL #}
```

### `parse_bit_range`

Parses bit range strings:

```jinja2
{% set info = "[7:4]" | parse_bit_range %}
{{ info.start }}    {# 4 #}
{{ info.end }}      {# 7 #}
{{ info.width }}    {# 4 #}
{{ info.shift }}    {# 4 #}
```

### `calculate_mask`

Calculates bit masks:

```jinja2
{{ "[7:4]" | calculate_mask }}    {# 0x000000F0 #}
{{ "[3]" | calculate_mask }}      {# 0x00000008 #}
```

### `parse_size`

Parses size strings:

```jinja2
{{ "512K" | parse_size }}    {# 524288 #}
{{ "2M" | parse_size }}      {# 2097152 #}
```

---

## Template Types

### 1. Register Structure Templates

**Purpose:** Generate register structure definitions
**Template:** `templates/registers/register_struct.hpp.j2`

#### Context Structure

```python
{
    'peripheral': {
        'name': 'PIO',
        'base_address': '0x400E0E00',
        'registers': [
            {
                'name': 'PER',
                'offset': '0x0000',
                'access': 'WO',
                'description': 'PIO Enable Register',
                'fields': [...]
            }
        ]
    }
}
```

#### Example Template

```jinja2
/**
 * {{ peripheral.name }} Register Structure
 * Base Address: {{ peripheral.base_address | format_hex }}
 *
 * Auto-generated on {{ generation_date }}
 */

#pragma once

#include <cstdint>

namespace {{ vendor | lower }}::{{ family | lower }}::{{ peripheral.name | lower }} {

struct {{ peripheral.name }}_Registers {
{% for register in peripheral.registers %}
    {% if register.access == 'RO' %}const {% endif %}uint32_t {{ register.name }};  ///< {{ register.description }}
{% endfor %}
} __attribute__((packed));

static_assert(sizeof({{ peripheral.name }}_Registers) == {{ peripheral.registers | length * 4 }},
              "Register structure size mismatch");

} // namespace
```

### 2. Bitfield Enum Templates

**Purpose:** Generate bitfield enums and manipulation functions
**Template:** `templates/bitfields/bitfield_enum.hpp.j2`

#### Example Template

```jinja2
namespace {{ vendor | lower }}::{{ family | lower }}::{{ peripheral | lower }} {

{% for register in registers %}
namespace {{ register.name | lower }} {
    {% for field in register.fields %}
    {% set info = field.bits | parse_bit_range %}

    /// {{ field.description }}
    enum class {{ field.name }} : uint32_t {
        {% for value in field.values %}
        {{ value.name }} = {{ value.value }},  ///< {{ value.description }}
        {% endfor %}
    };

    constexpr uint32_t {{ field.name | upper }}_MASK = {{ field.bits | calculate_mask }};
    constexpr uint32_t {{ field.name | upper }}_SHIFT = {{ info.shift }};

    {% endfor %}
} // namespace {{ register.name | lower }}
{% endfor %}

} // namespace
```

### 3. Platform HAL Templates

**Purpose:** Generate high-level HAL implementations
**Template:** `templates/platform/gpio.hpp.j2`

#### Context Structure

```python
{
    'peripheral_name': 'GPIO',
    'enums': [
        {
            'name': 'PinMode',
            'type': 'uint8_t',
            'values': [...]
        }
    ],
    'operations': {
        'initialize': {
            'return_type': 'Result<void, ErrorCode>',
            'parameters': [...],
            'steps': [...]
        }
    }
}
```

#### Example Template

```jinja2
/**
 * {{ peripheral_name }} HAL for {{ family }}
 * Generated: {{ generation_date }}
 */

#pragma once

#include "core/result.hpp"
#include "core/error.hpp"

namespace {{ vendor | lower }}::{{ family | lower }} {

{% for enum in metadata.enums %}
enum class {{ enum.name }} : {{ enum.type }} {
{% for value in enum.enum_values %}
    {{ value.name }} = {{ value.value }},  ///< {{ value.description }}
{% endfor %}
};

{% endfor %}

class {{ peripheral_name }} {
public:
    {% for op_name, op in metadata.operations.items() %}
    /**
     * {{ op.description | default(op_name) }}
     {% for param in op.parameters %}
     * @param {{ param.name }} {{ param.description | default(param.type) }}
     {% endfor %}
     * @return {{ op.return_type }}
     */
    static {{ op.return_type }} {{ op_name }}(
        {%- for param in op.parameters -%}
        {{ param.type }} {{ param.name }}{{ ", " if not loop.last else "" }}
        {%- endfor -%}
    );

    {% endfor %}
};

} // namespace
```

### 4. Startup Templates

**Purpose:** Generate startup code (vector table, init)
**Template:** `templates/startup/startup.cpp.j2`

#### Example Template

```jinja2
/**
 * Startup code for {{ mcu_name }}
 * Generated: {{ generation_date }}
 */

#include <cstdint>

// External symbols from linker script
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

// Main entry point
extern "C" int main();

// Reset handler
extern "C" void Reset_Handler() {
    // Copy .data section from flash to RAM
    uint32_t* src = &_sidata;
    uint32_t* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // Zero .bss section
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // Call main
    main();

    // Hang if main returns
    while (1);
}

// Vector table
__attribute__((section(".isr_vector")))
const void* vector_table[] = {
    &_estack,           // Initial stack pointer
    Reset_Handler,      // Reset handler
    // ... more vectors
};
```

### 5. Linker Script Templates

**Purpose:** Generate linker scripts
**Template:** `templates/linker/cortex_m.ld.j2`

#### Context Structure

```python
{
    'metadata': {
        'mcu_name': 'ATSAME70Q21',
        'memory_regions': [
            {
                'name': 'FLASH',
                'origin': '0x00400000',
                'size_kb': 2048,
                'permissions': 'rx'
            }
        ],
        'min_heap_size': '0x2000',
        'min_stack_size': '0x2000'
    }
}
```

#### Example Template

```jinja2
/**
 * Linker script for {{ metadata.mcu_name }}
 * Generated: {{ timestamp }}
 */

ENTRY({{ metadata.entry_point | default('Reset_Handler') }})

MEMORY
{
{% for region in metadata.memory_regions %}
    {{ region.name }} ({{ region.permissions }})  : ORIGIN = {{ region.origin }}, LENGTH = {{ region.size_kb }}K
{% endfor %}
}

_estack = ORIGIN({{ metadata.stack_region }}) + LENGTH({{ metadata.stack_region }});

SECTIONS
{
    .text : {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
    } >{{ metadata.flash_region }}

    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } >{{ metadata.ram_region }} AT>{{ metadata.flash_region }}

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } >{{ metadata.ram_region }}
}
```

---

## Best Practices

### 1. Header Guards

Always include header guards or `#pragma once`:

```jinja2
#pragma once

// or

#ifndef {{ family | upper }}_{{ peripheral | upper }}_HPP
#define {{ family | upper }}_{{ peripheral | upper }}_HPP

// content

#endif // {{ family | upper }}_{{ peripheral | upper }}_HPP
```

### 2. Documentation

Generate documentation comments:

```jinja2
/**
 * {{ description }}
 *
 * @param {{ param }} {{ param_description }}
 * @return {{ return_description }}
 *
 * @note Auto-generated from metadata
 */
```

### 3. Namespace Organization

Use consistent namespace hierarchy:

```jinja2
namespace {{ vendor | lower }}::{{ family | lower }}::{{ peripheral | lower }} {
    // content
} // namespace {{ vendor | lower }}::{{ family | lower }}::{{ peripheral | lower }}
```

### 4. Type Safety

Use strongly-typed enums:

```jinja2
enum class {{ name }} : {{ type }} {
    // values
};
```

### 5. Static Assertions

Add compile-time checks:

```jinja2
static_assert(sizeof({{ struct_name }}) == {{ expected_size }},
              "Size mismatch");
```

### 6. Whitespace Control

Use Jinja2 whitespace control:

```jinja2
{%- if condition -%}   {# Strip whitespace before and after #}
{{- variable -}}       {# Strip whitespace around variable #}
{% endif -%}
```

### 7. Error Handling

Include error checks in generated code:

```jinja2
if ({{ condition }}) {
    return Result<void, ErrorCode>::error(ErrorCode::{{ error_type }});
}
```

### 8. Generation Metadata

Include generation metadata in output:

```jinja2
/**
 * Auto-generated: {{ generation_date }}
 * Generator: {{ generator_name | default('unified_generator') }}
 * Metadata: {{ metadata_file }}
 *
 * DO NOT EDIT - Changes will be overwritten
 */
```

---

## Examples

### Example 1: Simple Register Structure

**Input Metadata:**
```json
{
  "registers": [
    {"name": "CR", "offset": "0x00", "access": "RW"},
    {"name": "SR", "offset": "0x04", "access": "RO"}
  ]
}
```

**Template:**
```jinja2
struct Registers {
{% for reg in registers %}
    {% if reg.access == 'RO' %}const {% endif %}uint32_t {{ reg.name }};
{% endfor %}
};
```

**Output:**
```cpp
struct Registers {
    uint32_t CR;
    const uint32_t SR;
};
```

### Example 2: Conditional Features

**Template:**
```jinja2
{% if metadata.features.fpu %}
    // FPU enabled
    #define HAS_FPU 1
{% else %}
    #define HAS_FPU 0
{% endif %}
```

### Example 3: Loop with Index

**Template:**
```jinja2
{% for pin in range(32) %}
static constexpr Pin PIN_{{ pin }} = {{ pin }};
{% endfor %}
```

### Example 4: Complex Nested Structure

**Template:**
```jinja2
{% for peripheral in peripherals %}
namespace {{ peripheral.name | lower }} {
    {% for instance in peripheral.instances %}
    constexpr uintptr_t {{ instance }}_BASE = {{ peripheral.base_addresses[loop.index0] | format_hex }};
    {% endfor %}
}
{% endfor %}
```

---

## Testing Templates

### 1. Unit Testing

Test templates with minimal metadata:

```python
from template_engine import TemplateEngine

engine = TemplateEngine('templates')
context = {'vendor': 'test', 'family': 'test'}
result = engine.render_template('test.hpp.j2', context)

assert 'namespace test::test' in result
```

### 2. Integration Testing

Test with real metadata:

```python
from unified_generator import UnifiedGenerator

gen = UnifiedGenerator(
    metadata_dir='metadata',
    schema_dir='schemas',
    template_dir='templates',
    output_dir='output'
)

result = gen.generate_platform_hal(
    family='same70',
    peripheral_type='gpio',
    dry_run=True
)

assert result is not None
```

### 3. Compilation Testing

Always test that generated code compiles:

```bash
# Generate code
python3 generate_gpio.py --family same70

# Compile
arm-none-eabi-g++ -c output/same70/gpio.hpp
```

### 4. Byte-for-Byte Comparison

Compare with manual reference implementation:

```bash
diff -u manual/gpio.hpp generated/gpio.hpp
```

---

## Troubleshooting

### Common Issues

#### Undefined Variable
```
UndefinedError: 'metadata' is undefined
```
**Fix:** Check context structure matches template expectations

#### Type Errors
```
TypeError: object of type 'NoneType' has no len()
```
**Fix:** Add conditional checks: `{% if list %}...{% endif %}`

#### Whitespace Issues
```
Extra blank lines in output
```
**Fix:** Use whitespace control: `{%-` and `-%}`

#### Filter Errors
```
FilterError: No filter named 'custom_filter'
```
**Fix:** Ensure filter is registered in `template_engine.py`

---

## Advanced Techniques

### Macros

Define reusable template blocks:

```jinja2
{% macro register_field(field) %}
    uint32_t {{ field.name }} : {{ field.width }};  ///< {{ field.description }}
{% endmacro %}

{% for field in fields %}
{{ register_field(field) }}
{% endfor %}
```

### Imports

Import other templates:

```jinja2
{% import 'common/header.j2' as header %}

{{ header.copyright() }}
{{ header.includes() }}
```

### Set Variables

Set intermediate variables:

```jinja2
{% set namespace = vendor | lower + '::' + family | lower %}

namespace {{ namespace }} {
    // content
}
```

### Custom Tests

Add custom conditional tests:

```jinja2
{% if field.bits is containing ':' %}
    // Bit range
{% else %}
    // Single bit
{% endif %}
```

---

## Reference

- **Jinja2 Documentation:** https://jinja.palletsprojects.com/
- **Template Engine:** `tools/codegen/cli/generators/template_engine.py`
- **Example Templates:** `tools/codegen/templates/`
- **Tests:** `tools/codegen/tests/test_template_engine.py`
- **Metadata Guide:** `tools/codegen/METADATA.md`
