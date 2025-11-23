# Testing Blink LED on MicroCore Boards

Quick guide to test the blink example on all supported hardware.

## Supported Boards

| Board | MCU | LED | Flash/RAM |
|-------|-----|-----|-----------|
| **nucleo_f401re** | STM32F401RE (Cortex-M4, 84MHz) | Green LD2 | ~3KB / ~20B |
| **nucleo_f722ze** | STM32F722ZE (Cortex-M7, 216MHz) | Green LD1 | ~3KB / ~20B |
| **nucleo_g071rb** | STM32G071RB (Cortex-M0+, 64MHz) | Green LD4 | ~2.6KB / ~20B |
| **nucleo_g0b1re** | STM32G0B1RE (Cortex-M0+, 64MHz) | Green LD4 | ~2.6KB / ~20B |
| **same70_xplained** | ATSAME70Q21 (Cortex-M7, 300MHz) | LED0 (green) | TBD |

## Quick Start

### 1. Prerequisites

Install ARM toolchain:

```bash
# Using xPack (recommended)
./scripts/install-xpack-toolchain.sh

# Or using Homebrew (macOS)
brew install --cask gcc-arm-embedded

# Or download from: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
```

### 2. Test All Boards (Build Only)

Build all boards to verify cross-platform compatibility:

```bash
./scripts/test-all-boards.sh
```

This will:
- Clean build each board
- Show binary sizes
- Report success/failure for each

### 3. Flash to Hardware (Interactive)

Flash and verify on physical boards:

```bash
./scripts/test-all-boards.sh flash
```

This will:
- Build each board
- Ask you to connect the board
- Flash via ST-Link
- Tell you what LED should be blinking

## Manual Build & Flash

### Single Board Build

```bash
# Configure for specific board
cmake -B build-nucleo_f401re \
      -DMICROCORE_BOARD=nucleo_f401re \
      -DMICROCORE_BUILD_TESTS=OFF \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-nucleo_f401re --target blink

# Flash (requires OpenOCD)
cmake --build build-nucleo_f401re --target flash
```

### OpenOCD Flash Commands

#### STM32 Nucleo Boards (F401, F722, G071, G0B1)

```bash
# Using OpenOCD directly
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \  # or stm32f7x.cfg, stm32g0x.cfg
        -c "program build-nucleo_f401re/examples/blink/blink.elf verify reset exit"
```

#### SAME70 Xplained

```bash
# Using JLink
JLinkExe -device ATSAME70Q21 -if SWD -speed 4000 -autoconnect 1
# In JLink console:
# loadfile build-same70_xplained/examples/blink/blink.elf
# r
# go
# exit
```

## Expected Behavior

All boards should show:
- **LED blinking at 1 Hz** (500ms ON, 500ms OFF)
- **Continuous operation** (no stops/crashes)
- **Consistent timing** (not too fast/slow)

### LED Locations

**Nucleo F401RE / F722ZE:**
- Green LED (LD2/LD1) - next to USB connector

**Nucleo G071RB / G0B1RE:**
- Green LED (LD4) - board top-right

**SAME70 Xplained:**
- LED0 (green) - board edge

## Troubleshooting

### Build Fails

**Error: arm-none-eabi-gcc not found**
```bash
# Check toolchain installation
which arm-none-eabi-gcc

# Add to PATH
export PATH="$HOME/.local/xpack-arm-toolchain/bin:$PATH"
```

**Error: CMake configuration failed**
```bash
# Check logs
cat build-<board>_config.log

# Common issues:
# - Wrong board name (check boards/ directory)
# - Missing toolchain file
# - Invalid MICROCORE_BOARD value
```

### Flash Fails

**Error: No ST-Link detected**
```bash
# Check USB connection
lsusb | grep -i "stm\|stlink"

# Permissions (Linux)
sudo usermod -a -G plugdev $USER
# Then logout/login
```

**Error: Target not found**
```bash
# Check OpenOCD config
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Update OpenOCD
brew upgrade openocd  # macOS
sudo apt install openocd  # Linux
```

### LED Not Blinking

**LED stays OFF:**
- Check if firmware flashed successfully
- Verify correct board selected (`-DMICROCORE_BOARD=...`)
- Try pressing RESET button
- Check board power (USB connected)

**LED always ON:**
- Code may have crashed in startup
- Check serial output (if UART configured)
- Reflash with debug build

**LED blinking too fast/slow:**
- Check system clock configuration in `board.cpp`
- Verify SysTick setup (should be 1000 Hz)

## Verification Checklist

For each board:

- [ ] **Build succeeds** (no errors)
- [ ] **Binary size reasonable** (<5KB flash, <100B RAM for blink)
- [ ] **Flash succeeds** (OpenOCD reports success)
- [ ] **LED blinks** (visible 1 Hz rate)
- [ ] **Timing accurate** (measure with timer/scope)
- [ ] **RESET works** (button restarts blink)

## Board-Specific Notes

### nucleo_f401re
- Uses HSE 8 MHz crystal → PLL → 84 MHz
- SysTick at 1000 Hz (1ms tick)
- LED on PA5 (Arduino D13)

### nucleo_f722ze
- Uses HSE 8 MHz crystal → PLL → 216 MHz
- SysTick at 1000 Hz (1ms tick)
- LED on PB7

### nucleo_g071rb / nucleo_g0b1re
- Uses HSI 16 MHz → PLL → 64 MHz
- SysTick at 1000 Hz (1ms tick)
- LED on PA5 (G071RB) or PB7 (G0B1RE)

### same70_xplained
- Uses external 12 MHz crystal → PLL → 300 MHz
- SysTick at 1000 Hz (1ms tick)
- LED0 on PC8

## Advanced Testing

### Measure Timing Accuracy

```bash
# Use logic analyzer or oscilloscope on LED pin
# Expected: 1 Hz square wave (500ms high, 500ms low)
# Tolerance: ±5%
```

### Serial Output (if enabled)

```bash
# Connect USB-UART adapter to UART pins
screen /dev/ttyUSB0 115200

# Expected output (if console enabled):
# "Blink example starting..."
# "LED: ON"
# "LED: OFF"
# ...
```

### Power Consumption Test

```bash
# Measure current on VDD
# Expected (approximate):
# - STM32F4: 15-25 mA
# - STM32F7: 25-35 mA
# - STM32G0: 5-10 mA (low power)
# - SAME70: 30-40 mA
```

## CI/CD Integration

The test script is designed for CI/CD:

```yaml
# .github/workflows/test.yml
- name: Build all boards
  run: ./scripts/test-all-boards.sh

- name: Upload binaries
  uses: actions/upload-artifact@v3
  with:
    name: firmware-binaries
    path: build-*/examples/blink/blink.*
```

## See Also

- [Board Porting Guide](docs/PORTING_NEW_BOARD.md)
- [Architecture Documentation](docs/ARCHITECTURE.md)
- [API Reference](docs/API_REFERENCE.md)
- [Examples README](examples/blink/README.md)
