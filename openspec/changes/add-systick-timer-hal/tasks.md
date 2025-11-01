# Tasks: Add SysTick Timer HAL

## Phase 1: Interface & Foundation (Day 1)

### Task 1.1: Create SysTick Interface Header
**Estimated**: 2 hours
**Dependencies**: None
**Deliverable**: `src/hal/interface/systick.hpp`

Create the platform-agnostic SysTick interface with:
- [x] `SystemTick<T>` concept definition
- [x] `SysTickConfig` struct (reserved for future use)
- [x] Global namespace functions (`alloy::systick::*`)
- [x] Helper functions (`micros_since()`, `is_timeout()`)
- [x] Comprehensive documentation with examples
- [x] Static assertions for concept validation

**Validation**:
```bash
# Compiles without errors
g++ -std=c++20 -c src/hal/interface/systick.hpp -I src/
```

**Files**:
- New: `src/hal/interface/systick.hpp`

---

### Task 1.2: Add SysTick to Peripheral Enum
**Estimated**: 30 minutes
**Dependencies**: Task 1.1
**Deliverable**: Updated `src/hal/interface/clock.hpp`

Add SysTick to Peripheral enum for clock management:
- [x] Add `SysTick = 0x0210` to `enum class Peripheral`
- [x] Document that SysTick peripheral is for system timing
- [x] Update comments

**Validation**:
```bash
# Compiles without errors
g++ -std=c++20 -c src/hal/interface/clock.hpp -I src/
```

**Files**:
- Modified: `src/hal/interface/clock.hpp`

---

## Phase 2: ARM Implementations (Day 1-2)

### Task 2.1: Implement STM32F1 SysTick
**Estimated**: 3 hours
**Dependencies**: Task 1.1
**Deliverable**: `src/hal/st/stm32f1/systick.hpp`

Implement SysTick for STM32F1 (72MHz Cortex-M3):
- [x] Create header with class definition
- [x] Implement `init()` - configure SysTick at 1MHz tick
- [x] Implement `micros()` - return current time
- [x] Implement `reset()` - reset counter to 0
- [x] Implement `is_initialized()` - check init status
- [x] Add `SysTick_Handler()` ISR (increments counter)
- [x] Configure reload value based on clock frequency
- [x] Use prescaler DIV8 for cleaner math
- [x] Static assert concept compliance
- [x] Add documentation

**Implementation Notes**:
- SysTick at 72MHz/8 = 9MHz
- Reload value 9000 for 1ms interrupts
- Software counter in multiples of 1ms
- Sub-ms from `(9000 - SysTick->VAL) / 9`

**Validation**:
```bash
# Compile STM32F1 example
cmake -B build/test -DBOARD=bluepill -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build/test --target blink_stm32f103
```

**Files**:
- New: `src/hal/st/stm32f1/systick.hpp`

---

### Task 2.2: Integrate STM32F1 SysTick into Board
**Estimated**: 1 hour
**Dependencies**: Task 2.1
**Deliverable**: Updated `boards/stm32f103c8/board.hpp`

Add SysTick to STM32F1 board initialization:
- [x] Include `hal/st/stm32f1/systick.hpp`
- [x] Call `SystemTick::init()` in `Board::initialize()`
- [x] Place after clock init, before peripheral enables
- [x] Update documentation

**Validation**:
```bash
# Test that board initializes and time works
# Run blink example with time measurements
```

**Files**:
- Modified: `boards/stm32f103c8/board.hpp`

---

### Task 2.3: Implement STM32F4 SysTick
**Estimated**: 2 hours
**Dependencies**: Task 2.1 (can parallelize)
**Deliverable**: `src/hal/st/stm32f4/systick.hpp`

