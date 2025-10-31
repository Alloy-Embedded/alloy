# Toolchain Setup Guide

This guide explains how to set up cross-compilation toolchains for building Alloy firmware for different target architectures.

## Overview

Alloy supports multiple embedded architectures, each requiring its own cross-compilation toolchain:

| Architecture | Toolchain | Boards |
|--------------|-----------|--------|
| ARM Cortex-M | arm-none-eabi-gcc | STM32F103, STM32F407, ATSAMD21, RP2040 |
| Xtensa LX6 | xtensa-esp32-elf-gcc | ESP32 |
| Host (native) | System compiler | Development/testing |

## ARM Cortex-M Toolchain

### Recommended: xPack ARM Toolchain (All Platforms)

**We recommend the xPack ARM toolchain** as it provides complete newlib support and works consistently across all platforms.

**Installation (all platforms):**

1. Install xpm (xPack Package Manager):
```bash
npm install -g xpm
```

2. Install the ARM toolchain:
```bash
xpm install --global @xpack-dev-tools/arm-none-eabi-gcc@latest
```

3. Add to PATH (the installer will show you the path):
```bash
# macOS/Linux - Add to ~/.bashrc or ~/.zshrc:
export PATH="$HOME/.local/xPacks/@xpack-dev-tools/arm-none-eabi-gcc/<version>/.content/bin:$PATH"

# Or use xpm to activate:
xpm run --global @xpack-dev-tools/arm-none-eabi-gcc prepare
```

**Download directly (without npm/xpm):**

Visit [xPack ARM Toolchain Releases](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/) and download for your platform:
- **macOS**: `xpack-arm-none-eabi-gcc-<version>-darwin-x64.tar.gz`
- **Linux**: `xpack-arm-none-eabi-gcc-<version>-linux-x64.tar.gz`
- **Windows**: `xpack-arm-none-eabi-gcc-<version>-win32-x64.zip`

Extract and add the `bin/` directory to your PATH.

### Alternative Installation Methods

**macOS (Homebrew) - NOT RECOMMENDED:**
```bash
brew install arm-none-eabi-gcc
```
⚠️ **Warning:** Homebrew version may lack newlib headers. Use xPack instead.

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```
✅ Includes newlib, but may be an older version.

**Arch Linux:**
```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib
```
✅ Includes newlib.

### Verification

```bash
arm-none-eabi-gcc --version
```

Expected output:
```
arm-none-eabi-gcc (GNU Arm Embedded Toolchain 10.3-2021.10) 10.3.1 20210824
```

### Supported Boards

- **STM32F103C8** (Blue Pill) - Cortex-M3
- **STM32F407VG** (Discovery) - Cortex-M4F with FPU
- **ATSAMD21G18** (Arduino Zero) - Cortex-M0+
- **RP2040** (Raspberry Pi Pico) - Dual Cortex-M0+

### Compiler Flags

The ARM toolchain uses the following flags (configured automatically):

**Common flags:**
- `-ffunction-sections -fdata-sections`: Enable per-function/data sections for better linking
- `-fno-exceptions -fno-rtti`: Disable C++ features to reduce code size
- `-specs=nano.specs`: Use newlib-nano (smaller C library)
- `-specs=nosys.specs`: Provide stub system calls

**CPU-specific flags (set per board):**
- Cortex-M0+: `-mcpu=cortex-m0plus -mthumb`
- Cortex-M3: `-mcpu=cortex-m3 -mthumb`
- Cortex-M4F: `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`

**Build types:**
- Debug: `-g -Og` (optimized for debugging)
- Release: `-O3 -DNDEBUG` (maximum optimization)
- MinSizeRel: `-Os -DNDEBUG` (optimize for size)

## Xtensa ESP32 Toolchain

### Installation

The Xtensa toolchain is typically installed as part of ESP-IDF.

**Option 1: ESP-IDF (Recommended)**
```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install prerequisites and toolchain
./install.sh esp32

