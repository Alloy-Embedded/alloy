# 🚀 Alloy Framework - Build Guide

**Professional build system with CMake Presets, Python CLI, and Makefile shortcuts**

This guide shows you how to build, test, and flash Alloy Framework projects using our modern, flexible build system.

---

## 🎯 Quick Start

### 0. **Discovery CLI** (Find MCUs and boards)

```bash
# List all available MCUs
./alloy-cli list mcus

# List all development boards
./alloy-cli list boards

# Show detailed board information
./alloy-cli show board nucleo-f401re

# Show MCU details
./alloy-cli show mcu stm32f401re

# Initialize new project
./alloy-cli init my-project --board nucleo-f401re
```

### 1. **Simplest Way: Makefile** (Recommended for most users)

```bash
# Show all available commands
make help

# Build for host (native development)
make build

# Run tests
make test

# Build for Nucleo F401RE
make nucleo-f401

# Flash to board
make flash PRESET=nucleo-f401re-debug
```

### 2. **Python CLI** (Most flexible)

```bash
# List all available presets
./alloy.py list

# Build specific preset
./alloy.py build nucleo-f401re-debug

# Run tests
./alloy.py test host-debug

# Flash to board
./alloy.py flash nucleo-f401re-release

# Run complete workflow (configure + build + test)
./alloy.py workflow dev
```

### 3. **Direct CMake** (Advanced users)

```bash
# Configure
cmake --preset nucleo-f401re-debug

# Build
cmake --build --preset nucleo-f401re-debug

# Test
ctest --preset host-debug

# Workflow
cmake --workflow --preset dev
```

---

## 🔍 Alloy CLI - Discovery and Project Management

The Alloy CLI (`./alloy-cli`) provides powerful tools for discovering MCUs, boards, and managing projects.

### Available Commands

```bash
# Discovery commands
./alloy-cli list mcus              # List all MCUs
./alloy-cli list boards            # List all boards
./alloy-cli show mcu <name>        # Show MCU details
./alloy-cli show board <name>      # Show board details
./alloy-cli show pinout <board>    # Show board pinout diagram
./alloy-cli search <query>         # Search MCUs and boards

# Project management
./alloy-cli init <name>            # Initialize new project
./alloy-cli build                  # Build current project
./alloy-cli validate               # Validate generated code

# Configuration
./alloy-cli config show            # Show current configuration
./alloy-cli config set <key> <val> # Set configuration value

# Documentation
./alloy-cli docs                   # Open documentation
./alloy-cli version                # Show CLI version
```

### Examples

```bash
# Find STM32F4 MCUs
./alloy-cli search stm32f4

# Create new project for Nucleo F401RE
./alloy-cli init my-project --board nucleo-f401re

# Show detailed pinout with color highlighting
./alloy-cli show pinout nucleo-f401re
```

---

## 📋 Available Presets

Run `./alloy.py list` or `make list` to see all presets. Here are the most common ones:

### Host Development
- **host-debug** - Native development with full debugging (default)
- **host-release** - Optimized native build
- **tests-all** - Comprehensive test suite for CI/CD

### STM32 Boards
- **nucleo-f401re-debug** - STM32F401RE Cortex-M4 (84MHz, 512KB Flash)
- **nucleo-f401re-release** - Optimized production build
- **nucleo-g071rb-debug** - STM32G071RB Cortex-M0+ (64MHz, 128KB Flash)
- **bluepill-debug** - STM32F103C8 Blue Pill (72MHz, 64KB Flash)

### Other Boards
- **same70-xpld-debug** - ATSAME70Q21B Cortex-M7 (300MHz, 2MB Flash)
- **rp-pico-debug** - Raspberry Pi Pico RP2040 (133MHz, 2MB Flash)

---

## 🔧 Common Workflows

### Development Workflow (Recommended)

Use the **dev** workflow for fast iteration:

```bash
# Makefile (easiest)
make dev

# Python CLI
./alloy.py workflow dev

# Direct CMake
cmake --workflow --preset dev
```

This runs: **Configure → Build → Test** in one command.

---

### Building for Embedded Board

```bash
# Method 1: Makefile shortcut
make nucleo-f401          # Debug build
make nucleo-f401-release  # Release build

# Method 2: Python CLI
./alloy.py build nucleo-f401re-debug

# Method 3: Direct CMake
cmake --preset nucleo-f401re-debug
cmake --build --preset nucleo-f401re-debug
```

---

### Running Tests

```bash
# Quick test (Makefile)
make test

# Verbose test output
make test-verbose

# Python CLI
./alloy.py test host-debug --verbose

# Run comprehensive test suite
make test-all
```

---

### Flashing Firmware

```bash
# Makefile
make flash PRESET=nucleo-f401re-debug

# Python CLI
./alloy.py flash nucleo-f401re-release

# With specific port (if needed)
./alloy.py flash nucleo-f401re-debug --port /dev/ttyACM0
```

---

### Code Generation

```bash
# Generate code for all platforms
make codegen

# Quick generation (no validation)
make codegen-quick

# Check generation status
make codegen-status

# Clean generated code
make codegen-clean
```

