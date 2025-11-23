# Getting Started {#getting_started}

This tutorial will guide you through building your first MicroCore application in **under 10 minutes**.

[TOC]

## Prerequisites

### Required Software

- **CMake** 3.25 or newer
- **ARM GCC toolchain** (arm-none-eabi-gcc)
- **Python** 3.10 or newer
- **Git**

### Optional Tools

- **st-link** - For flashing STM32 boards
- **openocd** - Alternative flashing tool
- **Doxygen** - For building documentation
- **clang-format** - For code formatting

### Installation

#### macOS

```bash
# Install via Homebrew
brew install cmake arm-none-eabi-gcc python@3.10
brew install stlink openocd  # Optional: for flashing
```

#### Linux (Ubuntu/Debian)

```bash
# Install build tools
sudo apt-get update
sudo apt-get install cmake gcc-arm-none-eabi python3 python3-pip

# Install flash tools (optional)
sudo apt-get install stlink-tools openocd
```

#### Windows

1. Install [CMake](https://cmake.org/download/)
2. Install [ARM GCC](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
3. Install [Python 3.10+](https://www.python.org/downloads/)

## Clone the Repository

```bash
git clone https://github.com/lgili/corezero.git
cd corezero
```

## Your First Build

### Step 1: List Available Boards

```bash
./ucore list boards
```

Output:
```
======================================================================
                           Supported Boards
======================================================================

nucleo_f401re [YAML + Build]
  Name:         Nucleo-F401RE
  Vendor:       STMicroelectronics
  MCU:          STM32F401RET6
  Architecture: cortex-m4
  Frequency:    84 MHz
  Platform:     stm32f4
  Flash:        512 KB
  RAM:          96 KB
  Flash tool:   st-flash

...
```

### Step 2: List Available Examples

```bash
./ucore list examples
```

Output:
```
Available Examples

  • blink
    Simple LED blink example
  • uart_logger
    UART logging example
  • systick_timer
    SysTick timer demonstration
  ...
```

### Step 3: Build the Blink Example

```bash
./ucore build nucleo_f401re blink
```

Output:
```
======================================================================
                    Building blink for nucleo_f401re
======================================================================

ℹ Board: STM32 Nucleo-F401RE (STM32F401RE)
ℹ Example: blink
ℹ Build type: Release

[1/2] Configuring...
✓ Configuration complete

[2/2] Building...
✓ Build complete

ℹ Binary location: build-nucleo_f401re/examples/blink/blink
ℹ Binary size: 1,234 bytes (1.2 KB)
```

### Step 4: Flash to Board (Optional)

Connect your board via USB, then:

```bash
./ucore flash nucleo_f401re blink
```

Output:
```
======================================================================
                  Flashing blink to nucleo_f401re
======================================================================

⚠ Connect your board now and press Enter to continue...

Flashing with st-flash...
✓ Flash complete!

🎉 blink is now running on nucleo_f401re!
```

## Understanding the Code

Let's examine the blink example:

```cpp
// examples/blink/main.cpp
#include "board.hpp"

int main() {
    // Initialize board hardware
    board::init();

    // Infinite loop
    while (true) {
        board::led::toggle();    // Toggle LED state
        board::delay_ms(500);    // Wait 500ms
    }
}
```

### What's Happening?

1. **`board::init()`** - Initializes:
   - System clock (PLL configuration)
   - GPIO peripheral clocks
   - SysTick timer (1ms tick)
   - LED GPIO pin

2. **`board::led::toggle()`** - Platform-independent LED control

3. **`board::delay_ms(500)`** - Precise millisecond delay using SysTick

### Board Abstraction

The `board` namespace provides a **portable API** that works across all boards:

```cpp
namespace board {
    void init();              // Initialize board
    void delay_ms(uint32_t);  // Delay in milliseconds

    namespace led {
        void init();          // Initialize LED GPIO
        void on();            // Turn LED on
        void off();           // Turn LED off
        void toggle();        // Toggle LED state
    }
}
```

## Creating Your Own Application

### Method 1: Using an Existing Example

Copy an example and modify it:

```bash
# Copy blink example
cp -r examples/blink examples/my_app

# Edit your code
vim examples/my_app/main.cpp
```

Build your app:

```bash
./ucore build nucleo_f401re my_app
```

### Method 2: From Scratch

Create a new directory:

```bash
mkdir -p examples/my_app
cd examples/my_app
```

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.25)

project(my_app
    DESCRIPTION "My custom application"
    LANGUAGES CXX
)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE
    ucore::board
)
```

Create `main.cpp`:

```cpp
#include "board.hpp"

int main() {
    board::init();

    // Your code here
    while (true) {
        board::led::on();
        board::delay_ms(100);
        board::led::off();
        board::delay_ms(900);
    }
}
```

Build:

```bash
cd ../..  # Return to project root
./ucore build nucleo_f401re my_app
```

## Using Different Peripherals

### UART Example

```cpp
#include "board.hpp"
#include "hal/api/uart_simple.hpp"

using namespace ucore::hal;

int main() {
    board::init();

    // Configure UART
    using UartConfig = SimpleUartConfigTxOnly<
        board::uart::TxPin,
        board::uart::Policy
    >;

    UartConfig uart{
        board::uart::peripheral_id,
        BaudRate{115200},
        8,  // Data bits
        UartParity::NONE,
        1   // Stop bits
    };

    auto result = uart.initialize();
    if (!result.is_ok()) {
        // Handle error
        while (true);
    }

    // Write to UART
    uart.write_string("Hello, MicroCore!\r\n");

    while (true) {
        uart.write_byte('.');
        board::delay_ms(1000);
    }
}
```

### GPIO Example

```cpp
#include "board.hpp"
#include "hal/gpio.hpp"

using namespace ucore::hal;

// Define custom GPIO pin
using MyLed = GpioPin<peripherals::GPIOB, 3>;

int main() {
    board::init();

    // Configure custom pin
    MyLed::configure_output();

    while (true) {
        MyLed::set_high();
        board::delay_ms(250);
        MyLed::set_low();
        board::delay_ms(250);
    }
}
```

## Working with Multiple Boards

MicroCore makes it easy to support multiple boards:

```cpp
#include "board.hpp"

int main() {
    board::init();

    // This code works on ANY board!
    while (true) {
        board::led::toggle();
        board::delay_ms(board::is_debug_build() ? 100 : 500);
    }
}
```

Build for different boards:

```bash
./ucore build nucleo_f401re my_app   # STM32F4
./ucore build nucleo_f722ze my_app   # STM32F7
./ucore build same70_xplained my_app # SAME70
```

## Debugging

### Debug Build

```bash
./ucore build nucleo_f401re blink --debug
```

This enables:
- Debug symbols (`-g`)
- No optimization (`-O0`)
- Assertions enabled
- Verbose logging

### Using GDB

```bash
# Start GDB server
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# In another terminal
arm-none-eabi-gdb build-nucleo_f401re/examples/blink/blink

# In GDB
(gdb) target remote localhost:3333
(gdb) load
(gdb) break main
(gdb) continue
```

## Host Platform Testing

Test your code **without hardware**:

```cpp
// test_my_app.cpp
#include <catch2/catch_test_macros.hpp>
#include "hal/vendors/host/gpio_hardware_policy.hpp"

using namespace ucore::hal::host;

TEST_CASE("LED toggle works") {
    using Led = HostGpioHardwarePolicy<0, 5>;

    Led::configure_output();
    Led::set_low();

    // Toggle should change state
    Led::toggle();
    REQUIRE(Led::read() == true);

    Led::toggle();
    REQUIRE(Led::read() == false);
}
```

Build and run:

```bash
cmake -B build-host -DMICROCORE_BOARD=host
cmake --build build-host
cd build-host && ctest
```

See @ref host_testing for details.

## Project Structure

```
corezero/
├── src/                 # MicroCore source code
│   ├── core/           # Core utilities (Result, error handling)
│   ├── hal/            # Hardware Abstraction Layer
│   │   ├── api/        # Platform-independent APIs
│   │   ├── concepts/   # C++20 concept definitions
│   │   └── vendors/    # Platform-specific implementations
│   └── rtos/           # RTOS integration (optional)
├── boards/             # Board configurations
│   ├── nucleo_f401re/
│   │   ├── board.yaml  # YAML configuration
│   │   ├── board.hpp   # Board abstraction
│   │   └── board.cpp   # Board initialization
│   └── ...
├── examples/           # Example applications
│   ├── blink/
│   ├── uart_logger/
│   └── ...
├── tests/              # Unit tests
│   └── host/          # Host platform tests
├── tools/              # Build tools
│   └── codegen/       # Code generation scripts
├── cmake/              # CMake modules
├── docs/               # Documentation
└── ucore               # Unified CLI tool
```

## Common Issues

### Problem: "arm-none-eabi-gcc not found"

**Solution:** Install ARM GCC toolchain:
```bash
# macOS
brew install arm-none-eabi-gcc

# Linux
sudo apt-get install gcc-arm-none-eabi
```

### Problem: "st-flash not found"

**Solution:** Install st-link tools:
```bash
# macOS
brew install stlink

# Linux
sudo apt-get install stlink-tools
```

### Problem: "Flash failed - no ST-LINK detected"

**Solution:**
1. Connect board via USB
2. Check board power LED is on
3. Try pressing reset button
4. Use `--reset` flag: `./ucore flash nucleo_f401re blink --reset`

### Problem: "undefined reference to __aeabi_..."

**Solution:** Link against appropriate libraries:
```cmake
target_link_libraries(my_app PRIVATE
    ucore::board
    m  # Math library
)
```

## Next Steps

Now that you've built your first application, explore:

- @ref adding_board - Create a custom board configuration
- @ref concepts - Learn about C++20 concepts
- @ref examples - Browse example projects
- @ref porting_platform - Port to a new platform

## Additional Resources

- [MicroCore GitHub](https://github.com/lgili/corezero)
- [API Reference](index.html)
- [Contributing Guide](../CONTRIBUTING.md)
- [Board Configuration Guide](../BOARD_CONFIGURATION.md)

**Happy coding with MicroCore!** 🚀
