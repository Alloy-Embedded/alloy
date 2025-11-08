# Metadata Format Guide

## Overview

The Corezero code generation system uses a hierarchical metadata structure to describe microcontrollers, peripherals, and their characteristics. This metadata drives template-based code generation for HAL implementations.

## Directory Structure

```
tools/codegen/
├── cli/generators/metadata/
│   ├── vendors/          # Vendor-level metadata
│   │   ├── atmel.json
│   │   └── st.json
│   ├── families/         # MCU family metadata
│   │   ├── same70.json
│   │   └── stm32f4.json
│   └── peripherals/      # Peripheral-specific metadata
│       ├── same70_pio.json
│       └── stm32f4_gpio.json
├── schemas/              # JSON Schema validation
│   ├── vendor.schema.json
│   ├── family.schema.json
│   └── peripheral.schema.json
└── templates/            # Jinja2 templates
```

## Metadata Hierarchy

The system uses a three-level hierarchy:

```
Vendor → Family → Peripheral
  ↓        ↓         ↓
Common   MCU-     Register
settings specific  definitions
```

Metadata is resolved by merging these levels, with child levels overriding parent settings.

---

## 1. Vendor Metadata

**Location:** `cli/generators/metadata/vendors/{vendor}.json`
**Schema:** `schemas/vendor.schema.json`

### Purpose
Defines vendor-wide settings and conventions that apply to all MCU families from this vendor.

### Example: `atmel.json`

```json
{
  "vendor": "Atmel",
  "architecture": "arm_cortex_m",
  "common": {
    "endianness": "little",
    "pointer_size": 32,
    "naming": {
      "register_case": "UPPER",
      "field_case": "UPPER_SNAKE",
      "enum_case": "PascalCase"
    },
    "features": {
      "fpu": true,
      "dsp": false,
      "mpu": true
    }
  },
  "families": [
    {
      "name": "SAME70",
      "description": "ARM Cortex-M7 high-performance MCUs",
      "core": "Cortex-M7",
      "metadata_file": "same70.json"
    }
  ]
}
```

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `vendor` | string | Vendor name (e.g., "Atmel", "ST") |
| `architecture` | string | Architecture family (e.g., "arm_cortex_m") |
| `common` | object | Common settings for all MCUs |
| `families` | array | List of MCU families |

### Common Settings

- **`endianness`**: `"little"` or `"big"`
- **`pointer_size`**: Address size in bits (typically 32)
- **`naming`**: Naming conventions
  - `register_case`: How registers are named ("UPPER", "lower", "PascalCase")
  - `field_case`: How bitfields are named ("UPPER_SNAKE", "lower_snake", "camelCase")
  - `enum_case`: How enums are named ("PascalCase", "SCREAMING_SNAKE")

---

## 2. Family Metadata

**Location:** `cli/generators/metadata/families/{family}.json`
**Schema:** `schemas/family.schema.json`

### Purpose
Defines MCU family characteristics, including core type, memory layout, and MCU variants.

### Example: `same70.json`

```json
{
  "family": "SAME70",
  "vendor": "Atmel",
  "architecture": "arm_cortex_m",
  "core": "Cortex-M7",
  "description": "ARM Cortex-M7 high-performance microcontrollers",

  "features": {
    "fpu": "double",
    "mpu": true,
    "cache": {
      "instruction": true,
      "data": true
    }
  },

  "mcus": {
    "SAME70Q21": {
      "description": "SAME70 with 2MB Flash, 384KB RAM",
      "package": "LQFP144",
      "flash": {
        "size_kb": 2048,
        "base_address": "0x00400000"
      },
      "ram": {
        "size_kb": 384,
        "base_address": "0x20400000"
      },
      "peripherals": {
        "PIO": {
          "instances": ["PIOA", "PIOB", "PIOC", "PIOD", "PIOE"],
          "base_addresses": [
            "0x400E0E00",
            "0x400E1000",
            "0x400E1200",
            "0x400E1400",
            "0x400E1600"
          ]
        },
        "UART": {
          "instances": ["UART0", "UART1", "UART2", "UART3", "UART4"],
          "base_addresses": [
            "0x400E0800",
            "0x400E0A00",
            "0x400E1A00",
            "0x400E1C00",
            "0x400E1E00"
          ]
        }
      }
    }
  },

  "peripheral_metadata": {
    "PIO": "same70_pio.json",
    "UART": "same70_uart.json"
  }
}
```

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `family` | string | Family name (e.g., "SAME70") |
| `vendor` | string | Vendor name (must match vendor metadata) |
| `core` | string | ARM core type (e.g., "Cortex-M7") |
| `mcus` | object | MCU variants in this family |
| `peripheral_metadata` | object | Links to peripheral metadata files |

### MCU Definition

Each MCU in the `mcus` object should include:

