# Template System Architecture

**Version**: 1.0.0
**Date**: 2025-01-19
**Phase**: 2.1 - Template Architecture Design

---

## Overview

This document describes the architecture of the Alloy HAL template system, including the template hierarchy, data flow, and code generation pipeline.

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     Template System                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐      ┌──────────────┐      ┌───────────┐ │
│  │   Metadata   │──────▶│  Generator   │──────▶│ Generated │ │
│  │   (JSON)     │      │   (Python)   │      │   Code    │ │
│  └──────────────┘      └──────────────┘      └───────────┘ │
│        │                      │                      │       │
│        │                      │                      │       │
│   ┌────▼────┐            ┌───▼────┐            ┌───▼────┐  │
│   │ Schema  │            │Template│            │Compiler│  │
│   │Validator│            │ Engine │            │  Check │  │
│   └─────────┘            └────────┘            └────────┘  │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Template Hierarchy

### Level 1: Base Templates

**Purpose**: Common reusable components

**Location**: `templates/common/`

**Files**:
- `header.j2` - Standard file header
- `namespace_begin.j2` - Namespace opening
- `namespace_end.j2` - Namespace closing
- `doxygen_comment.j2` - Doxygen documentation blocks

**Usage**: Included by other templates via `{% include %}`

**Example**:
```jinja2
{% include 'common/header.j2' %}
```

---

### Level 2: Platform Templates

**Purpose**: MCU/SoC-specific code generation

**Location**: `templates/platform/`

**Files**:
- `gpio.hpp.j2` - GPIO peripheral
- `uart.hpp.j2` - UART peripheral
- `uart_hardware_policy.hpp.j2` - UART hardware policy (Policy-Based Design)
- `spi.hpp.j2` - SPI peripheral
- `i2c.hpp.j2` - I2C peripheral
- `adc.hpp.j2` - ADC peripheral
- `timer.hpp.j2` - Timer peripheral
- `dma.hpp.j2` - DMA controller

**Input**: Platform metadata (`platforms/<name>/platform.json`)

**Output**: Platform-specific headers

**Characteristics**:
- MCU-agnostic API
- Hardware-specific implementation
- Register access abstraction
- Compile-time configuration

---

### Level 3: Peripheral Templates

**Purpose**: Detailed peripheral register definitions

**Location**: `templates/peripheral/`

**Files**:
- `peripheral_registers.hpp.j2` - Register structures
- `peripheral_bitfields.hpp.j2` - Bitfield definitions
- `peripheral_constants.hpp.j2` - Peripheral constants

**Input**: Peripheral metadata (`platforms/<name>/<peripheral>.json`)

**Output**: Peripheral register definitions

**Characteristics**:
- Memory-mapped register access
- Type-safe bitfield operations
- Zero-overhead abstractions

---

### Level 4: Startup Templates

**Purpose**: System initialization code

**Location**: `templates/startup/`

**Files**:
- `cortex_m_startup.cpp.j2` - Reset handler and initialization
- `vector_table.cpp.j2` - Interrupt vector table
- `system_init.cpp.j2` - System-specific initialization

**Input**: Platform + interrupt metadata

**Output**: Startup source files

**Characteristics**:
- Cortex-M specific
- Configurable FPU/MPU initialization
- Data/BSS initialization
- Static constructor calls

---

### Level 5: Linker Templates

**Purpose**: Memory layout definition

**Location**: `templates/linker/`

**Files**:
- `cortex_m.ld.j2` - Linker script
- `memory_map.ld.j2` - Memory region definitions

**Input**: Memory metadata from platform.json

**Output**: Linker scripts (.ld)

**Characteristics**:
- Flash/RAM layout
- Section definitions (.text, .data, .bss)
- Stack/heap configuration
- Peripheral memory regions

---

### Level 6: Board Templates

**Purpose**: Board-specific configurations

**Location**: `templates/board/`

