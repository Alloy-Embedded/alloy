# MicroCore Dependencies

This document lists all dependencies required to build and develop with MicroCore.

## Quick Check

Run the dependency check tool to see what's installed on your system:

```bash
./ucore doctor
```

This will scan your system and report the status of all required and optional dependencies.

## Required Dependencies

### 1. CMake (3.25+)

**Purpose**: Build system generator
**Minimum Version**: 3.25
**Recommended**: Latest stable release

CMake 3.25+ is required for modern C++20/23 support and improved cross-compilation.

#### Installation

**macOS**:
```bash
brew install cmake
```

**Linux (Ubuntu/Debian)**:
```bash
# Option 1: pip (recommended for latest version)
pip3 install --upgrade cmake

# Option 2: apt (may be older)
sudo apt-get update
sudo apt-get install cmake
```

**Windows**:
```bash
choco install cmake
```

**Verify**:
```bash
cmake --version
# Should show 3.25 or higher
```

### 2. Python 3 (3.10+)

**Purpose**: Code generation, board configuration, CLI tools
**Minimum Version**: 3.10
**Recommended**: 3.11 or 3.12

Python 3.10+ is required for modern type hints and pattern matching used in code generation.

#### Installation

**macOS**:
```bash
brew install python3
```

**Linux**:
```bash
sudo apt-get install python3 python3-pip
```

**Windows**:
```bash
choco install python3
```

**Verify**:
```bash
python3 --version
# Should show 3.10 or higher
```

#### Python Packages

**PyYAML** (Required for board configuration):
```bash
pip3 install pyyaml
```

### 3. Build System (Make or Ninja)

**Purpose**: Execute build commands
**Recommended**: Ninja (faster parallel builds)

#### Installation

**macOS**:
```bash
brew install ninja
```

**Linux**:
```bash
sudo apt-get install ninja-build
```

**Windows**:
```bash
choco install ninja
```

## Embedded Development Dependencies

These dependencies are required only for building firmware for embedded targets (STM32, SAME70, etc.). They are **not needed** for host platform development.

### 1. ARM GCC Toolchain (10.0+)

**Purpose**: Cross-compiler for ARM Cortex-M microcontrollers
**Minimum Version**: 10.0.0
**Recommended**: 12.0 or newer

Older versions have limited C++20/23 support.

#### Installation

**macOS**:
```bash
brew install --cask gcc-arm-embedded
```

**Linux**:
```bash
# Option 1: Package manager (may be old)
sudo apt-get install gcc-arm-none-eabi

# Option 2: Download from ARM directly (recommended)
# https://developer.arm.com/downloads/-/gnu-rm
```

**Windows**:
```bash
choco install gcc-arm-embedded
```

**Verify**:
```bash
arm-none-eabi-gcc --version
# Should show 10.0 or higher
```

#### Alternative: xPack ARM GCC

For more control over versions:

```bash
npm install --global xpm@latest
xpm install --global @xpack-dev-tools/arm-none-eabi-gcc@latest
```

## Flash and Debug Tools

These tools are optional but recommended for flashing firmware and debugging.

### 1. st-flash (for STM32 boards)

**Purpose**: Flash STM32 microcontrollers via ST-Link
**Required for**: All STM32 boards (Nucleo, Discovery, etc.)

#### Installation

**macOS**:
```bash
brew install stlink
```

**Linux**:
```bash
sudo apt-get install stlink-tools
```

**Windows**:
```bash
choco install stlink
```

**Verify**:
```bash
st-flash --version
```

### 2. OpenOCD (universal debugger)

**Purpose**: On-chip debugging for multiple platforms
**Supports**: STM32, SAME70, RP2040, and many others

#### Installation

**macOS**:
```bash
brew install openocd
```

**Linux**:
```bash
sudo apt-get install openocd
```

**Windows**:
```bash
choco install openocd
```

**Verify**:
```bash
openocd --version
```

### 3. JLink (commercial, optional)

**Purpose**: High-performance debugging and flashing
**Cost**: Free for evaluation, paid for commercial use

