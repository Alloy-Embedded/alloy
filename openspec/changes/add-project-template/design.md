# Design: Project Template Repository

## 1. Git Submodule Integration

### 1.1 Submodule Setup
```bash
# In template repository root
git submodule add https://github.com/user/alloy.git external/alloy
git submodule update --init --recursive

# Track specific version (stable release)
cd external/alloy
git checkout v1.0.0
cd ../..
git add external/alloy
git commit -m "Pin Alloy to v1.0.0"
```

### 1.2 .gitmodules Configuration
```ini
[submodule "external/alloy"]
    path = external/alloy
    url = https://github.com/user/alloy.git
    branch = main  # or specific release branch
```

### 1.3 User Workflow
```bash
# Clone with submodules
git clone --recursive https://github.com/user/my-project.git

# Or clone then init submodules
git clone https://github.com/user/my-project.git
git submodule update --init --recursive

# Update Alloy to latest
cd external/alloy
git pull origin main
cd ../..
git add external/alloy
git commit -m "Update Alloy to latest"
```

### 1.4 Setup Script
```bash
#!/bin/bash
# scripts/setup-submodule.sh

echo "Initializing Alloy submodule..."

if [ ! -d "external/alloy/.git" ]; then
    git submodule update --init --recursive
    echo "✓ Alloy submodule initialized"
else
    echo "✓ Alloy submodule already initialized"
fi

cd external/alloy
ALLOY_VERSION=$(git describe --tags)
echo "Alloy version: $ALLOY_VERSION"
cd ../..

echo "Setup complete!"
```

## 2. CMake Build System Design

### 2.1 Root CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)

# Project definition
project(AlloyProject
    VERSION 1.0.0
    DESCRIPTION "Alloy-based embedded project"
    LANGUAGES CXX C ASM
)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Configuration Options
#
set(BOARD "stm32f407vg" CACHE STRING "Target board (from Alloy or custom)")
set(TARGET "application" CACHE STRING "Target to build (bootloader/application/factory_test)")

option(BUILD_BOOTLOADER "Build bootloader" ON)
option(BUILD_APPLICATION "Build application" ON)
option(BUILD_TESTS "Build unit tests" OFF)

#
# Add Alloy framework
#
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/alloy/CMakeLists.txt")
    message(FATAL_ERROR
        "Alloy submodule not initialized. Run: ./scripts/setup-submodule.sh")
endif()

add_subdirectory(external/alloy)

#
# Custom boards search path
#
# Allows boards/ directory to override Alloy boards
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/boards")

#
# Common library (shared across targets)
#
add_subdirectory(common)

#
# Targets (firmwares)
#
if(BUILD_BOOTLOADER)
    add_subdirectory(bootloader)
endif()

if(BUILD_APPLICATION)
    add_subdirectory(application)
endif()

