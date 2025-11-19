# Design: Enhanced SVD-based MCU Generation System

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    SVD Source Management                         │
├─────────────────────────────────────────────────────────────────┤
│  Upstream CMSIS-SVD    │    Custom SVD Repository               │
│  (cmsis-svd-data)      │    (tools/codegen/custom-svd/)        │
│                        │                                         │
│  - 24+ vendors         │    - Community contributions           │
│  - 800+ MCUs           │    - Vendor-specific variants          │
│  - Auto-synced         │    - Custom/experimental MCUs          │
└────────────┬───────────┴──────────────┬──────────────────────────┘
             │                          │
             └──────────┬───────────────┘
                        ▼
         ┌──────────────────────────────┐
         │   svd_parser.py (Enhanced)   │
         │                              │
         │  Extracts:                   │
         │  - Peripherals               │
         │  - Registers                 │
         │  - Pin definitions ✨NEW     │
         │  - Package info ✨NEW        │
         │  - Memory layout             │
         └──────────────┬───────────────┘
                        ▼
         ┌──────────────────────────────┐
         │  JSON Database (Enriched)    │
         │                              │
         │  database/families/          │
         │    stm32f103.json            │
         │      ├─ peripherals          │
         │      ├─ pins ✨NEW           │
         │      └─ variants ✨NEW       │
         └──────────────┬───────────────┘
                        ▼
         ┌──────────────────────────────┐
         │   Code Generation Layer      │
         ├──────────────────────────────┤
         │  generate_peripherals.py     │
         │  generate_mcu_pins.py ✨NEW  │
         │  generate_traits.py ✨NEW    │
         └──────────────┬───────────────┘
                        ▼
┌────────────────────────────────────────────────────────────────┐
│              Generated C++ Code Structure                       │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  src/hal/st/stm32f1/                                           │
│    ├─ gpio.hpp                    (Family-level GPIO impl)     │
│    ├─ generated/                                               │
│    │   ├─ peripherals.hpp         (GPIO, UART, SPI regs)      │
│    │   └─ stm32f103c8/           (MCU-specific) ✨NEW         │
│    │       ├─ pins.hpp            (All pins for C8 variant)   │
│    │       ├─ traits.hpp          (Memory, flash, package)    │
│    │       └─ peripherals.hpp     (Instance counts)           │
│    └─ stm32f103cb/                                            │
│        └─ (similar structure)                                  │
│                                                                 │
│  cmake/boards/bluepill.cmake                                   │
│    set(ALLOY_MCU "stm32f103c8")  ← Selects MCU variant       │
│                                                                 │
│  examples/bluepill/board.hpp                                   │
│    #include <hal/st/stm32f1/stm32f103c8/pins.hpp>            │
│    using namespace stm32f103c8::pins;                         │
│    constexpr auto LED = pins::PC13;  ← Type-safe!            │
└────────────────────────────────────────────────────────────────┘
```

## Component Design

### 1. Custom SVD Repository

**Structure:**
```
tools/codegen/custom-svd/
├── README.md                    # How to contribute
├── vendors/
│   ├── STMicro/
│   │   ├── STM32F103C8.svd     # Specific variant
│   │   ├── STM32F103CB.svd
│   │   └── README.md           # Source/license info
│   ├── Nordic/
│   │   └── nRF52833_custom.svd
│   └── Community/              # User contributions
│       └── CustomBoard.svd
└── merge_policy.json            # Conflict resolution rules
```

**Merge Policy:**
```json
{
    "priority": ["custom-svd", "upstream"],
    "conflict_resolution": {
        "peripherals": "merge",
        "registers": "custom_override",
        "pins": "merge_unique"
    }
}
```

### 2. Enhanced SVD Parser

**New Fields Extracted:**

```python
# svd_parser.py additions
class PinDefinition:
    name: str           # "PC13"
    port: str           # "C"
    pin_number: int     # 13
    alternate_functions: List[str]  # ["TIM1_CH1", "SPI1_MOSI"]
    package_availability: Dict[str, bool]  # {"LQFP48": True, "LQFP64": True}

class McuVariant:
    name: str           # "STM32F103C8"
    flash_size: int     # 64 * 1024
    ram_size: int       # 20 * 1024
    package: str        # "LQFP48"
    pins: List[PinDefinition]
    peripheral_counts: Dict[str, int]  # {"USART": 3, "I2C": 2}
