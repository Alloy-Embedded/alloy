# Code Generation Consolidation Specification

## ADDED Requirements

### Requirement: Unified Code Generator

Project SHALL use single unified code generator instead of multiple platform-specific generators.

**Rationale**: Eliminates code duplication (95% similarity across generators), ensures consistent output, simplifies maintenance.

#### Scenario: Single entry point for all platforms

- **GIVEN** code needs to be generated for any platform
- **WHEN** code generation is invoked
- **THEN** single `tools/codegen/codegen.py` script SHALL be used
- **AND** platform SHALL be specified via command-line arguments

```bash
# Generate for STM32F4
python tools/codegen/codegen.py \
    --svd data/stm32f4.svd \
    --vendor st \
    --family stm32f4 \
    --output src/hal/vendors/st/stm32f4/generated/

# Generate for SAME70
python tools/codegen/codegen.py \
    --svd data/same70.svd \
    --vendor arm \
    --family same70 \
    --output src/hal/vendors/arm/same70/generated/
```

#### Scenario: Consistent output structure

- **GIVEN** code generator runs for any platform
- **WHEN** generation completes
- **THEN** output SHALL have consistent directory structure
- **AND** structure SHALL be: `registers/`, `bitfields/`, `peripherals.hpp`

```
generated/
├── .generated          # Marker file
├── registers/
│   ├── rcc_registers.hpp
│   ├── gpio_registers.hpp
│   └── uart_registers.hpp
├── bitfields/
│   ├── rcc_bitfields.hpp
│   ├── gpio_bitfields.hpp
│   └── uart_bitfields.hpp
└── peripherals.hpp     # Peripheral instances
```

---

### Requirement: Plugin-Based Generator Architecture

Code generator SHALL use plugin architecture for extensibility.

**Rationale**: Allows adding new output formats or platforms without modifying core generator.

#### Scenario: Register generator plugin

- **GIVEN** registers need to be generated
- **WHEN** generator runs
- **THEN** `RegisterGenerator` plugin SHALL be invoked
- **AND** plugin SHALL produce `registers/*.hpp` files

```python
# tools/codegen/generators/register_generator.py

from codegen.core.generator_base import GeneratorPlugin

class RegisterGenerator(GeneratorPlugin):
    def generate(self, svd_data, output_dir):
        """Generate register header files."""
        for peripheral in svd_data.peripherals:
            output_file = output_dir / "registers" / f"{peripheral.name.lower()}_registers.hpp"
            self.render_template("register.hpp.j2", peripheral, output_file)
```

#### Scenario: Generator validation mode

- **GIVEN** developer wants to verify generator output
- **WHEN** generator runs with `--dry-run`
- **THEN** generator SHALL show what files would be created
- **AND** generator SHALL NOT write any files

```bash
python tools/codegen/codegen.py \
    --svd data/stm32f4.svd \
    --vendor st \
    --family stm32f4 \
    --dry-run

# Output:
# Would generate:
#   src/hal/vendors/st/stm32f4/generated/registers/rcc_registers.hpp
#   src/hal/vendors/st/stm32f4/generated/registers/gpio_registers.hpp
#   ... (15 more files)
```

---

### Requirement: Template Consolidation

All generator templates SHALL be consolidated with platform-agnostic logic.

**Rationale**: Reduces template sprawl (15+ templates) to ~5 templates with conditional sections.

#### Scenario: Unified register template

- **GIVEN** register code needs to be generated
- **WHEN** template is rendered
- **THEN** `templates/register.hpp.j2` SHALL be used
- **AND** template SHALL handle all platforms via conditionals

```jinja2
{# templates/register.hpp.j2 #}
#pragma once

#include "hal/core/types.hpp"

namespace {{ vendor }}::{{ family }}::{{ peripheral_name.lower() }}_registers {

{% for register in registers %}
struct {{ register.name }} {
    static constexpr uintptr_t address = {{ register.address }};

    {% if register.is_read_write %}
    static inline void write({{ register.type }} value) {
        *reinterpret_cast<volatile {{ register.type }}*>(address) = value;
    }

    static inline {{ register.type }} read() {
        return *reinterpret_cast<volatile {{ register.type }}*>(address);
    }
    {% endif %}
};
{% endfor %}

} // namespace {{ vendor }}::{{ family }}::{{ peripheral_name.lower() }}_registers
```

---

## REMOVED Requirements

### Requirement: Platform-Specific Generators (REMOVED)

Old generators (`generate_stm32f4_registers.py`, `generate_stm32f7_registers.py`, etc.) have been removed and replaced with unified generator.

**Migration**: All platforms now use `codegen.py` with appropriate arguments.