#
# Unit tests (host platform)
#
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#
# Summary
#
message(STATUS "========================================")
message(STATUS "  Project Configuration")
message(STATUS "========================================")
message(STATUS "  Board:       ${BOARD}")
message(STATUS "  Target:      ${TARGET}")
message(STATUS "  Build Type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  Toolchain:   ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
```

### 2.2 Common Library CMakeLists.txt
```cmake
# common/CMakeLists.txt

add_library(common STATIC
    src/utils.cpp
    src/logging.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link against Alloy HAL
target_link_libraries(common PUBLIC
    alloy::core
    alloy::hal
)

# Compiler warnings
target_compile_options(common PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O2>
)
```

### 2.3 Application CMakeLists.txt
```cmake
# application/CMakeLists.txt

# Create executable
add_executable(application
    main.cpp
    tasks/sensor_task.cpp
    tasks/display_task.cpp
    tasks/comm_task.cpp
)

# Link libraries
target_link_libraries(application PRIVATE
    common
    alloy::rtos
    alloy::hal
)

# Apply board configuration (from Alloy or custom)
alloy_target_board_settings(application ${BOARD})

# Generate binary, hex, map files
alloy_generate_outputs(application)

# Print size information
alloy_print_size(application)
```

### 2.4 Custom Board CMake Discovery
```cmake
# In Alloy's cmake/board_discovery.cmake (extended)

function(alloy_find_board BOARD_NAME OUT_VAR)
    # 1. Check custom boards first (user project)
    if(EXISTS "${CMAKE_SOURCE_DIR}/boards/${BOARD_NAME}/board.hpp")
        set(${OUT_VAR} "${CMAKE_SOURCE_DIR}/boards/${BOARD_NAME}" PARENT_SCOPE)
        message(STATUS "Using custom board: ${BOARD_NAME}")
        return()
    endif()

    # 2. Check Alloy boards
    if(EXISTS "${ALLOY_ROOT}/boards/${BOARD_NAME}/board.hpp")
        set(${OUT_VAR} "${ALLOY_ROOT}/boards/${BOARD_NAME}" PARENT_SCOPE)
        message(STATUS "Using Alloy board: ${BOARD_NAME}")
        return()
    endif()

    # 3. Not found
    message(FATAL_ERROR "Board '${BOARD_NAME}' not found")
endfunction()
```

## 3. Custom Board Structure

### 3.1 Custom Board Directory Layout
```
boards/my_custom_board/
├── board.hpp            # Board definition (required)
├── board.cmake          # CMake config (optional)
├── linker.ld            # Linker script (optional, can inherit)
├── startup.cpp          # Startup code (optional, can inherit)
└── README.md            # Board documentation
```

### 3.2 board.hpp (Inheriting from Alloy Board)
```cpp
// boards/my_custom_board/board.hpp
#ifndef MY_CUSTOM_BOARD_HPP
#define MY_CUSTOM_BOARD_HPP

// Base board (similar hardware)
#include "external/alloy/boards/stm32f407vg/board.hpp"

namespace Board {
    // Override identification
    inline constexpr const char* name = "My Custom Board v1.0";
    inline constexpr const char* mcu = "STM32F407VGT6";

    // Inherit system_clock_hz from base board

    // Define custom pins
    namespace CustomPins {
        using StatusLed = alloy::hal::stm32f4::GpioPin<32>;  // PC0
        using ErrorLed  = alloy::hal::stm32f4::GpioPin<33>;  // PC1

        using SensorPower = alloy::hal::stm32f4::GpioPin<48>;  // PD0
        using SensorCs    = alloy::hal::stm32f4::GpioPin<49>;  // PD1
    }

    // Override or extend initialization
    inline void initialize() {
        // Call base initialization (clock, GPIO, etc.)
        using namespace alloy::hal::st::stm32f4;

        static SystemClock clock;
        auto result = clock.set_frequency(168'000'000);
        if (!result.is_ok()) {
            clock.set_frequency(16'000'000);
        }

        clock.enable_peripheral(alloy::hal::Peripheral::GpioA);
        clock.enable_peripheral(alloy::hal::Peripheral::GpioB);
        clock.enable_peripheral(alloy::hal::Peripheral::GpioC);
        clock.enable_peripheral(alloy::hal::Peripheral::GpioD);

        // Custom initialization
        CustomPins::StatusLed led;
        led.configure(alloy::hal::PinMode::Output);
        led.set_low();  // Initially off

        CustomPins::SensorPower power;
        power.configure(alloy::hal::PinMode::Output);
        power.set_high();  // Power on sensors
    }

    // Inherit delay_ms from base board
    using ::Board::delay_ms;

    // Inherit LED helpers or define custom ones
    namespace Led {
        inline void status_on() {
            static CustomPins::StatusLed led;
            led.set_high();
        }

        inline void status_off() {
            static CustomPins::StatusLed led;
            led.set_low();
        }

        inline void error_on() {
            static CustomPins::ErrorLed led;
            led.set_high();
        }
    }
}

#endif
```

### 3.3 board.hpp (From Scratch)
```cpp
// boards/completely_custom/board.hpp
#ifndef COMPLETELY_CUSTOM_BOARD_HPP
#define COMPLETELY_CUSTOM_BOARD_HPP

#include "hal/st/stm32f4/gpio.hpp"
#include "hal/st/stm32f4/clock.hpp"
#include "hal/st/stm32f4/delay.hpp"

namespace Board {
    inline constexpr const char* name = "Completely Custom Board";
    inline constexpr const char* mcu = "STM32F407VGT6";
    inline constexpr uint32_t system_clock_hz = 168'000'000;

    // Define all pins
    using Led1 = alloy::hal::stm32f4::GpioPin<12>;  // PA12
    using Led2 = alloy::hal::stm32f4::GpioPin<13>;  // PA13
    // ... etc

    inline void initialize() {
        // Implement full initialization
    }

    inline void delay_ms(uint32_t ms) {
        alloy::hal::st::stm32f4::delay_ms(ms);
    }
}

#endif
```

## 4. Multi-Target Build

### 4.1 Bootloader Target
```cpp
// bootloader/main.cpp
#include "board.hpp"
#include "common/include/config.h"

// Bootloader configuration
constexpr uint32_t APP_START_ADDRESS = 0x08008000;  // 32KB offset
constexpr uint32_t BOOT_TIMEOUT_MS = 2000;

void jump_to_application() {
    // Disable interrupts
    __disable_irq();

    // Set stack pointer
    uint32_t app_stack = *(volatile uint32_t*)APP_START_ADDRESS;
    __set_MSP(app_stack);

    // Get reset handler address
    uint32_t app_reset = *(volatile uint32_t*)(APP_START_ADDRESS + 4);

    // Jump to application
    void (*app_reset_handler)(void) = (void (*)(void))app_reset;
    app_reset_handler();
}

int main() {
    Board::initialize();

    // Check for bootloader entry condition
    // (e.g., button pressed, magic value in RAM, etc.)
    bool enter_bootloader = check_bootloader_condition();

    if (!enter_bootloader) {
        // Validate application
        if (is_app_valid(APP_START_ADDRESS)) {
            jump_to_application();
        }
    }

    // Bootloader mode
    bootloader_main_loop();

    return 0;
}
```

### 4.2 Application Target
```cpp
// application/main.cpp
#include "board.hpp"
#include "rtos/rtos.hpp"
#include "tasks/sensor_task.hpp"
#include "tasks/display_task.hpp"

// RTOS tasks
Task<512, Priority::High>   sensor_task(sensor_task_func);
Task<512, Priority::Normal> display_task(display_task_func);
Task<256, Priority::Low>    idle_task(idle_task_func);

int main() {
    Board::initialize();

    // Start RTOS
    RTOS::start();  // Never returns

    return 0;
}
```

### 4.3 Linker Script Modification (for bootloader offset)
```ld
/* application/linker.ld (derived from board linker) */

MEMORY
{
    /* Application starts at 32KB offset (after bootloader) */
    FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 992K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Rest of linker script... */
```

## 5. VSCode Integration

### 5.1 tasks.json
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Application (STM32F407)",
            "type": "shell",
            "command": "./scripts/build.sh",
            "args": ["application", "stm32f407vg"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Flash Application",
            "type": "shell",
            "command": "./scripts/flash.sh",
            "args": ["application", "stm32f407vg"],
            "dependsOn": ["Build Application (STM32F407)"]
        },
        {
            "label": "Clean Build",
            "type": "shell",
            "command": "./scripts/clean.sh"
        },
        {
            "label": "Build All Targets",
            "type": "shell",
            "command": "./scripts/build-all.sh",
            "args": ["stm32f407vg"]
        }
    ]
}
```

### 5.2 launch.json
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Application (OpenOCD)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceRoot}",
            "executable": "${workspaceRoot}/build/application/application.elf",
            "device": "STM32F407VG",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32f4x.cfg"
            ],
            "preLaunchTask": "Build Application (STM32F407)"
        }
    ]
}
```

### 5.3 settings.json
```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.cppStandard": "c++20",
    "C_Cpp.default.compilerPath": "/usr/bin/arm-none-eabi-g++",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/external/alloy/src",
        "${workspaceFolder}/common/include"
    ],
    "files.associations": {
        "*.hpp": "cpp",
        "*.tpp": "cpp"
    },
    "cmake.configureArgs": [
        "-DBOARD=stm32f407vg"
    ]
}
```

## 6. Helper Scripts

### 6.1 build.sh
```bash
#!/bin/bash
# scripts/build.sh <target> <board> [build_type]

