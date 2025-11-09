# SAME70 Xplained - Simple LED Blink Example

Simple LED blink example demonstrating GPIO control using the Alloy HAL for SAME70.

## Description

This is the **simplest possible example** for the SAME70 Xplained Ultra board. It demonstrates:

- Direct HAL GPIO usage with zero-overhead templates
- LED control on PIOC pin 8 (LED0)
- Basic busy-wait delay implementation
- Minimal startup code and linker script integration

The example blinks **LED0** (green LED) at approximately 1 Hz rate (ON for 500ms, OFF for 500ms).

## Hardware Requirements

- **Atmel SAME70 Xplained Ultra** evaluation board
- USB cable (for power and programming via EDBG)
- No external components needed

## LED Information

- **LED0** (Green): Connected to pin PC8 (PIOC, pin 8), active-low
- This example uses only LED0

## Building

### Quick Start (Using Makefile)

```bash
# Build the example
make same70-build

# Build and flash to board
make same70-flash
```

### Manual Build Steps

1. **Install Prerequisites**:
   ```bash
   # ARM GCC toolchain (xPack recommended)
   ./scripts/install-xpack-toolchain.sh

   # Or via Homebrew
   brew install arm-none-eabi-gcc

   # Build tools
   brew install cmake ninja openocd
   ```

2. **Configure and Build**:
   ```bash
   cmake -B build-same70 -S . \
       -G Ninja \
       -DCMAKE_BUILD_TYPE=Release \
       -DALLOY_BOARD=same70_xpld \
       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
       -DLINKER_SCRIPT=$(pwd)/boards/same70_xplained/ATSAME70Q21.ld

   cmake --build build-same70 --target same70_xplained_blink
   ```

3. **Build Output**:
   ```
   build-same70/examples/same70_xplained_blink/
   ├── same70_xplained_blink       (ELF executable)
   ├── same70_xplained_blink.hex   (Intel HEX format)
   └── same70_xplained_blink.bin   (Raw binary)
   ```

## Flashing

### Using Makefile (Recommended)

```bash
make same70-flash
```

### Using OpenOCD Directly

```bash
openocd -f board/atmel_same70_xplained.cfg \
    -c "program build-same70/examples/same70_xplained_blink/same70_xplained_blink verify reset exit"
```

### Using Atmel Studio / Microchip Studio

1. Open Atmel Studio
2. Tools → Device Programming
3. Select Tool: EDBG, Device: ATSAME70Q21
4. Flash the `.hex` or `.elf` file

## Expected Behavior

After flashing:

- **LED0** (green) blinks at ~1 Hz
  - ON for ~500ms
  - OFF for ~500ms
  - Repeats indefinitely

> **Note**: Timing is approximate as it uses busy-wait loops. Actual timing depends on clock configuration and compiler optimizations.

## Code Walkthrough

```cpp
#include "hal/platform/same70/gpio.hpp"
#include "hal/vendors/atmel/same70/atsame70q21/peripherals.hpp"

// Simple busy-wait delay (approximate timing)
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; ++i) {
        volatile uint32_t j = 0;
        while (j < 75000) {
            j = j + 1;
        }
    }
}

int main() {
    using namespace alloy::hal::same70;
    using namespace alloy::generated::atsame70q21;

    // LED0 on PIOC pin 8 (active low)
    constexpr uint32_t PIOC_BASE = peripherals::PIOC;
    constexpr uint8_t LED0_PIN = 8;

    GpioPin<PIOC_BASE, LED0_PIN> led0;

    // Initialize LED as OFF (set pin high for active-low)
    led0.set();

    // Main loop: blink LED0
    while (true) {
        led0.clear();    // Turn LED ON (active low)
        delay_ms(500);

        led0.set();      // Turn LED OFF (active low)
        delay_ms(500);
    }
}
```

### Key Points:

1. **`GpioPin<PORT_BASE, PIN_NUM>`** - Template-based GPIO with zero overhead
2. **`led0.clear()`** - Sets pin LOW (turns LED ON for active-low)
3. **`led0.set()`** - Sets pin HIGH (turns LED OFF for active-low)
4. **`delay_ms()`** - Simple busy-wait delay (not accurate, for demo only)

## Memory Usage

Actual memory footprint from build:

```
Memory region         Used Size  Region Size  %age Used
           FLASH:         836 B         2 MB      0.04%
             RAM:       16416 B       384 KB      4.17%

   text	   data	    bss	    dec	    hex	filename
    828	      8	  16416	  17252	   4364	same70_xplained_blink
```