---

## 🎨 Code Quality

### Format Code

```bash
# Format all C++ code with clang-format
make format
```

### Lint Code

```bash
# Run clang-tidy
make lint
```

### Full Quality Check

```bash
# Format + Lint + Test
make check
```

---

## 📊 Analysis & Information

### Show Build Info

```bash
make info
```

Output:
```
═══ Build Configuration ═══
  Preset:     nucleo-f401re-debug
  Root:       /path/to/alloy
  Build Dir:  build/nucleo-f401re-debug
```

### Binary Sizes

```bash
# Show sizes for all builds
make sizes

# Memory analysis for specific preset
make memory PRESET=nucleo-f401re-debug
```

### Generate compile_commands.json

```bash
# For IDE/LSP support (clangd, VSCode, etc.)
make compile-commands PRESET=host-debug
```

This creates a symlink to `build/host-debug/compile_commands.json`.

---

## 🏗️ Build System Architecture

```
alloy/
├── CMakePresets.json        # Modern CMake preset definitions
├── alloy.py                 # Python CLI tool
├── Makefile                 # Convenient shortcuts
├── cmake/                   # CMake modules
│   ├── toolchains/          # Cross-compilation toolchains
│   ├── compiler_options.cmake
│   ├── board_flags.cmake
│   └── flash_targets.cmake
└── build/                   # Build outputs (per preset)
    ├── host-debug/
    ├── nucleo-f401re-debug/
    └── ...
```

---

## ⚙️ Customizing Presets

### Using Existing Presets

Set the `PRESET` variable:

```bash
make build PRESET=nucleo-g071rb-debug
make test PRESET=host-release
```

### Creating Custom Presets

Edit `CMakePresets.json`:

```json
{
  "name": "my-custom-board",
  "displayName": "My Custom Board (Debug)",
  "inherits": "debug-base",
  "cacheVariables": {
    "ALLOY_BOARD": "my_board",
    "ALLOY_PLATFORM": "stm32f4",
    "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/cmake/toolchains/arm-none-eabi-gcc.cmake"
  }
}
```

Then use it:

```bash
./alloy.py build my-custom-board
```

---

## 🔍 Troubleshooting

### "Preset not found"

```bash
# List available presets
./alloy.py list

# Or
make list
```

### Build fails with toolchain error

Ensure ARM GCC toolchain is installed:

```bash
# Check ARM GCC
arm-none-eabi-gcc --version

# Install (Ubuntu/Debian)
sudo apt-get install gcc-arm-none-eabi

# Install (macOS)
brew install --cask gcc-arm-embedded
```

### CMake version too old

Alloy requires CMake 3.25+:

```bash
cmake --version

# Install latest (macOS)
brew install cmake

# Install latest (Ubuntu - via Kitware APT)
sudo apt-get install cmake
```

### Tests fail to run

```bash
# Ensure tests are built
make build PRESET=host-debug

# Run with verbose output
make test-verbose
```

---

## 📚 Examples

### Example 1: Build Blink for Nucleo

```bash
# Build debug version
make nucleo-f401

# Flash to board
make flash PRESET=nucleo-f401re-debug

# Monitor (if applicable)
minicom -D /dev/ttyACM0
```

### Example 2: Full Development Cycle

```bash
# 1. Make code changes
vim src/hal/gpio.cpp

# 2. Format code
make format

# 3. Build and test
make dev

# 4. Build for target board
make nucleo-f401-release

# 5. Flash
make flash PRESET=nucleo-f401re-release
```

### Example 3: CI/CD Pipeline

```bash
# Run full CI workflow
make ci

# Or with Python CLI
./alloy.py workflow ci
```

This runs:
1. Configure with tests
2. Build all targets
3. Run comprehensive test suite

---

## 🎯 Best Practices

### Development

1. **Use `make dev` for quick iteration** - configures, builds, and tests in one command
2. **Run `make format` before committing** - keeps code style consistent
3. **Use `make compile-commands`** - enables IDE features (autocomplete, navigation)

### Embedded

1. **Start with debug preset** - easier to debug issues
2. **Switch to release for production** - smaller, optimized binaries
3. **Always test on hardware** - simulation doesn't catch everything

### CI/CD

1. **Use `make ci` or `./alloy.py workflow ci`** - runs comprehensive test suite
2. **Check binary sizes** - use `make sizes` to track firmware growth
3. **Run `make check`** - format + lint + test before merging

---

## 📖 Further Reading

- [CMake Presets Documentation](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [Alloy API Reference](docs/API_REFERENCE.md)
- [Contributing Guide](CONTRIBUTING.md)

---

## 🆘 Getting Help

### Show Help

```bash
make help            # Makefile commands
./alloy.py --help    # Python CLI commands
```

### Report Issues

If you encounter problems:

1. Check this guide
2. Run `make info` to see configuration
3. Open an issue on GitHub with:
   - Output of `make info`
   - Error message
   - Steps to reproduce

---

**Happy Building!** 🚀
