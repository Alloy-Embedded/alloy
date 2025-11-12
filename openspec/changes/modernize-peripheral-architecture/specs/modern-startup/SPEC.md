# Modern ARM Startup - Technical Specification

**Version**: 1.0
**Date**: 2025-11-11
**Status**: PROPOSAL

---

## Table of Contents

1. [Part 1: Modern ARM Startup (C++23)](#part-1-modern-arm-startup-c23)
2. [Part 2: Per-MCU Startup Generation](#part-2-per-mcu-startup-generation)
3. [Part 3: Cleanup Legacy Code](#part-3-cleanup-legacy-code)
4. [Integration Points](#integration-points)
5. [Testing Strategy](#testing-strategy)

---

# Part 1: Modern ARM Startup (C++23)

## 1.1 Goals

- Replace legacy startup code with modern C++23 implementation
- Use constexpr/consteval for compile-time vector table generation
- Add flexible initialization hooks
- Maintain zero runtime overhead

## 1.2 Current State (Problems)

**Current startup in `src/hal/vendors/arm/same70/startup/`**:

```cpp
// Legacy C-style code
void Reset_Handler(void) {
    // Manual .data copy
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    // Manual .bss zero
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    // Call main
    main();
}
```

**Problems**:
- ❌ C-style code, not C++
- ❌ No initialization hooks
- ❌ Hardcoded logic
- ❌ Not reusable across MCUs
- ❌ No compile-time validation

## 1.3 New Design

### 1.3.1 File Structure

```
src/hal/vendors/arm/cortex_m7/
├── startup.hpp              # Public API
├── startup_impl.hpp         # Template implementation
├── vector_table.hpp         # Constexpr vector table builder
└── init_hooks.hpp           # Initialization hook system
```

### 1.3.2 Vector Table (Modern C++23)

**File**: `src/hal/vendors/arm/cortex_m7/vector_table.hpp`

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <concepts>

namespace alloy::hal::arm {

/**
 * @brief Concept for interrupt handlers
 */
template<typename T>
concept InterruptHandler = requires(T t) {
    { t() } -> std::same_as<void>;
};

/**
 * @brief Compile-time vector table builder
 *
 * @tparam VectorCount Number of interrupt vectors
 */
template<size_t VectorCount>
class VectorTableBuilder {
public:
    using HandlerType = void(*)();

    // Cortex-M standard exceptions (0-15)
    static constexpr size_t STACK_POINTER_IDX = 0;
    static constexpr size_t RESET_HANDLER_IDX = 1;
    static constexpr size_t NMI_HANDLER_IDX = 2;
    static constexpr size_t HARD_FAULT_HANDLER_IDX = 3;
    static constexpr size_t MEM_MANAGE_HANDLER_IDX = 4;
    static constexpr size_t BUS_FAULT_HANDLER_IDX = 5;
    static constexpr size_t USAGE_FAULT_HANDLER_IDX = 6;
    static constexpr size_t SVCALL_HANDLER_IDX = 11;
    static constexpr size_t DEBUG_MON_HANDLER_IDX = 12;
    static constexpr size_t PENDSV_HANDLER_IDX = 14;
    static constexpr size_t SYSTICK_HANDLER_IDX = 15;

    /**
     * @brief Build vector table at compile time
     */
    consteval VectorTableBuilder() {
        // Initialize all to default handler
        for (size_t i = 0; i < VectorCount; ++i) {
            vectors_[i] = &default_handler;
        }
    }

    /**
     * @brief Set handler for specific vector
     */
    constexpr VectorTableBuilder& set_handler(size_t index, HandlerType handler) {
        if (index < VectorCount) {
            vectors_[index] = handler;
        }
        return *this;
    }

    /**
     * @brief Set stack pointer (vector 0)
     */
    constexpr VectorTableBuilder& set_stack_pointer(uintptr_t sp) {
        vectors_[0] = reinterpret_cast<HandlerType>(sp);
        return *this;
    }

    /**
     * @brief Get vector table array
     */
    constexpr const auto& get() const { return vectors_; }

private:
    std::array<HandlerType, VectorCount> vectors_{};

    static void default_handler() {
        while (true) {}  // Infinite loop
    }
};

/**
 * @brief Helper to create vector table with fluent API
 */
template<size_t VectorCount>
consteval auto make_vector_table() {
    return VectorTableBuilder<VectorCount>{};
}

} // namespace alloy::hal::arm
```

### 1.3.3 Initialization Hooks

**File**: `src/hal/vendors/arm/cortex_m7/init_hooks.hpp`

```cpp
#pragma once

namespace alloy::hal::arm {

/**
 * @brief Initialization hook interface
 *
 * Applications can define these hooks to customize startup:
 * - early_init(): Before .data/.bss initialization
 * - pre_main_init(): After .data/.bss, before main()
 * - late_init(): Called from main() by board::init()
 */

// Default implementations (weak symbols)
extern "C" {

/**
 * @brief Called BEFORE .data/.bss initialization
 *
 * Use for:
 * - Flash wait states configuration
 * - Early clock setup (if needed before .data copy)
 * - Watchdog disable
 *
 * WARNING: Cannot access global variables yet!
 */
[[gnu::weak]]
void early_init() {
    // Default: do nothing
}

/**
 * @brief Called AFTER .data/.bss initialization, BEFORE main()
 *
 * Use for:
 * - Clock system configuration (PLL, etc.)
 * - Memory controller setup
 * - Cache/MPU configuration
 *
 * CAN access global variables now.
 */
[[gnu::weak]]
void pre_main_init() {
    // Default: do nothing
}

/**
 * @brief Called FROM main() via board::init()
 *
 * Use for:
 * - Peripheral initialization
 * - Board-specific setup
 * - Application-level init
 *
 * This is the recommended hook for most use cases.
 */
[[gnu::weak]]
void late_init() {
    // Default: do nothing
}

} // extern "C"

} // namespace alloy::hal::arm
```

### 1.3.4 Modern Startup Implementation

**File**: `src/hal/vendors/arm/cortex_m7/startup_impl.hpp`

```cpp
#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "init_hooks.hpp"

namespace alloy::hal::arm {

/**
 * @brief Modern C++23 startup implementation
 */
class StartupImpl {
public:
    /**
     * @brief Initialize .data section (copy from flash to RAM)
     */
    static void init_data_section(
        uint32_t* src_start,
        uint32_t* dst_start,
        uint32_t* dst_end
    ) {
        // Modern C++: use std::copy
        std::copy(
            src_start,
            src_start + (dst_end - dst_start),
            dst_start
        );
    }

    /**
     * @brief Initialize .bss section (zero memory)
     */
    static void init_bss_section(
        uint32_t* start,
        uint32_t* end
    ) {
        // Modern C++: use std::fill
        std::fill(start, end, 0);
    }

    /**
     * @brief Call static constructors
     */
    static void call_init_array(
        void (**start)(),
        void (**end)()
    ) {
        // Call each constructor in order
        std::for_each(start, end, [](auto fn) {
            if (fn) fn();
        });
    }

    /**
     * @brief Full startup sequence
     */
    template<typename Config>
    [[noreturn]]
    static void startup_sequence() {
        // 1. Early initialization (before .data/.bss)
        early_init();

        // 2. Initialize .data section
        init_data_section(
            Config::data_src_start(),
            Config::data_dst_start(),
            Config::data_dst_end()
        );

        // 3. Initialize .bss section
        init_bss_section(
            Config::bss_start(),
            Config::bss_end()
        );

        // 4. Call static constructors
        call_init_array(
            Config::init_array_start(),
            Config::init_array_end()
        );

        // 5. Pre-main initialization
        pre_main_init();

        // 6. Call main
        extern int main();
        main();

        // 7. If main returns, infinite loop
        while (true) {}
    }
};

} // namespace alloy::hal::arm
```

### 1.3.5 MCU-Specific Configuration

**File**: `src/hal/vendors/arm/same70/startup_config.hpp`

```cpp
#pragma once
#include <cstdint>

namespace alloy::hal::same70 {

/**
 * @brief SAME70-specific startup configuration
 *
 * Provides memory layout information from linker script
 */
struct StartupConfig {
    // Linker script symbols
    extern uint32_t _sidata;  // .data source in flash
    extern uint32_t _sdata;   // .data start in RAM
    extern uint32_t _edata;   // .data end in RAM
    extern uint32_t _sbss;    // .bss start
    extern uint32_t _ebss;    // .bss end
    extern uint32_t _estack;  // Stack top
    extern void (*__init_array_start)();
    extern void (*__init_array_end)();

    // Accessor methods
    static constexpr auto data_src_start() { return &_sidata; }
    static constexpr auto data_dst_start() { return &_sdata; }
    static constexpr auto data_dst_end() { return &_edata; }
    static constexpr auto bss_start() { return &_sbss; }
    static constexpr auto bss_end() { return &_ebss; }
    static constexpr auto stack_top() { return reinterpret_cast<uintptr_t>(&_estack); }
    static constexpr auto init_array_start() { return &__init_array_start; }
    static constexpr auto init_array_end() { return &__init_array_end; }
};

} // namespace alloy::hal::same70
```

### 1.3.6 Complete Example

**File**: `src/hal/vendors/arm/same70/startup_same70.cpp`

```cpp
#include "hal/vendors/arm/cortex_m7/startup_impl.hpp"
#include "hal/vendors/arm/cortex_m7/vector_table.hpp"
#include "startup_config.hpp"

using namespace alloy::hal::arm;
using namespace alloy::hal::same70;

// =============================================================================
// Handlers
// =============================================================================

extern "C" void Default_Handler() {
    while (true) {}
}

extern "C" [[noreturn]] void Reset_Handler() {
    StartupImpl::startup_sequence<StartupConfig>();
}

// =============================================================================
// Vector Table
// =============================================================================

// Build vector table at compile time
constexpr auto vector_table = make_vector_table<16 + 80>()  // 16 standard + 80 SAME70 IRQs
    .set_stack_pointer(StartupConfig::stack_top())
    .set_handler(1, &Reset_Handler)
    .set_handler(2, &Default_Handler)  // NMI
    .set_handler(3, &Default_Handler)  // HardFault
    // ... etc
    .get();

// Place in .isr_vector section
__attribute__((section(".isr_vector"), used))
const auto vectors = vector_table;
```

---

# Part 2: Per-MCU Startup Generation

## 2.1 Goals

- Auto-generate startup code from metadata
- Similar to hardware policy generation
- Per-MCU customization support
- Integration with existing codegen infrastructure

## 2.2 Metadata Format

**File**: `metadata/platform/same70_startup.json`

```json
{
  "family": "same70",
  "mcu": "ATSAME70Q21B",
  "arch": "cortex-m7",

  "memory": {
    "flash": {
      "base": "0x00400000",
      "size": "0x00200000"
    },
    "sram": {
      "base": "0x20400000",
      "size": "0x00060000"
    },
    "stack_size": "0x00001000"
  },

  "vector_table": {
    "standard_exceptions": 16,
    "irq_count": 80,
    "handlers": [
      {"index": 16, "name": "SUPC_Handler", "description": "Supply Controller"},
      {"index": 17, "name": "RSTC_Handler", "description": "Reset Controller"},
      {"index": 18, "name": "RTC_Handler", "description": "Real-time Clock"},
      {"index": 19, "name": "RTT_Handler", "description": "Real-time Timer"},
      {"index": 20, "name": "WDT_Handler", "description": "Watchdog Timer"},
      {"index": 21, "name": "PMC_Handler", "description": "Power Management Controller"},
      {"index": 22, "name": "EEFC_Handler", "description": "Enhanced Embedded Flash Controller"},
      {"index": 23, "name": "UART0_Handler", "description": "UART 0"},
      {"index": 24, "name": "UART1_Handler", "description": "UART 1"},
      {"index": 25, "name": "PIOA_Handler", "description": "PIO Controller A"},
      {"index": 26, "name": "PIOB_Handler", "description": "PIO Controller B"}
    ]
  },

  "clock": {
    "default_cpu_freq": 300000000,
    "default_systick_freq": 1000
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
  }
}
```

## 2.3 Jinja2 Template

**File**: `tools/codegen/cli/generators/templates/startup.cpp.j2`

```jinja2
/**
 * @file startup_{{ family }}.cpp
 * @brief Auto-generated startup code for {{ mcu }}
 *
 * Generated from: metadata/platform/{{ family }}_startup.json
 * Generator: tools/codegen/cli/generators/startup_generator.py
 *
 * DO NOT EDIT THIS FILE MANUALLY!
 */

#include "hal/vendors/arm/{{ arch }}/startup_impl.hpp"
#include "hal/vendors/arm/{{ arch }}/vector_table.hpp"
#include "startup_config.hpp"

using namespace alloy::hal::arm;
using namespace alloy::hal::{{ family }};

// =============================================================================
// Exception Handlers
// =============================================================================

extern "C" void Default_Handler() {
    while (true) {}
}

extern "C" [[noreturn]] void Reset_Handler() {
    StartupImpl::startup_sequence<StartupConfig>();
}

{% for handler in vector_table.handlers %}
/**
 * @brief {{ handler.description }}
 */
extern "C" [[gnu::weak, gnu::alias("Default_Handler")]]
void {{ handler.name }}();
{% endfor %}

// =============================================================================
// Vector Table
// =============================================================================

constexpr auto vector_table = make_vector_table<{{ vector_table.standard_exceptions + vector_table.irq_count }}>()
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
{% for handler in vector_table.handlers %}
    .set_handler({{ handler.index }}, &{{ handler.name }})
{% endfor %}
    .get();

// Place in .isr_vector section
__attribute__((section(".isr_vector"), used))
const auto vectors = vector_table;

/**
 * @brief Startup configuration for {{ mcu }}
 *
 * Memory layout:
 * - Flash: {{ memory.flash.base }} - {{ memory.flash.size }}
 * - SRAM:  {{ memory.sram.base }} - {{ memory.sram.size }}
 * - Stack: {{ memory.stack_size }}
 *
 * Clock:
 * - CPU: {{ clock.default_cpu_freq }} Hz
 * - SysTick: {{ clock.default_systick_freq }} Hz
 */
```

## 2.4 Generator Script

**File**: `tools/codegen/cli/generators/startup_generator.py`

```python
#!/usr/bin/env python3
"""
Startup Code Generator

Generates MCU-specific startup code from JSON metadata.
Similar to hardware_policy_generator.py but for startup code.
"""

import json
import sys
from pathlib import Path
from jinja2 import Environment, FileSystemLoader

class StartupGenerator:
    def __init__(self, metadata_dir: Path, template_dir: Path, output_dir: Path):
        self.metadata_dir = metadata_dir
        self.output_dir = output_dir
        self.env = Environment(
            loader=FileSystemLoader(template_dir),
            trim_blocks=True,
            lstrip_blocks=True
        )

    def load_metadata(self, family: str) -> dict:
        """Load startup metadata for a family"""
        metadata_file = self.metadata_dir / f"{family}_startup.json"

        if not metadata_file.exists():
            raise FileNotFoundError(f"Metadata not found: {metadata_file}")

        with open(metadata_file, 'r') as f:
            return json.load(f)

    def generate(self, family: str) -> bool:
        """Generate startup code for a family"""
        try:
            # Load metadata
            metadata = self.load_metadata(family)

            # Load template
            template = self.env.get_template('startup.cpp.j2')

            # Render
            output = template.render(**metadata)

            # Write output
            output_file = self.output_dir / f"startup_{family}.cpp"
            output_file.parent.mkdir(parents=True, exist_ok=True)

            with open(output_file, 'w') as f:
                f.write(output)

            print(f"✅ Generated: {output_file}")
            return True

        except Exception as e:
            print(f"❌ Error generating {family}: {e}", file=sys.stderr)
            return False

def main():
    if len(sys.argv) != 2:
        print("Usage: startup_generator.py <family>")
        sys.exit(1)

    family = sys.argv[1]

    # Paths
    root = Path(__file__).parent.parent.parent.parent.parent
    metadata_dir = root / "metadata" / "platform"
    template_dir = root / "tools" / "codegen" / "cli" / "generators" / "templates"
    output_dir = root / "src" / "hal" / "vendors" / "arm" / family

    # Generate
    generator = StartupGenerator(metadata_dir, template_dir, output_dir)
    success = generator.generate(family)

    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
```

## 2.5 Batch Generation Script

**File**: `tools/codegen/cli/generators/generate_startup.sh`

```bash
#!/bin/bash
# Generate startup code for a family

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <family>"
    echo "Example: $0 same70"
    exit 1
fi

FAMILY="$1"

echo "🚀 Generating startup code for $FAMILY..."
python3 "$SCRIPT_DIR/startup_generator.py" "$FAMILY"

echo "✅ Done!"
```

---

# Part 3: Cleanup Legacy Code

## 3.1 Goals

- Remove deprecated interrupt/systick classes
- Clean up old startup code
- Migrate to new hardware policies
- Update documentation

## 3.2 Files to Audit

### 3.2.1 Search for Legacy Code

```bash
# Find old interrupt classes
find src/hal -name "*interrupt*" -o -name "*irq*" | \
  grep -v "hardware_policy" | \
  grep -v "registers"

# Find old systick classes
find src/hal -name "*systick*" -o -name "*sys_tick*" | \
  grep -v "hardware_policy" | \
  grep -v "registers"

# Find old startup code
find src/hal -path "*/startup/*" -type f
```

### 3.2.2 Likely Candidates for Removal

Based on typical embedded C++ project structure:

```
src/hal/vendors/arm/same70/startup/
├── interrupt_manager.hpp     # Old class-based interrupt manager
├── interrupt_manager.cpp
├── systick.hpp               # Old SysTick wrapper
├── systick.cpp
├── startup_old.cpp           # Legacy startup
└── vector_table_old.cpp      # Old vector table
```

## 3.3 Migration Strategy

### Step 1: Identify All References

```bash
# Find all references to old classes
grep -r "InterruptManager" src/
grep -r "SysTick::" src/ | grep -v "SysTickHardware"
grep -r "#include.*startup/" src/
```

### Step 2: Create Migration Map

| Old API | New API | Notes |
|---------|---------|-------|
| `InterruptManager::enable(irq)` | `NVICHardware::enable_irq(irq)` | Direct hardware policy |
| `SysTick::init(ms)` | `SysTickHardware::configure_ms(ms)` | Hardware policy |
| `SysTick::get_ticks()` | `board::millis()` | Board abstraction |
| `#include "startup/..."` | `#include "hal/.../startup_impl.hpp"` | New location |

### Step 3: Update Examples

Update all examples to use new APIs:

```cpp
// OLD
#include "hal/vendors/arm/same70/startup/systick.hpp"
SysTick::init(1); // 1ms tick

// NEW
#include "boards/same70_xplained/board.hpp"
board::init(); // Handles SysTick setup
```

### Step 4: Remove Files

After confirming no references:

```bash
# Remove old startup directory
rm -rf src/hal/vendors/arm/same70/startup/

# Remove old interrupt classes
rm -f src/hal/vendors/arm/*/interrupt_manager.*

# Remove old systick classes
rm -f src/hal/vendors/arm/*/systick.{hpp,cpp}
```

### Step 5: Update Documentation

Update all documentation to reference new APIs:
- README files
- Architecture docs
- Migration guides
- Examples

## 3.4 Verification

```bash
# Ensure no references to old code
grep -r "InterruptManager" src/ && echo "❌ Found references" || echo "✅ Clean"
grep -r "startup/" src/ | grep include && echo "❌ Found includes" || echo "✅ Clean"

# Build all examples
cd examples
make -f Makefile.same70_board
# Should build successfully with new APIs
```

---

# Integration Points

## 4.1 With Existing Code

### Hardware Policies
- New startup uses existing hardware policies (SysTick, NVIC, PIO)
- No changes to hardware policy interface

### Board Abstraction
- Board layer calls new startup via `board::init()`
- Uses initialization hooks for configuration

### Examples
- Examples updated to use board abstraction
- Direct hardware access still possible for advanced users

## 4.2 Build System

### Makefile Changes

```makefile
# Add generated startup to build
CXX_SOURCES += \
    $(HAL_DIR)/vendors/arm/same70/startup_same70.cpp \
    $(BOARD_DIR)/board.cpp
```

### Generation Step

```makefile
# Generate startup before build
$(BUILD_DIR)/startup_same70.o: $(HAL_DIR)/vendors/arm/same70/startup_same70.cpp
    @echo "Note: Regenerate with ./tools/codegen/cli/generators/generate_startup.sh same70"
    $(CXX) $(CXXFLAGS) -c $< -o $@
```

---

# Testing Strategy

## 5.1 Unit Tests

Test individual components:

```cpp
// Test vector table builder
TEST(VectorTable, BasicConstruction) {
    constexpr auto vt = make_vector_table<16>()
        .set_stack_pointer(0x20000000)
        .get();

    EXPECT_EQ(vt[0], 0x20000000);
}

// Test startup sequence
TEST(Startup, DataInitialization) {
    uint32_t src[] = {1, 2, 3};
    uint32_t dst[3] = {0};

    StartupImpl::init_data_section(src, dst, dst + 3);

    EXPECT_EQ(dst[0], 1);
    EXPECT_EQ(dst[1], 2);
    EXPECT_EQ(dst[2], 3);
}
```

## 5.2 Integration Tests

Test complete startup on hardware:

```cpp
// Test LED blink with new startup
int main() {
    board::init();  // Uses new startup

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

## 5.3 Regression Tests

Ensure existing examples still work:

```bash
# Build all examples
cd examples
for makefile in Makefile.*; do
    make -f "$makefile" clean all
done
```

## 5.4 Binary Size Comparison

Compare binary sizes before/after:

```bash
# Before
arm-none-eabi-size build/example_old.elf

# After
arm-none-eabi-size build/example_new.elf

# Should be similar or smaller
```

---

# Success Metrics

| Metric | Target | Verification |
|--------|--------|--------------|
| Startup modernized | C++23 features | Code review |
| Auto-generation works | Generates for SAME70 | Run generator |
| Legacy code removed | 0 deprecated files | File audit |
| Examples work | All build & run | Test on hardware |
| Binary size | Same or smaller | Size comparison |
| Boot time | Same or faster | Timing measurement |
| Documentation | Complete | Doc review |

---

**Status**: Ready for implementation
**Estimated Effort**: 13-19 hours
**Risk Level**: LOW (well-defined, proven patterns)
