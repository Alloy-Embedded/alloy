# Tutorial: Adding a New ARM MCU to Alloy

This tutorial walks you through adding support for a new ARM Cortex-M microcontroller to the Alloy framework using the code generation system.

## Overview

Adding a new MCU involves three main steps:
1. **Obtain the SVD file** from the vendor
2. **Parse the SVD** to generate a database
3. **Integrate with CMake** to generate code automatically

## Prerequisites

- Python 3.8 or later
- CMSIS-SVD data repository synced
- Basic understanding of your target MCU

## Step 1: Sync SVD Repository

First, ensure you have the latest SVD files from CMSIS-SVD:

```bash
cd tools/codegen
python3 sync_svd.py --init
```

This downloads the CMSIS-SVD repository containing SVD files for hundreds of ARM MCUs.

To see available vendors:
```bash
python3 sync_svd.py --list-vendors
```

To see MCUs from a specific vendor:
```bash
python3 sync_svd.py --list-mcus STMicro
```

## Step 2: Locate Your MCU's SVD File

SVD files are organized by vendor in `upstream/cmsis-svd-data/data/`.

**Example:** For STM32F103:
```bash
ls upstream/cmsis-svd-data/data/STMicro/STM32F103*.svd
# Output: STM32F103xx.svd
```

**Common locations:**
- STMicro: `upstream/cmsis-svd-data/data/STMicro/`
- Nordic: `upstream/cmsis-svd-data/data/Nordic/`
- NXP: `upstream/cmsis-svd-data/data/NXP/`
- Atmel/Microchip: `upstream/cmsis-svd-data/data/Atmel/`

## Step 3: Parse the SVD File

Use `svd_parser.py` to convert the SVD XML to Alloy's JSON database format:

```bash
python3 svd_parser.py \
    --input upstream/cmsis-svd-data/data/STMicro/STM32F103xx.svd \
    --output database/families/stm32f1xx.json \
    --verbose
```

**Output:**
```
→ Parsing STM32F103xx.svd...
→ Device: STM32F103xx
→ Found 24 peripheral types
✓ Parsed STM32F103xx successfully
✓ Database written to database/families/stm32f1xx.json
```

### Parsing Multiple MCUs into One Family

If you want to add multiple MCUs to the same family:

```bash
# First MCU creates the family
python3 svd_parser.py \
    --input STM32F103C8.svd \
    --output database/families/stm32f1xx.json

# Subsequent MCUs merge into existing family
python3 svd_parser.py \
    --input STM32F103CB.svd \
    --output database/families/stm32f1xx.json \
    --merge
```

## Step 4: Validate the Database

Always validate after parsing:

```bash
python3 validate_database.py database/families/stm32f1xx.json
```

**Expected output:**
```
Validating: stm32f1xx.json
============================================================
✓ Database is valid!
✓ All 1 database(s) validated successfully!
```

**Common validation errors:**
- Missing required fields (flash, ram, interrupts)
- Invalid address format (must be 0xXXXXXXXX)
- Duplicate interrupt vector numbers
- Invalid register offsets

Fix these by manually editing the JSON file.

## Step 5: Customize the Database (Optional)

You may want to customize the generated database:

### Edit MCU Name

```json
{
  "mcus": {
    "STM32F103C8": {  // Rename from STM32F103xx to specific variant
      "description": "STM32F103C8 - 64KB Flash, 20KB RAM (Blue Pill)",
      ...
    }
  }
}
```

### Add Missing Memory Info

SVD files sometimes lack memory information:

```json
"flash": {
  "size_kb": 64,           // Update if wrong
  "base_address": "0x08000000",
  "page_size_kb": 1        // Add if missing
},
"ram": {
  "size_kb": 20,           // Update if wrong
  "base_address": "0x20000000"
}
```

### Simplify Peripherals (Optional)

For minimal builds, you can remove unused peripherals:

```json
"peripherals": {
  "GPIO": { ... },    // Keep
  "USART": { ... },   // Keep
  "CAN": { ... }      // Remove if not using CAN
}
```

## Step 6: Test Code Generation

Generate code to verify everything works:

```bash
python3 generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output /tmp/test_gen \
    --verbose
```

**Expected output:**
```
→ Generating code for STM32F103C8...
→ Generating startup code...
✓ Generated: /tmp/test_gen/startup.cpp
✓ Code generation complete in /tmp/test_gen
```

Inspect the generated file:
```bash
cat /tmp/test_gen/startup.cpp
```

**Check for:**
- Correct MCU name in header
- All expected interrupt handlers
- Proper Reset_Handler implementation
- No template artifacts (`{{` or `{%`)

## Step 7: Create a Board Configuration

Create a CMake board file for your hardware:

