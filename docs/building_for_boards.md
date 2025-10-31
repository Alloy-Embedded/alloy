# Building for Boards

This guide explains how to build Alloy Framework applications for different development boards.

## Prerequisites

### 1. CMake

CMake 3.25 or higher is required.

```bash
cmake --version
```

### 2. Toolchains

Different boards require different toolchains:

#### ARM Boards (STM32F103, STM32F407, RP2040, SAMD21)

**xPack ARM Embedded GCC** (Recommended):
```bash
# Install script (provided in scripts/)
./scripts/install-xpack-toolchain.sh

# Or install manually from:
# https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases
```

**Alternative - ARM GCC from distribution**:
```bash
# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi

# macOS (Homebrew)
brew install --cask gcc-arm-embedded
```

Verify installation:
```bash
arm-none-eabi-gcc --version
```

#### ESP32 Board

**ESP-IDF** (Required):
```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf

# Install
cd ~/esp/esp-idf
./install.sh esp32

# Setup environment (run this in every new terminal)
. ~/esp/esp-idf/export.sh
```

Verify installation:
```bash
xtensa-esp32-elf-gcc --version
```

## Basic Build Process

### Step 1: Clone Repository

```bash
git clone https://github.com/your-org/alloy.git
cd alloy
```

### Step 2: Create Build Directory

```bash
mkdir build
cd build
```

### Step 3: Configure with CMake

Specify the target board using `-DALLOY_BOARD=<board_id>`:

```bash
cmake -DALLOY_BOARD=<board_id> ..
```

### Step 4: Build

```bash
make -j$(nproc)
```

Or build a specific target:
```bash
make blink_<board_id>
```

## Board-Specific Instructions

### Blue Pill (STM32F103C8)

```bash
cd build
cmake -DALLOY_BOARD=bluepill ..
make blink_stm32f103

# Output:
# blink_stm32f103         # ELF file
# blink_stm32f103.bin     # Binary for flashing
# blink_stm32f103.hex     # Intel HEX format
```

**Expected binary size**: ~600-700 bytes

---

### STM32F4 Discovery (STM32F407VG)

```bash
cd build
cmake -DALLOY_BOARD=stm32f407vg ..
make blink_stm32f407

# Output:
# blink_stm32f407         # ELF file
# blink_stm32f407.bin     # Binary for flashing
# blink_stm32f407.hex     # Intel HEX format
```

**Expected binary size**: ~1000-1200 bytes

---

### Raspberry Pi Pico (RP2040)

```bash
cd build
cmake -DALLOY_BOARD=rp_pico ..
make blink_rp_pico

# Output:
# blink_rp_pico           # ELF file
# blink_rp_pico.bin       # Binary for flashing
# blink_rp_pico.uf2       # UF2 format (for drag-and-drop)
```

**Expected binary size**: ~800-900 bytes

**Note**: RP2040 requires second-stage bootloader (included automatically)

---

### Arduino Zero (ATSAMD21G18)

```bash
cd build
cmake -DALLOY_BOARD=arduino_zero ..
make blink_arduino_zero

# Output:
# blink_arduino_zero      # ELF file
# blink_arduino_zero.bin  # Binary for flashing
# blink_arduino_zero.hex  # Intel HEX format
```

**Expected binary size**: <10 KB

**Known Issue**: If you encounter nosys.specs error with xPack toolchain v14.2.1+:
```bash
# Workaround is being implemented
# See troubleshooting.md for current status
```

---

### ESP32 DevKit

```bash
# Source ESP-IDF first!
. ~/esp/esp-idf/export.sh

cd build
rm -rf *  # Clean build recommended for ESP32
cmake -DALLOY_BOARD=esp32_devkit \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/xtensa-esp32-elf.cmake \
      ..
make blink_esp32

# Output:
# blink_esp32             # ELF file
# blink_esp32.bin         # Binary for flashing
```

**Expected binary size**: <100 KB

**Known Issue**: GPIO peripheral structure incomplete (fix in progress)
```bash
# See troubleshooting.md for current status
```

## Advanced Build Options

### Build Type

```bash
# Debug build (default)
cmake -DALLOY_BOARD=bluepill -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized)
cmake -DALLOY_BOARD=bluepill -DCMAKE_BUILD_TYPE=Release ..

# Minimum size build
cmake -DALLOY_BOARD=bluepill -DCMAKE_BUILD_TYPE=MinSizeRel ..
```

### Parallel Builds

```bash
# Use all available cores
make -j$(nproc)

# Use specific number of cores
make -j4
```

### Verbose Output

```bash
make VERBOSE=1
```

### Clean Build

```bash
make clean

# Or remove entire build directory
cd ..
rm -rf build
mkdir build
cd build
```

## Testing All Boards

An automated test script is provided to build all boards:

```bash
# Source ESP-IDF first for ESP32 support
. ~/esp/esp-idf/export.sh

# Run test script
./scripts/test-all-boards.sh
```

This will:
1. Configure each board
2. Build the blink example
3. Report success/failure
4. Show binary sizes

## Troubleshooting

### "arm-none-eabi-gcc not found"

**Solution**: Install ARM toolchain (see Prerequisites above)

### "xtensa-esp32-elf-gcc not found"

**Solution**: Source ESP-IDF environment:
```bash
. ~/esp/esp-idf/export.sh
```

### "Board 'xxx' not found"

**Solution**: Check valid board IDs:
```bash
ls cmake/boards/
```

Valid IDs: `bluepill`, `stm32f407vg`, `rp_pico`, `arduino_zero`, `esp32_devkit`

### "nosys.specs: attempt to rename spec"

**Solution**: This is a known issue with xPack ARM toolchain v14.2.1+ on SAMD21.
See [troubleshooting guide](troubleshooting.md) for workarounds.

### Build warnings

If you see warnings during build, they can usually be ignored for examples. However:
- Check [known issues](troubleshooting.md)
- Report new warnings as GitHub issues

### Binary size larger than expected

**Causes**:
- Debug build type (use `-DCMAKE_BUILD_TYPE=MinSizeRel`)
- Logging/debugging code included
- Not using link-time optimization

**Solution**:
```bash
cmake -DALLOY_BOARD=bluepill -DCMAKE_BUILD_TYPE=MinSizeRel ..
make
```

## Next Steps

After successfully building:
1. **Flash to hardware**: See [Flashing Guide](flashing.md)
2. **Debug**: See [Debugging Guide](debugging.md) (coming soon)
3. **Create your own application**: See [Getting Started](getting_started.md) (coming soon)

## See Also

- [Supported Boards](boards.md) - Complete board list and specifications
- [Flashing Guide](flashing.md) - How to upload firmware to hardware
- [Troubleshooting Guide](troubleshooting.md) - Common issues and solutions
- [Toolchain Setup](toolchains.md) - Detailed toolchain installation
