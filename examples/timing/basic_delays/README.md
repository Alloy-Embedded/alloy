# Basic Delays - SysTick Timing Example

Demonstrates accurate millisecond and microsecond delays using the SysTick timer across all supported boards.

## Overview

This example showcases the fundamental timing capabilities of the Alloy HAL framework:
- **Millisecond delays** - Human-visible timing (LED blink patterns)
- **Microsecond delays** - High-precision timing for protocols and sensors
- **Timing validation** - Measuring actual delay accuracy
- **Mixed delay patterns** - Combining ms/us delays for complex timing

## Hardware Setup

### Required
- Any supported development board (see below)
- USB cable for programming and power

### Optional
- Logic analyzer or oscilloscope to measure timing accuracy
- Connect probe to LED pin to verify timing precision

## Supported Boards

| Board | MCU | Clock | LED Pin | Notes |
|-------|-----|-------|---------|-------|
| Nucleo-F401RE | STM32F401RE | 84 MHz | PA5 | Cortex-M4F |
| Nucleo-F722ZE | STM32F722ZE | 180 MHz | PB7 | Cortex-M7 |
| Nucleo-G071RB | STM32G071RB | 64 MHz | PA5 | Cortex-M0+ |
| Nucleo-G0B1RE | STM32G0B1RE | 64 MHz | PA5 | Cortex-M0+ |
| SAME70-Xplained | ATSAME70Q21B | 300 MHz | PC8 | Cortex-M7 |

## Build and Flash

### Build for your board
```bash
# From repository root
make nucleo-f401re-basic-delays-build
make nucleo-f722ze-basic-delays-build
make nucleo-g071rb-basic-delays-build
make nucleo-g0b1re-basic-delays-build
make same70-xplained-basic-delays-build
```

### Flash to board
```bash
make nucleo-f401re-basic-delays-flash
make nucleo-f722ze-basic-delays-flash
make nucleo-g071rb-basic-delays-flash
make nucleo-g0b1re-basic-delays-flash
make same70-xplained-basic-delays-flash
```

### Clean build
```bash
make nucleo-f401re-basic-delays-clean
```

## Expected Behavior

The LED will execute a sequence of timing patterns in a continuous loop:

### Pattern 1: Millisecond Delays (4 seconds)
- **1000ms ON** → **1000ms OFF** (1 Hz blink, human-visible)
- **500ms ON** → **500ms OFF** (2 Hz blink, typical LED rate)
- **100ms pulses × 5** (10 Hz blink, rapid flash)

### Pattern 2: Microsecond Delays (visible as rapid flicker)
- **1000us (1ms) pulses** - Human-visible blink
- **500us pulses × 10** - Visible flicker
- **100us pulses × 100** - Too fast to see, but measurable

### Pattern 3: Timing Validation
- Measures 1000ms delay accuracy using `millis()`
- Expected accuracy: ±1% (1000ms ±10ms)

### Pattern 4: Mixed Delays
- **50us pulse** → **500ms pause** (short pulse with long gap)
- **100us ON** → **100us OFF** × 5 (rapid burst)

**Total cycle time**: ~15 seconds, then repeats

## Timing Accuracy

### Millisecond Delays (`delay_ms`)
- **Expected**: ±1% accuracy (e.g., 1000ms ±10ms)
- **Limited by**: SysTick tick rate (1ms), interrupt latency
- **Best for**: LED control, human interface, timeouts > 10ms

### Microsecond Delays (`delay_us`)
- **Expected**: ±5% accuracy (e.g., 1000us ±50us)
- **Limited by**: CPU clock accuracy, busy-wait precision
- **Best for**: Sensor protocols (I2C, SPI), short pulses

### Factors Affecting Accuracy
1. **Clock source stability** - HSI vs HSE vs crystal
2. **Interrupt latency** - Other ISRs can delay SysTick
3. **Temperature drift** - Crystal frequency changes with temp
4. **Busy-wait overhead** - Loop counter increment time

## Measuring with Logic Analyzer

To verify timing accuracy:

1. Connect logic analyzer to LED pin
2. Trigger on LED pin edge
3. Measure pulse width and period
4. Compare with expected values

### Example Measurements (Nucleo-F401RE)
```
Pattern               Expected    Measured    Error
---------------------------------------------------
1000ms ON             1000.0ms    1000.2ms    +0.02%
500ms ON              500.0ms     499.8ms     -0.04%
100ms pulse           100.0ms     100.1ms     +0.10%
1000us pulse          1.000ms     1.003ms     +0.30%
500us pulse           0.500ms     0.502ms     +0.40%
100us pulse           0.100ms     0.105ms     +5.00%
```

## Code Explanation

### Delay Functions

```cpp
// Blocking millisecond delay (1ms resolution)
SysTickTimer::delay_ms<board::BoardSysTick>(1000);  // 1 second

// Blocking microsecond delay (1us resolution)
SysTickTimer::delay_us<board::BoardSysTick>(500);   // 500 microseconds
```

### Time Measurement

```cpp
// Get current time in milliseconds
u32 start = SysTickTimer::millis<board::BoardSysTick>();

// ... do something ...

// Calculate elapsed time
u32 elapsed = SysTickTimer::millis<board::BoardSysTick>() - start;
```

### Board-Portable Code

The example uses `board::BoardSysTick` type alias, which is defined per-board:

```cpp
// In board.hpp
using BoardSysTick = SysTick<84000000>;  // F401RE @ 84 MHz
using BoardSysTick = SysTick<180000000>; // F722ZE @ 180 MHz
using BoardSysTick = SysTick<64000000>;  // G071RB @ 64 MHz
```

This allows the same code to work on all boards without modification!

## Common Issues

### LED Not Blinking
- **Check**: Board initialized? `board::init()` called?
- **Check**: Correct board selected in build?
- **Check**: SysTick configured? (done in `board::init()`)

### Timing Inaccurate
- **Check**: Using stable clock source (HSE vs HSI)
- **Check**: No other high-priority interrupts blocking SysTick
- **Check**: CPU running at expected frequency

### Microsecond Delays Not Working
- **Check**: CPU clock fast enough (>1 MHz)
- **Check**: No integer overflow in delay calculation
- **Note**: Very short delays (<10us) may be inaccurate

## Learning Objectives

After working with this example, you should understand:

1. ✅ How to use `delay_ms()` and `delay_us()` for blocking delays
2. ✅ How to measure elapsed time using `millis()` and `micros()`
3. ✅ Expected accuracy and limitations of SysTick timing
4. ✅ How to write board-portable timing code
5. ✅ The difference between millisecond and microsecond precision

## Next Steps

- **Timeout Patterns**: Learn non-blocking timeouts for responsive code
- **Performance Measurement**: Measure function execution time
- **SysTick Demo**: Explore different tick rates and RTOS integration

## References

- **HAL API**: `src/hal/api/systick_simple.hpp`
- **Platform Layer**: `src/hal/platform/*/systick_platform.hpp`
- **Board Support**: `boards/*/board.cpp` (SysTick_Handler)
- **ARM Documentation**: ARM Cortex-M SysTick Timer
