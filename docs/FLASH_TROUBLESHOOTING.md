# Flash Troubleshooting Guide

Common issues and solutions when flashing MicroCore examples to boards.

## Common Error: "Cannot connect to target"

### Error Message
```
st-flash 1.8.0
2025-11-23T17:58:18 ERROR common.c: Can not connect to target. Please use 'connect under reset' and try again
Failed to connect to target
Failed to parse flash type or unrecognized flash type
✗ Flash failed
```

### Why This Happens

This error occurs when:
1. **Board is in a low-power state** - MCU is sleeping or in stop mode
2. **Previous code disabled debug pins** - SWD pins were reconfigured
3. **Previous code crashed** - MCU is in a fault state
4. **Flash is corrupted** - Previous flash operation was incomplete

### Solution 1: Automatic Retry (Built-in)

The `ucore` CLI now **automatically tries multiple methods**:

```bash
./ucore flash nucleo_f401re blink
```

It will:
1. ✅ Try standard flash first
2. ⚠️ If that fails → Try with `--connect-under-reset`
3. ⚠️ If that fails → Try OpenOCD as fallback

### Solution 2: Manual Reset Flash

If the automatic retry doesn't work, you can manually use reset mode:

```bash
# Step 1: Start the flash command
./ucore flash nucleo_f401re blink

# Step 2: When it fails, it will ask you to hold RESET button
# Step 3: Hold the RESET button on your board
# Step 4: Press Enter
# Step 5: Release RESET button when prompted
```

### Solution 3: Use OpenOCD Directly

OpenOCD is more robust for difficult connections:

```bash
# Install OpenOCD if not already installed
brew install openocd  # macOS
sudo apt-get install openocd  # Linux

# Flash with OpenOCD
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "init" \
        -c "reset init" \
        -c "program build-nucleo_f401re/examples/blink/blink.elf verify reset exit"
```

### Solution 4: Hardware Reset While Connecting

**Physical steps:**

1. Disconnect USB cable
2. Hold the RESET button on your board
3. Plug in USB cable (while still holding RESET)
4. Run flash command:
   ```bash
   ./ucore flash nucleo_f401re blink
   ```
5. Release RESET button when you see "Connecting to target"

### Solution 5: Check USB Connection

```bash
# Check if ST-Link is detected
st-info --probe

# Should show something like:
# Found 1 stlink programmers
# version:    V2J42M27
# serial:     066DFF525150896687134146
```

If no device is detected:
- Try a different USB cable (some are power-only)
- Try a different USB port
- Check if ST-Link drivers are installed
- On Linux, check udev rules

## Board-Specific Issues

### STM32 Nucleo Boards

**Issue:** Board not detected after flashing custom code

**Solution:** The STM32 has a bootloader that can always be accessed:
1. Disconnect board
2. Hold BOOT0 button (if available)
3. Connect board while holding BOOT0
4. Flash using OpenOCD or DFU mode

### SAME70 Xplained

**Issue:** Different programmer (EDBG, not ST-Link)

**Solution:** Use OpenOCD with CMSIS-DAP interface:
```bash
openocd -f interface/cmsis-dap.cfg \
        -f target/atsamv.cfg \
        -c "program build-same70_xplained/examples/blink/blink.elf verify reset exit"
```

## Debugging Connection Issues

### 1. Check Device Detection

**For ST-Link:**
```bash
# List USB devices
lsusb | grep -i stlink  # Linux
system_profiler SPUSBDataType | grep -i stlink  # macOS

# Probe ST-Link
st-info --probe

# Get detailed info
st-info --descr
```

**For OpenOCD:**
```bash
# Test OpenOCD connection
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init; reset; exit"
```

### 2. Verify Wiring (Custom Boards)

For custom boards with separate ST-Link:

| ST-Link Pin | Target Pin | Purpose |
|-------------|------------|---------|
| SWDIO | SWDIO | Data |
| SWCLK | SWCLK | Clock |
| GND | GND | Ground |
| 3.3V | 3.3V | Power (optional) |
| NRST | RESET | Reset (optional but helpful) |

### 3. Check for Hardware Issues

**Symptoms of hardware problems:**
- Board doesn't power on (no LED)
- Gets very hot
- Inconsistent detection (works sometimes)

**Checks:**
1. Measure voltage on 3.3V pin (should be ~3.3V)
2. Check for shorts with multimeter
3. Try a known-good board to rule out ST-Link issues

## Advanced Recovery

### Full Chip Erase

If all else fails, erase the entire chip:

**Using st-flash:**
```bash
st-flash erase
```

**Using OpenOCD:**
```bash
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "init" \
        -c "reset init" \
        -c "stm32f4x mass_erase 0" \
        -c "exit"
```

Then try flashing again.

### Unbrick Using System Bootloader (STM32)

If the chip is completely unresponsive:

1. Enter STM32 bootloader:
   - Short BOOT0 pin to 3.3V
   - Reset or power cycle
   - BOOT0 can now be released

2. Flash using DFU or UART:
   ```bash
   # Using dfu-util (if board has USB DFU)
   dfu-util -a 0 -s 0x08000000:leave -D build-nucleo_f401re/examples/blink/blink.bin

   # Or using stm32flash (via UART)
   stm32flash -w build-nucleo_f401re/examples/blink/blink.bin /dev/ttyUSB0
   ```

3. Return to normal mode:
   - Disconnect BOOT0 from 3.3V
   - Reset

## Preventing Future Issues

### 1. Don't Disable Debug Pins

**Avoid this in your code:**
```cpp
// BAD - Disables SWD debugging
GPIO_Init(SWDIO_Pin, GPIO_MODE_ANALOG);
GPIO_Init(SWCLK_Pin, GPIO_MODE_ANALOG);
```

If you MUST use these pins, add a startup delay:
```cpp
int main() {
    // Wait 3 seconds before doing anything
    // Allows debugger to connect
    for (volatile int i = 0; i < 3000000; i++);

    // Now safe to reconfigure pins
}
```

### 2. Use Watchdog Carefully

If using watchdog, make sure it's configured correctly or the board will constantly reset.

### 3. Test Flash Success

After flashing, verify the board runs:
```bash
# Check for serial output (if using UART)
screen /dev/tty.usbmodem* 115200  # macOS
screen /dev/ttyACM0 115200        # Linux

# Or watch for LED blink
```

## Quick Reference

| Problem | Solution |
|---------|----------|
| "Cannot connect to target" | Let `ucore` retry automatically, or use `--reset` |
| Board not detected | Check USB cable, try different port, run `st-info --probe` |
| Works sometimes | Hardware issue, check connections/power |
| Worked once, now fails | Previous code may have disabled debug, use reset mode |
| Completely dead | Try full chip erase, then STM32 bootloader |

## Getting Help

If you're still stuck:

1. **Check the error message carefully** - Often tells you exactly what's wrong
2. **Try the automatic retry** - `./ucore flash <board> <example>`
3. **Use verbose output** - Add `-v` flag if available
4. **Check hardware** - LED should light when connected
5. **Ask for help** - Include full error output and board model

## See Also

- [QUICKSTART.md](../QUICKSTART.md) - Basic flashing guide
- [TESTING_BOARDS.md](../TESTING_BOARDS.md) - Board-specific testing
- [OpenOCD Manual](http://openocd.org/doc/html/index.html) - OpenOCD documentation
- [ST-Link Utilities](https://www.st.com/en/development-tools/stsw-link004.html) - Official ST tools
