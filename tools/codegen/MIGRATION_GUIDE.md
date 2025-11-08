# Migration Guide: Legacy Generators → UnifiedGenerator

## Overview

This guide helps you migrate existing hardcoded generators to the template-based UnifiedGenerator system.

## Why Migrate?

### Before (Hardcoded Generator)
```python
def generate_gpio(family, mcu):
    code = f"""
    namespace {family.lower()} {{
        class GPIO {{
            // Hardcoded implementation
        }};
    }}
    """
    write_file(f"gpio_{family}.hpp", code)
```

**Problems:**
- ❌ Code duplication across generators
- ❌ Hard to maintain and update
- ❌ No separation of logic and output
- ❌ Difficult to test
- ❌ No validation

### After (UnifiedGenerator)
```python
generator = UnifiedGenerator(
    metadata_dir='metadata',
    schema_dir='schemas',
    template_dir='templates',
    output_dir='output'
)

generator.generate_platform_hal(
    family='same70',
    peripheral_type='gpio'
)
```

**Benefits:**
- ✅ Metadata-driven (JSON)
- ✅ Template-based (Jinja2)
- ✅ Validated against schemas
- ✅ Reusable and testable
- ✅ Consistent across families

---

## Migration Process

### Phase 1: Analysis
1. Identify what the generator produces
2. Extract hardcoded values
3. Identify patterns and variations
4. Document dependencies

### Phase 2: Metadata Creation
1. Create metadata JSON files
2. Validate against schemas
3. Test metadata loading

### Phase 3: Template Creation
1. Create Jinja2 templates
2. Test template rendering
3. Compare output with original

### Phase 4: Integration
1. Update CMake integration
2. Run tests
3. Validate compilation
4. Mark legacy code as deprecated

---

## Step-by-Step Example: GPIO Generator

### Step 1: Analyze Existing Generator

**File:** `cli/generators/generate_gpio_old.py`

```python
def generate_gpio_same70():
    """Generate GPIO HAL for SAME70."""
    code = """
#pragma once

namespace alloy::hal::atmel::same70 {

enum class PinMode : uint8_t {
    Input = 0,
    Output = 1
};

class GPIO {
public:
    static void initialize(uint8_t pin, PinMode mode) {
        auto* pio = reinterpret_cast<PIO_Registers*>(0x400E0E00);
        pio->PER = (1u << pin);
        if (mode == PinMode::Output) {
            pio->OER = (1u << pin);
        } else {
            pio->ODR = (1u << pin);
        }
    }
};

} // namespace
"""
    with open('src/hal/platform/same70/gpio.hpp', 'w') as f:
        f.write(code)
```

**Analysis:**
- Hardcoded namespace: `alloy::hal::atmel::same70`
- Hardcoded register base: `0x400E0E00`
- Hardcoded enum values
- Hardcoded register operations

### Step 2: Extract to Metadata

Create `cli/generators/platform/metadata/same70_gpio.json`:

```json
{
  "family": "same70",
  "vendor": "atmel",
  "peripheral_name": "GPIO",
  "underlying_peripheral": "PIO",
  "base_address": "0x400E0E00",

  "enums": [
    {
      "name": "PinMode",
      "type": "uint8_t",
      "enum_values": [
        {
          "name": "Input",
          "value": "0",
          "description": "Pin configured as input"
        },
        {
          "name": "Output",
          "value": "1",
          "description": "Pin configured as output"
        }
      ]
    }
  ],

  "operations": {
    "initialize": {
      "return_type": "void",
      "parameters": [
        {"name": "pin", "type": "uint8_t"},
        {"name": "mode", "type": "PinMode"}
      ],
      "steps": [
        {
          "register_op": "pio->PER = (1u << pin)",
          "description": "Enable PIO control"
        },
        {
          "conditional": "if (mode == PinMode::Output)",
          "register_op": "pio->OER = (1u << pin)",
          "else_op": "pio->ODR = (1u << pin)"
        }
      ]
    }
  }
}
```

