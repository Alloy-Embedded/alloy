# Hardware Validation Tests

Hardware validation tests run on actual embedded hardware to verify that the HAL implementations work correctly with real peripherals.

## Overview

These tests are **not** unit tests - they require actual hardware and manual verification. They validate:

- ✅ GPIO pin control (LED blinking)
- ✅ Clock frequency configuration
- ✅ Peripheral clock enables
- ✅ Board-specific features (buttons, multiple LEDs)
- ✅ Timing accuracy

## Available Tests

| Test | Description | Validation Method |
|------|-------------|-------------------|
| `hw_gpio_led_test` | GPIO + Clock basic functionality | Visual LED pattern |
| `hw_clock_validation` | Clock frequency and timing | Stopwatch timing |
| `hw_board_validation` | Board-specific features | Interactive button test |

## Prerequisites

### Hardware
- Supported Nucleo board:
  - Nucleo-G0B1RE
  - Nucleo-G071RB
  - Nucleo-F401RE
  - Nucleo-F722ZE
- USB cable for power/programming
- Optional: Stopwatch for timing tests

### Software
- ARM GCC toolchain
- OpenOCD or ST-Link tools for flashing
- CMake build system configured for embedded

## Building Hardware Tests

### 1. Configure for Target Board

```bash
# Example: Nucleo-G0B1RE
cmake -B build-hw \
  -DALLOY_BOARD=nucleo_g0b1re \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DALLOY_BUILD_TESTS=ON
```

### 2. Build Tests

```bash
cmake --build build-hw --target hw_gpio_led_test
cmake --build build-hw --target hw_clock_validation
cmake --build build-hw --target hw_board_validation
```

Binaries will be in `build-hw/tests/hardware/`:
- `hw_gpio_led_test.elf` / `.bin` / `.hex`
- `hw_clock_validation.elf` / `.bin` / `.hex`
- `hw_board_validation.elf` / `.bin` / `.hex`

## Flashing Tests

### Using OpenOCD

```bash
openocd -f interface/stlink.cfg \
        -f target/stm32g0x.cfg \
        -c "program build-hw/tests/hardware/hw_gpio_led_test.elf verify reset exit"
```

### Using ST-Link Utility

1. Open ST-Link Utility
2. Connect to board
3. Load `.hex` or `.bin` file
4. Program and verify

### Using STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD -w build-hw/tests/hardware/hw_gpio_led_test.elf -rst
```

## Running Tests

### Test 1: GPIO LED Blink (`hw_gpio_led_test`)

**Purpose**: Validate GPIO and Clock initialization

**Expected Behavior**:
- LED blinks in pattern: **3 quick blinks**, 1 second pause, repeat
- Blink rate: 100ms ON, 100ms OFF

**Pass Criteria**:
- ✅ LED blinks in correct pattern
- ✅ Pattern repeats consistently

**Failure Indicators**:
- ❌ Rapid continuous blinking → Assertion failure (check which step failed)
- ❌ No blinking → Clock or GPIO not working

**Debugging**:
```
Rapid blink = Assertion failed
- Check clock initialization
- Check GPIO clock enable
- Check pin configuration
```

---

### Test 2: Clock Frequency Validation (`hw_clock_validation`)

**Purpose**: Validate clock frequency and timing accuracy

**Expected Behavior**:
- LED blinks at **exactly 1 Hz** (1 second ON, 1 second OFF)
- Total period: 2 seconds

**Pass Criteria**:
- ✅ 30 blinks in 60 seconds (±3% tolerance = 29-31 blinks)
- ✅ Timing matches stopwatch
- ✅ No drift over 5 minutes

**Test Procedure**:
1. Flash test to board
2. Reset board and start stopwatch simultaneously
3. Count LED blinks for 60 seconds
4. Calculate frequency: `blinks / 60` should equal 0.5 Hz

**Failure Indicators**:
- ❌ Rapid blinking → Assertion failure
- ❌ Too fast/slow → Clock frequency misconfigured
- ❌ Drift over time → PLL instability

**Acceptable Results**:
| Blinks in 60s | Status | Notes |
|---------------|--------|-------|
| 29-31 | ✅ PASS | Within ±3% tolerance |
| 25-28 or 32-35 | ⚠️ Warning | Check clock config |
| <25 or >35 | ❌ FAIL | Major clock error |

---

### Test 3: Board Validation (`hw_board_validation`)

**Purpose**: Validate board-specific features

**Expected Behavior**:

**Phase 1 - LED Test (automatic)**:
- Each LED lights for 500ms sequentially
- Chase pattern (F722ZE) or blink pattern (other boards)

**Phase 2 - Button Test (interactive)**:
- Press blue user button → Green LED lights
- Release button → Green LED turns off

**Board-Specific Behavior**:

**Nucleo-G0B1RE / G071RB / F401RE**:
- 1 Green LED (PA5)
- 1 User Button (PC13)
- Button press → LED on

**Nucleo-F722ZE**:
- 3 LEDs (Green PB0, Blue PB7, Red PB14)
- 1 User Button (PC13)
- Sequential LED test
- Button press → Green + Blue LEDs on

**Pass Criteria**:
- ✅ All LEDs light during sequence
- ✅ Button press lights LED immediately
- ✅ Button release turns off LED
- ✅ No bouncing or flicker

**Failure Indicators**:
- ❌ Rapid blinking → Assertion failure
- ❌ LED doesn't light → GPIO pin error
- ❌ Button doesn't work → Input configuration error

## Test Results Documentation

Create a test report for each run:

```markdown
## Test Run: [Date]

