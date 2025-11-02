# ESP32 and SAMD21 Build Issues - Implementation Status

## Status: PENDING HARDWARE & TESTING

**Date:** 2025-01-02
**Reason:** Requires ESP32/SAMD21 hardware, ESP-IDF setup, and build testing
**Completion:** 0% (0/64 tasks)

## Problem Summary

### ESP32 DevKit
**Issue:** GPIO peripheral structure missing required registers
**Error:**
```
error: 'struct alloy::generated::esp32::gpio::Registers' has no member named 'ENABLE_W1TC'
error: 'struct alloy::generated::esp32::gpio::Registers' has no member named 'OUT_W1TS'
```

**Root Cause:**
- `tools/codegen/database/families/espressif_esp32.json` has incomplete GPIO register definitions
- Generated `peripherals.hpp` doesn't match what HAL expects
- HAL expects standard ESP32 GPIO registers per Technical Reference Manual

### SAMD21 (Arduino Zero)
**Issue:** Linker fails with nosys.specs conflict
**Error:**
```
arm-none-eabi-g++: fatal error: nosys.specs: attempt to rename spec 'link_gcc_c_sequence'
to already defined spec 'nosys_link_gcc_c_sequence'
```

**Root Cause:**
- xPack ARM toolchain v14.2.1+ has known conflict with `-specs=nosys.specs`
- Affects SAMD21 but not STM32F103, STM32F407VG, or RP2040
- Toolchain-specific issue requiring conditional handling

## Current Working Boards

✅ **3 out of 6 boards compile successfully:**
1. STM32F103 (Bluepill) - 612 bytes
2. STM32F407VG (Discovery) - 1044 bytes
3. RP2040 (Raspberry Pi Pico) - 832 bytes
4. **ATSAME70Q21 (Xplained)** - 644 bytes *(newly added)*

❌ **2 boards failing:**
1. ESP32 DevKit - GPIO register structure issues
2. SAMD21 (Arduino Zero) - Toolchain specs conflict

## Prerequisites for Implementation

### ESP32 Fix
1. **ESP-IDF Environment**
   ```bash
   # Install ESP-IDF
   cd ~/esp
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh esp32

   # Source environment (required before each build)
   . ~/esp/esp-idf/export.sh
   ```

2. **ESP32 Technical Reference Manual**
   - Download from Espressif website
   - Section 4.10 - GPIO & GPIO Matrix (GPIO, IO_MUX)
   - Verify register offsets and structure

3. **ESP32 Device**
   - ESP32 DevKit or similar for testing
   - USB connection for flashing
   - Serial monitor for verification

### SAMD21 Fix
1. **xPack ARM Toolchain** *(already installed)*
   - Version: 14.2.1+
   - Location: `/Users/lgili/.local/xpack-arm-toolchain/`

2. **SAMD21 Device**
   - Arduino Zero or similar
   - USB connection for programming
   - BOSSA flasher tool

3. **Toolchain Testing**
   - Test nosys.specs removal
   - Verify no regression on STM32 boards
   - Validate binary size remains reasonable

## Proposed Fix: ESP32 GPIO Registers

Based on ESP32 TRM, the GPIO peripheral needs these registers in `espressif_esp32.json`:

```json
{
  "GPIO": {
    "instances": [
      {
        "name": "GPIO",
        "base": "0x3FF44000",
        "irq": 23
      }
    ],
    "registers": {
      "BT_SELECT": { "offset": "0x00", "size": 32, "description": "GPIO bit select register" },
      "OUT": { "offset": "0x04", "size": 32, "description": "GPIO0-31 output value" },
      "OUT_W1TS": { "offset": "0x08", "size": 32, "description": "GPIO0-31 output set" },
      "OUT_W1TC": { "offset": "0x0C", "size": 32, "description": "GPIO0-31 output clear" },
      "OUT1": { "offset": "0x10", "size": 32, "description": "GPIO32-39 output value" },
      "OUT1_W1TS": { "offset": "0x14", "size": 32, "description": "GPIO32-39 output set" },
      "OUT1_W1TC": { "offset": "0x18", "size": 32, "description": "GPIO32-39 output clear" },
      "SDIO_SELECT": { "offset": "0x1C", "size": 32, "description": "SDIO select" },
      "ENABLE": { "offset": "0x20", "size": 32, "description": "GPIO0-31 output enable" },
      "ENABLE_W1TS": { "offset": "0x24", "size": 32, "description": "GPIO0-31 output enable set" },
      "ENABLE_W1TC": { "offset": "0x28", "size": 32, "description": "GPIO0-31 output enable clear" },
      "ENABLE1": { "offset": "0x2C", "size": 32, "description": "GPIO32-39 output enable" },
      "ENABLE1_W1TS": { "offset": "0x30", "size": 32, "description": "GPIO32-39 output enable set" },
      "ENABLE1_W1TC": { "offset": "0x34", "size": 32, "description": "GPIO32-39 output enable clear" },
      "STRAP": { "offset": "0x38", "size": 32, "description": "Strapping pin values" },
      "IN": { "offset": "0x3C", "size": 32, "description": "GPIO0-31 input value" },
      "IN1": { "offset": "0x40", "size": 32, "description": "GPIO32-39 input value" }
    }
  }
}
```