Implement SysTick for STM32F4 (168MHz Cortex-M4F):
- [x] Create header with class definition
- [x] Implement `init()` - configure at 168MHz/8 = 21MHz
- [x] Reload value 21000 for 1ms interrupts
- [x] Implement `micros()` with sub-ms interpolation
- [x] Implement `reset()` and `is_initialized()`
- [x] Add `SysTick_Handler()` ISR
- [x] Static assert concept compliance
- [x] Documentation

**Implementation Notes**:
- Similar to STM32F1 but different math
- Sub-ms: `(21000 - SysTick->VAL) / 21`

**Validation**:
```bash
# Compile STM32F4 example
cmake -B build/f4 -DBOARD=stm32f407vg -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build/f4 --target blink_stm32f407
```

**Files**:
- New: `src/hal/st/stm32f4/systick.hpp`

---

### Task 2.4: Integrate STM32F4 SysTick into Board
**Estimated**: 1 hour
**Dependencies**: Task 2.3
**Deliverable**: Updated `boards/stm32f407vg/board.hpp`

Add SysTick to STM32F4 board:
- [x] Include systick header
- [x] Add init call to `Board::initialize()`
- [x] Update docs

**Validation**:
```bash
# Build and verify
cmake --build build/f4 --target blink_stm32f407
```

**Files**:
- Modified: `boards/stm32f407vg/board.hpp`

---

## Phase 3: ESP32 & RP2040 (Day 2)

### Task 3.1: Implement ESP32 SysTick
**Estimated**: 3 hours
**Dependencies**: Task 1.1
**Deliverable**: `src/hal/espressif/esp32/systick.hpp`

Implement SysTick for ESP32 using esp_timer API:
- [x] Create header with class definition
- [x] Implement `init()` - mark as initialized
- [x] Implement `micros()` - use esp_timer_get_time()
- [x] No ISR needed (ESP-IDF handles it)
- [x] Implement `reset()` and `is_initialized()`
- [x] Static assert concept compliance
- [x] Documentation with ESP32-specific notes

**Implementation Notes**:
- Timer Group 0, Timer 0 configuration
- Read from `TIMGX_TN_LO_REG` directly
- 64-bit counter, mask to 32-bit
- Zero interrupt overhead

**Validation**:
```bash
# Compile ESP32 example (requires ESP-IDF)
source ~/esp/esp-idf/export.sh
cmake -B build/esp -DBOARD=esp32_devkit -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake
cmake --build build/esp --target blink_esp32.elf
```

**Files**:
- New: `src/hal/espressif/esp32/systick.hpp`

---

### Task 3.2: Integrate ESP32 SysTick into Board
**Estimated**: 1 hour
**Dependencies**: Task 3.1
**Deliverable**: Updated `boards/esp32_devkit/board.hpp`

Add SysTick to ESP32 board:
- [x] Include systick header
- [x] Add init call
- [x] Update docs

**Validation**:
```bash
cmake --build build/esp --target blink_esp32.elf
```

**Files**:
- Modified: `boards/esp32_devkit/board.hpp`

---

### Task 3.3: Implement RP2040 SysTick
**Estimated**: 1.5 hours
**Dependencies**: Task 1.1
**Deliverable**: `src/hal/raspberrypi/rp2040/systick.hpp`

Implement SysTick for RP2040 (easiest implementation):
- [x] Create header with class definition
- [x] Implement `init()` - basically a no-op (timer always running)
- [x] Implement `micros()` - read TIMELR register at 0x40054028
- [x] Implement `reset()` - write to TIMEHR/TIMELR (resets timer)
- [x] Implement `is_initialized()`
- [x] No ISR needed
- [x] Static assert concept compliance
- [x] Documentation

**Implementation Notes**:
- Hardware timer already at 1MHz
- Direct memory-mapped read
- Simplest implementation

**Validation**:
```bash
# Compile RP2040 example
cmake -B build/pico -DBOARD=rp_pico -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build/pico --target blink_rp_pico
```

**Files**:
- New: `src/hal/raspberrypi/rp2040/systick.hpp`

---