### Step 3: Create Template

Create `templates/platform/gpio.hpp.j2`:

```jinja2
#pragma once

#include <cstdint>

namespace alloy::hal::{{ vendor | lower }}::{{ family | lower }} {

{% for enum in metadata.enums %}
enum class {{ enum.name }} : {{ enum.type }} {
{% for value in enum.enum_values %}
    {{ value.name }} = {{ value.value }},  ///< {{ value.description }}
{% endfor %}
};

{% endfor %}

class {{ metadata.peripheral_name }} {
public:
    {% for op_name, op in metadata.operations.items() %}
    static {{ op.return_type }} {{ op_name }}(
        {%- for param in op.parameters -%}
        {{ param.type }} {{ param.name }}{{ ", " if not loop.last else "" }}
        {%- endfor -%}
    ) {
        auto* pio = reinterpret_cast<PIO_Registers*>({{ metadata.base_address }});
        {% for step in op.steps %}
        {% if step.conditional %}
        {{ step.conditional }} {
            {{ step.register_op }};
        } else {
            {{ step.else_op }};
        }
        {% else %}
        {{ step.register_op }};
        {% endif %}
        {% endfor %}
    }

    {% endfor %}
};

} // namespace
```

### Step 4: Test Generation

```python
from unified_generator import UnifiedGenerator

# Initialize generator
generator = UnifiedGenerator(
    metadata_dir='cli/generators/metadata',
    schema_dir='schemas',
    template_dir='templates',
    output_dir='output'
)

# Generate (dry-run first)
result = generator.generate_platform_hal(
    family='same70',
    peripheral_type='gpio',
    dry_run=True
)

print(result)
```

### Step 5: Compare Output

```bash
# Generate with new system
python3 -c "from unified_generator import UnifiedGenerator; ..."

# Compare
diff -u old/gpio.hpp new/gpio.hpp
```

**Expected:** Minimal or no differences (except comments/formatting)

### Step 6: Update Build System

Update `CMakeLists.txt`:

```cmake
# Old way (commented out)
# add_custom_command(
#     OUTPUT ${CMAKE_SOURCE_DIR}/src/hal/platform/same70/gpio.hpp
#     COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/codegen/cli/generators/generate_gpio_old.py
# )

# New way
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/src/hal/platform/same70/gpio.hpp
    COMMAND python3 -m tools.codegen.cli.generators.unified_generator
        --family same70
        --peripheral gpio
        --output ${CMAKE_SOURCE_DIR}/src/hal/platform/same70/gpio.hpp
)
```

### Step 7: Deprecate Legacy

Rename and mark legacy generator:

```bash
mv generate_gpio_old.py generate_gpio_legacy.py
```

Add deprecation warning:

```python
"""
DEPRECATED: This generator is deprecated.
Use UnifiedGenerator instead.

See: MIGRATION_GUIDE.md
"""

import warnings
warnings.warn("generate_gpio_legacy is deprecated", DeprecationWarning)
```

---

## Common Migration Patterns

### Pattern 1: Conditional Code Generation

**Before:**
```python
if family == 'same70':
    code += "// SAME70-specific code"
elif family == 'stm32f4':
    code += "// STM32F4-specific code"
```

**After (Metadata):**
```json
{
  "family_specific": {
    "same70": {
      "register_names": ["PER", "PDR"],
      "base_address": "0x400E0E00"
    },
    "stm32f4": {
      "register_names": ["MODER", "ODR"],
      "base_address": "0x40020000"
    }
  }
}
```

**Template:**
```jinja2
{% if family == 'same70' %}
// SAME70 implementation
{% elif family == 'stm32f4' %}
// STM32F4 implementation
{% endif %}
```

### Pattern 2: Register Address Calculation

**Before:**
```python
for i, instance in enumerate(['PIOA', 'PIOB', 'PIOC']):
    base_addr = 0x400E0E00 + (i * 0x200)
    code += f"constexpr uintptr_t {instance}_BASE = {hex(base_addr)};\n"
```

