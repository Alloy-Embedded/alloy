# Adding a New Board {#adding_board}

This guide shows you how to add support for a new development board to MicroCore using the **YAML-based board configuration system**.

[TOC]

## Overview

Adding a new board involves three main steps:

1. **Create board configuration** - Using interactive wizard or manual YAML
2. **Generate C++ code** - Auto-generate board_config.hpp from YAML
3. **Implement board initialization** - Write board-specific init code

Total time: **30-60 minutes** for a basic board.

## Prerequisites

- Development board with known MCU
- Board schematic or user manual
- Datasheet for the MCU
- Knowledge of:
  - LED GPIO pins
  - Button GPIO pins
  - UART pins (for console)
  - Clock configuration (crystal frequency, target speed)

## Method 1: Interactive Wizard (Recommended)

### Step 1: Run the Wizard

```bash
./ucore new-board my_custom_board
```

The wizard will guide you through:

```
======================================================================
                 New Board Wizard: my_custom_board
======================================================================

Board Information
Press Enter to use default value shown in [brackets]

  Board name [My Custom Board]:
```

### Step 2: Answer the Prompts

**Board Metadata:**
```
  Board name: My STM32 Discovery
  Vendor (e.g., STMicroelectronics): STMicroelectronics
  MCU part number (e.g., STM32F401RET6): STM32F407VGT6
```

**Platform Selection:**
```
Select Platform:
  1. stm32f0
  2. stm32f1
  3. stm32f4    ← Select this
  4. stm32f7
  5. stm32g0
  6. stm32g4
  7. stm32h7
  8. same70
  Platform number (1-8): 3
```

**Architecture:**
```
Select Architecture:
  1. cortex-m0
  2. cortex-m0+
  3. cortex-m3
  4. cortex-m4    ← Select this
  5. cortex-m7
  6. cortex-m33
  Architecture number (1-6): 4
```

**Clock Configuration:**
```
Clock Configuration:
  System clock frequency (MHz) [84]: 168
  Has external crystal? (y/n) [y]: y
```

**LED Configuration:**
```
LED Configuration:
  LED name [led_green]: led_green
  LED GPIO port (e.g., GPIOA) [GPIOA]: GPIOD
  LED pin number (0-15) [5]: 12
  LED active high? (y/n) [y]: y
```

### Step 3: Review Generated Files

The wizard creates:

```
boards/my_custom_board/
├── board.yaml       # YAML configuration (editable)
└── board.hpp        # C++ header (placeholder)
```

**Generated board.yaml:**
```yaml
# MicroCore Board Configuration: My STM32 Discovery

board:
  name: "My STM32 Discovery"
  vendor: "STMicroelectronics"
  version: "1.0.0"
  description: "My STM32 Discovery development board"

platform: stm32f4

mcu:
  part_number: "STM32F407VGT6"
  architecture: cortex-m4
  frequency_mhz: 168

clock:
  source: PLL
  system_clock_hz: 168000000
  hse_hz: 8000000  # TODO: Update with actual crystal frequency
  pll:
    m: 4   # TODO: Calculate PLL parameters
    n: 168
    p: 4
    q: 7
  ahb_prescaler: 1
  apb1_prescaler: 2
  apb2_prescaler: 1
  flash_latency: 2  # TODO: Update based on frequency

leds:
  - name: led_green
    port: GPIOD
    pin: 12
    active_high: true
    description: "User LED"

buttons: []

uart: []
```

## Method 2: Manual YAML Creation

### Step 1: Create Directory

```bash
mkdir -p boards/my_custom_board
```

### Step 2: Create board.yaml

Create `boards/my_custom_board/board.yaml`:

