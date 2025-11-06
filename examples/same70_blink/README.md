# ATSAME70 Xplained - LED Blink Example

Simple LED blink example for the ATSAME70 Xplained evaluation board.

## Hardware Requirements

- **Board:** ATSAME70 Xplained (ATSAME70-XPLD)
- **MCU:** ATSAME70Q21B
  - ARM Cortex-M7 @ 300MHz (max)
  - 2MB Flash
  - 384KB SRAM
  - Double-precision FPU
  - Instruction and Data Cache
- **LED:** User LED on PC8 (active LOW)

## Features Demonstrated

- ✅ **Cortex-M7 Initialization**
  - FPU enabled (double-precision)
  - I-Cache enabled (2-3x code speedup)
  - D-Cache enabled (2-3x data speedup)
- ✅ **Board Abstraction**
  - Clean board-level API
  - No direct register access in application
- ✅ **LED Control**
  - Simple on/off/toggle functions
  - Handles active-LOW LED automatically
- ✅ **Startup Code**
  - Modern C++20 startup framework
  - Proper .data/.bss initialization
  - C++ global constructor support

## Building

### Prerequisites

Install ARM GNU Toolchain:

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc

# Or download from ARM:
# https://developer.arm.com/downloads/-/gnu-rm
```

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure (Debug build)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Or for Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make

# Output files:
#   same70_blink.elf  - ELF executable
#   same70_blink.hex  - Intel HEX format
#   same70_blink.bin  - Raw binary
#   same70_blink.map  - Linker map file
```

## Flashing

### Using EDBG Debugger (built-in)

The ATSAME70 Xplained has an on-board EDBG debugger.

#### With OpenOCD:

```bash
# Flash using OpenOCD
openocd -f board/atmel_same70_xplained.cfg \
        -c "program same70_blink.elf verify reset exit"
```

#### With Atmel Studio / Microchip Studio:

1. Open Atmel Studio
2. Tools → Device Programming
3. Select EDBG debugger
4. Select ATSAME70Q21
5. Flash `same70_blink.hex`

#### With JLink:

```bash
# Flash using JLink
JLinkExe -device ATSAME70Q21 -if SWD -speed 4000
> loadbin same70_blink.bin 0x00400000
> r
> g
> q
```

## Expected Behavior

After flashing:

1. User LED (PC8) starts blinking
2. LED is ON for 500ms
3. LED is OFF for 500ms
4. Cycle repeats forever

## Debugging

### With OpenOCD + GDB:

```bash
# Terminal 1: Start OpenOCD
openocd -f board/atmel_same70_xplained.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb same70_blink.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

### With Atmel Studio:

1. Open project in Atmel Studio
2. Set breakpoints
3. Press F5 to debug

## Code Structure

```
same70_blink/
├── main.cpp              # Application code (LED blink)
├── CMakeLists.txt        # Build configuration
└── README.md             # This file

../../boards/atmel_same70_xpld/
├── board.hpp             # Board abstraction (LED, delays)
├── startup.cpp           # Startup code (reset handler, vectors)
└── ATSAME70Q21.ld        # Linker script (memory layout)

../../src/startup/
├── startup_common.hpp         # Common startup utilities
└── arm_cortex_m7/
    ├── cortex_m7_init.hpp    # Cortex-M7 initialization
    ├── fpu_m7.hpp            # FPU initialization
    └── cache_m7.hpp          # Cache initialization
```

## Memory Usage

Example memory usage (Release build with -O3):

```
Memory region         Used Size  Region Size  %age Used
           FLASH:        2048 B         2 MB      0.10%
             RAM:          16 B       384 KB      0.00%
```

The SAME70 has plenty of resources for much larger applications!

## Performance Notes

### With Cortex-M7 Features Enabled:

- **FPU:** 10-100x faster floating-point vs software emulation
- **I-Cache:** 2-3x faster code execution
- **D-Cache:** 2-3x faster data access
- **Overall:** 3-10x system performance improvement

### Clock Configuration:

Current: Using internal RC oscillator (4-12 MHz)
- Simple and works out of the box
- Not the fastest option

Future: Configure PLL for 300MHz:
- Requires PMC (Power Management Controller) HAL
- Will provide 25-75x speedup over RC oscillator
- Requires proper flash wait state configuration

## Next Steps

After this basic example works, try:

1. **Button Input** - Add button reading (SW0 on PA9)
2. **UART Debug** - Add printf over UART0 (debug console)
3. **Multiple LEDs** - Use external LEDs on GPIO
4. **Timer-based Blink** - Use TC (Timer Counter) instead of delay
5. **Interrupt-based Button** - Use GPIO interrupt for button press

## Troubleshooting

### LED doesn't blink:

1. Check board power (USB connected)
2. Verify flash was successful
3. Try reset button (RESET)
4. Check EDBG firmware is up to date

### Build fails:

1. Verify ARM toolchain is installed: `arm-none-eabi-gcc --version`
2. Check CMake version: `cmake --version` (need 3.20+)
3. Clean build: `rm -rf build && mkdir build`

### OpenOCD can't connect:

1. Check USB connection
2. Update EDBG firmware
3. Try different USB cable/port
4. Check permissions: `sudo usermod -a -G dialout $USER` (Linux)

## References

- [ATSAME70 Datasheet](https://www.microchip.com/en-us/product/ATSAME70Q21)
- [ATSAME70 Xplained User Guide](https://www.microchip.com/en-us/development-tool/ATSAME70-XPLD)
- [ARM Cortex-M7 Technical Reference Manual](https://developer.arm.com/documentation/ddi0489/latest/)
- [Alloy Framework Documentation](../../README.md)

## License

This example is part of the Alloy embedded framework.