**After (Metadata):**
```json
{
  "instances": ["PIOA", "PIOB", "PIOC"],
  "base_addresses": [
    "0x400E0E00",
    "0x400E1000",
    "0x400E1200"
  ]
}
```

**Template:**
```jinja2
{% for instance in metadata.instances %}
constexpr uintptr_t {{ instance }}_BASE = {{ metadata.base_addresses[loop.index0] | format_hex }};
{% endfor %}
```

### Pattern 3: Enum Generation

**Before:**
```python
enums = {
    'PinMode': [('Input', 0), ('Output', 1)],
    'PullMode': [('None', 0), ('Up', 1), ('Down', 2)]
}

for enum_name, values in enums.items():
    code += f"enum class {enum_name} : uint8_t {{\n"
    for name, value in values:
        code += f"    {name} = {value},\n"
    code += "};\n"
```

**After (Metadata):**
```json
{
  "enums": [
    {
      "name": "PinMode",
      "type": "uint8_t",
      "enum_values": [
        {"name": "Input", "value": "0"},
        {"name": "Output", "value": "1"}
      ]
    }
  ]
}
```

**Template:**
```jinja2
{% for enum in metadata.enums %}
enum class {{ enum.name }} : {{ enum.type }} {
{% for value in enum.enum_values %}
    {{ value.name }} = {{ value.value }},
{% endfor %}
};
{% endfor %}
```

---

## Testing Migrated Generators

### 1. Unit Tests

Test metadata loading:

```python
def test_load_gpio_metadata():
    loader = MetadataLoader('metadata', 'schemas')
    metadata = loader.load_peripheral('same70_gpio')

    assert metadata['peripheral_name'] == 'GPIO'
    assert len(metadata['enums']) > 0
```

### 2. Integration Tests

Test complete generation:

```python
def test_generate_same70_gpio():
    generator = UnifiedGenerator(...)
    result = generator.generate_platform_hal(
        family='same70',
        peripheral_type='gpio',
        dry_run=True
    )

    assert 'namespace alloy::hal::atmel::same70' in result
    assert 'enum class PinMode' in result
```

### 3. Compilation Tests

```python
def test_generated_code_compiles():
    generator.generate_platform_hal(
        family='same70',
        peripheral_type='gpio',
        dry_run=False
    )

    # Compile
    result = subprocess.run([
        'arm-none-eabi-g++',
        '-c',
        'output/same70/gpio.hpp',
        '-I', 'src'
    ])

    assert result.returncode == 0
```

### 4. Regression Tests

Compare with reference implementation:

```python
def test_output_matches_reference():
    # Generate with new system
    new_output = generator.generate_platform_hal(
        family='same70',
        peripheral_type='gpio',
        dry_run=True
    )

    # Load reference
    with open('reference/same70_gpio.hpp') as f:
        reference = f.read()

    # Normalize (remove timestamps, comments)
    new_normalized = normalize(new_output)
    ref_normalized = normalize(reference)

    assert new_normalized == ref_normalized
```

---

## Migration Checklist

### For Each Generator

- [ ] **Analyze** existing generator
  - [ ] Document what it generates
  - [ ] List all hardcoded values
  - [ ] Identify patterns

- [ ] **Create Metadata**
  - [ ] Create JSON file
  - [ ] Validate against schema
  - [ ] Test loading

- [ ] **Create Template**
  - [ ] Write Jinja2 template
  - [ ] Test rendering
  - [ ] Check syntax highlighting

- [ ] **Test Generation**
  - [ ] Dry-run generation
  - [ ] Compare with original
  - [ ] Fix discrepancies

- [ ] **Write Tests**
  - [ ] Unit tests
  - [ ] Integration tests
  - [ ] Compilation tests

- [ ] **Update Build**
  - [ ] Update CMakeLists.txt
  - [ ] Update CI/CD
  - [ ] Test full build

- [ ] **Documentation**
  - [ ] Update README
  - [ ] Add examples
  - [ ] Document metadata format