**`cmake/boards/my_board.cmake`:**
```cmake
# Alloy Board: My Custom Board
# Description of your board

# Board identification
set(ALLOY_BOARD_NAME "My Custom Board" CACHE STRING "Board name")
set(ALLOY_MCU "STM32F103C8" CACHE STRING "MCU model")
set(ALLOY_ARCH "arm-cortex-m3" CACHE STRING "CPU architecture")

# Clock configuration
set(ALLOY_CLOCK_FREQ_HZ 72000000 CACHE STRING "System clock frequency")

# Peripherals available
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)

# Memory
set(ALLOY_FLASH_SIZE "64KB" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "20KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake"
    CACHE FILEPATH "Toolchain file")

# Include code generation
include(codegen)

if(ALLOY_CODEGEN_AVAILABLE)
    alloy_generate_code(
        MCU STM32F103C8
        FAMILY stm32f1xx
    )

    # Make generated files available
    include_directories(${ALLOY_GENERATED_DIR})
endif()

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
```

## Step 8: Build with CMake

Configure and build your project:

```bash
mkdir -p build
cd build

cmake -DALLOY_BOARD=my_board ..
```

**Expected output:**
```
-- Code generation: ENABLED
--   Generator: .../tools/codegen/generator.py
--   Databases: .../tools/codegen/database/families
-- Generating code for STM32F103C8 (marker file missing)...
-- Code generation complete: .../build/generated/STM32F103C8
-- Generated sources:
--   - .../build/generated/STM32F103C8/startup.cpp
```

The code is automatically generated during CMake configuration!

## Step 9: Use Generated Code

In your application, you can now override weak handlers:

**`main.cpp`:**
```cpp
#include <cstdint>

// Override weak SystemInit
extern "C" void SystemInit() {
    // Configure system clock to 72MHz
    // Enable peripherals
}

// Override weak interrupt handler
extern "C" void USART1_IRQHandler() {
    // Handle USART1 interrupt
}

int main() {
    // Your application code
    while (true) {
        __asm__ volatile("wfi");
    }
    return 0;
}
```

The generated `startup.cpp` provides:
- `Reset_Handler` with .data/.bss initialization
- Vector table with all interrupts
- Weak default handlers you can override
- C++ static constructor support

## Troubleshooting

### SVD File Not Found

**Problem:** Can't find SVD for your MCU.

**Solutions:**
1. Check vendor website for SVD files
2. Use a similar MCU's SVD as starting point
3. Create minimal SVD manually (see CMSIS-SVD spec)

### Parser Warnings

**Warning:** `Flash info not found in SVD, using defaults`

**Solution:** This is normal. Edit the generated JSON to set correct memory sizes:
```json
"flash": {
  "size_kb": 128,  // Update this
  "base_address": "0x08000000"
}
```

### CMake Can't Find MCU

**Error:** `MCU 'MY_MCU' not found in database`

**Solution:** Ensure MCU name in board file matches database:
```cmake
set(ALLOY_MCU "STM32F103C8")  # Must match JSON key
```

### Generated Code Won't Compile

**Problem:** Compilation errors in generated code.

**Solutions:**
1. Check that database has correct interrupt vectors
2. Verify peripheral addresses are correct
3. Re-run parser with `--verbose` to see warnings

## Advanced: Creating Custom SVD

If no SVD exists for your MCU, create a minimal one:

**`my_mcu.svd`:**
```xml
<?xml version="1.0" encoding="utf-8"?>
<device schemaVersion="1.1">
  <name>MY_MCU</name>
  <description>My Custom MCU</description>

  <cpu>
    <name>CM3</name>
    <revision>r2p1</revision>
    <endian>little</endian>
  </cpu>

  <peripherals>
    <peripheral>
      <name>GPIOA</name>
      <baseAddress>0x40010800</baseAddress>
      <registers>
        <register>
          <name>ODR</name>
          <addressOffset>0x0C</addressOffset>
          <size>32</size>
        </register>
      </registers>
    </peripheral>
  </peripherals>
</device>
```

Then parse normally:
```bash
python3 svd_parser.py --input my_mcu.svd --output database/families/my_family.json
```

## Summary

You've successfully added a new MCU to Alloy! The steps were:

1. ✅ Synced SVD repository
2. ✅ Located MCU's SVD file
3. ✅ Parsed SVD to JSON database
4. ✅ Validated database
5. ✅ Tested code generation
6. ✅ Created board configuration
7. ✅ Integrated with CMake build
8. ✅ Built and tested

Your MCU is now fully supported with auto-generated startup code!

## Next Steps

- Create a blinky example for your board
- Add HAL implementation for your peripherals
- Contribute your board configuration back to Alloy
- Document pin mappings for your board

## Resources

- CMSIS-SVD Specification: https://arm-software.github.io/CMSIS_5/SVD/html/index.html
- Alloy Code Generator README: `tools/codegen/README.md`
- Database Schema: `tools/codegen/database_schema.json`
- Template Documentation: `tools/codegen/docs/TEMPLATES.md`
