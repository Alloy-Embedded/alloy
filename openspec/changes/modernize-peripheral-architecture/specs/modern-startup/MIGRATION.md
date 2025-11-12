# Modern ARM Startup - Migration Guide

This guide helps you migrate from the old startup system to the modern C++23 implementation.

---

## Table of Contents

1. [Migration Overview](#migration-overview)
2. [Pre-Migration Checklist](#pre-migration-checklist)
3. [Step-by-Step Migration](#step-by-step-migration)
4. [API Reference](#api-reference)
5. [Common Issues](#common-issues)
6. [Rollback Plan](#rollback-plan)

---

# Migration Overview

## What's Changing

| Component | Old | New | Impact |
|-----------|-----|-----|--------|
| **Startup Code** | Manual C-style | Generated C++23 | High |
| **Vector Table** | Static array | Constexpr builder | Medium |
| **Interrupt Manager** | Class-based | Hardware policy | Medium |
| **SysTick** | Class wrapper | Hardware policy + board API | Medium |
| **Initialization** | Hardcoded | Flexible hooks | Low |

## Migration Effort

| Project Size | Estimated Time | Complexity |
|--------------|----------------|------------|
| Small (1-5 files) | 1-2 hours | LOW |
| Medium (5-20 files) | 3-6 hours | MEDIUM |
| Large (20+ files) | 1-2 days | HIGH |

## Benefits After Migration

- ✅ **90% less boilerplate** - Auto-generated startup
- ✅ **Type safety** - Compile-time checks
- ✅ **Flexibility** - Initialization hooks
- ✅ **Portability** - Easy to support new MCUs
- ✅ **Maintainability** - Modern C++23 code

---

# Pre-Migration Checklist

## 1. Inventory Current Code

Run these commands to identify what needs migration:

```bash
# Find all files using old startup
grep -r "startup/" src/ --include="*.cpp" --include="*.hpp" | wc -l

# Find old interrupt manager usage
grep -r "InterruptManager" src/ | wc -l

# Find old SysTick usage
grep -r "SysTick::" src/ | grep -v "SysTickHardware" | wc -l

# List all files that need updates
grep -r -l "startup/\|InterruptManager\|SysTick::" src/ --include="*.cpp" --include="*.hpp"
```

**Create a migration list**:

```bash
# Save list of files to migrate
grep -r -l "startup/\|InterruptManager\|SysTick::" src/ --include="*.cpp" --include="*.hpp" > migration_files.txt

echo "Files to migrate: $(wc -l < migration_files.txt)"
```

## 2. Backup Current State

```bash
# Create migration branch
git checkout -b migration/modern-startup

# Tag current state
git tag pre-modern-startup

# Now safe to make changes
```

## 3. Build Baseline

```bash
# Build and test current system
cd examples
make -f Makefile.same70_board clean all

# Record binary size
arm-none-eabi-size build/*.elf > baseline_size.txt

# Run tests (if available)
make test
```

## 4. Document Current Behavior

Create a checklist of functionality to verify after migration:

```markdown
# Functionality Checklist

- [ ] LED blinks at 1 Hz
- [ ] UART console works
- [ ] Interrupts fire correctly
- [ ] Timing is accurate (measure with scope)
- [ ] No crashes on boot
- [ ] Binary size <= baseline
```

---

# Step-by-Step Migration

## Phase 1: Generate Modern Startup (30 min)

### Step 1.1: Create Startup Metadata

```bash
# Create metadata file
cat > metadata/platform/same70_startup.json << 'EOF'
{
  "family": "same70",
  "mcu": "ATSAME70Q21B",
  "arch": "cortex-m7",
  "memory": {
    "flash": {"base": "0x00400000", "size": "0x00200000"},
    "sram": {"base": "0x20400000", "size": "0x00060000"},
    "stack_size": "0x00001000"
  },
  "vector_table": {
    "standard_exceptions": 16,
    "irq_count": 80,
    "handlers": [
      {"index": 16, "name": "SUPC_Handler", "description": "Supply Controller"},
      {"index": 23, "name": "UART0_Handler", "description": "UART 0"},
      {"index": 25, "name": "PIOA_Handler", "description": "PIO Controller A"}
    ]
  },
  "clock": {
    "default_cpu_freq": 300000000,
    "default_systick_freq": 1000
  }
}
EOF
```

**Fill in all 80 handlers** from SAME70 datasheet (see complete example in metadata/).

### Step 1.2: Generate Startup Code

```bash
# Run generator
./tools/codegen/cli/generators/generate_startup.sh same70

# Verify output
ls -lh src/hal/vendors/arm/same70/startup_same70.cpp
```

### Step 1.3: Test Build

```bash
# Update Makefile to use generated startup
cd examples

# Edit Makefile.same70_board
# Add: $(HAL_DIR)/vendors/arm/same70/startup_same70.cpp

# Build
make -f Makefile.same70_board clean all

# Should compile successfully
```

---

## Phase 2: Migrate Interrupt Manager (1-2 hours)

### Step 2.1: Find All Usages

```bash
# List files using InterruptManager
grep -r -l "InterruptManager" src/ > interrupt_mgr_files.txt
```

### Step 2.2: Update Each File

For each file in `interrupt_mgr_files.txt`:

**Before**:
```cpp
#include "hal/vendors/arm/same70/startup/interrupt_manager.hpp"

void init_uart() {
    InterruptManager::enable(IRQ_UART0);
    InterruptManager::set_priority(IRQ_UART0, 5);
}
```

**After**:
```cpp
#include "hal/vendors/arm/same70/nvic_hardware_policy.hpp"

void init_uart() {
    using NVIC = alloy::hal::same70::NVICHardware;
    NVIC::enable_irq(7);  // UART0 is IRQ 7
    NVIC::set_priority(7, 5);
}
```

### Step 2.3: IRQ Number Mapping

Create a reference file for IRQ numbers:

**File**: `src/hal/vendors/arm/same70/irq_numbers.hpp`

```cpp
#pragma once

namespace alloy::hal::same70 {

// IRQ numbers for SAME70Q21B
enum class IRQ : uint8_t {
    SUPC    = 0,   // Supply Controller
    RSTC    = 1,   // Reset Controller
    RTC     = 2,   // Real-time Clock
    RTT     = 3,   // Real-time Timer
    WDT     = 4,   // Watchdog Timer
    PMC     = 5,   // Power Management
    EEFC    = 6,   // Flash Controller
    UART0   = 7,   // UART 0
    UART1   = 8,   // UART 1
    PIOA    = 9,   // PIO Controller A
    PIOB    = 10,  // PIO Controller B
    // ... all 80 IRQs
};

} // namespace alloy::hal::same70
```

**Usage**:
```cpp
#include "hal/vendors/arm/same70/irq_numbers.hpp"

using IRQ = alloy::hal::same70::IRQ;
NVIC::enable_irq(static_cast<uint8_t>(IRQ::UART0));
```

### Step 2.4: Verify Changes

```bash
# Build after each file
make -f Makefile.same70_board

# Ensure no InterruptManager references remain
grep -r "InterruptManager" src/ && echo "❌ Still found" || echo "✅ Clean"
```

---

## Phase 3: Migrate SysTick (1-2 hours)

### Step 3.1: Find All Usages

```bash
# List files using old SysTick
grep -r -l "SysTick::" src/ | grep -v "SysTickHardware" > systick_files.txt
```

### Step 3.2: Choose Migration Path

Two options:

**Option A: Direct Hardware Policy** (for low-level code)
```cpp
// Before
#include "hal/vendors/arm/same70/startup/systick.hpp"
SysTick::init(1);

// After
#include "hal/vendors/arm/same70/systick_hardware_policy.hpp"
alloy::hal::same70::SysTickHardware::configure_ms(1);
```

**Option B: Board Abstraction** (recommended for applications)
```cpp
// Before
#include "hal/vendors/arm/same70/startup/systick.hpp"
SysTick::init(1);
delay_ms(500);

// After
#include "boards/same70_xplained/board.hpp"
board::init();  // Handles SysTick setup
board::delay_ms(500);
```

### Step 3.3: Migrate Timing Functions

**Before**:
```cpp
volatile uint32_t tick_count = 0;

extern "C" void SysTick_Handler() {
    tick_count++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = tick_count;
    while ((tick_count - start) < ms) {
        __asm volatile("wfi");
    }
}
```

**After**: Delete this code! Use `board::delay_ms()` instead.

The board layer already provides:
- `board::delay_ms(ms)` - Precise delay
- `board::millis()` - Get uptime
- `SysTick_Handler()` - Auto-implemented in board.cpp

### Step 3.4: Update Examples

**Before** (`examples/old_blink.cpp`):
```cpp
#include "hal/vendors/arm/same70/startup/systick.hpp"
#include "hal/vendors/atmel/same70/pio_hardware_policy.hpp"

int main() {
    // Manual setup
    PioCHardware::enable_pio(1u << 8);
    PioCHardware::enable_output(1u << 8);
    SysTick::init(1);

    while (true) {
        PioCHardware::toggle_output(1u << 8);
        delay_ms(500);
    }
}
```

**After** (`examples/new_blink.cpp`):
```cpp
#include "boards/same70_xplained/board.hpp"

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

**90% less code!**

---

## Phase 4: Update Initialization (30 min)

### Step 4.1: Identify Init Code

Find manual initialization:

```bash
# Find files with init sequences
grep -r "enable_pio\|configure_clock\|init_peripherals" src/
```

### Step 4.2: Move to Hooks

**Before** (scattered in main):
```cpp
int main() {
    // Clock setup
    configure_pll();
    configure_flash_wait_states();

    // Peripheral init
    init_gpio();
    init_uart();
    init_spi();

    // Application
    run_app();
}
```

**After** (organized with hooks):
```cpp
// In board.cpp or application
extern "C" void pre_main_init() {
    // Clock setup (before main)
    configure_pll();
    configure_flash_wait_states();
}

extern "C" void late_init() {
    // Peripheral init (from board::init)
    init_uart();
    init_spi();
    // GPIO init handled by board layer
}

int main() {
    board::init();  // Calls late_init() internally
    run_app();
}
```

### Step 4.3: Update board::init()

Edit `boards/same70_xplained/board.cpp`:

```cpp
void init() {
    using SysTick = alloy::hal::same70::SysTickHardware;

    // SysTick for timing
    SysTick::configure_ms(1);

    // LED GPIO
    led::init();

    // Call late init hook
    late_init();

    // Future: UART console, buttons, etc.
}
```

---

## Phase 5: Remove Legacy Code (1 hour)

### Step 5.1: Verify No References

```bash
# Ensure nothing uses old code
grep -r "startup/" src/ --include="*.cpp" --include="*.hpp"
grep -r "InterruptManager" src/
grep -r "SysTick::" src/ | grep -v "SysTickHardware"

# All should return empty (or only comments)
```

### Step 5.2: Remove Old Files

```bash
# List files to remove
find src/hal -path "*/startup/*" -type f

# Backup first
mkdir -p backup_old_startup
find src/hal -path "*/startup/*" -type f -exec cp {} backup_old_startup/ \;

# Remove
rm -rf src/hal/vendors/arm/same70/startup/
```

### Step 5.3: Update Build System

Remove old files from Makefiles:

```makefile
# OLD (remove these lines)
CXX_SOURCES += \
    $(HAL_DIR)/vendors/arm/same70/startup/startup.cpp \
    $(HAL_DIR)/vendors/arm/same70/startup/interrupt_manager.cpp \
    $(HAL_DIR)/vendors/arm/same70/startup/systick.cpp

# NEW (add this line)
CXX_SOURCES += \
    $(HAL_DIR)/vendors/arm/same70/startup_same70.cpp \
    $(BOARD_DIR)/board.cpp
```

### Step 5.4: Update Documentation

Update README files to mention new system:

```markdown
# Startup System

This project uses the modern C++23 startup system:

- **Startup Code**: Auto-generated from `metadata/platform/same70_startup.json`
- **Vector Table**: Compile-time constexpr builder
- **Interrupts**: Hardware policies (NVIC)
- **Timing**: Board abstraction (`board::delay_ms()`)

See `docs/STARTUP.md` for details.
```

---

## Phase 6: Test & Verify (1-2 hours)

### Step 6.1: Build All Examples

```bash
cd examples

for makefile in Makefile.*; do
    echo "Building $makefile..."
    make -f "$makefile" clean all || exit 1
done

echo "✅ All examples build successfully"
```

### Step 6.2: Compare Binary Sizes

```bash
# Compare with baseline
arm-none-eabi-size build/*.elf > new_size.txt
diff baseline_size.txt new_size.txt

# Should be similar or smaller
```

### Step 6.3: Test on Hardware

```bash
# Flash to board
make -f Makefile.same70_board flash

# Verify functionality:
# - LED blinks at correct rate
# - UART works
# - No crashes
# - Interrupts fire
```

### Step 6.4: Run Automated Tests

```bash
# Run test suite (if available)
cd tests
./run_all_tests.sh

# Should all pass
```

### Step 6.5: Timing Verification

Test with oscilloscope:

```cpp
// Add test code
int main() {
    board::init();

    while (true) {
        board::led::toggle();
        board::delay_ms(500);  // Should be exactly 500ms
    }
}
```

Measure LED toggle period:
- **Expected**: 1000ms (500ms on + 500ms off)
- **Tolerance**: ±1% (990-1010ms)

---

# API Reference

## Old vs New API Mapping

### Interrupt Management

| Old API | New API | Notes |
|---------|---------|-------|
| `InterruptManager::enable(irq)` | `NVICHardware::enable_irq(irq)` | Direct hardware access |
| `InterruptManager::disable(irq)` | `NVICHardware::disable_irq(irq)` | |
| `InterruptManager::set_priority(irq, prio)` | `NVICHardware::set_priority(irq, prio)` | |
| `InterruptManager::is_pending(irq)` | `NVICHardware::is_pending(irq)` | |
| `InterruptManager::clear_pending(irq)` | `NVICHardware::clear_pending(irq)` | |

### SysTick & Timing

| Old API | New API | Notes |
|---------|---------|-------|
| `SysTick::init(ms)` | `SysTickHardware::configure_ms(ms)` | Low-level |
| | `board::init()` | High-level (recommended) |
| `SysTick::get_ticks()` | `board::millis()` | Returns uptime in ms |
| `delay_ms(ms)` (custom) | `board::delay_ms(ms)` | Built-in, power-efficient |
| `SysTick::enable()` | `SysTickHardware::enable_interrupt()` | |
| `SysTick::disable()` | `SysTickHardware::disable_interrupt()` | |

### Startup & Initialization

| Old API | New API | Notes |
|---------|---------|-------|
| `#include "startup/startup.hpp"` | Auto-included | No manual include needed |
| Manual `Reset_Handler()` | Auto-generated | From metadata |
| Hardcoded init sequence | `early_init()` hook | Before .data/.bss |
| | `pre_main_init()` hook | After .data/.bss |
| | `late_init()` hook | From board::init() |

### GPIO (unchanged, for reference)

| API | Notes |
|-----|-------|
| `PioCHardware::enable_pio(mask)` | Still works |
| `PioCHardware::toggle_output(mask)` | Still works |
| `board::led::toggle()` | Higher-level API (recommended) |

---

# Common Issues

## Issue 1: Build Errors After Migration

**Error**:
```
error: 'InterruptManager' has not been declared
```

**Cause**: Forgot to update an include or usage

**Fix**:
```bash
# Find remaining references
grep -r "InterruptManager" src/

# Update to NVICHardware
```

---

## Issue 2: Linker Errors

**Error**:
```
undefined reference to `SysTick_Handler'
```

**Cause**: Missing SysTick handler implementation

**Fix**: Add to `board.cpp`:
```cpp
extern "C" void SysTick_Handler() {
    board::system_ticks_ms++;
}
```

---

## Issue 3: Wrong IRQ Numbers

**Error**: Interrupts don't fire

**Cause**: IRQ number mismatch between old and new system

**Fix**: Check SAME70 datasheet for correct IRQ numbers:
- UART0 is IRQ 7 (vector index 23)
- Not IRQ 23!

Use `irq_numbers.hpp` for reference.

---

## Issue 4: Timing Inaccurate

**Symptom**: `delay_ms(500)` is not 500ms

**Possible Causes**:
1. SysTick not configured correctly
2. CPU frequency wrong
3. Timer overflow

**Fix**:
```cpp
// Verify SysTick configuration
// Should be called in board::init()
SysTickHardware::configure_ms(1);  // 1ms tick

// Verify in board_config.hpp
static constexpr uint32_t cpu_freq_hz = 300'000'000;  // Correct?
```

---

## Issue 5: Binary Size Increased

**Symptom**: New binary is larger than old

**Possible Causes**:
1. Including unnecessary headers
2. Non-inlined functions
3. Debug symbols

**Fix**:
```bash
# Check what's bloating
arm-none-eabi-nm --size-sort --print-size build/app.elf | tail -20

# Ensure optimization
CXXFLAGS += -O2 -flto

# Ensure inlining
# Functions should be 'inline' or in headers
```

---

# Rollback Plan

If migration fails, you can rollback:

## Quick Rollback

```bash
# Return to pre-migration state
git checkout pre-modern-startup

# Or revert branch
git checkout main
git branch -D migration/modern-startup
```

## Selective Rollback

Keep new code but restore old temporarily:

```bash
# Restore specific files from backup
cp backup_old_startup/* src/hal/vendors/arm/same70/startup/

# Update Makefile to use old files
# Build and test
```

## Hybrid Approach

Run both systems in parallel:

```cpp
// Use preprocessor to switch
#define USE_MODERN_STARTUP 1

#if USE_MODERN_STARTUP
    #include "boards/same70_xplained/board.hpp"
    #define INIT() board::init()
#else
    #include "hal/vendors/arm/same70/startup/systick.hpp"
    #define INIT() SysTick::init(1)
#endif

int main() {
    INIT();
    // Rest of code unchanged
}
```

---

# Migration Success Checklist

Use this checklist to verify migration is complete:

## Code Changes
- [ ] All files using `InterruptManager` updated
- [ ] All files using old `SysTick` updated
- [ ] All startup includes removed
- [ ] Initialization moved to hooks
- [ ] Examples updated to use board API

## Build System
- [ ] Makefile updated (removed old files)
- [ ] Makefile updated (added new files)
- [ ] All examples build successfully
- [ ] No warnings about old APIs

## Testing
- [ ] Binary size <= baseline
- [ ] All functionality works on hardware
- [ ] Timing accuracy verified (oscilloscope)
- [ ] Interrupts fire correctly
- [ ] No crashes or hangs

## Cleanup
- [ ] Old startup files removed
- [ ] Old interrupt manager removed
- [ ] Old systick removed
- [ ] Documentation updated
- [ ] Migration branch merged

## Final Steps
- [ ] Tag as `post-modern-startup`
- [ ] Update CHANGELOG
- [ ] Notify team
- [ ] Close migration tickets

---

# Getting Help

If you encounter issues during migration:

1. **Check this guide** - Most common issues are covered
2. **Check examples** - See working code in `examples/`
3. **Check specs** - See `SPEC.md` for technical details
4. **Check generated code** - Look at `startup_same70.cpp`
5. **Ask for help** - Create GitHub issue with details

---

**Migration Status**: Ready for implementation
**Estimated Total Time**: 4-8 hours (depending on project size)
**Risk Level**: LOW (rollback available, proven approach)
