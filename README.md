# Alloy

**The modern C++20 framework for bare-metal embedded systems**

![Status](https://img.shields.io/badge/status-in%20development-yellow)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## 🎯 Vision

Alloy is a C++20 framework designed to make embedded systems development **powerful, flexible, and radically easy to use**. We believe developers should spend their time solving domain problems, not fighting build systems or deciphering complex APIs.

### Why "Alloy"?

Like a metal alloy combines different elements to create something stronger than its parts, **Alloy** combines:
- Modern C++20 (Concepts, Ranges, constexpr)
- Embedded systems expertise
- Exceptional developer experience (DX)

The result: A framework that's simultaneously robust, efficient, and a pleasure to use.

---

## ✨ Features

- **🚀 Modern C++20**: Leverages Concepts, Ranges, and compile-time programming
- **🔧 Zero Vendor Lock-in**: Pure CMake, no custom build tools
- **⚡ Zero Overhead**: Compile-time configuration, no runtime penalties
- **🧪 Testable by Design**: Mock HAL for unit tests on host
- **🎯 Bare-Metal First**: No RTOS dependencies (RTOS support planned)
- **📦 No Dynamic Allocation**: Everything static or stack-based in HAL
- **🌍 Cross-Platform**: Consistent API across different MCUs
- **🤖 Auto Code Generation**: Startup code and peripherals generated from SVD files

---

## 🤖 Code Generation

Alloy includes an **automated code generation system** that creates startup code, vector tables, and peripheral definitions from CMSIS-SVD files. This allows adding support for new ARM MCUs in minutes instead of weeks.

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
alloy_generate_code(MCU STM32F103C8)
```

### Documentation

- **[Tutorial: Adding a New MCU](tools/codegen/docs/TUTORIAL_ADDING_MCU.md)**
- **[CMake Integration Guide](tools/codegen/docs/CMAKE_INTEGRATION.md)**
- **[Template Customization](tools/codegen/docs/TEMPLATES.md)**
- **[Troubleshooting Guide](tools/codegen/docs/TROUBLESHOOTING.md)**
- **[Code Generator README](tools/codegen/README.md)**

---

## 🛠️ Supported Hardware

### Phase 0 (Current - Development)
- ✅ **Host** (Linux/macOS/Windows) - Simulated HAL for development

### Phase 1 (Planned)
- 🔄 **Raspberry Pi Pico** (RP2040)
- 🔄 **STM32F446RE** (Nucleo board)

### Phase 2+ (Future)
- ⏳ STM32L4 (low-power)
- ⏳ ESP32-C6 (RISC-V + WiFi)
- ⏳ nRF52840 (BLE)

---

## 🚀 Quick Start

> **Note**: Alloy is currently in **Phase 0 (Foundation)**. The API shown below represents our design vision and is not yet fully implemented.

### Prerequisites

- **CMake** 3.25+
- **C++ Compiler** with C++20 support:
  - GCC 11+ (for ARM: arm-none-eabi-gcc 11+)
  - Clang 13+
- **Python** 3.10+ (for code generator)
- **Google Test** (for unit tests)

### Example: Blinky

```cpp
#include <alloy/hal/gpio.hpp>
#include <alloy/platform/delay.hpp>

using namespace alloy::hal;

int main() {
    // Create an output pin (compile-time configured)
    GpioPin<25, PinMode::Output> led;
    led.initialize();

    while (true) {
        led.toggle();
        alloy::platform::delay_ms(500);
    }
}
```

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.25)

# Select your board
set(ALLOY_BOARD "rp_pico")

# Include Alloy framework
add_subdirectory(external/alloy)

project(blinky CXX)

add_executable(blinky src/main.cpp)
target_link_libraries(blinky PRIVATE alloy::hal::gpio)
```

---

## 📚 Documentation

- **[Plan](plan.md)** - Project vision and roadmap
- **[Architecture](architecture.md)** - Technical architecture details
- **[Decisions (ADR)](decisions.md)** - Architecture decision records
- **[Code Generation](codegen-system.md)** - How code generation works

---

## 🏗️ Project Structure

```
alloy/
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
- [ ] CLI tool (`alloy-cli`)
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

Alloy is in **active development** (Phase 0). Contributions are welcome!

### How to Contribute

1. Check [open issues](https://github.com/alloy-embedded/alloy/issues)
2. Read our [Architecture](architecture.md) and [Decisions](decisions.md)
3. Fork the repo and create a feature branch
4. Follow our [naming conventions](decisions.md#adr-011-naming-conventions-snake_case)
5. Submit a pull request

### Development Setup

```bash
# Clone the repository
git clone https://github.com/alloy-embedded/alloy.git
cd alloy

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
- **Namespaces**: `alloy::hal::`
- **CMake variables**: `ALLOY_BOARD`, `ALLOY_MCU`

See [ADR-011](decisions.md#adr-011-naming-conventions-snake_case) for complete conventions.

---

## 📊 Comparison with Existing Frameworks

| Feature | Alloy | modm | Arduino |
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

Alloy is released under the **MIT License**. See [LICENSE](LICENSE) for details.

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

*Alloy: Combining the best of C++20 and embedded systems*

</div>
