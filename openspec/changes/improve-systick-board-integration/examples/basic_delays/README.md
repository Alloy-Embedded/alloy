# Basic Delays Example

## Overview

This example demonstrates accurate millisecond and microsecond delays using the SysTick timer. It validates timing accuracy and showcases portable timing code that works across all boards.

## Features

- ✅ Millisecond delays (1ms to 1000ms)
- ✅ Microsecond delays (100us to 5000us)
- ✅ Timing accuracy validation (±1% target)
- ✅ Continuous LED blinking with accuracy monitoring
- ✅ Works on all ARM Cortex-M boards

## Hardware Requirements

- Any supported board:
  - Nucleo-F401RE (84 MHz)
  - Nucleo-F722ZE (216 MHz)
  - Nucleo-G071RB (64 MHz)
  - Nucleo-G0B1RE (64 MHz)
  - SAME70-Xplained (300 MHz)
- On-board LED
- USB connection for console output (optional)

## Expected Behavior

### Console Output

```
╔══════════════════════════════════════════════════════╗
║  Basic Delays Example - SysTick Timing Validation   ║
╚══════════════════════════════════════════════════════╝

Board: Nucleo-F401RE
System Clock: 84 MHz
SysTick Resolution: 1 ms

--- Testing 1 ms delay ---
  Expected: 1 ms (1000 us)
  Actual:   1 ms (1002 us)
  Error:    0.20%
  ✓ Timing within ±1% (PASS)

--- Testing 10 ms delay ---
  Expected: 10 ms (10000 us)
  Actual:   10 ms (10005 us)
  Error:    0.05%
  ✓ Timing within ±1% (PASS)

... (more delay tests)

--- Testing 100 us delay ---
  Toggling LED 10 times...
  Expected: 1000 us (10 toggles × 100 us)
  Actual:   1003 us
  Error:    0.30%
  ✓ Timing within ±2% (PASS)

--- Continuous Blink Test (500ms period) ---
LED should toggle every 500ms. Press reset to restart.

Toggles: 10, Elapsed: 5000234 us, Error: 0.005%
Toggles: 20, Elapsed: 10000467 us, Error: 0.005%
...
```

### LED Behavior

1. **Delay Tests (first ~10 seconds)**:
   - LED toggles rapidly during microsecond tests
   - LED toggles slowly during millisecond tests

2. **Continuous Blink**:
   - LED toggles every 500ms
   - Consistent, visible blinking
   - Timing accuracy printed every 5 seconds

## Building

### Configure for your board

```bash
cmake -B build -DALLOY_BOARD=nucleo_f401re
```

### Build

```bash
cmake --build build --target basic_delays
```

### Flash

```bash
cmake --build build --target flash_basic_delays
```

## Timing Accuracy

### Expected Results

| Board | Clock | 100ms Delay | 1000ms Delay | Notes |
|-------|-------|-------------|--------------|-------|
| F401RE | 84 MHz | ±0.5% | ±0.2% | Typical |
| F722ZE | 216 MHz | ±0.3% | ±0.1% | Excellent |
| G071RB | 64 MHz | ±0.8% | ±0.3% | Good |
| G0B1RE | 64 MHz | ±0.8% | ±0.3% | Good |
| SAME70 | 300 MHz | ±0.2% | ±0.1% | Excellent |

### Factors Affecting Accuracy

1. **Clock Source Accuracy**: Crystal oscillator (typically ±50 ppm)
2. **Interrupt Latency**: ~1-10 µs on Cortex-M
3. **Temperature Drift**: Minimal for short-term measurements
4. **Load**: Timing accuracy independent of CPU load

## Troubleshooting

### Timing is off by >1%

**Possible causes**:
- Clock configuration incorrect
- SysTick not initialized
- Interrupt priority too low (starved by other ISRs)

**Solutions**:
1. Verify `board::init()` was called
2. Check clock configuration in `board_config.hpp`
3. Ensure SysTick interrupt priority is reasonable

### LED doesn't blink

**Possible causes**:
- LED pin configuration incorrect
- GPIO clocks not enabled
- LED polarity inverted

**Solutions**:
1. Check LED configuration in `board_config.hpp`
2. Verify `led_green_active_high` setting
3. Test with `board::led::on()` and `board::led::off()`

### No console output

**Possible causes**:
- UART not configured
- Baud rate mismatch
- No ITM/SWO trace configured

**Solutions**:
1. This example doesn't require console (LED-only mode works)
2. Console output is optional for debugging
3. Check UART configuration if printf() is expected

## Code Walkthrough

### Key Functions

#### `demonstrate_ms_delay(u32 delay_ms)`
```cpp
// Captures start time
u64 start_us = SysTickTimer::micros<board::BoardSysTick>();

// Performs delay
SysTickTimer::delay_ms<board::BoardSysTick>(delay_ms);

// Measures actual duration
u64 end_us = SysTickTimer::micros<board::BoardSysTick>();
u64 actual_us = end_us - start_us;

// Calculates accuracy
float error_percent = ((actual_us - expected_us) / expected_us) * 100.0f;
```

**Key points**:
- Uses `micros()` for high-resolution measurement
- Calculates percentage error
- Works across all boards (compile-time polymorphism)

#### `demonstrate_us_delay(u32 delay_us)`
```cpp
// Toggle LED 10 times with microsecond precision
for (int i = 0; i < 10; i++) {
    board::led::toggle();
    SysTickTimer::delay_us<board::BoardSysTick>(delay_us);
}
```

**Key points**:
- Shows microsecond-precision delays
- Validates with repeated operations
- LED toggle provides visual feedback

## Learning Objectives

After running this example, you should understand:

1. ✅ How to use `delay_ms()` and `delay_us()` for accurate timing
2. ✅ How to measure elapsed time with `micros()` and `millis()`
3. ✅ How to validate timing accuracy in your code
4. ✅ Trade-offs between millisecond and microsecond resolution
5. ✅ That timing code is portable across boards (no #ifdefs needed!)

## Next Steps

- **timeout_patterns**: Learn non-blocking timeout patterns
- **performance**: Measure function execution time
- **systick_demo**: Understand SysTick tick rates and RTOS integration

## References

- [SysTick Timer HAL Specification](../../specs/board-systick/spec.md)
- [ARM Cortex-M SysTick Documentation](https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-timer--systick)