set -e

TARGET=${1:-application}
BOARD=${2:-stm32f407vg}
BUILD_TYPE=${3:-Debug}

echo "Building $TARGET for $BOARD ($BUILD_TYPE)..."

# Create build directory
mkdir -p build/$TARGET

# Configure
cmake -B build/$TARGET \
      -DBOARD=$BOARD \
      -DTARGET=$TARGET \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# Build
cmake --build build/$TARGET -j$(nproc)

echo "✓ Build complete: build/$TARGET/${TARGET}.elf"
```

### 6.2 flash.sh
```bash
#!/bin/bash
# scripts/flash.sh <target> <board>

set -e

TARGET=${1:-application}
BOARD=${2:-stm32f407vg}

ELF="build/$TARGET/${TARGET}.elf"

if [ ! -f "$ELF" ]; then
    echo "Error: $ELF not found. Build first."
    exit 1
fi

echo "Flashing $TARGET to $BOARD..."

case $BOARD in
    stm32*)
        openocd -f interface/stlink.cfg \
                -f target/stm32f4x.cfg \
                -c "program $ELF verify reset exit"
        ;;
    esp32*)
        esptool.py --chip esp32 write_flash 0x10000 build/$TARGET/${TARGET}.bin
        ;;
    rp_pico)
        picotool load -f $ELF
        ;;
    *)
        echo "Unknown board: $BOARD"
        exit 1
        ;;
