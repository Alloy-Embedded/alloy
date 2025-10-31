# MCU Database Documentation

This directory contains normalized JSON databases describing MCU configurations for code generation.

## Directory Structure

```
database/
├── svd/                    # Organized symlinks to vendor SVD files
│   ├── STMicro/           # → ../../upstream/cmsis-svd-data/data/STMicro/
│   ├── Nordic/            # → ../../upstream/cmsis-svd-data/data/Nordic/
│   └── ...
│
├── families/              # Normalized JSON databases
│   ├── example.json       # Example/template database
│   ├── stm32f1xx.json    # STM32F1 family (multiple MCUs)
│   └── stm32f4xx.json    # STM32F4 family
│
├── database_schema.json   # JSON schema for validation
└── README.md             # This file
```

## Database Format

Each JSON file represents a **family** of MCUs that share similar architecture and peripherals.

### Top-Level Structure

```json
{
  "family": "STM32F1",
  "architecture": "arm-cortex-m3",
  "vendor": "ST",
  "mcus": {
    "STM32F103C8": { /* MCU config */ },
    "STM32F103RB": { /* MCU config */ }
  }
}
```

### MCU Configuration

Each MCU entry contains:

#### Memory Layout

```json
"flash": {
  "size_kb": 64,
  "base_address": "0x08000000",
  "page_size_kb": 1
},
"ram": {
  "size_kb": 20,
  "base_address": "0x20000000"
}
```

#### Peripherals

```json
"peripherals": {
  "GPIO": {
    "instances": [
      {"name": "GPIOA", "base": "0x40010800"},
      {"name": "GPIOB", "base": "0x40010C00"}
    ],
    "registers": {
      "ODR": {"offset": "0x0C", "size": 32, "description": "Output data register"},
      "IDR": {"offset": "0x08", "size": 32, "description": "Input data register"}
    }
  }
}
```

**Peripheral Types:**
- `GPIO` - General-purpose I/O
- `USART` / `UART` - Serial communication
- `SPI` - Serial Peripheral Interface
- `I2C` - Inter-Integrated Circuit
- `ADC` - Analog-to-Digital Converter
- `TIM` - Timers
- `RTC` - Real-Time Clock
- `DMA` - Direct Memory Access

#### Interrupt Vectors

```json
"interrupts": {
  "count": 68,
  "vectors": [
    {"number": 0, "name": "Initial_SP"},
    {"number": 1, "name": "Reset_Handler"},
    {"number": 2, "name": "NMI_Handler"},
    {"number": 3, "name": "HardFault_Handler"},
    {"number": 15, "name": "SysTick_Handler"},
    {"number": 37, "name": "USART1_IRQHandler"}
  ]
}
```

**Standard ARM Cortex-M vectors (0-15):**
- 0: Initial Stack Pointer
- 1: Reset Handler
- 2: NMI Handler
- 3: HardFault Handler
- 4-10: Other fault handlers
- 11: SVC Handler
- 14: PendSV Handler
- 15: SysTick Handler

#### Clock Configuration (Optional)

```json
"clock": {
  "max_freq_mhz": 72,
  "hse_freq_mhz": 8,
  "hsi_freq_mhz": 8
}
```

## Creating a Database

### Option 1: Parse from SVD (ARM MCUs)

```bash
# Parse SVD file to JSON
python svd_parser.py \
    --input database/svd/STMicro/STM32F103xx.svd \
    --output database/families/stm32f1xx.json

# Merge additional MCUs into existing family
python svd_parser.py \
    --input database/svd/STMicro/STM32F103xB.svd \
    --output database/families/stm32f1xx.json \
    --merge
```

### Option 2: Manual Creation (Non-ARM MCUs)

1. Copy `example.json` as template
2. Fill in MCU details from datasheet
3. Validate against schema:

```bash
python validate_database.py database/families/your_mcu.json
```

## Validation

Validate database files against the schema:

```bash
# Validate single file
python validate_database.py database/families/stm32f1xx.json

# Validate all databases
python validate_database.py database/families/*.json
```

## Naming Conventions

### MCU Names
- Use official vendor names (e.g., `STM32F103C8`, not `stm32f103c8`)
- Match the marketing name exactly

### Peripheral Instances
- Use vendor naming (e.g., `GPIOA`, `USART1`, `TIM2`)
- Maintain alphabetical/numerical order

### Register Names
- Use official names from datasheet/SVD
- All uppercase (e.g., `ODR`, `BSRR`, `CR1`)

### Addresses
- Always hex strings with `0x` prefix
- Pad to 8 digits for memory addresses: `0x08000000`
- Pad to 2-4 digits for offsets: `0x0C`, `0x100`

## Best Practices

### Grouping MCUs
Group MCUs by family when they share:
- Same architecture (Cortex-M3, Cortex-M4, etc.)
- Similar peripheral set
- Compatible register layouts

Example: `STM32F103C8`, `STM32F103CB`, `STM32F103RB` → `stm32f1xx.json`

### Minimal vs Complete
For MVP, include only:
- Flash/RAM layout
- GPIO registers (for blinky)
- UART registers (for serial)
- Interrupt vectors

Can expand later with:
- All peripherals
- Clock tree details
- DMA channels
- Pin multiplexing

### Testing
After creating/modifying a database:

1. **Validate schema**:
   ```bash
   python validate_database.py your_database.json
   ```

2. **Test generation**:
   ```bash
   python generator.py \
       --mcu YOUR_MCU \
       --database your_database.json \
       --output /tmp/test_gen
   ```

3. **Compile generated code**:
   ```bash
   arm-none-eabi-g++ -c /tmp/test_gen/startup.cpp
   ```

## Examples

See `example.json` for a complete, documented example database.

## Schema Reference

Full JSON schema is available in `database_schema.json`.

Validate with:
```bash
jsonschema -i database/families/your_file.json database_schema.json
```

## Contributing

When adding support for a new MCU family:

1. Parse or create database JSON
2. Validate against schema
3. Test code generation
4. Document any vendor-specific quirks
5. Add to index in pull request

## Troubleshooting

**Problem:** Parser fails on SVD

- Check SVD file is valid XML
- Look for `derivedFrom` inheritance (may need parent MCU)
- Try verbose mode: `python svd_parser.py -v ...`

**Problem:** Generated code doesn't compile

- Validate addresses are correct (check datasheet)
- Verify register offsets match reference manual
- Check interrupt numbers are sequential

**Problem:** Validation fails

- Check all addresses use `0x` prefix
- Verify register sizes are 8, 16, or 32 bits
- Ensure required fields are present