```yaml
# MicroCore Board Configuration

board:
  name: "My Custom Board"
  vendor: "My Company"
  version: "1.0.0"
  description: "Custom development board with STM32F4"
  url: "https://example.com/my-board"

platform: stm32f4

mcu:
  part_number: "STM32F407VGT6"
  architecture: cortex-m4
  flash_kb: 1024
  ram_kb: 192
  frequency_mhz: 168

clock:
  source: PLL
  system_clock_hz: 168000000
  hse_hz: 8000000
  pll:
    m: 8    # HSE / 8 = 1 MHz (PLL input)
    n: 336  # 1 MHz × 336 = 336 MHz (VCO)
    p: 2    # 336 MHz / 2 = 168 MHz (SYSCLK)
    q: 7    # 336 MHz / 7 = 48 MHz (USB)
  ahb_prescaler: 1    # AHB = 168 MHz
  apb1_prescaler: 4   # APB1 = 42 MHz
  apb2_prescaler: 2   # APB2 = 84 MHz
  flash_latency: 5    # 5 wait states for 168 MHz

leds:
  - name: led_green
    color: green
    port: GPIOD
    pin: 12
    active_high: true
    description: "Green LED (LD4)"

  - name: led_orange
    color: orange
    port: GPIOD
    pin: 13
    active_high: true
    description: "Orange LED (LD3)"

buttons:
  - name: button_user
    port: GPIOA
    pin: 0
    active_high: true
    pull: down
    description: "User button (B1)"

uart:
  - name: console
    instance: USART2
    tx_port: GPIOA
    tx_pin: 2
    rx_port: GPIOA
    rx_pin: 3
    baud_rate: 115200
    description: "Console UART"

spi:
  - name: spi1
    instance: SPI1
    sck_port: GPIOA
    sck_pin: 5
    mosi_port: GPIOA
    mosi_pin: 7
    miso_port: GPIOA
    miso_pin: 6
    description: "SPI1 peripheral"

i2c:
  - name: i2c1
    instance: I2C1
    scl_port: GPIOB
    scl_pin: 6
    sda_port: GPIOB
    sda_pin: 7
    speed_hz: 100000
    description: "I2C1 peripheral"
```

## Step 3: Calculate PLL Parameters

Use the **PLL calculation formula** for your platform:

### STM32F4 PLL Formula

```
VCO_IN  = HSE / PLLM           (Must be 1-2 MHz)
VCO_OUT = VCO_IN × PLLN        (Must be 100-432 MHz)
SYSCLK  = VCO_OUT / PLLP       (Target frequency)
USB_CLK = VCO_OUT / PLLQ       (Should be 48 MHz for USB)
```

### Example Calculation (168 MHz from 8 MHz crystal)

```
Target: 168 MHz SYSCLK, 48 MHz USB

PLLM = 8   → VCO_IN  = 8 MHz / 8 = 1 MHz ✓
PLLN = 336 → VCO_OUT = 1 MHz × 336 = 336 MHz ✓
PLLP = 2   → SYSCLK  = 336 MHz / 2 = 168 MHz ✓
PLLQ = 7   → USB_CLK = 336 MHz / 7 = 48 MHz ✓
```

### SAME70 PLL Formula

```
PLLA_OUT = (HSE × (MULA + 1)) / DIVA
MCK      = PLLA_OUT / PRESCALER
```

### Example Calculation (300 MHz from 12 MHz crystal)

```
DIVA = 1   (M parameter)
MULA = 24  (N-1 parameter, so N = 25)
PRES = 2   (P parameter)

PLLA = (12 MHz × 25) / 1 = 300 MHz ✓
MCK  = 300 MHz / 2 = 150 MHz ✓
```

## Step 4: Validate Configuration

```bash
./ucore validate-board my_custom_board
```

Expected output:
```
======================================================================
                Validating Board: my_custom_board
======================================================================

ℹ️  Validating: boards/my_custom_board/board.yaml
✅ Valid board configuration

✓ Board configuration is valid!

Board Summary:
  Name:      My Custom Board
  Vendor:    My Company
  MCU:       STM32F407VGT6
  Platform:  stm32f4
  Clock:     168 MHz
  LEDs:      2
  Buttons:   1
  UART:      1
```

Common validation errors and fixes:

