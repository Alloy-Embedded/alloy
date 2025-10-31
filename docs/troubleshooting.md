# Troubleshooting Guide

Common issues and solutions when working with Alloy Framework.

## Build Issues

### SAMD21: nosys.specs Error

**Error**:
```
arm-none-eabi-g++: fatal error: nosys.specs: attempt to rename spec 'link_gcc_c_sequence'
to already defined spec 'nosys_link_gcc_c_sequence'
```

**Affected**:
- Arduino Zero (ATSAMD21G18)
- xPack ARM toolchain v14.2.1 and higher

**Status**: ⚠️ Known issue - Fix in progress

**Root Cause**: xPack ARM toolchain has internal spec conflict where `-specs=nosys.specs` attempts to rename a spec that's already defined.

**Workarounds**:

1. **Use older toolchain** (Temporary):
   ```bash
   # Use xPack v13.x or earlier
   # Or use distribution toolchain:
   sudo apt install gcc-arm-none-eabi  # Ubuntu/Debian
   ```

2. **Remove nosys.specs** (Manual CMake edit):
   ```cmake
   # In cmake/toolchains/arm-none-eabi.cmake
   # Comment out or conditionally disable:
   # set(CMAKE_EXE_LINKER_FLAGS "-specs=nosys.specs ...")
   ```

3. **Use nano.specs instead**:
   ```bash
   # Edit linker flags to use nano.specs without nosys
   cmake -DALLOY_BOARD=arduino_zero \
         -DCMAKE_EXE_LINKER_FLAGS="-specs=nano.specs" ..
   ```

**Permanent Fix**: Track progress in OpenSpec change `fix-esp32-samd21-build-issues`

---

### ESP32: GPIO Peripheral Errors

**Error**:
```
error: 'struct alloy::generated::esp32::gpio::Registers' has no member named 'OUT_W1TS'
error: 'struct alloy::generated::esp32::gpio::Registers' has no member named 'ENABLE_W1TC'
```

**Affected**:
- ESP32 DevKit (ESP32-WROOM-32)
- All ESP32 variants

**Status**: ⚠️ Known issue - Fix in progress

**Root Cause**: ESP32 JSON database (`espressif_esp32.json`) has incomplete GPIO peripheral definitions. Generated peripheral structure is missing critical registers.

**Workarounds**:

1. **Use alternative GPIO library** (Temporary):
   ```cpp
   // Use ESP-IDF GPIO driver directly instead of Alloy HAL
   #include "driver/gpio.h"
   gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
   gpio_set_level(GPIO_NUM_2, 1);
   ```

2. **Wait for fix**: Track progress in OpenSpec change `fix-esp32-samd21-build-issues`

**Permanent Fix**:
- Update `espressif_esp32.json` with correct GPIO register definitions
- Regenerate ESP32 peripherals
- Estimated timeline: 1-2 days

---

### Toolchain Not Found

**Error**:
```
CMake Error: Could not find toolchain file: arm-none-eabi-gcc
```

**Solution**:
```bash
# ARM boards - Install xPack toolchain
./scripts/install-xpack-toolchain.sh

# ESP32 - Source ESP-IDF
. ~/esp/esp-idf/export.sh

# Verify
arm-none-eabi-gcc --version  # ARM
xtensa-esp32-elf-gcc --version  # ESP32
```

---

### Board Not Found

**Error**:
```
CMake Warning: Board 'xxx' not found, using defaults
```

**Solution**:
Check valid board IDs:
```bash
ls cmake/boards/
# Valid: bluepill.cmake, stm32f407vg.cmake, rp_pico.cmake,
#        arduino_zero.cmake, esp32_devkit.cmake
```

Use correct board ID:
```bash
cmake -DALLOY_BOARD=bluepill ..  # ✓ Correct
cmake -DALLOY_BOARD=stm32f103 ..  # ✗ Wrong
```

---

### Binary Size Too Large

**Problem**: Binary exceeds flash size for target MCU.

**Diagnosis**:
```bash
# Check binary size
arm-none-eabi-size blink_stm32f103
#    text    data     bss     dec     hex filename
#   50000     100   10000   60100    eac4 blink_stm32f103

# STM32F103C8 has only 64KB flash → 50KB code is too much
```

**Solutions**:

1. **Use MinSizeRel build**:
   ```bash
   cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DALLOY_BOARD=bluepill ..
   make
   ```