```

**Enhanced Database Schema:**
```json
{
    "family": "stm32f1",
    "variants": [
        {
            "name": "STM32F103C8",
            "package": "LQFP48",
            "flash_kb": 64,
            "ram_kb": 20,
            "pins": {
                "PA0": {"port": "A", "pin": 0, "packages": ["LQFP48", "LQFP64"]},
                "PA1": {"port": "A", "pin": 1, "packages": ["LQFP48", "LQFP64"]},
                "PC13": {"port": "C", "pin": 13, "packages": ["LQFP48"]},
                "PG15": {"port": "G", "pin": 15, "packages": ["LQFP64"]}
            },
            "peripherals": {
                "GPIO_ports": 7,
                "USART": 3,
                "I2C": 2,
                "SPI": 2,
                "ADC": 2,
                "ADC_channels": 10
            }
        }
    ]
}
```

### 3. MCU Pin Generation

**Generated Code Structure:**

```cpp
// src/hal/st/stm32f1/stm32f103c8/pins.hpp
#ifndef ALLOY_HAL_STM32F1_STM32F103C8_PINS_HPP
#define ALLOY_HAL_STM32F1_STM32F103C8_PINS_HPP

#include <cstdint>
#include <type_traits>

namespace alloy::hal::stm32f1::stm32f103c8 {

/// Pin definitions for STM32F103C8 (LQFP48 package, 64KB Flash, 20KB RAM)
/// Auto-generated from: STM32F103C8.svd
/// Package: LQFP48 (48 pins)
/// GPIO Pins: 37 pins available
namespace pins {

// Port A pins (16 pins)
constexpr uint8_t PA0  = 0;
constexpr uint8_t PA1  = 1;
constexpr uint8_t PA2  = 2;
constexpr uint8_t PA3  = 3;
constexpr uint8_t PA4  = 4;
constexpr uint8_t PA5  = 5;
constexpr uint8_t PA6  = 6;
constexpr uint8_t PA7  = 7;
constexpr uint8_t PA8  = 8;
constexpr uint8_t PA9  = 9;
constexpr uint8_t PA10 = 10;
constexpr uint8_t PA11 = 11;
constexpr uint8_t PA12 = 12;
constexpr uint8_t PA13 = 13;  // SWDIO
constexpr uint8_t PA14 = 14;  // SWCLK
constexpr uint8_t PA15 = 15;

// Port B pins (16 pins)
constexpr uint8_t PB0  = 16;
// ... etc

// Port C pins (3 pins available on LQFP48)
constexpr uint8_t PC13 = 45;
constexpr uint8_t PC14 = 46;
constexpr uint8_t PC15 = 47;

// Port D pins (2 pins available on LQFP48)
constexpr uint8_t PD0  = 48;  // OSC_IN
constexpr uint8_t PD1  = 49;  // OSC_OUT

/// Pin validation concept
template<uint8_t Pin>
struct is_valid_pin : std::bool_constant<
    (Pin >= PA0 && Pin <= PA15) ||
    (Pin >= PB0 && Pin <= PB15) ||
    (Pin >= PC13 && Pin <= PC15) ||
    (Pin >= PD0 && Pin <= PD1)
> {};

template<uint8_t Pin>
constexpr bool is_valid_pin_v = is_valid_pin<Pin>::value;

/// Compile-time pin validation
template<uint8_t Pin>
concept ValidPin = is_valid_pin_v<Pin>;

/// Get port enum from pin number (constexpr)
constexpr Port get_port(uint8_t pin) {
    if (pin <= PA15) return Port::A;
    if (pin >= PB0 && pin <= PB15) return Port::B;
    if (pin >= PC13 && pin <= PC15) return Port::C;
    if (pin >= PD0 && pin <= PD1) return Port::D;
    return Port::A;  // Should never reach here if ValidPin concept is used
}

/// Get pin bit (0-15) within port
constexpr uint8_t get_pin_bit(uint8_t pin) {
    return pin % 16;
}

} // namespace pins

/// MCU traits
struct Traits {
    static constexpr const char* name = "STM32F103C8";
    static constexpr const char* package = "LQFP48";
    static constexpr uint32_t flash_size = 64 * 1024;
    static constexpr uint32_t ram_size = 20 * 1024;
    static constexpr uint32_t gpio_pin_count = 37;

    static constexpr uint32_t usart_count = 3;
    static constexpr uint32_t i2c_count = 2;
    static constexpr uint32_t spi_count = 2;
    static constexpr uint32_t adc_count = 2;
    static constexpr uint32_t adc_channel_count = 10;
    static constexpr uint32_t timer_count = 7;
};

} // namespace alloy::hal::stm32f1::stm32f103c8

#endif
```

### 4. Family GPIO Integration

**Updated gpio.hpp** to use MCU-specific pins:

```cpp
// src/hal/st/stm32f1/gpio.hpp
#ifndef ALLOY_HAL_STM32F1_GPIO_HPP
#define ALLOY_HAL_STM32F1_GPIO_HPP

#include "../../interface/gpio.hpp"

// Include MCU-specific pins based on CMake configuration
#if defined(ALLOY_MCU_STM32F103C8)
    #include "stm32f103c8/pins.hpp"
    namespace mcu = alloy::hal::stm32f1::stm32f103c8;