Download from: [SEGGER JLink](https://www.segger.com/downloads/jlink/)

## Code Quality Tools (Optional)

These tools improve code quality but are not required for building.

### 1. clang-format

**Purpose**: Automatic code formatting
**Config**: `.clang-format` in repository root

#### Installation

**macOS**:
```bash
brew install llvm
# clang-format will be in /opt/homebrew/opt/llvm/bin/
```

**Linux**:
```bash
sudo apt-get install clang-format
```

**Usage**:
```bash
# Format a file
clang-format -i src/hal/gpio.hpp

# Format all C++ files
find src -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i
```

### 2. clang-tidy

**Purpose**: Static analysis and lint checks
**Config**: `.clang-tidy` in repository root

#### Installation

**macOS**:
```bash
brew install llvm
# clang-tidy will be in /opt/homebrew/opt/llvm/bin/
```

**Linux**:
```bash
sudo apt-get install clang-tidy
```

**Usage**:
```bash
# Check a file
clang-tidy src/hal/gpio.hpp -- -std=c++23

# Check all files
clang-tidy src/**/*.{hpp,cpp} -- -std=c++23
```

### 3. Doxygen (for documentation)

**Purpose**: Generate API documentation from source code
**Required for**: Building documentation (`make docs`)

#### Installation

**macOS**:
```bash
brew install doxygen graphviz
```

**Linux**:
```bash
sudo apt-get install doxygen graphviz
```

**Windows**:
```bash
choco install doxygen.install graphviz
```

**Usage**:
```bash
# Generate documentation
doxygen Doxyfile

# Open documentation
open docs/api/html/index.html
```

## Platform-Specific Tools

### For ESP32 Development

**ESP-IDF** (Espressif IoT Development Framework):

```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh

# Activate environment
. ./export.sh
```

### For RP2040 Development

**picotool** (Raspberry Pi Pico SDK):

```bash
# macOS
brew install picotool

# Linux
sudo apt-get install picotool
```

## Dependency Summary Table

| Tool | Version | Required | Purpose |
|------|---------|----------|---------|
| CMake | 3.25+ | ✅ Yes | Build system |
| Python | 3.10+ | ✅ Yes | Code generation, CLI |
| PyYAML | Latest | ✅ Yes | Board configuration |
| Make/Ninja | Any | ✅ Yes | Build executor |
| ARM GCC | 10.0+ | Embedded only | ARM cross-compiler |
| st-flash | Latest | STM32 only | STM32 flashing |
| OpenOCD | Latest | ❌ Optional | Debugging |
| JLink | Latest | ❌ Optional | Debugging (commercial) |
| clang-format | Latest | ❌ Optional | Code formatting |
| clang-tidy | Latest | ❌ Optional | Static analysis |
| Doxygen | Latest | ❌ Optional | Documentation |

## Troubleshooting

### CMake version too old

**Problem**: CMake 3.24 or older
**Solution**:
```bash
# Uninstall old version
sudo apt-get remove cmake  # or brew uninstall cmake

# Install latest via pip
pip3 install --upgrade cmake
```

### ARM GCC not found

**Problem**: `arm-none-eabi-gcc: command not found`
**Solution**:
```bash
# Verify installation
which arm-none-eabi-gcc

# If not found, add to PATH
export PATH="/path/to/arm-gcc/bin:$PATH"

# Or use xPack version
npm install --global xpm
xpm install --global @xpack-dev-tools/arm-none-eabi-gcc@latest
```

### Python package import errors

**Problem**: `ModuleNotFoundError: No module named 'yaml'`
**Solution**:
```bash
# Install missing package
pip3 install pyyaml

# Or install all requirements at once
pip3 install -r requirements.txt  # if available
```

### st-flash permission denied (Linux)

**Problem**: `Error: Permission denied` when flashing
**Solution**:
```bash
# Add udev rules for ST-Link
sudo cp /usr/share/stlink/stlink-blacklist.conf /etc/modprobe.d/
sudo udevadm control --reload-rules
sudo udevadm trigger

# Add user to dialout group
sudo usermod -a -G dialout $USER

# Logout and login again
```

## Automated Validation

MicroCore includes automated dependency checking in two places:

### 1. CMake Configuration Time

When you run `cmake`, it automatically validates:
- CMake version
- Python version and packages
- ARM toolchain (for embedded targets)
- Flash tools

Example output:
```
-- Python 3.12.0 found: /usr/bin/python3
-- ARM GCC 12.3.1 found: /usr/bin/arm-none-eabi-gcc
-- st-flash found: /usr/bin/st-flash
```

### 2. CLI Doctor Command

The `ucore doctor` command performs comprehensive system checks:

```bash
./ucore doctor
```

Output includes:
- ✓ Installed and validated
- ✗ Missing (required)
- ⚠ Found but outdated
- ○ Not found (optional)

Run this command before starting development to ensure your environment is ready.

## Continuous Integration (CI)

GitHub Actions automatically validates dependencies in CI:

- `.github/workflows/build.yml` - Build validation
- `.github/workflows/documentation.yml` - Documentation build

These workflows install all dependencies from scratch, ensuring reproducible builds.

## Related Documents

- [BUILD_GUIDE.md](BUILD_GUIDE.md) - How to build MicroCore projects
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Development workflow
- [DOCUMENTATION_GUIDE.md](DOCUMENTATION_GUIDE.md) - Building documentation

## Support

If you encounter dependency issues:

1. Run `./ucore doctor` to diagnose
2. Check this document for installation instructions
3. Open an issue with `ucore doctor` output
