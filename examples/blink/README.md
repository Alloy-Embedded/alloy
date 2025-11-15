# Universal Blink LED Example

## Overview

This example demonstrates the **power of the Alloy HAL abstraction** - a single source file that compiles and runs on multiple microcontroller architectures without modification.

## Supported Boards

| Board | MCU | Architecture | LED Pin | Clock |
|-------|-----|--------------|---------|-------|
| **SAME70 Xplained** | ATSAME70Q21 | ARM Cortex-M7 | PC8 (green) | 12 MHz (HSI) |
| **Nucleo-G0B1RE** | STM32G0B1RET6 | ARM Cortex-M0+ | PA5 (green) | 64 MHz (PLL) |

## How It Works

The example uses **conditional compilation** to include the correct board header:

```cpp
#if defined(ALLOY_BOARD_SAME70_XPLAINED)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#endif
```

The build system automatically defines the correct `ALLOY_BOARD_*` macro based on the selected board.

### Application Code

The application code is **100% portable**:

```cpp
int main() {
    board::init();              // Initialize hardware

    while (true) {
        board::led::on();       // Turn LED on
        SysTickTimer::delay_ms<board::BoardSysTick>(500);

        board::led::off();      // Turn LED off
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
```

**Same code, multiple architectures!**

## Building

### For SAME70 Xplained

```bash
make same70-blink-build
```

### For Nucleo-G0B1RE

```bash
make nucleo-g0b1re-blink-build
```

## Flashing

### SAME70 Xplained

```bash
make same70-blink-flash
```

### Nucleo-G0B1RE

```bash
make nucleo-g0b1re-blink-flash
```

## Expected Behavior

The green onboard LED blinks at 1 Hz (500ms ON, 500ms OFF) on both boards.

## Binary Size Comparison

| Board | Flash Used | RAM Used | Total Binary |
|-------|-----------|----------|--------------|
| SAME70 Xplained | ~900 bytes | ~200 bytes | ~1.1 KB |
| Nucleo-G0B1RE | 1028 bytes | 8232 bytes | ~9.3 KB |

**Note**: STM32G0 uses more RAM due to 8KB stack allocation.

## Adding New Boards

To add support for a new board:

1. **Create board support package** in `boards/<board_name>/`
   - `board.hpp` - Board API declaration
   - `board.cpp` - Hardware initialization
   - `board_config.hpp` - Hardware configuration

2. **Implement board API**:
   ```cpp
   namespace board {
       void init();           // Initialize hardware

       namespace led {
           void init();       // Initialize LED GPIO
           void on();         // Turn LED on
           void off();        // Turn LED off
       }

       using BoardSysTick = SysTick<CLOCK_HZ>;
   }
   ```

3. **Update build system**:
   - Add board to `CMakeLists.txt` examples section
   - Add `ALLOY_BOARD_*` conditional in `main.cpp`

## Architecture

```
examples/blink/main.cpp (portable application)
        ↓
    board API (board::init, board::led::*)
        ↓
boards/<board>/board.cpp (board-specific)
        ↓
src/hal/platform/<vendor>/ (platform layer)
        ↓
    Hardware registers
```

## Key Features

- **Zero runtime overhead** - Compile-time selection
- **Type-safe** - Platform types resolved at compile time
- **Clean abstraction** - Application code knows nothing about hardware
- **Easy to port** - Just add board support package

## Testing

Compile for both boards to verify portability:

```bash
# Build for SAME70
make same70-blink-build

# Build for STM32
make nucleo-g0b1re-blink-build
```

Both should compile without warnings or errors.

## License

Copyright (c) 2025 Alloy Project