#elif defined(ALLOY_MCU_STM32F103CB)
    #include "stm32f103cb/pins.hpp"
    namespace mcu = alloy::hal::stm32f1::stm32f103cb;
#elif defined(ALLOY_MCU_STM32F103RC)
    #include "stm32f103rc/pins.hpp"
    namespace mcu = alloy::hal::stm32f1::stm32f103rc;
#else
    #error "No MCU variant defined. Set ALLOY_MCU in CMakeLists.txt"
#endif

namespace alloy::hal::stm32f1 {

/// GPIO class using MCU-specific pins
template<uint8_t Pin>
    requires mcu::pins::ValidPin<Pin>  // Compile-time validation!
class Gpio : public interface::Gpio {
public:
    static constexpr auto port = mcu::pins::get_port(Pin);
    static constexpr auto pin_bit = mcu::pins::get_pin_bit(Pin);

    static void configure(Mode mode, Pull pull) {
        enable_gpio_clock(port);
        // ... configure using generated registers
    }

    static void set() {
        auto* regs = get_gpio_port(port);
        regs->BSRR = (1U << pin_bit);
    }

    // ... rest of implementation
};

} // namespace alloy::hal::stm32f1

#endif
```

### 5. Board Configuration

**Simplified board.hpp**:

```cpp
// examples/bluepill/board.hpp
#ifndef BOARD_HPP
#define BOARD_HPP

#include <hal/st/stm32f1/gpio.hpp>
#include <hal/st/stm32f1/stm32f103c8/pins.hpp>  // MCU-specific pins

namespace board {

// Import all pins from MCU definition
using namespace alloy::hal::stm32f1::stm32f103c8::pins;

// Board-specific pin assignments
constexpr auto LED = PC13;       // Type-safe! Compile error if PC13 doesn't exist
constexpr auto UART_TX = PA9;    // Type-safe!
constexpr auto UART_RX = PA10;   // Type-safe!
constexpr auto I2C_SCL = PB6;    // Type-safe!
constexpr auto I2C_SDA = PB7;    // Type-safe!

// This would cause compile error:
// constexpr auto BAD_PIN = PG15;  // ❌ Error: PG15 not available on STM32F103C8 (LQFP48)

// Create type-safe GPIO instances
using Led = alloy::hal::stm32f1::Gpio<LED>;
using UartTx = alloy::hal::stm32f1::Gpio<UART_TX>;
using UartRx = alloy::hal::stm32f1::Gpio<UART_RX>;

} // namespace board

#endif
```

### 6. Support Matrix Generator

**Python Tool: `generate_support_matrix.py`**

```python
#!/usr/bin/env python3
"""Generate MCU support matrix for README"""

import json
import glob
from pathlib import Path

def scan_generated_mcus():
    """Scan src/hal for generated MCU definitions"""
    mcus = []

    for pins_file in Path("src/hal").rglob("*/pins.hpp"):
        # Extract info from generated file
        with open(pins_file) as f:
            content = f.read()
            # Parse comments for MCU info
            mcu_info = extract_mcu_info(content)
            mcus.append(mcu_info)

    return sorted(mcus, key=lambda x: (x['vendor'], x['family'], x['name']))

def generate_markdown_table(mcus):
    """Generate markdown table"""
    lines = ["## Supported MCUs\n"]

    by_vendor = group_by(mcus, 'vendor')

    for vendor, vendor_mcus in by_vendor.items():
        lines.append(f"\n### {vendor}\n")
        lines.append("| MCU | Package | Flash | RAM | GPIO | UART | I2C | SPI | ADC | Status |")
        lines.append("|-----|---------|-------|-----|------|------|-----|-----|-----|--------|")

        for mcu in vendor_mcus:
            lines.append(
                f"| {mcu['name']} "
                f"| {mcu['package']} "
                f"| {mcu['flash_kb']}KB "
                f"| {mcu['ram_kb']}KB "
                f"| {mcu['gpio_pins']} pins "
                f"| {mcu['usart']} "
                f"| {mcu['i2c']} "
                f"| {mcu['spi']} "
                f"| {mcu['adc_ch']}ch "
                f"| {mcu['status']} |"
            )

    return "\n".join(lines)

if __name__ == "__main__":
    mcus = scan_generated_mcus()
    table = generate_markdown_table(mcus)

    # Update README.md
    update_readme_section("MCU_SUPPORT_MATRIX", table)
    print(f"✅ Updated support matrix with {len(mcus)} MCUs")
```

**Generated Table in README.md**:
```markdown
## Supported MCUs