# Set up environment
. ./export.sh
```

**Option 2: Standalone Toolchain**
Download prebuilt toolchain from [ESP32 Toolchain Releases](https://github.com/espressif/crosstool-NG/releases)

### Verification

```bash
xtensa-esp32-elf-gcc --version
```

Expected output:
```
xtensa-esp32-elf-gcc (crosstool-NG esp-2021r2) 8.4.0
```

### Supported Boards

- **ESP32 DevKit** - Dual-core Xtensa LX6

### Compiler Flags

The Xtensa toolchain uses the following flags (configured automatically):

**Common flags:**
- `-mlongcalls`: Required for ESP32 memory layout (allows calls across large address ranges)
- `-mtext-section-literals`: Place literal data in text sections
- `-ffunction-sections -fdata-sections`: Enable per-function/data sections
- `-fno-exceptions -fno-rtti`: Disable C++ features

**Build types:**
- Debug: `-g -Og`
- Release: `-O3 -DNDEBUG`
- MinSizeRel: `-Os -DNDEBUG`

## Host Toolchain (Native Development)

For testing and simulation on your development machine, Alloy can use the native compiler.

### Requirements

Any modern C++20 compiler:
- GCC 10+ or Clang 10+

### Installation

**macOS:**
```bash
xcode-select --install
```

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential
```

**Windows:**
Install [MSYS2](https://www.msys2.org/) or [Visual Studio](https://visualstudio.microsoft.com/)

## CMake Configuration

Alloy automatically selects the appropriate toolchain based on the selected board.

### Building for a Board

```bash
# Configure for STM32F407 (uses ARM toolchain)
cmake -B build -DALLOY_BOARD=stm32f407vg

# Configure for ESP32 (uses Xtensa toolchain)
cmake -B build -DALLOY_BOARD=esp32_devkit

# Configure for host (uses native compiler)
cmake -B build -DALLOY_BOARD=host
```

### Build Types

```bash
# Debug build (default)
cmake -B build -DALLOY_BOARD=stm32f407vg -DCMAKE_BUILD_TYPE=Debug

# Release build (maximum optimization)
cmake -B build -DALLOY_BOARD=stm32f407vg -DCMAKE_BUILD_TYPE=Release

# Minimum size build (optimize for flash size)
cmake -B build -DALLOY_BOARD=stm32f407vg -DCMAKE_BUILD_TYPE=MinSizeRel
```

### Toolchain Validation

Alloy automatically validates that the required toolchain is installed:

```bash
cmake -B build -DALLOY_BOARD=stm32f407vg
```

If the toolchain is missing, you'll see an error with installation instructions:
```
CMake Error: ARM toolchain not found!

  Required: arm-none-eabi-gcc

  Install instructions:
    macOS:   brew install arm-none-eabi-gcc
    Ubuntu:  sudo apt-get install gcc-arm-none-eabi
    ...
```

## Troubleshooting

### Command not found

**Symptom:** `arm-none-eabi-gcc: command not found`

**Solution:** The toolchain is not in your PATH. Add it:
```bash
# Find where the toolchain is installed
which arm-none-eabi-gcc

# Add to PATH in your shell profile (~/.bashrc, ~/.zshrc, etc)
export PATH="/path/to/toolchain/bin:$PATH"
```

### Wrong Architecture

**Symptom:** `arm-none-eabi-gcc: error: unrecognized option '-mcpu=cortex-m4'`

**Solution:** Your toolchain version is too old. Update to a recent version (10.3+).

### Linker Errors

**Symptom:** `undefined reference to '_sbrk'` or similar

**Solution:** Ensure `-specs=nosys.specs` is in your linker flags (handled automatically by Alloy).

### Flash/RAM Overflow

**Symptom:** `region 'FLASH' overflowed by X bytes`

**Solution:**
1. Use `-DCMAKE_BUILD_TYPE=MinSizeRel` for size optimization
2. Enable LTO (Link Time Optimization): `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`
3. Remove unused peripherals/features
4. Check the memory map in your board's `.ld` file

## Advanced Topics

### Link Time Optimization (LTO)

Enable LTO for smaller binaries:

```bash
cmake -B build \
    -DALLOY_BOARD=stm32f407vg \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

LTO can reduce code size by 10-30% but increases build time.

### Custom Toolchain Path

If you have multiple toolchain versions or non-standard installation:

```bash
cmake -B build \
    -DALLOY_BOARD=stm32f407vg \
    -DCMAKE_C_COMPILER=/opt/arm-gcc/bin/arm-none-eabi-gcc \
    -DCMAKE_CXX_COMPILER=/opt/arm-gcc/bin/arm-none-eabi-g++
```

### Debugging Symbols

Debug builds include full debugging symbols by default. To keep symbols in release builds:

```bash
cmake -B build \
    -DALLOY_BOARD=stm32f407vg \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

This uses `-O2 -g` for good optimization while keeping debug symbols.

## See Also

- [Building for Boards](building_for_boards.md) - How to build firmware
- [Flashing Guide](flashing.md) - How to flash firmware to hardware
- [Board Support](boards.md) - Detailed board specifications