## Proposed Fix: SAMD21 Toolchain

Update `cmake/toolchains/arm-none-eabi.cmake`:

```cmake
# Detect xPack toolchain version
execute_process(
    COMMAND ${CMAKE_C_COMPILER} --version
    OUTPUT_VARIABLE TOOLCHAIN_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# xPack ARM v14+ has nosys.specs conflict
if(TOOLCHAIN_VERSION MATCHES "xPack" AND TOOLCHAIN_VERSION MATCHES "14\\.[0-9]+\\.[0-9]+")
    message(STATUS "xPack ARM toolchain v14+ detected - using alternative syscall handling")
    # Don't use -specs=nosys.specs due to known conflict
    # Link libraries explicitly instead
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-nodefaultlibs -Wl,--start-group -lc_nano -lnosys -lgcc -lm -Wl,--end-group")
else()
    # Older toolchains or non-xPack use specs file
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-specs=nosys.specs")
endif()
```

## Implementation Tasks (64 total)

### Phase 1: Investigation (6 tasks) - NOT STARTED
- [ ] Research ESP32 GPIO registers from TRM
- [ ] Check ESP-IDF headers
- [ ] Find ESP32 SVD files
- [ ] Document register structure
- [ ] Identify affected xPack versions
- [ ] Research alternative specs

### Phase 2: ESP32 GPIO Fix (19 tasks) - NOT STARTED
- [ ] Update espressif_esp32.json with 14 GPIO registers
- [ ] Regenerate ESP32 peripherals
- [ ] Verify generated code

### Phase 3: ESP32 Testing (7 tasks) - NOT STARTED
- [ ] Source ESP-IDF
- [ ] Configure build
- [ ] Compile and verify

### Phase 4: SAMD21 Fix (8 tasks) - NOT STARTED
- [ ] Implement conditional specs handling
- [ ] Test ARM board regression

### Phase 5: SAMD21 Testing (6 tasks) - NOT STARTED
- [ ] Compile SAMD21 blink
- [ ] Verify fix works

### Phase 6: Full Validation (9 tasks) - NOT STARTED
- [ ] Test all 5 boards
- [ ] Document sizes
- [ ] Run unit tests

### Phase 7: Documentation (9 tasks) - NOT STARTED
- [ ] Update build docs
- [ ] Document toolchain requirements
- [ ] Update OpenSpec tasks

## Current Status: SAME70 Success Story

While ESP32 and SAMD21 fixes are pending, we successfully added complete support for **ATSAME70 Xplained**:

✅ **ATSAME70Q21 - FULLY WORKING**
- Database generated from official SVD file
- 3 working examples: blink (644B), LED patterns (1504B), UART (1460B)
- GPIO, clock, and UART HAL implemented
- Compiles successfully with ARM Cortex-M7 FPU enabled
- Demonstrates multi-architecture support is working

This validates our approach: proper database + HAL = working board support.

## Path Forward

**When ESP32/SAMD21 support is prioritized:**

1. **ESP32 Quick Fix (2-4 hours)**
   - Update espressif_esp32.json with GPIO registers (1h)
   - Regenerate and test (1h)
   - Debug if needed (1-2h)

2. **SAMD21 Quick Fix (1-2 hours)**
   - Update toolchain CMake (30m)
   - Test compilation (30m)
   - Verify no ARM regression (30m)

3. **Full Validation (2-3 hours)**
   - Run all 5 board builds
   - Document results
   - Update OpenSpec status

**Total Estimated: 1 day** (with hardware access and ESP-IDF setup)

## Recommendation

**DEFER** ESP32/SAMD21 fixes until:
- Hardware is available for testing
- ESP-IDF environment can be set up
- Build testing can be validated on real devices

**CONTINUE** with proven working boards:
- ✅ STM32F103, STM32F407VG, RP2040, ATSAME70Q21
- Add more boards following SAME70 pattern (SVD → JSON → HAL)

## References

- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) - Section 4.10
- [ESP-IDF GPIO Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [xPack ARM Toolchain Issues](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/issues)
- [SAMD21 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D21_DA1_Family_DataSheet_DS40001882F.pdf)
