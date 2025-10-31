# Alloy Code Generation System

Automated code generation from CMSIS-SVD files to C++ peripheral definitions for 24+ vendors and 800+ MCUs.

## Quick Start

### One-Command Update for Any Vendor

```bash
# Generate for STMicroelectronics
python3 update_all_vendors.py --vendor STMicro

# Generate for Nordic Semiconductor
python3 update_all_vendors.py --vendor Nordic

# Generate for specific family
python3 update_all_vendors.py --vendor STMicro --family stm32f1

# List available vendors
python3 update_all_vendors.py --list

# Process all vendors (may take 5-10 minutes)
python3 update_all_vendors.py --all
```

That's it! The script automatically:
1. Finds all SVD files for the vendor
2. Parses them to JSON databases
3. Generates C++ peripheral code

## Pipeline Overview

```
SVD Files → svd_parser.py → JSON Database → generate_all.py → C++ Code
                     ↑                                            ↑
              update_all_vendors.py (orchestrates everything)
```

## Supported Vendors

**24+ vendors, 800+ MCUs supported:**

| Vendor | Families | MCU Count |
|--------|----------|-----------|
| **STMicroelectronics** | STM32F0/F1/F2/F3/F4/F7, STM32G0/G4, STM32H7, STM32L0/L1/L4, STM32U0/U5 | 112 |
| **Nordic Semiconductor** | nRF51, nRF52, nRF5340, nRF9160 | 11 |
| **Atmel/Microchip** | SAMD, SAME, SAML, SAM9, SAMA5 | 220 |
| **NXP/Freescale** | Kinetis (MKL, MK, MKE, MKV, MKW) | 166 |
| **Espressif** | ESP32, ESP32-C2/C3/C6, ESP32-S2/S3, ESP32-H2/P4 | 11 |
| **Renesas** | RA2, RA4, RA6 series | 21 |
| **Infineon** | XMC1000, XMC4000, TLE98xx | 18 |
| **Raspberry Pi** | RP2040, RP2350 | 2 |
| **GigaDevice** | GD32VF103 (RISC-V) | 1 |
| **SiFive** | E310X, FU540, FU740 (RISC-V) | 3 |
| **Fujitsu** | MB9AF, MB9BF series | 100 |

Plus: AlifSemi, ArteryTek, Cypress, Holtek, Kendryte, Nuvoton, Spansion, and more!

## Tools

### `update_all_vendors.py` - Complete Pipeline (NEW!)

One script to rule them all. Orchestrates SVD parsing and code generation for any vendor.

```bash
# Process specific vendor
python3 update_all_vendors.py --vendor STMicro

# Process specific family
python3 update_all_vendors.py --vendor Nordic --family nrf52

# Parse only (skip code generation)
python3 update_all_vendors.py --vendor Espressif --parse-only

# Generate code only (from existing databases)
python3 update_all_vendors.py --vendor STMicro --generate-only

# List all available vendors and families
python3 update_all_vendors.py --list
```

**Workflow:**
1. Scans `upstream/cmsis-svd-data/` for SVD files
2. Groups by vendor and family
3. Runs `svd_parser.py` on each SVD file
4. Runs `generate_all.py` to create C++ code
5. Outputs to `src/generated/{vendor}/{family}/{mcu}/`

### `svd_parser.py` - SVD to JSON Parser

Parses CMSIS-SVD XML files and extracts peripheral information.

```bash
python3 svd_parser.py \
    --input upstream/cmsis-svd-data/data/STMicro/STM32F103xx.svd \
    --output database/families/stm32f103.json \
    --verbose
```

**Features:**
- Multi-vendor peripheral classification
- Automatic bit field extraction
- Supports 20+ peripheral types
- Handles vendor naming variations:
  - I2C: I2C, TWI, TWIM, TWIS
  - UART: USART, UART, UARTE
  - SPI: SPI, SPIM, SPIS, QSPI
  - Timer: TIM, TIMER, TC, TCC, TPM, PIT, LPTMR, TIMG

### `generate_all.py` - Batch Code Generator