| Error | Cause | Fix |
|-------|-------|-----|
| `Invalid platform` | Platform not in schema | Use: stm32f0/f1/f4/f7/g0/g4/h7/same70 |
| `Invalid pin number` | Pin > 15 | Use 0-15 |
| `Invalid GPIO port` | Wrong format | Use GPIOA, GPIOB, etc. |
| `Invalid name` | Not snake_case | Use led_green not LED-Green |

## Step 5: Generate C++ Code

```bash
./ucore generate-board my_custom_board
```

This generates `boards/my_custom_board/board_config.hpp`:

```cpp
#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for My Custom Board
 *
 * Auto-generated from board.yaml
 * DO NOT EDIT MANUALLY - Run `ucore generate-board` to regenerate
 */

#include <cstdint>
#include "hal/vendors/st/stm32f4/gpio.hpp"
#include "hal/vendors/st/stm32f4/stm32f407/peripherals.hpp"

namespace my_custom_board {

using namespace ucore::hal::st::stm32f4;
using namespace ucore::generated::stm32f407vgt6;

// Board Information
struct BoardInfo {
    static constexpr const char* name = "My Custom Board";
    static constexpr const char* vendor = "My Company";
    // ...
};

// Clock Configuration with auto-calculated comments
/**
 * Clock Source: PLL
 * System Clock: 168000000 Hz (168 MHz)
 * HSE: 8000000 Hz (8 MHz external crystal)
 *
 * PLL Configuration:
 *   VCO input: HSE / M = 8000000 Hz / 8 = 1000000 Hz
 *   VCO:       1000000 Hz × 336 = 336000000 Hz
 *   SYSCLK:    VCO / P = 336000000 Hz / 2 = 168000000 Hz
 *   USB:       VCO / Q = 336000000 Hz / 7 = 48000000 Hz
 */
struct ClockConfig {
    static constexpr uint32_t hse_hz = 8000000;
    static constexpr uint32_t system_clock_hz = 168000000;
    static constexpr uint32_t pll_m = 8;
    static constexpr uint32_t pll_n = 336;
    // ...
};

// LED Configuration
struct LedConfig {
    using led_green = GpioPin<peripherals::GPIOD, 12>;
    static constexpr bool led_green_active_high = true;
    // ...
};

// ... more configurations
}
```

## Step 6: Implement Board Initialization

Create `boards/my_custom_board/board.cpp`:

```cpp
#include "board.hpp"
#include "hal/vendors/st/stm32f4/rcc.hpp"
#include "hal/vendors/st/stm32f4/systick_platform.hpp"

namespace board {

using namespace my_custom_board;
using namespace ucore::hal::st::stm32f4;

void init() {
    // 1. Configure system clock
    ClockConfig clock_config{
        .hse_hz = ClockConfig::hse_hz,
        .system_clock_hz = ClockConfig::system_clock_hz,
        .pll_m = ClockConfig::pll_m,
        .pll_n = ClockConfig::pll_n,
        .pll_p_div = ClockConfig::pll_p_div,
        .pll_q = ClockConfig::pll_q,
        .ahb_prescaler = ClockConfig::ahb_prescaler,
        .apb1_prescaler = ClockConfig::apb1_prescaler,
        .apb2_prescaler = ClockConfig::apb2_prescaler,
        .flash_latency = ClockConfig::flash_latency
    };

    auto clock_result = Clock::initialize(clock_config);
    if (!clock_result.is_ok()) {
        // Clock initialization failed - handle error
        while (true);
    }

    // 2. Enable GPIO peripheral clocks
    RCC::enable_gpioa_clock();
    RCC::enable_gpiod_clock();

    // 3. Initialize SysTick for delays
    SysTick<ClockConfig::system_clock_hz>::init_ms(1);

    // 4. Initialize LED
    led::init();

    // 5. Enable interrupts
    __enable_irq();
}

namespace led {
    using Led = LedConfig::led_green;

    void init() {
        Led::configure_output();
        off();  // Start with LED off
    }

    void on() {
        if (LedConfig::led_green_active_high) {
            Led::set_high();
        } else {
            Led::set_low();
        }
    }

    void off() {
        if (LedConfig::led_green_active_high) {
            Led::set_low();
        } else {
            Led::set_high();
        }
    }

    void toggle() {
        Led::toggle();
    }
}

void delay_ms(uint32_t ms) {
    SysTick<ClockConfig::system_clock_hz>::delay_ms(ms);
}

} // namespace board
```

