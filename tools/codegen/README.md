# Alloy Code Generation System

Automated code generation from SVD files and JSON databases for supporting hundreds of MCUs.

## Quick Start

```bash
# 1. Initialize SVD database (first time only)
python tools/codegen/sync_svd.py --init

# 2. Sync/update SVDs from CMSIS repository
python tools/codegen/sync_svd.py --update --vendor STM32

# 3. Parse an SVD file to JSON database
python tools/codegen/svd_parser.py \
    --input database/svd/STM32/STM32F103xx.svd \
    --output database/families/stm32f1xx.json

# 4. Generate code for an MCU
python tools/codegen/generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output ../../build/generated/STM32F103C8

# 5. CMake will do this automatically!
# Just set ALLOY_MCU and build:
cmake -B build -DALLOY_MCU=STM32F103C8
cmake --build build
```

## Tools

### `sync_svd.py` - SVD Synchronization

Downloads and organizes SVD files from ARM CMSIS-Pack repository.

**Commands:**
```bash
# Initial setup
python sync_svd.py --init

# Update from upstream
python sync_svd.py --update

# Filter by vendor
python sync_svd.py --update --vendor STM32,nRF

# List available vendors
python sync_svd.py --list-vendors

# List MCUs for a vendor
python sync_svd.py --list-mcus STM32

# Dry run (preview only)
python sync_svd.py --dry-run --update
```

### `svd_parser.py` - SVD to JSON Converter

Parses SVD XML files into normalized JSON databases.

**Usage:**
```bash
# Parse single SVD
python svd_parser.py \
    --input database/svd/STM32/STM32F103xx.svd \
    --output database/families/stm32f1xx.json

# Merge into existing database
python svd_parser.py \
    --input database/svd/STM32/STM32F446xx.svd \
    --output database/families/stm32f4xx.json \
    --merge

# Verbose mode
python svd_parser.py -v --input STM32F103.svd --output out.json
```

### `generator.py` - Code Generator

Generates C++ startup code, register definitions, and linker scripts from JSON databases.

**Usage:**
```bash
# Generate all files for an MCU
python generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output build/generated/STM32F103C8

# Validate only (don't write files)
python generator.py --validate --mcu STM32F103C8 --database stm32f1xx.json

# Verbose mode
python generator.py -v --mcu STM32F103C8 --database stm32f1xx.json --output out/
```

## Directory Structure

```
tools/codegen/
├── sync_svd.py              # SVD downloader/sync script
├── svd_parser.py            # SVD XML → JSON converter
├── generator.py             # Code generator (JSON → C++)
├── validate_database.py     # Database schema validator
│
├── database/
│   ├── svd/                 # Organized symlinks to SVDs
│   │   ├── STM32/          # ST Microelectronics
│   │   ├── nRF/            # Nordic Semiconductor
│   │   └── ...
│   │
│   └── families/            # Normalized JSON databases
│       ├── stm32f1xx.json  # STM32F1 family
│       ├── stm32f4xx.json  # STM32F4 family
│       └── ...
│
├── templates/               # Jinja2 code templates
│   ├── common/             # Shared macros and headers
│   ├── startup/            # Startup code templates
│   ├── registers/          # Register definition templates
│   └── linker/             # Linker script templates
│
├── upstream/                # Git submodules (ignored)
│   └── cmsis-svd-data/     # CMSIS-SVD data repository
│
└── tests/                   # Unit and integration tests
    ├── test_parser.py
    ├── test_generator.py
    └── fixtures/
```

## Database Format

JSON databases contain normalized MCU information:

```json
{
  "family": "STM32F1",
  "architecture": "arm-cortex-m3",
  "vendor": "ST",
  "mcus": {
    "STM32F103C8": {
      "flash": {
        "size_kb": 64,
        "base_address": "0x08000000"
      },
      "ram": {
        "size_kb": 20,
        "base_address": "0x20000000"
      },
      "peripherals": {
        "GPIO": {
          "instances": [
            {"name": "GPIOA", "base": "0x40010800"}
          ],
          "registers": {
            "ODR": {"offset": "0x0C", "size": 32}
          }
        }
      },
      "interrupts": {
        "vectors": [
          {"number": 1, "name": "Reset_Handler"}
        ]
      }
    }
  }
}
```

## Generated Files

For each MCU, the generator produces:

- **`startup.cpp`** - Reset handler, `.data`/`.bss` initialization
- **`vectors.cpp`** - Interrupt vector table
- **`registers.hpp`** - Peripheral register structs
- **`{mcu}.ld`** - Linker script with memory layout

## CMake Integration

Code generation is automatic and transparent:

```cmake
# User CMakeLists.txt
set(ALLOY_MCU "STM32F103C8")

# CMake automatically:
# 1. Detects MCU needs code generation
# 2. Runs generator.py if needed
# 3. Adds generated files to build
# 4. Uses generated linker script
```

## Adding a New ARM MCU

1. **Download SVD** (if not already synced):
   ```bash
   python sync_svd.py --update --vendor STM32
   ```

2. **Parse to JSON**:
   ```bash
   python svd_parser.py \
       --input database/svd/STM32/STM32F446xx.svd \
       --output database/families/stm32f4xx.json \
       --merge
   ```

3. **Test generation**:
   ```bash
   python generator.py \
       --mcu STM32F446RE \
       --database database/families/stm32f4xx.json \
       --output /tmp/test_gen
   ```

4. **Use in project**:
   ```cmake
   set(ALLOY_MCU "STM32F446RE")
   ```

## Dependencies

- Python 3.8+
- pip packages: `jinja2`, `lxml`, `requests`
- Git (for submodules)

Install dependencies:
```bash
pip install jinja2 lxml requests
```

## Testing

```bash
# Run all tests
pytest tools/codegen/tests/

# Run specific test
pytest tools/codegen/tests/test_parser.py

# Integration test
python tools/codegen/tests/integration_test.py
```

## Troubleshooting

**Problem:** `sync_svd.py --init` fails

**Solution:** Check git submodule is properly initialized:
```bash
git submodule update --init --recursive --depth 1
```

**Problem:** Parser fails on SVD file

**Solution:** Check SVD format, try verbose mode:
```bash
python svd_parser.py -v --input file.svd --output out.json
```

**Problem:** Generated code doesn't compile

**Solution:** Validate database first:
```bash
python validate_database.py database/families/*.json
```

## License

Same as Alloy framework (see root LICENSE file).
