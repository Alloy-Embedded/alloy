# Alloy Build Guide

Complete guide for building Alloy projects across different platforms.

## Prerequisites

### All Platforms

- **CMake** 3.25 or later
- **Python** 3.10 or later (for code generator)
- **Git** (for cloning and version control)

### Host Development (Linux/macOS/Windows)

- **GCC** 11+ or **Clang** 13+ with C++20 support
- **Make** or **Ninja** build system

**macOS:**
```bash
brew install cmake gcc@11
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake g++-11 python3 python3-pip
```

**Windows:**
- Install [MSYS2](https://www.msys2.org/)
- Or use Visual Studio 2022 with C++20 support

### ARM Cortex-M (Embedded Targets)

- **arm-none-eabi-gcc** 11+ toolchain
- **OpenOCD** or **pyOCD** for flashing/debugging

**macOS:**
```bash
brew install --cask gcc-arm-embedded
brew install openocd
```

**Ubuntu/Debian:**
```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch openocd
```

### Other Architectures

- **RL78**: GNURL78 or CC-RL compiler
- **ESP32**: ESP-IDF or xtensa-esp32-elf-gcc

## Project Structure

```
alloy/
├── src/                    # Source code
│   ├── core/              # Core utilities
│   ├── hal/               # Hardware Abstraction Layer
│   ├── drivers/           # External peripheral drivers
│   └── platform/          # Generated platform-specific code
├── cmake/                 # CMake modules
│   ├── boards/           # Board definitions
│   └── toolchains/       # Toolchain files
├── examples/             # Example projects
├── tests/                # Unit and integration tests
└── tools/                # Development tools
```

## Building for Host (Native)

Host builds are for development and testing without hardware.

### Configuration

```bash
# Configure for host (default)
cmake -B build -S . -DALLOY_BOARD=host

# Or specify build type
cmake -B build -S . -DALLOY_BOARD=host -DCMAKE_BUILD_TYPE=Release
```

### Build

```bash
# Build all targets
cmake --build build

# Build specific target
cmake --build build --target blinky

# Parallel build (faster)
cmake --build build -j$(nproc)
```

### Run Examples

```bash
# Run blinky (simulated GPIO on console)
./build/examples/blinky/blinky

# Run UART echo (stdin/stdout)
./build/examples/uart_echo/uart_echo
```

### Run Tests

```bash
# Run all tests
ctest --test-dir build

# Run tests with verbose output
ctest --test-dir build --verbose

# Run specific test
ctest --test-dir build -R test_gpio_host
```

## Building for ARM Targets

### Raspberry Pi Pico (RP2040)

```bash
# Configure
cmake -B build-pico -S . \
  -DALLOY_BOARD=rp_pico \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-pico

# Output files
ls build-pico/examples/blinky/
# → blinky.elf, blinky.bin, blinky.hex, blinky.uf2
```

**Flash to Pico:**
```bash
# Hold BOOTSEL button while plugging in
cp build-pico/examples/blinky/blinky.uf2 /Volumes/RPI-RP2/

# Or use picotool
picotool load build-pico/examples/blinky/blinky.elf
```

### STM32F103 (BluePill)

```bash
# Configure
cmake -B build-bluepill -S . \
  -DALLOY_BOARD=bluepill \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-bluepill
```

**Flash to BluePill:**
```bash
# Using ST-Link
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build-bluepill/examples/blinky/blinky.elf verify reset exit"

# Or using st-flash
st-flash write build-bluepill/examples/blinky/blinky.bin 0x08000000
```

## Building for Other Architectures

### Renesas RL78

```bash
# Configure
cmake -B build-rl78 -S . \
  -DALLOY_BOARD=cf_rl78 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rl78-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-rl78
```

**Flash to RL78:**
- Use Renesas Flash Programmer
- Or E2 Lite debugger

### ESP32

```bash
# Configure
cmake -B build-esp32 -S . \
  -DALLOY_BOARD=esp32_devkit \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/esp32.cmake \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-esp32
```

**Flash to ESP32:**
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 \
  write_flash 0x10000 build-esp32/examples/blinky/blinky.bin
```

## CMake Options

### Board Selection

```bash
-DALLOY_BOARD=<board>
```

Available boards (defined in `cmake/boards/`):
- `host` - Native compilation (default)
- `rp_pico` - Raspberry Pi Pico (RP2040)
- `bluepill` - STM32F103C8T6 (BluePill)
- `cf_rl78` - Renesas RL78
- `esp32_devkit` - ESP32-DevKitC

### Build Types

```bash
-DCMAKE_BUILD_TYPE=<type>
```

- `Debug` - No optimization, debug symbols (`-O0 -g`)
- `Release` - Full optimization (`-O3`)
- `RelWithDebInfo` - Optimization with debug info (`-O2 -g`)
- `MinSizeRel` - Size optimization (`-Os`)

### Advanced Options

```bash
# Enable verbose build
cmake --build build --verbose

# Clean build
cmake --build build --target clean

# Specify generator
cmake -B build -S . -G Ninja

# Export compile commands (for IDEs)
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## IDE Integration

### Visual Studio Code

1. Install CMake Tools extension
2. Select kit (GCC 11+ or arm-none-eabi-gcc)
3. Select board via CMake configure
4. Use CMake: Build command

**`.vscode/settings.json`:**
```json
{
  "cmake.configureArgs": [
    "-DALLOY_BOARD=host"
  ],
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

### CLion

1. Open project (CLion auto-detects CMake)
2. Go to Settings → Build → CMake
3. Add CMake options: `-DALLOY_BOARD=host`
4. Build and run

## Troubleshooting

### CMake can't find compiler

**Problem:** `Could not find arm-none-eabi-gcc`

**Solution:**
```bash
# Verify compiler is in PATH
which arm-none-eabi-gcc

# Or specify explicitly
cmake -B build -S . -DCMAKE_C_COMPILER=/path/to/arm-none-eabi-gcc
```

### C++20 not supported

**Problem:** `CMake Error: C++20 is not supported`

**Solution:** Update GCC/Clang:
```bash
# Ubuntu
sudo apt install g++-11
export CXX=g++-11

# macOS
brew install gcc@11
export CXX=g++-11
```

### Linker errors on ARM

**Problem:** `undefined reference to _exit`

**Solution:** Ensure `-specs=nosys.specs` is in linker flags (already in `cmake/toolchains/arm-none-eabi.cmake`)

### Permission denied when flashing

**Problem:** `/dev/ttyUSB0: Permission denied`

**Solution:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in
```

## Clean Rebuild

```bash
# Remove build directory
rm -rf build

# Reconfigure from scratch
cmake -B build -S . -DALLOY_BOARD=host
cmake --build build
```

## Further Reading

- [CMake Documentation](https://cmake.org/documentation/)
- [ARM GCC Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain)
- [CONTRIBUTING.md](CONTRIBUTING.md) - Development guidelines
- [architecture.md](architecture.md) - Technical architecture

---

**Questions?** Open an issue on [GitHub](https://github.com/alloy-embedded/alloy/issues)