Generates C++ code from JSON databases for all MCUs.

```bash
# Generate for specific vendor
python3 generate_all.py --vendor STMicro

# Generate for all vendors
python3 generate_all.py --all
```

**Output:**
- `peripherals.hpp` - Register structures and bit definitions
- `startup.cpp` - Startup code and interrupt vectors
- `README.md` - Documentation for each vendor

### `generator.py` - Core Code Generator

Low-level generator used by `generate_all.py`. Uses Jinja2 templates.

```bash
python3 generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output src/generated/st/stm32f1/stm32f103c8
```

## Directory Structure

```
tools/codegen/
├── update_all_vendors.py    # 🆕 Main pipeline orchestrator
├── svd_parser.py             # SVD → JSON parser
├── generate_all.py           # JSON → C++ batch generator
├── generator.py              # Core code generator
├── validate_database.py      # Database validator
│
├── database/
│   └── families/             # Generated JSON databases
│       ├── st_stm32f1.json   # STM32F1 family
│       ├── st_stm32f4.json   # STM32F4 family
│       ├── nordic_nrf52.json # Nordic nRF52
│       └── ...
│
├── templates/                # Jinja2 templates
│   ├── common/               # Shared headers
│   ├── peripherals/          # Register definitions
│   └── startup/              # Startup code
│
└── upstream/
    └── cmsis-svd-data/       # Git submodule (SVD files)
```

## Generated Code Structure

```
src/generated/
├── st/                           # STMicroelectronics
│   ├── stm32f1/
│   │   ├── stm32f103c8/
│   │   │   ├── peripherals.hpp   # 5450 lines, 23 peripherals
│   │   │   └── startup.cpp
│   │   └── ...
│   ├── stm32f4/
│   └── README.md
│
├── nordic/                       # Nordic Semiconductor
│   ├── nrf52/
│   │   ├── nrf52/
│   │   ├── nrf52840/
│   │   └── ...
│   └── README.md
│
├── espressif/                    # Espressif
│   ├── esp32/
│   ├── esp32c3/
│   └── README.md
│
└── INDEX.md                      # Master index of all MCUs
```

## Peripheral Support

The parser automatically detects and normalizes peripherals from all vendors:

### Standard Peripherals
- **GPIO**: GPIO, GPIOTE (Nordic), PORT (Atmel/NXP)
- **UART**: USART, UART, UARTE (Nordic)
- **SPI**: SPI, SPIM/SPIS (Nordic), QSPI
- **I2C**: I2C, TWI, TWIM/TWIS (Nordic)
- **Timer**: TIM (ST), TIMER (Nordic), TC/TCC (Atmel), TPM/PIT/LPTMR (NXP), TIMG (ESP32)
- **ADC**: ADC, SAADC (Nordic), SARADC (ESP32)
- **DAC**: DAC
- **DMA**: DMA, DMAC (Atmel), DMAMUX
- **RTC**: RTC
- **WDG**: WWDG, IWDG (ST), WDT (Nordic/others)

### Vendor-Specific
- **SERCOM** (Atmel): Multi-function serial (UART/SPI/I2C)
- **TWAI** (ESP32): CAN variant
- **LEDC** (ESP32): LED PWM controller
- **PWM**: PWM controllers
- **RCC/CLOCK**: Reset and clock control (varies by vendor)
- **CRYPTO**: AES, SHA, RSA, HMAC
- **RADIO**: BLE, Radio peripherals (Nordic, ESP32)
- **I2S**: Audio interfaces

## Using Generated Code

### In CMakeLists.txt

```cmake
# Set your MCU
set(ALLOY_MCU "STM32F103C8")

# Generated code directory
set(ALLOY_GENERATED_DIR
    "${CMAKE_SOURCE_DIR}/src/generated/st/stm32f1/stm32f103c8")

# Add startup code
target_sources(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}/startup.cpp
)

# Include peripheral definitions
target_include_directories(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}
)
```

### In C++ Code