Create `boards/my_custom_board/board.hpp`:

```cpp
#pragma once

#include "board_config.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/vendors/st/stm32f4/systick_platform.hpp"

namespace board {

using namespace my_custom_board;
using namespace ucore::hal;
using namespace ucore::hal::st::stm32f4;

// Board-specific SysTick
using BoardSysTick = SysTick<ClockConfig::system_clock_hz>;

/**
 * @brief Initialize all board hardware
 */
void init();

/**
 * @brief Delay in milliseconds
 */
void delay_ms(uint32_t ms);

/**
 * @brief LED control
 */
namespace led {
    void init();
    void on();
    void off();
    void toggle();
}

} // namespace board
```

## Step 7: Test Your Board

### Build Blink Example

```bash
./ucore build my_custom_board blink
```

### Flash to Board

```bash
./ucore flash my_custom_board blink
```

### Verify

The LED should blink at 1 Hz (on for 500ms, off for 500ms).

## Advanced Configuration

### Multiple LEDs

```yaml
leds:
  - name: led_red
    color: red
    port: GPIOD
    pin: 14
    active_high: true

  - name: led_blue
    color: blue
    port: GPIOD
    pin: 15
    active_high: true
```

Access in code:
```cpp
using RedLed = LedConfig::led_red;
using BlueLed = LedConfig::led_blue;
```

### UART Configuration

```yaml
uart:
  - name: debug
    instance: USART1
    tx_port: GPIOA
    tx_pin: 9
    rx_port: GPIOA
    rx_pin: 10
    baud_rate: 115200
```

### SPI and I2C

See the complete YAML example above for SPI and I2C configuration.

## CMake Integration

MicroCore automatically includes your board if it has a `board.yaml` file.

To build manually:

```bash
cmake -B build-my_custom_board \
  -DMICROCORE_BOARD=my_custom_board \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build-my_custom_board
```

## Troubleshooting

### Clock Not Working

**Symptom:** Board doesn't respond, LED doesn't blink

**Causes:**
1. Incorrect PLL parameters
2. Wrong HSE frequency
3. Flash latency too low

**Solution:** Double-check datasheet and recalculate PLL parameters.

### LED Not Blinking

**Symptom:** Code runs but LED doesn't light up

**Causes:**
1. Wrong GPIO port/pin
2. Incorrect active_high setting
3. GPIO clock not enabled

**Solution:**
1. Verify pin mapping from schematic
2. Try toggling active_high
3. Check `RCC::enable_gpioX_clock()` called in `board::init()`

### Build Fails

**Error:** `undefined reference to board::init`

**Solution:** Make sure `board.cpp` is compiled. Check `boards/my_custom_board/CMakeLists.txt` exists.

## Best Practices

1. **Use wizard first** - It generates a valid template
2. **Document everything** - Add descriptions to YAML
3. **Validate early** - Run `validate-board` after YAML changes
4. **Test incrementally** - Start with just LED, then add more peripherals
5. **Follow naming conventions** - Use snake_case for identifiers
6. **Calculate carefully** - Verify PLL math before testing

## Next Steps

- @ref porting_platform - Port to a completely new MCU platform
- @ref concepts - Learn about C++20 concepts used in MicroCore
- @ref examples - Browse example applications

## See Also

- [Board Configuration Guide](../BOARD_CONFIGURATION.md) - Complete YAML reference
- [Host Platform Testing](../HOST_PLATFORM_TESTING.md) - Test without hardware