### Task 3.4: Integrate RP2040 SysTick into Board
**Estimated**: 1 hour
**Dependencies**: Task 3.3
**Deliverable**: Updated `boards/raspberry_pi_pico/board.hpp`

Add SysTick to RP2040 board:
- [x] Include systick header
- [x] Add init call
- [x] Update docs

**Validation**:
```bash
cmake --build build/pico --target blink_rp_pico
```

**Files**:
- Modified: `boards/raspberry_pi_pico/board.hpp`

---

## Phase 4: SAMD21 (Day 3)

### Task 4.1: Implement SAMD21 SysTick
**Estimated**: 4 hours
**Dependencies**: Task 1.1
**Deliverable**: `src/hal/microchip/samd21/systick.hpp`

Implement SysTick for SAMD21 using TC3:
- [x] Create header with class definition (simplified version)
- [x] Implement `init()` - mark as initialized
- [x] Implement `micros()` - return software counter
- [x] Implement `reset()` and `is_initialized()`
- [x] Static assert concept compliance
- [x] Documentation
- [ ] TODO: Full TC3 configuration (deferred)

**Implementation Notes**:
- Most complex implementation (no SysTick peripheral)
- Sub-ms: `(ms_counter * 1000) + (TC3->COUNT / 3)`
- Need to enable peripheral clock

**Validation**:
```bash
# Compile SAMD21 example
cmake -B build/zero -DBOARD=arduino_zero -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build/zero --target blink_arduino_zero
```

**Files**:
- New: `src/hal/microchip/samd21/systick.hpp`

---

### Task 4.2: Integrate SAMD21 SysTick into Board
**Estimated**: 1 hour
**Dependencies**: Task 4.1
**Deliverable**: Updated `boards/arduino_zero/board.hpp`

Add SysTick to SAMD21 board:
- [x] Include systick header
- [x] Add SysTick init call
- [x] Update docs
- [ ] TODO: Add TC3 clock enable (deferred)

**Validation**:
```bash
cmake --build build/zero --target blink_arduino_zero
```

**Files**:
- Modified: `boards/arduino_zero/board.hpp`

---

## Phase 5: Testing & Documentation (Day 3-4)

### Task 5.1: Create SysTick Example
**Estimated**: 2 hours
**Dependencies**: All implementations
**Deliverable**: `examples/systick_demo/main.cpp`

Create comprehensive example demonstrating SysTick:
- [ ] Create `examples/systick_demo/` directory
- [ ] Create `main.cpp` with demo
- [ ] Measure blink timing accuracy
- [ ] Demonstrate `micros()`, `micros_since()`, `is_timeout()`
- [ ] Show overflow handling
- [ ] Add CMakeLists.txt for all 5 boards
- [ ] Add README with explanation

**Example code**:
```cpp
#include "board.hpp"
#include <cstdio>

int main() {
    Board::initialize();
    Board::Led::init();

    printf("SysTick Demo\n");

    // Test 1: Basic timing
    uint32_t start = alloy::systick::micros();
    Board::delay_ms(100);
    uint32_t elapsed = alloy::systick::micros_since(start);
    printf("100ms delay took %u us\n", elapsed);

    // Test 2: Timeout check
    start = alloy::systick::micros();
    while (!alloy::systick::is_timeout(start, 500000)) {
        // Wait 500ms
    }
    printf("Timeout worked\n");

    // Test 3: Blink with timing
    while (true) {
        start = alloy::systick::micros();
        Board::Led::toggle();
        while (!alloy::systick::is_timeout(start, 1000000));
        elapsed = alloy::systick::micros_since(start);
        printf("Blink period: %u us\n", elapsed);
    }
}
```

**Validation**:
```bash
# Build for all 5 boards
./scripts/test-all-builds.sh
```

**Files**:
- New: `examples/systick_demo/main.cpp`
- New: `examples/systick_demo/CMakeLists.txt`
- New: `examples/systick_demo/README.md`