```cpp
#include "peripherals.hpp"

using namespace alloy::generated::stm32f103c8;

void blink_led() {
    // Enable GPIOC clock
    auto* rcc = rcc::RCC;
    rcc->APB2ENR |= rcc::apb2enr_bits::IOPCEN;

    // Configure PC13 as output
    auto* gpioc = gpio::GPIOC;
    gpioc->CRH &= ~(0xF << 20);  // Clear bits
    gpioc->CRH |= (0x2 << 20);   // Output 2MHz

    // Toggle LED
    gpioc->ODR ^= (1U << 13);
}

void uart_init() {
    auto* usart1 = usart::USART1;
    auto* rcc = rcc::RCC;

    // Enable USART1 clock
    rcc->APB2ENR |= rcc::apb2enr_bits::USART1EN;

    // Configure 115200 baud, 8N1
    usart1->BRR = 625;  // 72MHz / 115200
    usart1->CR1 |= usart::cr1_bits::TE | usart::cr1_bits::RE;
    usart1->CR1 |= usart::cr1_bits::UE;
}
```

## Examples

### Update All STM32 Code

```bash
python3 update_all_vendors.py --vendor STMicro
```

**Result:**
- 70+ families processed
- 112 MCUs supported
- 23 peripheral types per MCU
- ~100MB of generated code

### Update Nordic nRF52 Family

```bash
python3 update_all_vendors.py --vendor Nordic --family nrf52
```

**Result:**
- 8 nRF52 variants (nRF52805/810/811/820/833/840, nRF5340, nRF9160)
- UARTE, SPIM/SPIS, TWIM/TWIS support
- 43 peripheral types including RADIO, BLE

### Update ESP32 Family

```bash
python3 update_all_vendors.py --vendor Espressif
```

**Result:**
- 8 families (ESP32, C2, C3, C6, H2, P4, S2, S3)
- TWAI (CAN), LEDC (PWM), TIMG (Timer Groups)
- 27 peripheral types

## Updating Generated Code

When SVD files are updated:

```bash
# 1. Update SVD submodule
cd tools/codegen
git submodule update --remote upstream/cmsis-svd-data

# 2. Regenerate for specific vendor
python3 update_all_vendors.py --vendor STMicro

# 3. Or regenerate everything (takes ~10 minutes)
python3 update_all_vendors.py --all

# 4. Commit changes
git add src/generated/ tools/codegen/database/
git commit -m "Update generated code from latest SVD files"
```

## Database Format

Generated JSON databases follow this structure:

```json
{
  "family": "STM32F1",
  "architecture": "arm-cortex-m3",
  "vendor": "STMicro",
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
            {"name": "GPIOA", "base": "0x40010800", "irq": 6}
          ],
          "registers": {
            "CRL": {"offset": "0x00", "size": 32, "description": "..."}
          },
          "bits": {
            "CRL": {
              "MODE0": {"bit": 0, "width": 2, "description": "..."}
            }
          }
        }
      }
    }
  }
}
```

## Requirements

- Python 3.7+
- Jinja2: `pip install jinja2`
- CMSIS-SVD data: `git submodule update --init --recursive`

## Performance

- **SVD Parsing**: ~1-2 seconds per file
- **Code Generation**: ~100ms per MCU
- **Full Pipeline** (single vendor): ~1-2 minutes
- **All Vendors**: ~5-10 minutes

## Troubleshooting

### No SVD Files Found

```bash
# Initialize submodule
git submodule update --init --recursive

# Or clone with submodules
git clone --recursive https://github.com/your-org/corezero
```

### Parse Errors

Some SVD files have errors. The parser will skip problematic files and continue with others.

### Missing Peripherals

If a peripheral isn't being detected, check `svd_parser.py:271` (`_classify_peripheral()`) and add the pattern.

Example:
```python
elif 'CUSTOM_PERIPH' in name_upper:
    return 'CUSTOM'
```

## Contributing

To add support for a new peripheral naming convention:

1. Edit `tools/codegen/svd_parser.py`
2. Add detection pattern in `_classify_peripheral()` method
3. Test with representative SVD file
4. Run parser and verify classification

## License

See LICENSE file in repository root.