```json
{
  "description": "Human-readable description",
  "flash": {
    "size_kb": 2048,
    "base_address": "0x00400000"
  },
  "ram": {
    "size_kb": 384,
    "base_address": "0x20400000"
  },
  "peripherals": {
    "PERIPHERAL_NAME": {
      "instances": ["INSTANCE1", "INSTANCE2"],
      "base_addresses": ["0xADDRESS1", "0xADDRESS2"]
    }
  }
}
```

---

## 3. Peripheral Metadata

**Location:** `cli/generators/metadata/peripherals/{family}_{peripheral}.json`
**Schema:** `schemas/peripheral.schema.json`

### Purpose
Defines register structures, bitfields, and peripheral-specific operations for code generation.

### Example: `same70_pio.json`

```json
{
  "peripheral": "PIO",
  "family": "SAME70",
  "vendor": "Atmel",
  "description": "Parallel Input/Output Controller",
  "base_address": "0x400E0E00",

  "registers": [
    {
      "name": "PER",
      "offset": "0x0000",
      "access": "WO",
      "reset_value": "0x00000000",
      "description": "PIO Enable Register",
      "fields": [
        {
          "name": "P0",
          "bits": "[0]",
          "description": "PIO Enable for Pin 0"
        },
        {
          "name": "P1",
          "bits": "[1]",
          "description": "PIO Enable for Pin 1"
        }
      ]
    },
    {
      "name": "PDR",
      "offset": "0x0004",
      "access": "WO",
      "reset_value": "0x00000000",
      "description": "PIO Disable Register"
    }
  ],

  "bitfields": {
    "mode": {
      "description": "Pin mode configuration",
      "values": [
        {
          "name": "Input",
          "value": "0x0",
          "description": "Pin configured as input"
        },
        {
          "name": "Output",
          "value": "0x1",
          "description": "Pin configured as output"
        }
      ]
    }
  }
}
```

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `peripheral` | string | Peripheral name (e.g., "PIO", "UART") |
| `family` | string | MCU family (must match family metadata) |
| `vendor` | string | Vendor name |
| `description` | string | Human-readable description |
| `registers` | array | Register definitions |

### Register Definition

```json
{
  "name": "REGISTER_NAME",
  "offset": "0xOFFSET",
  "access": "RW|RO|WO",
  "reset_value": "0xVALUE",
  "description": "Register description",
  "fields": [
    {
      "name": "FIELD_NAME",
      "bits": "[7:4]" or "[3]",
      "description": "Field description"
    }
  ]
}
```

**Access Types:**
- `RW` - Read/Write
- `RO` - Read-Only
- `WO` - Write-Only
- `RC` - Read to Clear
- `W1C` - Write 1 to Clear

**Bit Range Format:**
- Single bit: `"[3]"`
- Range: `"[7:4]"` (MSB:LSB)

---

## 4. Platform HAL Metadata

**Location:** `cli/generators/platform/metadata/{family}_{peripheral}.json`

### Purpose
Defines high-level HAL operations, state management, and error handling for platform peripherals.

### Example: `same70_gpio.json`

```json
{
  "family": "same70",
  "vendor": "atmel",
  "peripheral_name": "GPIO",
  "underlying_peripheral": "PIO",
  "description": "GPIO HAL for SAME70 using PIO controller",

  "pin_config": {
    "type": "PinConfig",
    "fields": [
      {
        "name": "mode",
        "type": "PinMode",
        "description": "Pin mode (Input/Output)"
      },
      {
        "name": "pull",
        "type": "PullMode",
        "description": "Pull resistor configuration"
      }
    ]
  },

  "enums": [
    {
      "name": "PinMode",
      "type": "uint8_t",
      "values": [
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
    },
    {
      "name": "PullMode",
      "type": "uint8_t",
      "values": [
        {
          "name": "None",
          "value": "0",
          "description": "No pull resistor"
        },
        {
          "name": "Up",
          "value": "1",
          "description": "Pull-up enabled"
        },
        {
          "name": "Down",
          "value": "2",
          "description": "Pull-down enabled"
        }
      ]
    }
  ],

  "operations": {
    "initialize": {
      "return_type": "Result<void, ErrorCode>",
      "parameters": [
        {
          "name": "pin",
          "type": "Pin"
        },
        {
          "name": "config",
          "type": "const PinConfig&"
        }
      ],
      "implementation": "template_driven",
      "steps": [
        {
          "check_state": "if (!is_valid_pin(pin))",
          "return_error": "ErrorCode::InvalidPin"
        },
        {
          "register_op": "PIO->PER = (1u << pin)",
          "description": "Enable PIO control"
        },
        {
          "conditional": "if (config.mode == PinMode::Output)",
          "register_op": "PIO->OER = (1u << pin)",
          "else_op": "PIO->ODR = (1u << pin)"
        }
      ]
    },
    "set": {
      "return_type": "Result<void, ErrorCode>",
      "parameters": [
        {
          "name": "pin",
          "type": "Pin"
        }
      ],
      "implementation": "template_driven",
      "steps": [
        {
          "register_op": "PIO->SODR = (1u << pin)"
        }
      ]
    }
  }
}
```

### Operation Definition