2. **Enable link-time optimization**:
   ```bash
   cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
         -DCMAKE_BUILD_TYPE=MinSizeRel \
         -DALLOY_BOARD=bluepill ..
   ```

3. **Disable unused features**:
   ```cmake
   # In CMakeLists.txt
   add_compile_options(-fno-exceptions -fno-rtti)
   ```

4. **Remove debug symbols**:
   ```bash
   arm-none-eabi-strip blink_stm32f103
   ```

---

## Flash/Upload Issues

### ST-Link Not Detected

**Error**:
```
Error: No ST-Link detected
```

**Solutions**:

1. **Check USB connection**: Try different cable/port

2. **Linux: Add udev rules**:
   ```bash
   sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   # Reconnect ST-Link
   ```

3. **Update ST-Link firmware**:
   - Use STM32CubeProgrammer or ST-Link Utility
   - Download latest firmware

4. **Check permissions** (Linux):
   ```bash
   sudo usermod -a -G plugdev $USER
   # Log out and log back in
   ```

---

### ESP32 Not Entering Bootloader

**Problem**: ESP32 doesn't respond to esptool.

**Solutions**:

1. **Manual bootloader entry**:
   ```
   1. Hold BOOT button
   2. Press and release RESET button
   3. Release BOOT button
   4. Try flashing within 5 seconds
   ```

2. **Check USB driver** (Windows):
   - Install CP210x USB to UART driver
   - Or CH340 driver (depending on board)

3. **Try different baud rate**:
   ```bash
   # Default 115200 may not work, try slower
   esptool.py --port /dev/ttyUSB0 --baud 9600 write_flash ...
   ```

4. **Check USB cable**: Must be data cable, not charge-only

---

### RP2040 Not Appearing as USB Drive

**Problem**: Pico doesn't show up as "RPI-RP2" drive.

**Solutions**:

1. **Proper BOOTSEL sequence**:
   ```
   1. Disconnect USB
   2. Hold BOOTSEL button
   3. Connect USB while holding BOOTSEL
   4. Wait 1 second
   5. Release BOOTSEL
   ```

2. **Check USB cable**: Must support data transfer

3. **Try different USB port**: Some USB hubs don't work

4. **Erase flash** (if corrupted):
   ```bash
   # Download flash_nuke.uf2 from Raspberry Pi
   # Enter BOOTSEL mode and copy flash_nuke.uf2
   # This erases flash and returns to bootloader
   ```

---

### Permission Denied on /dev/ttyUSB0

**Error**:
```
Error: Permission denied: '/dev/ttyUSB0'
```

**Solution** (Linux):
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Alternative: temporary fix for current session only
sudo chmod 666 /dev/ttyUSB0

# Then log out and log back in (or reboot)
```

---

## Runtime Issues

### LED Not Blinking

**Diagnosis Steps**:

1. **Check LED polarity**:
   - Blue Pill PC13: Active LOW (led.set_low() = ON)
   - Pico GPIO25: Active HIGH (led.set_high() = ON)
   - Check board docs for your board

2. **Check pin number**:
   ```cpp
   // STM32: Port × 16 + Pin
   // PC13 = 2 × 16 + 13 = 45
   constexpr uint8_t LED_PIN = 45;  // ✓ Correct
   constexpr uint8_t LED_PIN = 13;  // ✗ Wrong (this is PA13)
   ```

3. **Check GPIO initialization**:
   ```cpp
   Board::Led::init();  // Must call before use
   ```

4. **Verify flash**:
   ```bash
   # Check if code actually flashed
   arm-none-eabi-objdump -d blink.elf | head -50
   # Should show Reset_Handler and main
   ```

5. **Check power**:
   - Verify 3.3V power LED is on
   - Measure 3.3V rail with multimeter

---

### Wrong Blink Frequency

**Problem**: LED blinks too fast or too slow.

**Causes**:

1. **Clock not configured**:
   ```cpp
   // Make sure clock is initialized!
   Board::initialize();  // Sets up PLL and system clock
   ```

2. **Using wrong delay function**:
   ```cpp
   // Using busy-wait instead of timer
   Board::delay_ms(500);  // ✓ Correct (uses system clock)

   // Manual delay loop - frequency dependent!
   for(volatile int i=0; i<1000000; i++);  // ✗ Timing varies
   ```

3. **Flash wait states incorrect** (STM32):
   - 72 MHz requires 2 wait states
   - Missing flash latency config causes CPU slowdown

---

### Hard Fault / Crash on Startup

**Common Causes**:

1. **Stack overflow**:
   - Check linker script stack size
   - STM32F103 has only 20KB RAM - be conservative

2. **Invalid peripheral access**:
   - Check clock enable before accessing peripheral
   - Verify peripheral base addresses

3. **Uninitialized .data section**:
   - Verify startup code copies .data from flash to RAM
   - Check `initialize_runtime()` in startup.cpp

4. **Vector table misalignment**:
   ```cpp
   // Vector table must be at 0x08000000 (or correct offset)
   // Check linker script FLASH origin
   ```

**Debug**:
```bash
# Use GDB with OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg &
arm-none-eabi-gdb blink.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) continue
# On crash:
(gdb) backtrace
```

---

## Code Generation Issues

### Invalid C++ Identifiers

**Error**:
```
error: expected unqualified-id before numeric constant
```

**Cause**: Generated code has invalid identifiers (e.g., starting with digit, using reserved keywords).

**Status**: ✅ Fixed (generator sanitizes identifiers)

**Solution**: Regenerate peripherals with latest generator:
```bash
python3 tools/codegen/generator.py \
    --mcu <mcu_name> \
    --database tools/codegen/database/families/<family>.json \
    --output src/generated/<vendor>/<mcu>/<mcu>