### STMicroelectronics STM32F1
| MCU | Package | Flash | RAM | GPIO | UART | I2C | SPI | ADC | Status |
|-----|---------|-------|-----|------|------|-----|-----|-----|--------|
| STM32F103C6 | LQFP48 | 32KB | 10KB | 37 pins | 3 | 2 | 2 | 10ch | ✅ Full |
| STM32F103C8 | LQFP48 | 64KB | 20KB | 37 pins | 3 | 2 | 2 | 10ch | ✅ Full |
| STM32F103CB | LQFP48 | 128KB | 20KB | 37 pins | 3 | 2 | 2 | 10ch | ✅ Full |
| STM32F103R8 | LQFP64 | 64KB | 20KB | 51 pins | 5 | 2 | 3 | 16ch | ✅ Full |
| STM32F103RC | LQFP64 | 256KB | 48KB | 51 pins | 5 | 2 | 3 | 16ch | ✅ Full |
| STM32F103VE | LQFP100 | 512KB | 64KB | 80 pins | 5 | 2 | 3 | 16ch | ⚠️ GPIO Only |

### Nordic Semiconductor nRF52
| MCU | Package | Flash | RAM | GPIO | UART | I2C | SPI | ADC | Status |
|-----|---------|-------|-----|------|------|-----|-----|-----|--------|
| nRF52832 | QFN48 | 512KB | 64KB | 32 pins | 1 | 2 | 3 | 8ch | ✅ Full |
| nRF52833 | QFN48 | 512KB | 128KB | 32 pins | 2 | 2 | 4 | 8ch | ✅ Full |
| nRF52840 | QFN73 | 1MB | 256KB | 48 pins | 2 | 2 | 4 | 8ch | ✅ Full |
```

## Implementation Strategy

### Phase 1: Custom SVD Repository (2-3 days)
1. Create `tools/codegen/custom-svd/` structure
2. Add README with contribution guidelines
3. Implement merge policy in `svd_parser.py`
4. Add 3 STM32F1 variants as proof of concept

### Phase 2: Enhanced SVD Parser & Pin Generation (3-4 days)
1. Extend `svd_parser.py` to extract pin/package info
2. Create new `generate_mcu_pins.py` script
3. Generate pin definitions for STM32F103C8, C B, RC
4. Add compile-time validation concepts

### Phase 3: GPIO Integration (2 days)
1. Update family GPIO to use MCU-specific pins
2. Add CMake support for ALLOY_MCU variants
3. Update bluepill board to use new system
4. Test compile-time pin validation

### Phase 4: Support Matrix Tool (1-2 days)
1. Create `generate_support_matrix.py`
2. Scan generated code for MCU info
3. Generate markdown tables
4. Add Makefile target: `make update-mcu-matrix`

### Phase 5: Documentation & Migration (1-2 days)
1. Write migration guide for existing boards
2. Create examples for 3 different MCU variants
3. Update main README with support matrix
4. Add contribution guidelines for custom SVDs

## Trade-offs & Decisions

### Decision 1: Compile-time vs Runtime Validation
**Chosen**: Compile-time with C++20 concepts
**Rationale**: Zero runtime overhead, catch errors at compile time, better for embedded
**Trade-off**: Requires C++20, less flexible for dynamic pin assignment

### Decision 2: Per-MCU Files vs Single Database
**Chosen**: Per-MCU pin definition files
**Rationale**: Better IDE support, faster compilation (only include used MCU), clearer organization
**Trade-off**: More files, but organized and discoverable

### Decision 3: Custom SVD Location
**Chosen**: Separate `custom-svd/` directory
**Rationale**: Clear separation, easy to manage, doesn't interfere with upstream sync
**Trade-off**: Must maintain merge policy, but provides flexibility

### Decision 4: Template-based GPIO vs Inheritance
**Chosen**: Template-based with concepts
**Rationale**: Zero runtime overhead, compile-time safety, constexpr everything
**Trade-off**: More complex template code, but better performance and safety

## Validation & Testing

### Validation Tools
1. **SVD Completeness Check**: Verify all required fields present
2. **Pin Overlap Detection**: Ensure no duplicate pin definitions
3. **Peripheral Count Validation**: Match datasheet specifications
4. **Compilation Test**: Generate code and compile for all variants

### Test Cases
1. Compile with valid pin (should succeed)
2. Compile with invalid pin (should fail with clear error)
3. Verify pin numbers match datasheet
4. Verify peripheral counts match datasheet
5. Generate support matrix and verify accuracy

## Future Enhancements

1. **Peripheral Pin Muxing**: Add alternate function support
2. **DMA Channel Mapping**: Auto-generate DMA request mappings
3. **Clock Tree Generation**: Generate clock configuration code
4. **Power Modes**: Extract low-power mode capabilities
5. **Interrupt Vectors**: Auto-generate NVIC configuration
6. **GUI Tool**: Web-based MCU/pin selector and configurator