### Board: Nucleo-G0B1RE
### Tester: [Name]

#### Test 1: GPIO LED Blink
- Status: ✅ PASS
- Notes: Pattern correct, timing good

#### Test 2: Clock Validation
- Status: ✅ PASS
- Blinks in 60s: 30
- Timing drift: <0.5%

#### Test 3: Board Validation
- Status: ✅ PASS
- LED sequence: OK
- Button response: Immediate
```

## Troubleshooting

### No LED Activity

1. Check power (USB connected)
2. Verify correct binary flashed
3. Check LED pin matches board config
4. Try different board

### Rapid Continuous Blinking

This indicates an **assertion failure**. Debug steps:

1. Connect debugger (GDB)
2. Check which `HW_ASSERT` failed
3. Common failures:
   - Clock init failed
   - GPIO clock enable failed
   - Pin configuration failed

### Timing Issues

1. Verify clock frequency in code matches hardware
2. Check crystal/oscillator is working
3. Review PLL configuration
4. Measure with oscilloscope if available

### Button Not Responding

1. Check pin mapping (PC13 on most Nucleo boards)
2. Verify pull-up configuration
3. Test with multimeter (should read 3.3V when not pressed)
4. Check for solder bridges or damaged button

## Automation (Future)

While these tests currently require manual verification, they can be automated with:

- Logic analyzer to verify timing
- Automated button pressing (servo/relay)
- Camera/light sensor for LED detection
- CI/CD pipeline with hardware-in-the-loop

## Notes

- These tests use **busy-wait delays** (not optimal for production)
- Tests are blocking (no RTOS required)
- Flash size: ~2-4 KB per test
- RAM usage: <100 bytes
- No dynamic allocation
- No printf/stdio (pure embedded)

## Adding New Hardware Tests

To add a new hardware test:

1. Create `hw_my_test.cpp` in `tests/hardware/`
2. Add to `CMakeLists.txt`:
   ```cmake
   add_hardware_test(hw_my_test
       BOARD ${ALLOY_BOARD}
       SOURCES hw_my_test.cpp
   )
   ```
3. Use platform-specific includes
4. Provide visual/measurable validation
5. Document expected behavior
6. Add to this README

## See Also

- [Unit Tests](../unit/README.md) - Host-based tests
- [Integration Tests](../integration/README.md) - System tests
- [Regression Tests](../regression/README.md) - Bug documentation