```json
{
  "operation_name": {
    "return_type": "Result<T, ErrorCode>",
    "parameters": [
      {"name": "param_name", "type": "param_type"}
    ],
    "implementation": "template_driven|custom",
    "steps": [
      {
        "check_state": "if (condition)",
        "return_error": "ErrorCode::ErrorType"
      },
      {
        "register_op": "REGISTER->FIELD = value",
        "description": "What this does"
      },
      {
        "conditional": "if (condition)",
        "register_op": "operation_if_true",
        "else_op": "operation_if_false"
      }
    ]
  }
}
```

---

## 5. Linker Metadata

**Location:** `cli/generators/linker/metadata/{mcu}_linker.json`

### Purpose
Defines memory layout and linker script configuration for specific MCUs.

### Example: `same70q21_linker.json`

```json
{
  "family": "same70",
  "vendor": "atmel",
  "mcu_name": "ATSAME70Q21",
  "board_name": "Atmel SAME70 Xplained",
  "core": "Cortex-M7",

  "features": [
    "ARM Cortex-M7 @ 300MHz",
    "2MB Flash",
    "384KB SRAM",
    "FPU (double precision)",
    "DSP instructions",
    "Cache (I-Cache and D-Cache)"
  ],

  "memory_regions": [
    {
      "name": "FLASH",
      "origin": "0x00400000",
      "size_kb": 2048,
      "permissions": "rx",
      "description": "2MB (2048KB)"
    },
    {
      "name": "RAM",
      "origin": "0x20400000",
      "size_kb": 384,
      "permissions": "rwx",
      "description": "384KB (SRAM - starts at 0x20400000!)"
    }
  ],

  "flash_region": "FLASH",
  "ram_region": "RAM",
  "stack_region": "RAM",

  "min_heap_size": "0x2000",
  "min_stack_size": "0x2000",

  "support_cpp": true,
  "support_exceptions": true,
  "ram_functions": true,

  "entry_point": "Reset_Handler"
}
```

---

## Metadata Validation

All metadata files are validated against JSON Schemas before use:

```python
from metadata_loader import MetadataLoader

loader = MetadataLoader(
    metadata_dir="cli/generators/metadata",
    schema_dir="schemas"
)

# Automatically validates against schema
vendor = loader.load_vendor('atmel')
family = loader.load_family('same70')
peripheral = loader.load_peripheral('same70_pio')
```

---

## Best Practices

### 1. Naming Conventions

- **Vendors:** Lowercase (e.g., `atmel.json`, `st.json`)
- **Families:** Lowercase (e.g., `same70.json`, `stm32f4.json`)
- **Peripherals:** `{family}_{peripheral}.json` (e.g., `same70_pio.json`)

### 2. Addresses

- Use hexadecimal strings: `"0x400E0E00"`
- Always include `0x` prefix
- Use uppercase for hex digits: `0xABCD` (not `0xabcd`)

### 3. Documentation

- Provide `description` fields for all major elements
- Include register and field descriptions from datasheets
- Document non-obvious design decisions

### 4. Consistency

- Use consistent naming across all metadata files
- Follow vendor's official naming (e.g., PIO vs GPIO)
- Match datasheet terminology

### 5. Validation

- Always validate metadata with schemas
- Test generated code compilation
- Verify against datasheet specifications

---

## Common Patterns

### Multi-Instance Peripherals

When a peripheral has multiple instances (e.g., UART0, UART1):

```json
{
  "peripherals": {
    "UART": {
      "instances": ["UART0", "UART1", "UART2"],
      "base_addresses": [
        "0x400E0800",
        "0x400E0A00",
        "0x400E1A00"
      ]
    }
  }
}
```

### Conditional Features

Use flags to enable/disable features:

```json
{
  "features": {
    "fpu": "double",
    "cache": {
      "instruction": true,
      "data": true
    }
  }
}
```

### Custom Implementations

For complex operations, use custom implementations:

```json
{
  "initialize": {
    "implementation": "custom",
    "custom_code": "// C++ code here"
  }
}
```

---

## Migration Guide

### From Hardcoded to Metadata-Driven

1. **Extract register definitions** from existing code
2. **Create peripheral metadata** JSON file
3. **Define operations** in platform metadata
4. **Test generation** with templates
5. **Compare output** byte-for-byte with original
6. **Fix discrepancies** in metadata or templates

---

## Troubleshooting

### Validation Errors

```
ValidationError: 'vendor' is a required property
```
**Fix:** Add missing required field to metadata

### Template Errors

```
UndefinedError: 'dict object' has no attribute 'examples'
```
**Fix:** Ensure template context matches metadata structure

### Generation Failures

```
ValueError: MCU 'same70q21' not found in family 'same70'
```
**Fix:** Add MCU definition to family metadata

---

## Reference

- **Schemas:** `tools/codegen/schemas/`
- **Examples:** `tools/codegen/cli/generators/metadata/`
- **Templates:** `tools/codegen/templates/`
- **Tests:** `tools/codegen/tests/test_metadata_loader.py`