```

---

### %s Tokens in Generated Code

**Error**:
```
error: expected ';' before '%' token
```

**Status**: ✅ Fixed (generator strips %s placeholders)

**Cause**: Some SVD files have `%s` placeholders for array-style registers.

**Solution**: Use latest generator (fix applied in commit fixing SAMD21).

---

## CMake Issues

### "Variable ALLOY_BOARD not used"

**Warning**:
```
CMake Warning: Manually-specified variables were not used by the project: ALLOY_BOARD
```

**Cause**: Typo in variable name or CMake cache pollution.

**Solution**:
```bash
# Clean CMake cache
rm -rf build/*
cd build

# Use correct variable name (case-sensitive!)
cmake -DALLOY_BOARD=bluepill ..  # ✓ Correct
cmake -DBOARD=bluepill ..         # ✗ Wrong variable name
```

---

### Architecture Detection Failed

**Warning**:
```
Unknown architecture '' for blink_esp32
```

**Cause**: Board config didn't set `ALLOY_ARCH` with FORCE.

**Solution**:
```cmake
# In cmake/boards/board_name.cmake
set(ALLOY_ARCH "arm-cortex-m3" CACHE STRING "CPU architecture" FORCE)
#                                                              ^^^^^ Important!
```

---

## Getting More Help

### Collect Debug Information

When reporting issues, include:

1. **System info**:
   ```bash
   uname -a                    # OS
   cmake --version             # CMake
   arm-none-eabi-gcc --version # Toolchain
   ```

2. **Build commands** (exact commands used)

3. **Full error output** (not just the last line)

4. **CMake configuration**:
   ```bash
   cmake -DALLOY_BOARD=bluepill .. 2>&1 | tee cmake_log.txt
   ```

5. **Binary info**:
   ```bash
   arm-none-eabi-size blink.elf
   arm-none-eabi-objdump -h blink.elf  # Section headers
   ```

### Resources

- **GitHub Issues**: Report bugs and request features
- **Documentation**: Check other docs in `docs/`
- **OpenSpec Changes**: See `openspec/changes/` for known issues and fixes
- **Vendor Docs**: Reference manuals and datasheets

### Known Issues Tracker

See `openspec/changes/fix-esp32-samd21-build-issues/` for:
- ESP32 GPIO peripheral fix (in progress)
- SAMD21 toolchain compatibility fix (in progress)

---

## Quick Solutions Summary

| Problem | Quick Fix |
|---------|-----------|
| nosys.specs error (SAMD21) | Use distribution gcc-arm-none-eabi |
| ESP32 GPIO errors | Wait for fix or use ESP-IDF GPIO directly |
| Toolchain not found | Source ESP-IDF or install xPack |
| ST-Link not detected | Check USB cable, update firmware, add udev rules |
| Permission denied /dev/ttyUSB0 | `sudo usermod -a -G dialout $USER` |
| LED not blinking | Check polarity, pin number, clock init |
| Binary too large | Use `-DCMAKE_BUILD_TYPE=MinSizeRel` |
| RP2040 no USB drive | Hold BOOTSEL while connecting USB |
| ESP32 won't flash | Hold BOOT, press RESET, release BOOT, flash |

For issues not covered here, check the documentation or create a GitHub issue with full debug information.
