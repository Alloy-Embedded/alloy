# MicroCore

**The modern C++20 framework for bare-metal embedded systems**

![Status](https://img.shields.io/badge/status-in%20development-yellow)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![License](https://img.shields.io/badge/license-MIT-green)

**Quick Links:**
[Quick Start](#-quick-start) •
[Integration Paths](docs/INTEGRATION_PATHS.md) •
[Codegen Contract](docs/CODEGEN_INTERFACE_CONTRACT.md) •
[Code Generation](#-code-generation) •
[Generation Commands](#code-generation-commands) •
[Supported Hardware](#️-supported-hardware) •
[Documentation](#-documentation) •
[Contributing](#-contributing) •
[Coding Style](CODING_STYLE.md) •
[Code Quality](CODE_QUALITY_SETUP.md)

---

## 🎯 Vision

MicroCore is a C++20 framework designed to make embedded systems development **powerful, flexible, and radically easy to use**. We believe developers should spend their time solving domain problems, not fighting build systems or deciphering complex APIs.

### Why "MicroCore"?

**MicroCore** represents the essential core of modern embedded development, combining:
- Modern C++20 (Concepts, Ranges, constexpr)
- Embedded systems expertise
- Exceptional developer experience (DX)

The result: A framework that's simultaneously robust, efficient, and a pleasure to use.

---

## 🚀 Quick Start

### 1. Install Development Environment

**Automatic Setup (Recommended):**
```bash
./setup-dev-env.sh
```

This script will install:
- **Clang 21** (macOS) or **Clang 14** (Linux) - Required for C++20 support
- CMake 3.25+
- Ninja build system

**macOS Manual Setup:**
```bash
brew install llvm@21 cmake ninja
export PATH="$(brew --prefix llvm@21)/bin:$PATH"
export CC=$(brew --prefix llvm@21)/bin/clang
export CXX=$(brew --prefix llvm@21)/bin/clang++
```

**Linux Manual Setup:**
```bash
sudo apt-get install clang-14 cmake ninja-build
export CC=clang-14
export CXX=clang++-14
```

### 2. Install ARM Toolchain (For Embedded Targets)

**Automatic Installation:**
```bash
./scripts/install-xpack-toolchain.sh
```

This will download and install the [xPack ARM toolchain](https://xpack-dev-tools.github.io/arm-none-eabi-gcc-xpack/) which includes complete newlib support.

**After installation, add to your PATH:**
```bash
export PATH="$HOME/.local/xpack-arm-toolchain/bin:$PATH"
```

### 3. Build and Flash an Example

**Using the unified CLI (recommended):**

```bash
# List supported boards
./ucore list boards

# List available examples
./ucore list examples

# Build blink for Nucleo F401RE
./ucore build nucleo_f401re blink

# Build and flash in one command
./ucore flash nucleo_f401re blink
```

**Supported boards:**
- `nucleo_f401re` (STM32F401RE)
- `nucleo_f722ze` (STM32F722ZE)
- `nucleo_g071rb` (STM32G071RB)
- `nucleo_g0b1re` (STM32G0B1RE)
- `same70_xplained` (ATSAME70Q21B)

📖 **See [QUICKSTART.md](QUICKSTART.md) for complete guide**

### SDK Integration Options

MicroCore supports three integration paths for application projects:
- `add_subdirectory` (local/monorepo source dependency)
- `FetchContent` (source fetched by CMake)
- `find_package(microcore CONFIG REQUIRED)` (installed SDK package)

📖 **Details and examples:** [docs/INTEGRATION_PATHS.md](docs/INTEGRATION_PATHS.md)
📖 **Codegen boundary + compatibility contract:** [docs/CODEGEN_INTERFACE_CONTRACT.md](docs/CODEGEN_INTERFACE_CONTRACT.md)

### 4. Your First Blink

```cpp
#include "boards/nucleo_f401re/board.hpp"
#include "hal/api/systick_simple.hpp"

using namespace ucore::hal;

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
```

📖 **Full documentation:** [docs/toolchains.md](docs/toolchains.md)

---

## ✨ Features

### Modern C++20 Design
- **🔍 Concepts**: Type-safe interfaces with compile-time validation
- **🚀 Zero Overhead**: Template-based, fully inlined, single-instruction operations
- **🛡️ Type Safety**: Strong typing prevents configuration errors at compile-time
- **⚡ Compile-time Evaluation**: All configuration resolved at compile-time

### Architecture
- **🏗️ 5-Layer Architecture**: Generated Register → Hardware Policy → Platform → Board → Application
- **📐 Policy-Based Design**: Hardware policies provide zero-overhead abstractions
- **🔌 Interface Validation**: C++20 concepts ensure API compliance across platforms
- **📊 Automatic Code Generation**: Register and bitfield definitions from CMSIS-SVD files

### Developer Experience
- **🔧 Pure CMake**: No custom build tools, perfect IDE integration
- **🧪 Testable by Design**: Mock HAL for unit tests on host
- **📚 Comprehensive Documentation**: Architecture, porting guides, and tutorials
- **🌍 Consistent APIs**: Same interface across STM32F4, STM32F7, STM32G0, SAME70

### Embedded-Optimized
- **🎯 Bare-Metal First**: No RTOS dependencies (RTOS support planned)
- **📦 No Dynamic Allocation**: Everything static or stack-based
- **⚙️ Direct Register Access**: Thin abstraction over hardware
- **🔄 Result<T,E>**: Rust-style error handling without exceptions

---

## 🤖 Code Generation

MicroCore includes an **automated code generation system** that creates startup code, vector tables, and peripheral definitions from CMSIS-SVD files. This allows adding support for new ARM MCUs in minutes instead of weeks.

### Features

- **Automatic Startup Code**: `.data`/`.bss` initialization, static constructors, vector tables
- **SVD Parser**: Converts CMSIS-SVD XML to normalized JSON databases
- **CMake Integration**: Code generated transparently during build configuration
- **Hundreds of MCUs**: Supports STM32, nRF, RP2040, and more via CMSIS-SVD
- **Validated Output**: 38 automated tests ensure correct generation

### Quick Example

```bash
# Parse an SVD file to create database
python3 tools/codegen/svd_parser.py \
    --input STM32F103.svd \
    --output database/families/stm32f1xx.json

# Generate code for your MCU
python3 tools/codegen/generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output build/generated
```

Or let CMake handle it automatically:
```cmake
include(codegen)
microcore_generate_code(MCU STM32F103C8)
```

### Code Generation Commands

The code generator supports generating different types of code from CMSIS-SVD files. All commands should be run from the project root directory.

#### Generate Everything (Recommended)

Generate all code for all supported MCUs:

```bash
cd tools/codegen
python3 codegen.py generate --all
```

This will generate:
- **Startup code** (vector tables, initialization)
- **Register structures** (peripheral register layouts)
- **Bitfield definitions** (register bit masks and positions)
- **Enumerations** (peripheral-specific enums)
- **Pin functions** (alternate function mappings)
- **Complete register maps** (single-file includes)

#### Generate Specific Components

Generate only startup code for all MCUs:
```bash
python3 codegen.py generate --startup
```

Generate only register structures and bitfields:
```bash
python3 codegen.py generate --registers
```

Generate only pin alternate functions:
```bash
python3 codegen.py generate --pin-functions
```

Generate only peripheral enumerations:
```bash
python3 codegen.py generate --enums
```

#### Generate for Specific Vendor

Generate pin headers for ST Microelectronics only:
```bash
python3 codegen.py generate --pins --vendor st
```

Generate everything for Atmel only:
```bash
python3 codegen.py generate --all --vendor atmel
```

Supported vendors: `st`, `atmel`, `microchip`

#### Platform-Specific HAL Generation

Generate all platform HAL implementations (GPIO, UART, I2C, SPI, etc.):
```bash
python3 cli/generators/unified_generator.py
```

Generate specific peripheral for specific platform:
```bash
# GPIO for STM32F4
python3 cli/generators/platform/generate_gpio.py

# UART for SAME70
python3 cli/generators/platform/generate_uart.py
```

#### Utility Commands

Check generation status:
```bash
python3 codegen.py status
```

Show vendor information:
```bash
python3 codegen.py vendors
```

Show current configuration:
```bash
python3 codegen.py config
```

Clean generated files (dry-run):
```bash
python3 codegen.py clean --dry-run
```

Test SVD parser on specific file:
```bash
python3 codegen.py test-parser STMicro/STM32F103.svd --verbose
```

#### Complete Generation (Recommended)

**NEW**: Single command to generate everything, format, and validate:

```bash
cd tools/codegen
python3 codegen.py generate-complete
```

This will automatically:
1. Generate all vendor code (pins, startup, registers, enums)
2. Generate all platform HAL implementations
3. Format all generated code with clang-format
4. Validate startup files with clang-tidy

**Options**:
```bash
# Skip formatting step
python3 codegen.py generate-complete --skip-format

# Skip validation step
python3 codegen.py generate-complete --skip-validate

# Continue even if a step fails
python3 codegen.py generate-complete --continue-on-error
```

#### Manual Generation Sequence (Alternative)

If you prefer to run each step manually:

```bash
cd tools/codegen

# 1. Generate vendor code (registers, startup, pins)
python3 codegen.py generate --all

# 2. Generate platform HAL implementations
python3 cli/generators/unified_generator.py

# 3. Format all generated code
bash scripts/format_generated_code.sh

# 4. Validate with clang-tidy
bash scripts/validate_clang_tidy.sh
```

### Documentation

- **[Tutorial: Adding a New MCU](tools/codegen/docs/TUTORIAL_ADDING_MCU.md)**
- **[CMake Integration Guide](tools/codegen/docs/CMAKE_INTEGRATION.md)**
- **[Template Customization](tools/codegen/docs/TEMPLATES.md)**
- **[Troubleshooting Guide](tools/codegen/docs/TROUBLESHOOTING.md)**
- **[Code Generator README](tools/codegen/README.md)**

---

## 🛠️ Supported Hardware

### Currently Supported MCUs

MicroCore now supports **4 STM32 families + 1 Atmel SAME70** with **standardized APIs** validated by C++20 concepts:

| Platform | Core | Freq | Flash | RAM | API Validation | Status |
|----------|------|------|-------|-----|----------------|--------|
| **STM32F4** | Cortex-M4F | 168 MHz | 1MB | 192KB | ✅ ClockPlatform<br>✅ GpioPin | ✅ Phase 6 |
| **STM32F7** | Cortex-M7 | 216 MHz | 1MB | 512KB | ✅ ClockPlatform<br>✅ GpioPin | ✅ Phase 6 |
| **STM32G0** | Cortex-M0+ | 64 MHz | 512KB | 144KB | ✅ ClockPlatform<br>✅ GpioPin | ✅ Phase 6 |
| **SAME70** | Cortex-M7 | 300 MHz | 2MB | 384KB | 🔄 In Progress | 🔄 Planned |

### Phase 6 Achievement: API Standardization

All STM32 platforms now have:
- **Consistent APIs**: Same interface across STM32F4, F7, and G0
- **Concept Validation**: Compile-time verification with `static_assert`
- **Type Safety**: Strong typing prevents misuse
- **Zero Overhead**: Template-based, fully inlined implementations

### Architecture Support

- ✅ **ARM Cortex-M0+** (ATSAMD21, RP2040)
- ✅ **ARM Cortex-M3** (STM32F1)
- ✅ **ARM Cortex-M4F** (STM32F4 with FPU)
- ✅ **Xtensa LX6** (ESP32)
- ✅ **Host** (Linux/macOS/Windows) - Simulated HAL for development

### Peripheral Support by MCU

#### STM32F103C8 (Blue Pill)
- ✅ Clock: HSE (8MHz) + PLL → 72MHz
- ✅ GPIO: CRL/CRH configuration
- 🔄 UART, I2C, SPI, ADC, PWM, Timer (planned)

#### ESP32 (DevKit)
- ✅ Clock: 40MHz XTAL + PLL → 80/160/240MHz
- ✅ GPIO: 40 pins (GPIO0-39)
- 🔄 WiFi, Bluetooth, UART, I2C, SPI (planned)

#### STM32F407VG (Discovery)
- ✅ Clock: HSE (8MHz) + VCO PLL → 168MHz
- ✅ GPIO: MODER-based, 9 ports
- ✅ FPU: Hardware floating-point (enabled)
- 🔄 UART, I2C, SPI, ADC, DAC, PWM, DMA, USB OTG (planned)

#### ATSAMD21G18 (Arduino Zero)
- ✅ Clock: 32kHz XOSC32K + DFLL48M → 48MHz
- ✅ GPIO: PORT-based, atomic operations
- 🔄 SERCOM (UART/I2C/SPI), USB, ADC, DAC (planned)

#### RP2040 (Raspberry Pi Pico)
- ✅ Clock: 12MHz XOSC + PLL → 125/133MHz
- ✅ GPIO: SIO (single-cycle IO), 30 pins
- ✅ XIP: Execute-in-place from flash
- ✅ Dual-core support (startup ready)
- 🔄 PIO, UART, I2C, SPI, ADC, PWM, USB (planned)

### Board-Specific Details

Each board includes:
- ✅ Complete linker script (`.ld`)
- ✅ Startup code with vector table
- ✅ Board definition header with pin mappings
- ✅ Working blink example
- ✅ CMake build configuration

### Coming Soon

- ⏳ STM32L4 (ultra-low-power)
- ⏳ nRF52840 (Bluetooth LE)
- ⏳ STM32H7 (high-performance, 480MHz)
- ⏳ More peripherals (UART, I2C, SPI, ADC, PWM, DMA)

---

## 🔧 Peripheral Support by MCU Family

This table shows the implementation status of peripheral drivers (hardware policies) for each MCU family. Each peripheral has auto-generated hardware policies that provide low-level register access with zero runtime overhead.

### Legend
- ✅ **Implemented** - Hardware policy generated and tested
- 🔄 **Planned** - Will be implemented soon
- ⏳ **Future** - Support planned for later releases
- ➖ **Not Available** - Peripheral not present in this family

### Peripheral Support Matrix

| MCU Family | GPIO | UART | LPUART | SPI | I2C | ADC | DAC | Timer | DMA | USB | Ethernet | CAN | RTC | Watchdog | Total |
|-----------|------|------|--------|-----|-----|-----|-----|-------|-----|-----|----------|-----|-----|----------|-------|
| **SAME70** (Cortex-M7) | ✅ | ✅ | ➖ | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | ✅ | ✅ | ✅ | ⏳ | ⏳ | **4/14** |
| **STM32G0** (Cortex-M0+) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ➖ | ⏳ | ⏳ | ⏳ | **10/14** |
| **STM32F1** (Cortex-M3) | ✅ | 🔄 | ➖ | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | ➖ | ⏳ | ⏳ | ⏳ | **1/14** |
| **STM32F4** (Cortex-M4F) | ✅ | 🔄 | ➖ | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | ✅ | ⏳ | ⏳ | ⏳ | **2/14** |
| **ATSAMD21** (Cortex-M0+) | ✅ | 🔄 | ➖ | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | 🔄 | ➖ | ➖ | ⏳ | ⏳ | **1/14** |
| **RP2040** (Cortex-M0+ Dual) | ✅ | 🔄 | ➖ | 🔄 | 🔄 | 🔄 | ➖ | 🔄 | 🔄 | 🔄 | ➖ | ➖ | ⏳ | ⏳ | **1/14** |

### Family-Specific Details

#### SAME70 (Atmel ARM Cortex-M7)
- **Status**: Primary development platform
- **Peripherals**: 33 register files + 33 bitfield files
- **Hardware Policies**: GPIO, UART, USB, Ethernet (4/33)
- **Clock**: Up to 300 MHz
- **Unique Features**: Ethernet MAC, QSPI, HSMCI

#### STM32G0 (ST ARM Cortex-M0+)
- **Status**: ✅ **10/33 peripherals implemented**
- **Peripherals**: 33 register files + 33 bitfield files
- **Hardware Policies**: GPIO, UART, LPUART, SPI, I2C, ADC, DAC, Timer, DMA, USB (10/33)
- **Clock**: Up to 64 MHz
- **Unique Features**: Low-power UART, USB-C Power Delivery
- **Documentation**: See [docs/adding-new-mcu-family.md](docs/adding-new-mcu-family.md)

#### STM32F1 (ST ARM Cortex-M3)
- **Status**: Basic GPIO support
- **Hardware Policies**: GPIO (1/?)
- **Clock**: Up to 72 MHz
- **Unique Features**: Blue Pill board support

#### STM32F4 (ST ARM Cortex-M4F)
- **Status**: Basic GPIO support
- **Hardware Policies**: GPIO (1/?)
- **Clock**: Up to 168 MHz
- **Unique Features**: Hardware FPU, Discovery board support

#### ATSAMD21 (Microchip ARM Cortex-M0+)
- **Status**: Basic GPIO support
- **Hardware Policies**: GPIO (1/?)
- **Clock**: Up to 48 MHz
- **Unique Features**: Arduino Zero compatibility, SERCOM flexibility

#### RP2040 (Raspberry Pi ARM Cortex-M0+ Dual-Core)
- **Status**: Basic GPIO support
- **Hardware Policies**: GPIO (1/?)
- **Clock**: Up to 133 MHz
- **Unique Features**: Dual-core, PIO state machines, unique SIO

### Notes

1. **Auto-Generated Code**: All register definitions and bitfields are auto-generated from CMSIS-SVD files
2. **Hardware Policies**: Use Policy-Based Design pattern for zero runtime overhead
3. **Type-Safe**: All bitfield operations are compile-time validated
4. **Testable**: Mock hooks available for all hardware policies
5. **Documentation**: Each peripheral has metadata describing all policy methods

**Want to add support for a peripheral?** See [docs/adding-new-mcu-family.md](docs/adding-new-mcu-family.md) for a complete guide.

---

## 🚀 Quick Start

> **Note**: MicroCore is currently in **Phase 0 (Foundation)**. The API shown below represents our design vision and is not yet fully implemented.

### Prerequisites

- **CMake** 3.25+
- **C++ Compiler** with C++20 support:
  - GCC 11+ (for ARM: arm-none-eabi-gcc 11+)
  - Clang 13+
- **Python** 3.10+ (for code generator)
- **Google Test** (for unit tests)

### Example: Blinky

```cpp
#include "hal/gpio.hpp"
#include "hal/platform/delay.hpp"

using namespace ucore::hal;

int main() {
    // Create an output pin (compile-time configured)
    GpioPin<25, PinMode::Output> led;
    led.initialize();

    while (true) {
        led.toggle();
        ucore::platform::delay_ms(500);
    }
}
```

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.25)

# Select your board
set(MICROCORE_BOARD "nucleo_f401re")

# Include MicroCore framework
add_subdirectory(external/microcore)

project(blinky CXX)

add_executable(blinky src/main.cpp)
target_link_libraries(blinky PRIVATE ucore::hal::gpio)
```

---

## 🏛️ Architecture Overview

MicroCore uses a **5-layer architecture** with C++20 concepts for interface validation:

```
┌─────────────────────────────────────────────────────────┐
│  Layer 5: Application Layer                             │
│  ├─ User code (main.cpp)                                │
│  └─ Application logic                                   │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  Layer 4: Board Layer                                   │
│  ├─ Board-specific pin mappings                         │
│  ├─ Clock configuration                                 │
│  └─ Linker scripts                                      │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  Layer 3: Platform Implementation Layer                 │
│  ├─ gpio.hpp (satisfies GpioPin concept)               │
│  ├─ clock_platform.hpp (satisfies ClockPlatform)       │
│  └─ User-facing API with Result<T,E> error handling    │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  Layer 2: Hardware Policy Layer                         │
│  ├─ Low-level register manipulation                     │
│  ├─ Compile-time peripheral access                      │
│  └─ Zero runtime overhead                               │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  Layer 1: Generated Register Layer                      │
│  ├─ *_registers.hpp (auto-generated from SVD)           │
│  ├─ *_bitfields.hpp (type-safe bitfield access)         │
│  └─ Peripheral base addresses and structures            │
└─────────────────────────────────────────────────────────┘
```

### Key Design Patterns

1. **Policy-Based Design**: Hardware policies provide compile-time abstraction
2. **C++20 Concepts**: Interface validation ensures API compliance
3. **Template Metaprogramming**: All configuration resolved at compile-time
4. **Result<T,E>**: Rust-style error handling without exceptions

**Learn more**: See [Architecture Documentation](docs/ARCHITECTURE.md)

---

## 📚 Documentation

### Core Documentation
- **[Architecture](docs/ARCHITECTURE.md)** - Complete system architecture and design patterns
- **[Plan](plan.md)** - Project vision and roadmap
- **[Decisions (ADR)](decisions.md)** - Architecture decision records

### Developer Guides
- **[Porting a New Board](docs/PORTING_NEW_BOARD.md)** - Step-by-step guide to add new boards
- **[Porting a New Platform](docs/PORTING_NEW_PLATFORM.md)** - Guide to add new MCU families
- **[Code Generation](docs/CODE_GENERATION.md)** - Complete code generation system guide
- **[Toolchains](docs/toolchains.md)** - Toolchain setup and configuration

---

## 🏗️ Project Structure

```
corezero/
├── cmake/              # CMake scripts and toolchains
├── src/
│   ├── core/          # Core types, concepts, utilities
│   ├── hal/           # Hardware Abstraction Layer
│   │   ├── interface/ # Platform-agnostic interfaces
│   │   ├── rp2040/    # RP2040 implementation
│   │   ├── stm32f4/   # STM32F4 implementation
│   │   └── host/      # Host (mocked) implementation
│   ├── drivers/       # External peripheral drivers
│   └── platform/      # Generated code (startup, linker, etc)
├── tools/
│   └── codegen/       # Code generator (Python)
├── examples/          # Example projects
└── tests/             # Unit and integration tests
```

---

## 🎨 Design Philosophy

### 1. Developer Experience First
- **Pure CMake** - No custom build tools, perfect IDE integration
- **Clear Documentation** - The code should be self-documenting
- **Fast Iteration** - Quick compile-test cycles

### 2. Modern C++20 Done Right
- **Concepts** - Type-safe APIs with clear compiler errors
- **constexpr** - Compile-time validation and optimization
- **Ranges** - Expressive data manipulation
- **No Modules** - Sticking with headers for compatibility

### 3. Testability is Not Optional
- **Host-first Development** - Develop and test without hardware
- **HAL Mocks** - Unit test your application logic
- **Dependency Injection** - Clean architecture enables easy testing

### 4. Zero Overhead, Always
- **No Dynamic Allocation in HAL** - Predictable memory usage
- **Compile-time Configuration** - No runtime penalties
- **Direct Register Access** - Thin abstraction over hardware

---

## 🗺️ Roadmap

### Phase 0: Foundation (Current - 2-3 months)
- [ ] Directory structure and basic CMake setup
- [ ] GPIO interface (concepts) + host implementation
- [ ] Blinky example running on host
- [ ] Error handling system (`Result<T, Error>`)
- [ ] Google Test integration

### Phase 1: Hardware Support (3-4 months)
- [ ] RP2040 HAL (GPIO, UART, I2C, SPI, ADC, PWM)
- [ ] STM32F4 HAL (GPIO, UART)
- [ ] Board system (2-3 pre-defined boards)
- [ ] Code generator MVP
- [ ] Examples: blinky, uart_echo, i2c_sensor

### Phase 2: Ecosystem (4-6 months)
- [x] CLI tool (`ucore`)
- [ ] STM32F4 complete support
- [ ] 10+ external peripheral drivers
- [ ] CI/CD with hardware-in-the-loop tests
- [ ] First beta release

### Phase 3: Growth (6+ months)
- [ ] More MCU families (STM32L4, ESP32-C6, nRF52)
- [ ] Optional RTOS support (FreeRTOS)
- [ ] Community contribution system
- [ ] 1.0 Release

---

## 🤝 Contributing

MicroCore is in **active development** (Phase 0). Contributions are welcome!

### How to Contribute

1. Check [open issues](https://github.com/microcore-embedded/microcore/issues)
2. Read our [Architecture](architecture.md) and [Decisions](decisions.md)
3. Fork the repo and create a feature branch
4. Follow our [naming conventions](decisions.md#adr-011-naming-conventions-snake_case)
5. Submit a pull request

### Development Setup

```bash
# Clone the repository
git clone https://github.com/yourusername/corezero.git
cd corezero

# Build for host (no hardware needed)
cmake -B build -S .
cmake --build build

# Run tests
cd build && ctest
```

---

## 📋 Naming Conventions

- **Files**: `snake_case.hpp`, `snake_case.cpp`
- **Classes/Structs**: `PascalCase`
- **Functions/Methods**: `snake_case()`
- **Variables**: `snake_case`
- **Constants**: `UPPER_SNAKE_CASE`
- **Namespaces**: `ucore::hal::`
- **CMake variables**: `MICROCORE_BOARD`, `MICROCORE_MCU`

See [ADR-011](decisions.md#adr-011-naming-conventions-snake_case) for complete conventions.

---

## 📊 Comparison with Existing Frameworks

| Feature | MicroCore | modm | Arduino |
|---------|-------|------|---------|
| **Build System** | Pure CMake | lbuild (custom) | Arduino IDE |
| **C++ Standard** | C++20 | C++23 | C++11/17 |
| **IDE Integration** | Native | Limited | Arduino only |
| **Learning Curve** | Moderate | High | Low |
| **Testability** | Built-in | Limited | Poor |
| **Overhead** | Zero | Zero | Some |
| **MCU Support** | ~10 (growing) | 3,887 | Many |

---

## 📜 License

MicroCore is released under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

Inspired by:
- **modm** - For showing what's possible in embedded C++
- **Rust** - For error handling design (`Result<T, E>`)
- **CMSIS** - For standardizing ARM Cortex-M interfaces

---

## 📞 Contact & Community

- **GitHub**: [alloy-embedded/alloy](https://github.com/alloy-embedded/alloy)
- **Issues**: [Report bugs or request features](https://github.com/alloy-embedded/alloy/issues)
- **Discussions**: [Join the conversation](https://github.com/alloy-embedded/alloy/discussions)

---

<div align="center">

**Made with ❤️ for the embedded systems community**

*MicroCore: Combining the best of C++20 and embedded systems*

</div>