**Files**:
- `board_config.hpp.j2` - Board pin definitions
- `board_peripherals.hpp.j2` - Configured peripherals
- `board_components.hpp.j2` - Component definitions

**Input**: Board metadata (`boards/<name>.json`)

**Output**: Board configuration headers

**Characteristics**:
- Pin mappings (Arduino, connectors)
- Peripheral assignments (UART for console, etc.)
- Component definitions (LEDs, buttons, sensors)
- Board-specific constants

---

## Data Flow

### 1. Metadata Loading

```
metadata/platforms/stm32f4/
├── platform.json       ──┐
├── gpio.json           ──├──▶ Validator ──▶ Schema Check ──▶ Loaded Metadata
├── uart.json           ──┤
└── spi.json            ──┘
```

### 2. Schema Validation

```python
# Validate platform metadata
jsonschema.validate(platform_data, platform_schema)

# Validate peripheral metadata
jsonschema.validate(peripheral_data, peripheral_schema)

# Validate board metadata
jsonschema.validate(board_data, board_schema)
```

### 3. Template Rendering

```python
# Load template
template = jinja_env.get_template('platform/gpio.hpp.j2')

# Render with metadata
output = template.render(
    platform=platform_data,
    peripheral=gpio_data,
    instances=gpio_instances,
    generation_date=now(),
    **context
)

# Write output
with open('generated/gpio.hpp', 'w') as f:
    f.write(output)
```

### 4. Code Generation Pipeline

```
Metadata (JSON)
    ↓
Schema Validation
    ↓
Template Loading (Jinja2)
    ↓
Context Preparation
    ↓
Template Rendering
    ↓
Output Writing
    ↓
Syntax Validation (Compiler)
    ↓
Generated Code (Ready for Use)
```

---

## Template Design Patterns

### Pattern 1: Include Pattern

**Purpose**: Reuse common template fragments

```jinja2
{# Include common header #}
{% include 'common/header.j2' %}

{# Include namespace helpers #}
{% include 'common/namespace_begin.j2' %}

// Your code here

{% include 'common/namespace_end.j2' %}
```

---

### Pattern 2: Macro Pattern

**Purpose**: Reusable template logic

```jinja2
{# Define macro #}
{% macro register_definition(name, offset, size) %}
struct {{ name }} {
    static constexpr uint32_t OFFSET = {{ offset }};
    static constexpr uint32_t SIZE = {{ size }};
};
{% endmacro %}

{# Use macro #}
{% for reg_name, reg_data in registers.items() %}
{{ register_definition(reg_name, reg_data.offset, reg_data.size) }}
{% endfor %}
```

---

### Pattern 3: Conditional Pattern

**Purpose**: Handle optional features

```jinja2
{# Only generate if FPU is available #}
{% if architecture.fpu %}
    /**
     * @brief Initialize FPU
     */
    static inline void init_fpu() {
        // Enable FPU
        SCB->CPACR |= (0xF << 20);
    }
{% endif %}
```

---

### Pattern 4: Loop Pattern

**Purpose**: Generate repetitive code

```jinja2
{# Generate for all instances #}
{% for instance in instances %}
/// {{ instance.name }} base address
static constexpr uint32_t {{ instance.name }}_BASE = {{ instance.base_address }};

/// {{ instance.name }} IRQ number
static constexpr int {{ instance.name }}_IRQ = {{ instance.irq_number }};
{% endfor %}
```

---

## Code Generation Strategy

### Multi-Pass Generation

Some templates require multiple passes:

1. **Pass 1**: Generate register definitions
2. **Pass 2**: Generate bitfield enums
3. **Pass 3**: Generate peripheral classes

### Dependency Resolution

Templates may depend on other generated files:

```jinja2
{# uart.hpp.j2 depends on uart_registers.hpp #}
#include "{{ platform }}/uart_registers.hpp"
```

**Resolution Strategy**:
1. Generate in dependency order
2. Use forward declarations where possible
3. Document dependencies in template

### Incremental Generation

