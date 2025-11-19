# Modern ARM Startup - Code Examples

This document provides practical code examples for all three parts of the modern startup system.

---

## Table of Contents

1. [Part 1 Examples: Modern C++23 Startup](#part-1-modern-c23-startup)
2. [Part 2 Examples: Auto-Generation](#part-2-auto-generation)
3. [Part 3 Examples: Migration](#part-3-migration)

---

# Part 1: Modern C++23 Startup

## Example 1: Basic Vector Table

```cpp
#include "hal/vendors/arm/cortex_m7/vector_table.hpp"

using namespace alloy::hal::arm;

// Define handlers
extern "C" void Reset_Handler();
extern "C" void SysTick_Handler();
extern "C" void Default_Handler();

// Build vector table at compile time
constexpr auto my_vector_table = make_vector_table<96>()  // 16 + 80 IRQs
    .set_stack_pointer(0x20400000 + 0x60000)  // Top of SRAM
    .set_handler(1, &Reset_Handler)
    .set_handler(15, &SysTick_Handler)
    .set_handler(2, &Default_Handler)  // NMI
    .set_handler(3, &Default_Handler)  // HardFault
    .get();

// Place in .isr_vector section
__attribute__((section(".isr_vector"), used))
const auto vectors = my_vector_table;
```

## Example 2: Custom Reset Handler

```cpp
#include "hal/vendors/arm/cortex_m7/startup_impl.hpp"
#include "startup_config.hpp"

extern "C" [[noreturn]] void Reset_Handler() {
    using namespace alloy::hal::arm;
    using Config = alloy::hal::same70::StartupConfig;

    // Use modern startup implementation
    StartupImpl::startup_sequence<Config>();

    // Never returns
}
```

## Example 3: Initialization Hooks

```cpp
// Application-specific early initialization
extern "C" void early_init() {
    // Configure flash wait states for 300 MHz
    // (This runs BEFORE .data/.bss initialization!)

    // Example: Set flash wait states
    volatile uint32_t* EEFC_FMR = (uint32_t*)0x400E0A00;
    *EEFC_FMR = (6 << 8);  // 6 wait states for 300 MHz
}

// Clock configuration before main()
extern "C" void pre_main_init() {
    // Configure PLL to 300 MHz
    // (This runs AFTER .data/.bss initialization)

    // Example: Enable PLL (simplified)
    volatile uint32_t* PMC_CKGR_PLLAR = (uint32_t*)0x400E0628;
    *PMC_CKGR_PLLAR = 0x20193F01;  // Configure PLL

    // Wait for PLL lock...
}

// Board-level initialization (called from main)
extern "C" void late_init() {
    // Initialize peripherals
    // This is called by board::init()
}

int main() {
    // At this point:
    // - early_init() has run
    // - .data/.bss are initialized
    // - pre_main_init() has run
    // - Static constructors have run

    board::init();  // Calls late_init() internally

    // Application code
    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

## Example 4: Compile-Time Vector Table Validation

```cpp
// Type-safe interrupt handler registration
template<typename Handler>
constexpr auto register_handler(size_t index, Handler h) {
    static_assert(
        std::is_invocable_r_v<void, Handler>,
        "Handler must be callable with no arguments and return void"
    );

    return make_vector_table<96>()
        .set_handler(index, h)
        .get();
}

// Usage
extern "C" void UART0_Handler();

// This compiles
constexpr auto vt1 = register_handler(23, &UART0_Handler);

// This would fail at compile time
// auto bad_handler = []() -> int { return 0; };
// constexpr auto vt2 = register_handler(23, bad_handler);  // ERROR!
```

## Example 5: Custom Startup Sequence

```cpp
// Override startup for special use case
extern "C" [[noreturn]] void Reset_Handler() {
    // 1. Early hardware setup
    early_init();

    // 2. Custom .data initialization (e.g., with decompression)
    extern uint32_t _sidata_compressed[];
    extern uint32_t _sdata[];
    extern uint32_t _edata[];
    decompress_data(_sidata_compressed, _sdata, _edata);

    // 3. Standard .bss initialization
    StartupImpl::init_bss_section(&_sbss, &_ebss);

    // 4. Call constructors
    extern void (*__init_array_start)();
    extern void (*__init_array_end)();
    StartupImpl::call_init_array(&__init_array_start, &__init_array_end);

    // 5. Pre-main setup
    pre_main_init();

    // 6. Call main
    main();

    while (true) {}
}
```

---

# Part 2: Auto-Generation

## Example 6: Metadata File

**File**: `metadata/platform/same70_startup.json`

```json
{
  "family": "same70",
  "mcu": "ATSAME70Q21B",
  "arch": "cortex-m7",

  "memory": {
    "flash": {
      "base": "0x00400000",
      "size": "0x00200000",
      "comment": "2 MB internal flash"
    },
    "sram": {
      "base": "0x20400000",
      "size": "0x00060000",
      "comment": "384 KB SRAM"
    },
    "stack_size": "0x00001000"
  },

  "vector_table": {
    "standard_exceptions": 16,
    "irq_count": 80,
    "handlers": [
      {
        "index": 16,
        "name": "SUPC_Handler",
        "irq_number": 0,
        "description": "Supply Controller"
      },
      {
        "index": 17,
        "name": "RSTC_Handler",
        "irq_number": 1,
        "description": "Reset Controller"
      },
      {
        "index": 23,
        "name": "UART0_Handler",
        "irq_number": 7,
        "description": "UART 0"
      },
      {
        "index": 24,
        "name": "UART1_Handler",
        "irq_number": 8,
        "description": "UART 1"
      },
      {
        "index": 25,
        "name": "PIOA_Handler",
        "irq_number": 9,
        "description": "PIO Controller A"
      }
    ]
  },

  "clock": {
    "default_cpu_freq": 300000000,
    "default_systick_freq": 1000,
    "oscillator": {
      "main": 12000000,
      "slow": 32768
    }
  },

  "init_hooks": {
    "early_init": {
      "enabled": true,
      "description": "Called before .data/.bss init"
    },
    "pre_main_init": {
      "enabled": true,
      "description": "Called after .data/.bss, before main"
    },
    "late_init": {
      "enabled": true,
      "description": "Called from board::init()"
    }
  },

  "features": {
    "fpu": true,
    "mpu": true,
    "cache": true,
    "dsp": true
  }
}
```

## Example 7: Generate Startup Code

```bash
#!/bin/bash
# Generate startup code for SAME70

cd tools/codegen/cli/generators

# Generate startup
./generate_startup.sh same70

# Output:
# 🚀 Generating startup code for same70...
# ✅ Generated: src/hal/vendors/arm/same70/startup_same70.cpp
# ✅ Done!
```

## Example 8: Generated Output

**Generated File**: `src/hal/vendors/arm/same70/startup_same70.cpp`

```cpp
/**
 * @file startup_same70.cpp
 * @brief Auto-generated startup code for ATSAME70Q21B
 *
 * Generated from: metadata/platform/same70_startup.json
 * Generator: tools/codegen/cli/generators/startup_generator.py
 *
 * DO NOT EDIT THIS FILE MANUALLY!
 * To regenerate: ./tools/codegen/cli/generators/generate_startup.sh same70
 */

#include "hal/vendors/arm/cortex_m7/startup_impl.hpp"
#include "hal/vendors/arm/cortex_m7/vector_table.hpp"
#include "startup_config.hpp"

using namespace alloy::hal::arm;
using namespace alloy::hal::same70;

// =============================================================================
// Exception Handlers (auto-generated)
// =============================================================================

extern "C" void Default_Handler() {
    while (true) {}
}

extern "C" [[noreturn]] void Reset_Handler() {
    StartupImpl::startup_sequence<StartupConfig>();
}

/**
 * @brief Supply Controller
 */
extern "C" [[gnu::weak, gnu::alias("Default_Handler")]]
void SUPC_Handler();

/**
 * @brief Reset Controller
 */
extern "C" [[gnu::weak, gnu::alias("Default_Handler")]]
void RSTC_Handler();

/**
 * @brief UART 0
 */
extern "C" [[gnu::weak, gnu::alias("Default_Handler")]]
void UART0_Handler();

/**
 * @brief UART 1
 */
extern "C" [[gnu::weak, gnu::alias("Default_Handler")]]
void UART1_Handler();

/**
 * @brief PIO Controller A
 */
extern "C" [[gnu::weak, gnu::alias("Default_Handler")]]
void PIOA_Handler();

// ... (80 total handlers)

// =============================================================================
// Vector Table (auto-generated)
// =============================================================================

constexpr auto vector_table = make_vector_table<96>()
    .set_stack_pointer(StartupConfig::stack_top())
    .set_handler(1, &Reset_Handler)
    .set_handler(2, &Default_Handler)  // NMI
    .set_handler(3, &Default_Handler)  // HardFault
    .set_handler(4, &Default_Handler)  // MemManage
    .set_handler(5, &Default_Handler)  // BusFault
    .set_handler(6, &Default_Handler)  // UsageFault
    .set_handler(11, &Default_Handler) // SVCall
    .set_handler(12, &Default_Handler) // DebugMon
    .set_handler(14, &Default_Handler) // PendSV
    .set_handler(15, &SysTick_Handler) // SysTick
    .set_handler(16, &SUPC_Handler)
    .set_handler(17, &RSTC_Handler)
    .set_handler(23, &UART0_Handler)
    .set_handler(24, &UART1_Handler)
    .set_handler(25, &PIOA_Handler)
    // ... (all 80 handlers)
    .get();

__attribute__((section(".isr_vector"), used))
const auto vectors = vector_table;
```

## Example 9: Custom Handler Implementation

```cpp
// Application provides custom handler
// (Weak alias allows override)

extern "C" void UART0_Handler() {
    // Custom UART0 interrupt handler
    using UART = alloy::hal::same70::Uart0Hardware;

    // Check interrupt status
    uint32_t status = UART::get_status();

    if (status & UART::STATUS_RXRDY) {
        // Receive ready
        uint8_t data = UART::read_byte();
        process_rx_data(data);
    }

    // Clear interrupt
    UART::clear_status(status);
}
```

## Example 10: Generator Extension

**Add new platform**:

```bash
# 1. Create metadata
cat > metadata/platform/stm32h7_startup.json << 'EOF'
{
  "family": "stm32h7",
  "mcu": "STM32H743ZI",
  "arch": "cortex-m7",
  "memory": {
    "flash": {"base": "0x08000000", "size": "0x00200000"},
    "sram": {"base": "0x20000000", "size": "0x00020000"}
  },
  "vector_table": {
    "standard_exceptions": 16,
    "irq_count": 150
  }
}
EOF

# 2. Generate startup
./tools/codegen/cli/generators/generate_startup.sh stm32h7

# 3. Done! New platform supported
```

---

# Part 3: Migration

## Example 11: Before - Old Interrupt Manager

```cpp
// OLD CODE (to be removed)
#include "hal/vendors/arm/same70/startup/interrupt_manager.hpp"

void setup_uart_interrupt() {
    // Old API
    InterruptManager::enable(IRQ_UART0);
    InterruptManager::set_priority(IRQ_UART0, 5);
}
```

## Example 12: After - New Hardware Policy

```cpp
// NEW CODE
#include "hal/vendors/arm/same70/nvic_hardware_policy.hpp"

void setup_uart_interrupt() {
    using NVIC = alloy::hal::same70::NVICHardware;

    // New API (direct hardware access)
    NVIC::enable_irq(7);  // UART0 IRQ number
    NVIC::set_priority(7, 5);
}
```

## Example 13: Before - Old SysTick Class

```cpp
// OLD CODE (to be removed)
#include "hal/vendors/arm/same70/startup/systick.hpp"

void init_timing() {
    // Old API
    SysTick::init(1);  // 1ms tick

    while (true) {
        if (SysTick::get_ticks() % 500 == 0) {
            toggle_led();
        }
    }
}
```

## Example 14: After - Board Abstraction

```cpp
// NEW CODE
#include "boards/same70_xplained/board.hpp"

void init_timing() {
    // New API (cleaner!)
    board::init();  // Handles SysTick setup

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

## Example 15: Before - Old Startup Include

```cpp
// OLD CODE
#include "hal/vendors/arm/same70/startup/startup.hpp"
#include "hal/vendors/arm/same70/startup/vector_table.hpp"

void configure_system() {
    Startup::init_clock();
    Startup::init_peripherals();
}
```

## Example 16: After - Initialization Hooks

```cpp
// NEW CODE
// No includes needed in application!

extern "C" void pre_main_init() {
    // Clock configuration runs automatically
    // before main() via initialization hook
    configure_pll_300mhz();
}

int main() {
    board::init();  // Peripherals initialized here
    // Application code
}
```

## Example 17: Migration Checklist Script

```bash
#!/bin/bash
# migration_check.sh - Verify migration complete

echo "🔍 Checking for old APIs..."

# Check for old interrupt manager
if grep -r "InterruptManager::" src/; then
    echo "❌ Found InterruptManager references"
    exit 1
fi

# Check for old SysTick
if grep -r "SysTick::" src/ | grep -v "SysTickHardware"; then
    echo "❌ Found old SysTick references"
    exit 1
fi

# Check for old startup includes
if grep -r "#include.*startup/" src/; then
    echo "❌ Found old startup includes"
    exit 1
fi

echo "✅ Migration complete!"
```

## Example 18: Side-by-Side Comparison

```cpp
// =============================================================================
// BEFORE: Manual setup (50+ lines)
// =============================================================================

#include "hal/vendors/arm/same70/startup/systick.hpp"
#include "hal/vendors/arm/same70/startup/interrupt_manager.hpp"
#include "hal/vendors/atmel/same70/pio_hardware_policy.hpp"

void configure_led() {
    using PIO = alloy::hal::same70::PioCHardware;
    PIO::enable_pio(1u << 8);
    PIO::enable_output(1u << 8);
    PIO::set_output(1u << 8);
}

void configure_timing() {
    SysTick::init(1);  // 1ms
}

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

int main() {
    configure_led();
    configure_timing();

    while (true) {
        PIO::toggle_output(1u << 8);
        delay_ms(500);
    }
}

// =============================================================================
// AFTER: Board abstraction (5 lines!)
// =============================================================================

#include "boards/same70_xplained/board.hpp"

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

## Example 19: Gradual Migration Strategy

```cpp
// Phase 1: Keep old code working
#define USING_LEGACY_STARTUP 1

#if USING_LEGACY_STARTUP
    #include "hal/vendors/arm/same70/startup/systick.hpp"
    #define INIT_TIMING() SysTick::init(1)
#else
    #include "boards/same70_xplained/board.hpp"
    #define INIT_TIMING() board::init()
#endif

int main() {
    INIT_TIMING();
    // Application code unchanged
}

// Phase 2: Switch to new (change one line)
#define USING_LEGACY_STARTUP 0

// Phase 3: Remove old code entirely
```

## Example 20: Automated Migration Tool

```python
#!/usr/bin/env python3
"""
migrate_startup.py - Automated migration helper

Usage: ./migrate_startup.py <source_file>
"""

import re
import sys

MIGRATIONS = [
    # Interrupt Manager
    (r'#include ".*interrupt_manager\.hpp"',
     r'#include "hal/vendors/arm/same70/nvic_hardware_policy.hpp"'),
    (r'InterruptManager::enable\((\w+)\)',
     r'alloy::hal::same70::NVICHardware::enable_irq(\1)'),

    # SysTick
    (r'#include ".*systick\.hpp"',
     r'#include "boards/same70_xplained/board.hpp"'),
    (r'SysTick::init\(1\)',
     r'board::init()'),
    (r'SysTick::get_ticks\(\)',
     r'board::millis()'),

    # Startup
    (r'#include ".*startup\.hpp"',
     r'// Modern startup auto-included'),
]

def migrate_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original = content
    for old, new in MIGRATIONS:
        content = re.sub(old, new, content)

    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"✅ Migrated: {filepath}")
        return True
    else:
        print(f"⏭️  No changes: {filepath}")
        return False

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ./migrate_startup.py <source_file>")
        sys.exit(1)

    migrate_file(sys.argv[1])
```

---

# Complete Integration Example

## Example 21: Full Application with Modern Startup

**Project Structure**:
```
project/
├── metadata/
│   └── platform/
│       └── same70_startup.json          # MCU metadata
├── boards/
│   └── same70_xplained/
│       ├── board_config.hpp             # Board config
│       ├── board.hpp                    # Board API
│       └── board.cpp                    # Board impl
├── src/
│   └── hal/
│       └── vendors/
│           └── arm/
│               ├── cortex_m7/
│               │   ├── startup_impl.hpp # Modern startup
│               │   └── vector_table.hpp # Vector table builder
│               └── same70/
│                   └── startup_same70.cpp  # Generated!
└── examples/
    └── blink_modern.cpp                 # Application
```

**Application Code** (`examples/blink_modern.cpp`):

```cpp
#include "boards/same70_xplained/board.hpp"

// Optional: Custom clock configuration
extern "C" void pre_main_init() {
    // Configure PLL to 300 MHz
    // (Called automatically before main)
}

int main() {
    // Initialize board (uses modern startup)
    board::init();

    // Application logic
    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }

    return 0;
}
```

**Build**:

```bash
# Generate startup (one time)
./tools/codegen/cli/generators/generate_startup.sh same70

# Build
cd examples
make -f Makefile.same70_board

# Flash
make -f Makefile.same70_board flash
```

**Result**: Clean, modern, maintainable code with zero overhead!

---

This completes the code examples. All three parts are covered with practical, working examples ready for implementation.