- [ ] **Deprecation**
  - [ ] Rename legacy generator
  - [ ] Add deprecation warning
  - [ ] Update references

---

## Common Issues and Solutions

### Issue 1: Metadata Structure Mismatch

**Problem:** Template expects `metadata.enums` but gets `enums`

**Solution:** Update template context preparation in `unified_generator.py`:

```python
template_context = {
    'metadata': {
        'enums': config.get('enums', []),
        ...
    }
}
```

### Issue 2: Missing Custom Filters

**Problem:** `FilterError: No filter named 'custom_filter'`

**Solution:** Add filter to `template_engine.py`:

```python
def _filter_custom(self, value):
    # Implementation
    return result

env.filters['custom_filter'] = _filter_custom
```

### Issue 3: Complex Custom Logic

**Problem:** Logic too complex for templates

**Solution:** Use custom implementation in metadata:

```json
{
  "initialize": {
    "implementation": "custom",
    "custom_code": "// Complex C++ code here"
  }
}
```

### Issue 4: MCU-Specific Variations

**Problem:** Different behavior for different MCUs

**Solution:** Add MCU-specific overrides in family metadata:

```json
{
  "mcus": {
    "SAME70Q21": {
      "gpio_override": {
        "max_pins": 128
      }
    }
  }
}
```

---

## Best Practices

### 1. Incremental Migration

Migrate one generator at a time:
1. Start with simplest (e.g., register structures)
2. Move to moderate complexity (e.g., bitfields)
3. Finish with complex (e.g., platform HAL)

### 2. Maintain Backward Compatibility

Keep legacy generators working during migration:
```python
# New system (preferred)
generator.generate_platform_hal(...)

# Legacy fallback
if not os.path.exists(output_file):
    generate_gpio_legacy()
```

### 3. Document Decisions

Add comments explaining non-obvious choices:

```json
{
  "register_offset": "0x0200",
  "_comment": "Offset differs from datasheet due to errata XYZ"
}
```

### 4. Version Metadata

Include version information:

```json
{
  "metadata_version": "1.0",
  "last_updated": "2025-11-07",
  "author": "Corezero Team"
}
```

### 5. Test Thoroughly

- Test all supported families
- Test all MCU variants
- Test edge cases
- Test error handling

---

## Migration Timeline

### Estimated Effort

| Generator Type | Complexity | Estimated Time |
|---------------|-----------|----------------|
| Register structures | Low | 2-4 hours |
| Bitfield enums | Low | 2-4 hours |
| GPIO HAL | Medium | 4-8 hours |
| UART HAL | Medium | 4-8 hours |
| SPI/I2C HAL | Medium | 4-8 hours |
| Startup code | High | 8-16 hours |
| Linker scripts | High | 8-16 hours |

### Suggested Order

1. ✅ Foundation (metadata loader, template engine) - **DONE**
2. ✅ Register structures - **DONE**
3. ✅ Bitfield enums - **DONE**
4. 🚧 GPIO HAL - **IN PROGRESS**
5. UART HAL
6. SPI HAL
7. I2C HAL
8. Startup code
9. Linker scripts

---

## Success Criteria

Migration is successful when:

- ✅ Generated code compiles without errors
- ✅ Generated code matches reference output
- ✅ All tests pass
- ✅ Build system integration works
- ✅ Documentation is updated
- ✅ Legacy code is deprecated
- ✅ CI/CD pipeline works

---

## Getting Help

- **Documentation:** `METADATA.md`, `TEMPLATE_GUIDE.md`
- **Examples:** `templates/`, `cli/generators/metadata/`
- **Tests:** `tests/test_unified_generator*.py`
- **Issues:** Submit to project issue tracker

---

## Next Steps

After migration:

1. **Remove legacy generators** (after grace period)
2. **Add new MCU families** using metadata
3. **Extend templates** with new features
4. **Improve metadata schemas** based on feedback
5. **Optimize generation** for performance