Only regenerate when:
- Metadata changes
- Template changes
- Generator script changes

**Implementation**:
```python
def should_regenerate(output_file, template_file, metadata_file):
    if not os.path.exists(output_file):
        return True

    output_mtime = os.path.getmtime(output_file)
    template_mtime = os.path.getmtime(template_file)
    metadata_mtime = os.path.getmtime(metadata_file)

    return template_mtime > output_mtime or metadata_mtime > output_mtime
```

---

## Platform Support Matrix

| Platform | GPIO | UART | SPI | I2C | ADC | Timer | DMA | Status |
|----------|------|------|-----|-----|-----|-------|-----|--------|
| STM32F4 | ✅ | ✅ | ✅ | ✅ | ⏸️ | ⏸️ | ⏸️ | In Progress |
| SAME70 | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | Planned |
| STM32G0 | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | Planned |

Legend:
- ✅ Complete
- 🔄 In Progress
- ⏸️ Planned
- ❌ Not Supported

---

## Extension Points

### Adding New Platform

1. Create platform directory: `metadata/platforms/<name>/`
2. Add `platform.json` with complete metadata
3. Add peripheral metadata files (`gpio.json`, `uart.json`, etc.)
4. Validate against schemas
5. Run generator: `python generators/platform_generator.py <name>`
6. Compile generated code
7. Test on real hardware

### Adding New Peripheral

1. Create peripheral template: `templates/platform/<peripheral>.hpp.j2`
2. Create peripheral schema (if needed): `metadata/schema/<peripheral>.schema.json`
3. Add peripheral metadata to platform: `<platform>/<peripheral>.json`
4. Update generator script
5. Generate and test

### Adding New Board

1. Create board metadata: `metadata/boards/<board_name>.json`
2. Reference existing platform in `mcu.platform`
3. Define pin mappings
4. Define peripheral configurations
5. Generate board headers
6. Validate with hardware

---

## Quality Assurance

### Pre-Generation Checks

- ✅ Metadata validates against schema
- ✅ All required fields present
- ✅ Cross-references are valid
- ✅ No circular dependencies

### Post-Generation Checks

- ✅ Generated code compiles without errors
- ✅ Generated code follows coding standards
- ✅ All peripherals have complete API
- ✅ Documentation is complete

### Automated Testing

```bash
# Run all generators
./generate_all.sh

# Compile all generated code
./compile_generated.sh

# Run syntax checks
./lint_generated.sh
```

---

## Performance Considerations

### Template Complexity

- Keep templates simple and focused
- Use macros for complex logic
- Avoid deeply nested loops
- Profile generation time for large platforms

### Generated Code Size

- Use `constexpr` to minimize runtime code
- Prefer `static inline` for small functions
- Avoid vtables (use CRTP instead)
- Enable compiler optimizations (-O2, -O3)

### Build Time

- Use precompiled headers for common includes
- Minimize template instantiations
- Use forward declarations
- Cache generated files (don't regenerate unnecessarily)

---

## Future Enhancements

### Planned Features

1. **SVD Import**: Generate metadata from CMSIS-SVD files
2. **Interactive Generator**: Web-based configuration tool
3. **Code Validation**: Static analysis of generated code
4. **Multi-Language**: Support for Rust, Zig in addition to C++
5. **Board Database**: Centralized repository of board configurations

### Research Topics

1. **AI-Assisted Generation**: Use LLMs to suggest optimizations
2. **Formal Verification**: Prove correctness of generated code
3. **Cross-Platform Testing**: Automated testing across platforms

---

## References

- **Jinja2 Documentation**: https://jinja.palletsprojects.com/
- **JSON Schema**: https://json-schema.org/
- **CMSIS-SVD**: https://open-cmsis-pack.github.io/svd-spec/
- **Policy-Based Design**: Andrei Alexandrescu, Modern C++ Design

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-01-19 | Initial architecture document |

---

**Next Steps**: See Phase 2.2 for GPIO template implementation.
