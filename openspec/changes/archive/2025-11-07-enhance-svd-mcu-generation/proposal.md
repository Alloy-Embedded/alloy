# Proposal: Enhance SVD-based MCU Generation System

## Problem Statement

The current SVD-based code generation system has several limitations that make it difficult for users to adopt new MCUs and for maintainers to track supported devices:

### Current Issues

1. **Limited SVD Sources**: Only relies on upstream `cmsis-svd-data` repository, missing many vendor-specific and community SVD files
2. **Manual Board Configuration**: Users must manually configure GPIO pins and peripherals in board files, even though this information could be generated
3. **No MCU-Specific Pin Definitions**: GPIO classes are family-level, requiring users to know pin mappings for their specific MCU variant
4. **Poor Discoverability**: No easy way to see which MCUs are supported or what peripherals are available
5. **Scattered GPIO Logic**: Pin definitions mixed with board files instead of being part of the MCU's GPIO implementation
6. **No Validation**: Missing SVD files or incomplete definitions aren't caught until compilation

### User Impact

When a user wants to use STM32F103C8 (64KB):
- ❌ Must manually list all available pins in board file
- ❌ No compile-time validation that pins exist on this variant
- ❌ Can't easily discover which pins/peripherals are available
- ❌ Can't determine if the library supports their specific MCU

## Proposed Solution

Create a **comprehensive MCU support system** that:

1. **Multi-Source SVD Repository**
   - Keep upstream cmsis-svd-data as primary source
   - Add `custom-svd/` directory for community/vendor SVDs
   - Automated merging and conflict resolution

2. **MCU-Specific GPIO Pin Definitions**
   - Generate per-MCU pin trait classes (e.g., `stm32f103c8::pins`)
   - Compile-time pin validation using C++20 concepts
   - Auto-generated pin enumerations (PA0, PA1, ..., PC13)
   - Package-specific pin availability (LQFP48, LQFP64, etc.)

3. **Template-Based MCU Instantiation**
   - Use C++20 traits and concepts for MCU configuration
   - Board files simply select MCU and use its pins
   - Zero runtime overhead, all compile-time

4. **Automated Support Matrix**
   - Python script scans generated code
   - Generates README table with all supported MCUs
   - Shows available peripherals per MCU (GPIO, UART, I2C, SPI, ADC, etc.)
   - Links to datasheets and SVD sources

5. **Improved Code Generation Pipeline**
   - Enhanced `svd_parser.py` to extract pin/package info
   - New `generate_mcu_pins.py` for pin definitions
   - Validation tool to check completeness

## Benefits

### For End Users
✅ **Discover**ability: `make list-mcus` shows all supported devices
✅ **Type Safety**: Compile error if using non-existent pin for MCU variant
✅ **Ease of Use**: Just select MCU, get all pins automatically
✅ **Documentation**: Auto-generated tables show what's supported

### For Maintainers
✅ **Scalability**: Easy to add new MCU variants
✅ **Validation**: Automated checks for missing SVDs or incomplete definitions
✅ **Visibility**: Clear tracking of supported MCUs and peripherals
✅ **Community**: Accept community-contributed SVD files

### Example: Before vs After

**Before** (manual board configuration):
```cpp
// cmake/boards/bluepill.cmake
set(ALLOY_MCU "STM32F103C8")  // Which pins does C8 have? User must know!

// board.hpp - user must manually list pins
namespace board {
    constexpr auto LED = 13;  // PC13 - is this valid for C8 variant?
    // What other pins are available? User must check datasheet
}
```

**After** (auto-generated from SVD):
```cpp
// cmake/boards/bluepill.cmake
set(ALLOY_MCU "STM32F103C8")  // Automatically loads stm32f103c8::pins

// board.hpp - type-safe, auto-complete friendly
#include <hal/st/stm32f1/stm32f103c8/pins.hpp>

namespace board {
    using namespace stm32f103c8::pins;  // All pins for this MCU

    constexpr auto LED = pins::PC13;  // Type-safe, compile-time checked
    constexpr auto UART_TX = pins::PA9;   // IDE auto-complete shows available pins
    constexpr auto UART_RX = pins::PA10;

    // Compile error if pin doesn't exist on LQFP48 package:
    // constexpr auto BAD = pins::PG15;  // ❌ Error: PG15 not available on STM32F103C8
}
```

**Support Matrix** (auto-generated in README):
```markdown
## Supported MCUs

### STMicroelectronics STM32F1
| MCU | Package | Flash | RAM | GPIO | UART | I2C | SPI | ADC | Status |
|-----|---------|-------|-----|------|------|-----|-----|-----|--------|
| STM32F103C8 | LQFP48 | 64KB | 20KB | 37 pins | 3 | 2 | 2 | 10ch | ✅ Full |
| STM32F103CB | LQFP48 | 128KB | 20KB | 37 pins | 3 | 2 | 2 | 10ch | ✅ Full |
| STM32F103RC | LQFP64 | 256KB | 48KB | 51 pins | 5 | 2 | 3 | 16ch | ✅ Full |
```

## Scope

### In Scope
- Multi-source SVD repository structure
- MCU-specific pin definition generation
- Compile-time pin validation with concepts
- Support matrix generation tool
- Documentation and examples
- Migration guide for existing boards

### Out of Scope (Future)
- Peripheral-specific pin muxing (e.g., which pins can be UART TX)
- Automated board detection from connected hardware
- GUI tool for MCU/pin selection
- Runtime pin validation

## Dependencies

- C++20 compiler (for concepts and constexpr)
- Python 3.9+ (for enhanced svd_parser.py)
- Existing codegen infrastructure

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| SVD file quality varies | Generated code may be incomplete | Validation tool to detect missing fields |
| Pin naming inconsistencies | Confusing pin names | Standardization layer in generator |
| Breaking changes to board files | Existing projects break | Provide migration tool and backwards compatibility layer |
| Increased build time | Slower compilation | Lazy instantiation, only generate used MCUs |

## Success Criteria

1. ✅ At least 3 STM32F1 variants with full pin definitions
2. ✅ Support matrix auto-generates correctly
3. ✅ Zero runtime overhead vs manual configuration
4. ✅ Documentation with migration guide
5. ✅ Validation catches missing SVD files
6. ✅ User can add custom SVD and regenerate

## Timeline Estimate

- **Phase 1** (Custom SVD support): 2-3 days
- **Phase 2** (Pin generation): 3-4 days
- **Phase 3** (Support matrix tool): 1-2 days
- **Phase 4** (Documentation & examples): 1-2 days
- **Total**: 1-2 weeks

## Open Questions

1. Should we support runtime pin validation for debugging?
2. How to handle MCU variants with identical pinouts?
3. Should pin definitions include alternate functions (TIM, PWM, etc.)?
4. How to handle community SVD contributions (review process)?