- **Code (text)**: 828 bytes
- **Data**: 8 bytes
- **BSS (stack)**: 16,416 bytes (~16 KB)
- **Total**: ~17 KB

Extremely efficient! Uses less than 0.5% of available memory.

## Customization

### Change Blink Speed

Modify the delay values in `main.cpp`:

```cpp
// Fast blink (~4 Hz)
delay_ms(125);

// Slow blink (~0.5 Hz)
delay_ms(1000);
```

### Use Different GPIO Pin

```cpp
// Example: Use LED1 on PC9 instead
constexpr uint8_t LED1_PIN = 9;
GpioPin<PIOC_BASE, LED1_PIN> led1;
```

### Add More LEDs

```cpp
GpioPin<PIOC_BASE, 8> led0;  // PC8
GpioPin<PIOC_BASE, 9> led1;  // PC9

while (true) {
    led0.clear();
    led1.set();
    delay_ms(500);

    led0.set();
    led1.clear();
    delay_ms(500);
}
```

## Troubleshooting

### Build Errors

**"arm-none-eabi-gcc not found"**
```bash
# Install xPack toolchain
./scripts/install-xpack-toolchain.sh

# Or via Homebrew
brew install arm-none-eabi-gcc
```

**"Ninja not found"**
```bash
brew install ninja
```

**CMake configuration errors**
- Ensure you're in the repository root directory
- Check that `LINKER_SCRIPT` path is correct
- Verify board is set to `same70_xpld`

### Flash Errors

**"OpenOCD not found"**
```bash
brew install openocd
```

**"No device found"**
- Connect USB cable to **EDBG port** (not TARGET)
- Ensure board power LED is on
- Try a different USB cable/port
- Check drivers (macOS: automatic, Linux: see udev rules)

**"Target voltage too low"**
- Board must be powered via USB
- Check EDBG USB connection
- Verify board is not damaged

### Runtime Issues

**No LED activity**
1. Verify flash was successful
2. Press RESET button on board
3. Check power LED is on
4. Try re-flashing: `make same70-clean same70-flash`

**Wrong blink rate**
- Delay timing is approximate (busy-wait loop)
- Clock may not be configured (example runs on default clock)
- For accurate timing, use hardware timers

## Technical Details

### Startup Sequence

1. **Reset_Handler** (from `startup.cpp`):
   - Copies `.data` section from Flash to RAM
   - Zeros `.bss` section
   - Calls `SystemInit()` (if defined)
   - Calls `main()`

2. **Linker Script** (`ATSAME70Q21.ld`):
   - Flash: 2 MB @ 0x00400000
   - RAM: 384 KB @ 0x20400000
   - Stack: 8 KB (default)

3. **GPIO Control**:
   - Direct register access via templates
   - Zero runtime overhead (fully inlined)
   - Compile-time port/pin validation

### Why Active-Low LEDs?

The SAME70 Xplained board uses **active-low** LEDs:
- **clear()** → Pin LOW → LED ON
- **set()** → Pin HIGH → LED OFF

This is a common design to reduce current through the MCU pins.

## Next Steps

After getting this example working:

1. **Add Clock Configuration** - Configure PLL for proper timing
2. **Add UART Output** - Debug messages over serial
3. **Use Hardware Timers** - Replace busy-wait with precise timing
4. **Add Button Input** - Interactive control
5. **Use Board Package** - Explore `boards/same70_xplained/board.hpp` (advanced)

## Related Files

- **Main Code**: `examples/same70_xplained_blink/main.cpp`
- **Startup**: `src/hal/vendors/atmel/same70/atsame70q21/startup.cpp`
- **Linker Script**: `boards/same70_xplained/ATSAME70Q21.ld`
- **HAL GPIO**: `src/hal/platform/same70/gpio.hpp`
- **Peripherals**: `src/hal/vendors/atmel/same70/atsame70q21/peripherals.hpp`

## Quick Reference

```bash
# Build from scratch
make same70-clean same70-build

# Build and flash
make same70-flash

# Clean only
make same70-clean

# Rebuild everything
make same70-rebuild
```

## License

Part of the Alloy Framework - MIT License

## Support

- [Main README](../../README.md)
- [SAME70 Quick Start Guide](../../SAME70_QUICK_START.md)
- [Alloy Framework Documentation](../../docs/)
- GitHub Issues: Report bugs and request features
