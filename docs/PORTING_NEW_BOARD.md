# Porting a New Board to Alloy

**Difficulty:** ⭐⭐☆☆☆ (Beginner-Friendly)
**Time Required:** 30-60 minutes
**Prerequisites:** Basic C++ knowledge, board schematic/datasheet

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Step-by-Step Guide](#step-by-step-guide)
4. [Example: Adding Nucleo-F401RE](#example-adding-nucleo-f401re)
5. [Testing Your Board](#testing-your-board)
6. [Troubleshooting](#troubleshooting)
7. [Best Practices](#best-practices)

---

## Overview

Adding a new board to Alloy is straightforward if the MCU platform is already supported. You'll create:

1. **Board directory** - Contains all board-specific files
2. **Board header** (`board.hpp`) - Pin definitions and board API
3. **Board source** (`board.cpp`) - Initialization code
4. **Linker script** (`linker.ld`) - Memory layout

**Example boards to reference:**
- `boards/nucleo_f401re/` (STM32F4, simple)
- `boards/nucleo_f722ze/` (STM32F7, complex)
- `boards/nucleo_g0b1re/` (STM32G0, minimal)

---

## Prerequisites

### Information You Need

1. **MCU Model** - Exact part number (e.g., STM32F401RET6)
2. **Memory Layout** - Flash size, RAM size, addresses
3. **Board Schematic** - Pin connections (LED, button, UART, etc.)
4. **Crystal Frequency** - HSE/XTAL frequency (e.g., 8 MHz)

### Example: Gathering Info for Nucleo-F401RE

```
MCU:         STM32F401RET6
Core:        ARM Cortex-M4F
Flash:       512 KB at 0x08000000
RAM:         96 KB at 0x20000000
HSE:         8 MHz crystal
LED:         PA5 (LD2, green)
Button:      PC13 (B1, blue)
UART:        USART2 (PA2=TX, PA3=RX) via ST-LINK
```

**Where to find this:**
- Board user manual (UM1724 for Nucleo-F401RE)
- MCU datasheet (DS9716 for STM32F401)
- Board schematic

---

## Step-by-Step Guide

### Step 1: Create Board Directory

```bash
cd boards/
mkdir my_board_name    # Use snake_case (e.g., nucleo_f401re)
cd my_board_name
```

**Naming convention:**
- Use lowercase with underscores: `nucleo_f401re`, `disco_f407vg`
- Be descriptive but concise
- Include board series if ambiguous: `arduino_zero`, `arduino_mega`

### Step 2: Create Linker Script

**File:** `boards/my_board/linker.ld`

```ld
/* Linker script for [BOARD NAME] */

/* Memory layout from datasheet */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K  /* Adjust for your MCU */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K   /* Adjust for your MCU */
}

/* Entry point */
ENTRY(Reset_Handler)

/* Stack and heap sizes */
_Min_Stack_Size = 0x400;  /* 1 KB stack */
_Min_Heap_Size  = 0x200;  /* 512 bytes heap */

SECTIONS
{
    /* Interrupt vector table at start of FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Code section */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* Static constructors (C++) */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } >FLASH

    /* Initialized data (copied from FLASH to RAM at startup) */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* Uninitialized data (zero-initialized at startup) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    /* Heap (grows upward) */
    .heap :
    {
        . = ALIGN(4);
        _sheap = .;
        . = . + _Min_Heap_Size;
        . = ALIGN(4);
        _eheap = .;
    } >RAM

    /* Stack (grows downward from end of RAM) */
    .stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
        _estack = .;
    } >RAM

    /* Set initial stack pointer to end of RAM */
    PROVIDE(_estack = ORIGIN(RAM) + LENGTH(RAM));
}
```

**Key points:**
- **MEMORY section**: Set FLASH and RAM addresses/sizes from datasheet
- **_estack**: Initial stack pointer (top of RAM)
- **Alignment**: Keep all `.` = ALIGN(4) for ARM Cortex-M

### Step 3: Create Board Header

**File:** `boards/my_board/board.hpp`

```cpp
/**
 * @file board.hpp
 * @brief Board support for [BOARD NAME]
 *
 * MCU:    [MCU MODEL]
 * Core:   [ARM CORE]
 * Flash:  [SIZE]
 * RAM:    [SIZE]
 * HSE:    [FREQUENCY] MHz
 */

#pragma once

#include "core/error.hpp"
#include "core/result.hpp"
#include "hal/vendors/st/stm32f4/clock_platform.hpp"  // Adjust for your platform
#include "hal/vendors/st/stm32f4/gpio.hpp"
#include "hal/vendors/st/stm32f4/systick_platform.hpp"
#include "hal/vendors/st/stm32f4/stm32f401/peripherals.hpp"  // MCU-specific

namespace alloy::board {

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::st::stm32f4;

// ============================================================================
// Clock Configuration
// ============================================================================

struct BoardClockConfig {
    static constexpr uint32_t hse_hz = 8'000'000;          // 8 MHz HSE
    static constexpr uint32_t system_clock_hz = 84'000'000; // 84 MHz system clock
    static constexpr uint32_t pll_m = 4;                    // HSE / 4 = 2 MHz
    static constexpr uint32_t pll_n = 168;                  // 2 MHz × 168 = 336 MHz VCO
    static constexpr uint32_t pll_p_div = 4;                // 336 MHz / 4 = 84 MHz
    static constexpr uint32_t pll_q = 7;                    // 336 MHz / 7 = 48 MHz (USB)
    static constexpr uint32_t flash_latency = 2;            // 2 wait states for 84 MHz
    static constexpr uint32_t ahb_prescaler = 1;            // AHB = 84 MHz
    static constexpr uint32_t apb1_prescaler = 2;           // APB1 = 42 MHz (max 45 MHz)
    static constexpr uint32_t apb2_prescaler = 1;           // APB2 = 84 MHz
};

using BoardClock = st::stm32f4::Stm32f4Clock<BoardClockConfig>;

// ============================================================================
// GPIO Pin Definitions
// ============================================================================

// LED (green, LD2 on Nucleo boards)
using LedGreen = GpioPin<peripherals::GPIOA, 5>;

// User button (blue, B1 on Nucleo boards)
using Button = GpioPin<peripherals::GPIOC, 13>;

// USART2 (ST-LINK virtual COM port)
// TX = PA2, RX = PA3

// ============================================================================
// Board API
// ============================================================================

/**
 * @brief Initialize board (clocks, GPIO, etc.)
 *
 * Sets up:
 * - System clock (84 MHz from 8 MHz HSE)
 * - GPIO clocks
 * - SysTick (1ms tick)
 */
Result<void, ErrorCode> init();

/**
 * @brief Delay for specified milliseconds
 *
 * Uses SysTick for timing.
 *
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms);

} // namespace alloy::board
```

**Key points:**
- **Clock Configuration**: Calculate PLL values for target frequency
  - Formula: `VCO = (HSE / pll_m) × pll_n`
  - System clock: `VCO / pll_p_div`
  - USB clock: `VCO / pll_q` (must be 48 MHz for USB)
- **GPIO Pins**: Define all important pins (LED, button, UART, etc.)
- **Namespace**: Always use `alloy::board::`

### Step 4: Create Board Source

**File:** `boards/my_board/board.cpp`

```cpp
#include "board.hpp"
#include "hal/vendors/st/stm32f4/systick_platform.hpp"

namespace alloy::board {

// SysTick counter (incremented every 1ms)
static volatile uint32_t systick_counter = 0;

Result<void, ErrorCode> init() {
    // 1. Initialize system clock
    auto clock_result = BoardClock::initialize();
    if (clock_result.is_err()) {
        return clock_result;
    }

    // 2. Enable GPIO clocks
    auto gpio_result = BoardClock::enable_gpio_clocks();
    if (gpio_result.is_err()) {
        return gpio_result;
    }

    // 3. Configure LED as output
    LedGreen led;
    led.setDirection(hal::PinDirection::Output);
    led.clear();  // LED off initially

    // 4. Configure button as input with pull-up
    Button button;
    button.setDirection(hal::PinDirection::Input);
    button.setPull(hal::PinPull::PullUp);

    // 5. Initialize SysTick (1ms tick)
    using SysTick = st::stm32f4::Stm32SysTick<BoardClockConfig::system_clock_hz>;
    SysTick::initialize(1000);  // 1 kHz = 1ms tick

    return Ok();
}

void delay_ms(uint32_t ms) {
    uint32_t start = systick_counter;
    while ((systick_counter - start) < ms) {
        __asm__ volatile("nop");
    }
}

} // namespace alloy::board

// ============================================================================
// Interrupt Handlers
// ============================================================================

extern "C" {

/**
 * @brief SysTick interrupt handler
 *
 * Called every 1ms by SysTick timer.
 */
void SysTick_Handler(void) {
    alloy::board::systick_counter++;
}

} // extern "C"
```

**Key points:**
- **Initialization order**: Clock → GPIO clocks → peripherals → SysTick
- **Error handling**: Propagate errors with `Result<T, E>`
- **SysTick**: Set up 1ms tick for delay_ms()
- **Interrupt handlers**: Must be `extern "C"`

### Step 5: Create CMakeLists.txt

**File:** `boards/my_board/CMakeLists.txt`

```cmake
# Board: [BOARD NAME]
# MCU:   [MCU MODEL]

# Set board name
set(ALLOY_BOARD "my_board_name")

# Platform will be auto-detected from board name
# You can override if needed:
# set(ALLOY_PLATFORM "stm32f4")

# Board-specific source files
set(BOARD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/board.cpp
)

# Board-specific include directories
set(BOARD_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
)

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/linker.ld)
```

### Step 6: Register Board in Platform Config

**File:** `cmake/platforms/stm32f4.cmake` (adjust for your platform)

Add your board to the board-to-platform mapping if needed:

```cmake
function(alloy_board_to_platform board_name out_platform)
    # Map board names to platforms
    if(board_name MATCHES "^nucleo_f4")
        set(${out_platform} "stm32f4" PARENT_SCOPE)
    elseif(board_name STREQUAL "my_board_name")  # <-- Add your board
        set(${out_platform} "stm32f4" PARENT_SCOPE)
    # ... more boards
    endif()
endfunction()
```

### Step 7: Build and Test

```bash
# Configure
cmake -B build-my-board \
    -DALLOY_BOARD=my_board_name \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
    -DALLOY_BUILD_TESTS=OFF

# Build
cmake --build build-my-board

# Flash (if you have OpenOCD configured)
cmake --build build-my-board --target flash
```

---

## Example: Adding Nucleo-F401RE

Let's walk through a real example: adding the Nucleo-F401RE board.

### Step 1: Gather Information

```
Board:       NUCLEO-F401RE (ST Microelectronics)
MCU:         STM32F401RET6
Core:        ARM Cortex-M4F @ 84 MHz
Flash:       512 KB at 0x08000000
RAM:         96 KB at 0x20000000
HSE:         8 MHz crystal (X2)
LED:         PA5 (LD2, green)
Button:      PC13 (B1, blue, active low)
UART:        USART2 on PA2/PA3 (ST-LINK VCP)
```

**References:**
- UM1724: Nucleo-64 board user manual
- DS9716: STM32F401xD/xE datasheet
- Schematic: MB1136-DEFAULT-C03

### Step 2: Calculate Clock Configuration

**Goal:** 84 MHz system clock from 8 MHz HSE

```
HSE = 8 MHz
VCO input = HSE / pll_m = 8 MHz / 4 = 2 MHz  (must be 1-2 MHz)
VCO = VCO input × pll_n = 2 MHz × 168 = 336 MHz  (must be 192-432 MHz)
System clock = VCO / pll_p_div = 336 MHz / 4 = 84 MHz  ✓
USB clock = VCO / pll_q = 336 MHz / 7 = 48 MHz  ✓ (required for USB)

APB1 (max 45 MHz) = 84 MHz / 2 = 42 MHz  ✓
APB2 (max 84 MHz) = 84 MHz / 1 = 84 MHz  ✓
```

**Configuration:**
```cpp
struct BoardClockConfig {
    static constexpr uint32_t hse_hz = 8'000'000;
    static constexpr uint32_t system_clock_hz = 84'000'000;
    static constexpr uint32_t pll_m = 4;       // ÷4
    static constexpr uint32_t pll_n = 168;     // ×168
    static constexpr uint32_t pll_p_div = 4;   // ÷4 (system)
    static constexpr uint32_t pll_q = 7;       // ÷7 (USB)
    static constexpr uint32_t flash_latency = 2;  // 2 wait states @ 84 MHz
    static constexpr uint32_t ahb_prescaler = 1;
    static constexpr uint32_t apb1_prescaler = 2;
    static constexpr uint32_t apb2_prescaler = 1;
};
```

### Step 3: Memory Layout (linker.ld)

```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}
```

### Step 4: Pin Definitions

From schematic MB1136:
```cpp
// LED2 (green) = PA5
using LedGreen = GpioPin<peripherals::GPIOA, 5>;

// Button B1 (blue) = PC13, active low
using Button = GpioPin<peripherals::GPIOC, 13>;

// USART2 (ST-LINK VCP)
// TX = PA2 (D1, Arduino connector)
// RX = PA3 (D0, Arduino connector)
```

### Step 5: Complete Files

See `boards/nucleo_f401re/` for the complete implementation!

---

## Testing Your Board

### Test 1: Build Blink Example

```bash
cmake -B build-my-board -DALLOY_BOARD=my_board_name
cmake --build build-my-board --target blink
```

**Expected output:**
```
[ 28%] Built target alloy-hal
[100%] Built target blink
Memory region         Used Size  Region Size  %age Used
           FLASH:        3088 B       512 KB      0.59%
             RAM:        8232 B        96 KB      8.37%
```

### Test 2: Flash and Run

```bash
# Flash with OpenOCD
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program build-my-board/examples/blink/blink.elf verify reset exit"
```

**Expected behavior:**
- LED should blink at 1 Hz (500ms on, 500ms off)

### Test 3: Serial Output (if UART configured)

```bash
# Connect to serial port
screen /dev/ttyACM0 115200

# Or with minicom
minicom -D /dev/ttyACM0 -b 115200
```

---

## Troubleshooting

### Problem: Build Fails with "undefined reference to `Reset_Handler`"

**Cause:** Missing startup code

**Solution:** Ensure your MCU has startup code in `src/hal/vendors/.../startup.cpp`

```bash
# Check if startup exists
ls src/hal/vendors/st/stm32f4/stm32f401/startup.cpp
```

If missing, see [PORTING_NEW_PLATFORM.md](PORTING_NEW_PLATFORM.md) for generating startup code.

### Problem: LED Doesn't Blink

**Checklist:**
1. ✓ Is GPIO clock enabled? (Check `board.cpp` calls `enable_gpio_clocks()`)
2. ✓ Is LED pin correct? (Check schematic)
3. ✓ Is pin configured as output? (Check `setDirection(PinDirection::Output)`)
4. ✓ Is delay working? (Check SysTick initialization)

**Debug:** Add UART output to verify code is running:
```cpp
// In board.cpp
uart_write("Board initialized\n");
```

### Problem: Linker Error "section `.text' will not fit in region `FLASH'"

**Cause:** Code too large for available flash

**Solutions:**
1. Increase FLASH size in linker script if you have more memory
2. Enable optimization: `-DCMAKE_BUILD_TYPE=Release`
3. Remove unused code

### Problem: Hard Fault on Startup

**Common causes:**
1. **Invalid stack pointer** - Check `_estack` in linker script points to valid RAM
2. **Clock not configured** - Ensure `BoardClock::initialize()` succeeds
3. **Wrong linker script** - Verify FLASH/RAM addresses match datasheet

**Debug:**
```cpp
// Add at start of main()
while (1) {
    led.toggle();
    for (volatile int i = 0; i < 1000000; i++);  // Simple delay
}
```

If this works, problem is in `board::init()`.

---

## Best Practices

### 1. Use Descriptive Names

```cpp
// ✅ Good
using UartDebug = /* ... */;
using LedStatus = /* ... */;
using ButtonUser = /* ... */;

// ❌ Bad
using Pin1 = /* ... */;
using Led = /* ... */;
using Btn = /* ... */;
```

### 2. Document Pin Mappings

```cpp
// ✅ Good - Include schematic reference
// LED2 (green) - LD2 on schematic, PA5
using LedGreen = GpioPin<peripherals::GPIOA, 5>;

// USART2_TX - Arduino D1, CN9 pin 1, PA2
// USART2_RX - Arduino D0, CN9 pin 2, PA3
```

### 3. Validate Clock Configuration

Add static assertions:
```cpp
// Ensure VCO frequency is in valid range (192-432 MHz for STM32F4)
static constexpr uint32_t vco_freq =
    (BoardClockConfig::hse_hz / BoardClockConfig::pll_m) * BoardClockConfig::pll_n;
static_assert(vco_freq >= 192'000'000 && vco_freq <= 432'000'000,
              "VCO frequency out of range");

// Ensure APB1 doesn't exceed max frequency
static constexpr uint32_t apb1_freq =
    BoardClockConfig::system_clock_hz / BoardClockConfig::apb1_prescaler;
static_assert(apb1_freq <= 45'000'000,
              "APB1 frequency exceeds 45 MHz maximum");
```

### 4. Provide Multiple Clock Configurations

```cpp
// High-performance: 84 MHz
struct HighPerformanceClockConfig { /* ... */ };

// Low-power: 16 MHz (HSI, no PLL)
struct LowPowerClockConfig { /* ... */ };

// USB-enabled: Ensure 48 MHz USB clock
struct UsbClockConfig { /* ... */ };

// Default
using BoardClockConfig = HighPerformanceClockConfig;
```

### 5. Include README

**File:** `boards/my_board/README.md`

```markdown
# [BOARD NAME]

## Board Information
- MCU: [MODEL]
- Core: [ARM CORE]
- Frequency: [MAX MHz]
- Flash: [SIZE]
- RAM: [SIZE]

## Pin Mapping
| Function | Pin | Arduino | Notes |
|----------|-----|---------|-------|
| LED      | PA5 | D13     | Green |
| Button   | PC13| -       | Blue  |
| UART TX  | PA2 | D1      | USART2|
| UART RX  | PA3 | D0      | USART2|

## Building
\`\`\`bash
cmake -B build -DALLOY_BOARD=my_board_name
cmake --build build
\`\`\`

## Flashing
\`\`\`bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\
    -c "program build/examples/blink/blink.elf verify reset exit"
\`\`\`
```

---

## Next Steps

- ✅ Board working? Share it! Create a pull request
- 📖 Want to add a new MCU family? See [PORTING_NEW_PLATFORM.md](PORTING_NEW_PLATFORM.md)
- 🔧 Need to generate startup code? See [CODE_GENERATION.md](CODE_GENERATION.md)

---

**Questions?** Open an issue on GitHub or check the [troubleshooting guide](#troubleshooting).