---

### Task 5.2: Add Unit Tests
**Estimated**: 3 hours
**Dependencies**: All implementations
**Deliverable**: `tests/hal/systick_test.cpp`

Create unit tests for SysTick:
- [ ] Test concept compliance for all implementations
- [ ] Test `micros_since()` overflow handling
- [ ] Test `is_timeout()` with various scenarios
- [ ] Mock timer for deterministic tests
- [ ] Test initialization and reset
- [ ] Add to test suite

**Validation**:
```bash
# Run tests
cmake -B build/tests -DBOARD=host
cmake --build build/tests --target alloy_tests
./build/tests/alloy_tests
```

**Files**:
- New: `tests/hal/systick_test.cpp`
- Modified: `tests/CMakeLists.txt`

---

### Task 5.3: Integration Tests on Real Hardware
**Estimated**: 4 hours
**Dependencies**: Task 5.1, all implementations
**Deliverable**: Validation report

Test on real hardware for all 5 boards:
- [ ] STM32F1 Blue Pill - verify 1µs precision
- [ ] STM32F4 Discovery - verify 1µs precision
- [ ] ESP32 DevKit - verify 1µs precision
- [ ] RP2040 Pico - verify 1µs precision (should be perfect)
- [ ] Arduino Zero - verify 1µs precision
- [ ] Test overflow handling (run for >71 minutes)
- [ ] Measure interrupt overhead (ARM platforms)
- [ ] Verify accuracy with oscilloscope
- [ ] Document results

**Validation**:
- Upload to each board
- Run systick_demo
- Measure LED blink period with oscilloscope
- Compare to expected 1.000s ±5%

**Files**:
- New: `docs/SYSTICK_VALIDATION.md`

---

### Task 5.4: Write Documentation
**Estimated**: 3 hours
**Dependencies**: All tasks
**Deliverable**: Updated documentation

Create comprehensive documentation:
- [ ] Add SysTick section to HAL docs
- [ ] API reference for all functions
- [ ] Usage examples (quick start, advanced)
- [ ] Platform-specific notes
- [ ] Overflow handling guide
- [ ] Best practices
- [ ] Troubleshooting
- [ ] Update README with SysTick info

**Files**:
- New: `docs/hal/SYSTICK.md`
- Modified: `README.md`

---

### Task 5.5: Update Build Scripts
**Estimated**: 1 hour
**Dependencies**: Task 5.1
**Deliverable**: Updated build system

Ensure build system handles SysTick:
- [ ] Verify all examples compile with SysTick
- [ ] Update `test-all-builds.sh` to include systick_demo
- [ ] Verify linker eliminates unused code
- [ ] Check binary sizes (should increase minimally)

**Validation**:
```bash
./scripts/test-all-builds.sh
```

**Files**:
- Modified: `scripts/test-all-builds.sh`

---

## Summary

**Total Estimated Time**: 32.5 hours (4-5 days)

**Parallelization Opportunities**:
- Tasks 2.1 and 2.3 can be done in parallel (STM32F1 and STM32F4)
- Tasks 3.1 and 3.3 can be done in parallel (ESP32 and RP2040)
- Tasks 2.1, 2.3, 3.1, 3.3 can all be done in parallel if multiple developers

**Critical Path**:
1. Task 1.1 (interface) → All implementations depend on this
2. Implementations → Board integrations depend on these
3. Board integrations → Examples depend on these
4. Examples → Tests depend on these

**Risk Mitigation**:
- Start with easiest platform (RP2040) to validate interface
- Test each platform thoroughly before moving to next
- Keep implementations simple and well-documented
- Use static assertions to catch errors early

**Deliverables Checklist**:
- [ ] Interface header
- [ ] 5 platform implementations
- [ ] 5 board integrations
- [ ] Example code
- [ ] Unit tests
- [ ] Integration tests
- [ ] Documentation
- [ ] Build system updates