esac

echo "✓ Flash complete"
```

## 7. Documentation Structure

### README.md Structure
```markdown
# My Alloy Project

Brief description of your project.

## Quick Start

### Prerequisites
- CMake 3.20+
- ARM GCC toolchain
- OpenOCD (for STM32)

### Build
```bash
git clone --recursive <your-repo>
cd <your-repo>
./scripts/build.sh application stm32f407vg
```

### Flash
```bash
./scripts/flash.sh application stm32f407vg
```

## Project Structure
- `application/` - Main firmware
- `bootloader/` - Bootloader (optional)
- `common/` - Shared code
- `boards/` - Custom boards
- See [STRUCTURE.md](docs/STRUCTURE.md) for details

## Custom Boards
See [CUSTOM_BOARDS.md](docs/CUSTOM_BOARDS.md)

## License
MIT
```

## 8. Memory Layout (Multi-Target)

```
Flash (1MB STM32F407):
┌────────────────────────────────────┐ 0x08000000
│                                    │
│    Bootloader (32KB)               │
│                                    │
├────────────────────────────────────┤ 0x08008000
│                                    │
│    Application (960KB)             │
│                                    │
│                                    │
├────────────────────────────────────┤ 0x080F8000
│    Configuration (32KB)            │
└────────────────────────────────────┘ 0x08100000

RAM (128KB):
┌────────────────────────────────────┐ 0x20000000
│    Bootloader/App Shared (4KB)     │ - Boot reason, version
├────────────────────────────────────┤ 0x20001000
│                                    │
│    Application RAM (124KB)         │
│    - Stacks                        │
│    - Heap (if used)                │
│    - .data/.bss                    │
│                                    │
└────────────────────────────────────┘ 0x20020000
```

## 9. Build Performance

**Targets**:
- Incremental build: <5 seconds (change one .cpp file)
- Clean build (application): ~20 seconds
- Clean build (all targets): ~45 seconds

**Optimizations**:
- Parallel builds: `cmake --build build -j$(nproc)`
- Ccache integration (optional)
- Precompiled headers for common includes
