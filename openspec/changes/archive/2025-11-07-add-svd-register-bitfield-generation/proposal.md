# Proposal: SVD-Based Register and Bitfield Code Generation

## Why

The current SVD code generation system extracts basic peripheral addresses and interrupt vectors but **ignores rich register-level data** available in SVD files. This forces developers to:

1. Manually define register structures and bit fields (error-prone, time-consuming)
2. Use magic numbers for register offsets and bit positions (unreadable, unmaintainable)
3. Lack compile-time type safety for register access (runtime bugs)
4. Duplicate CMSIS-style definitions manually instead of auto-generating from SVD

SVD files contain **complete register maps** with:
- 100+ registers per peripheral with offsets, sizes, reset values
- 157+ enumerated constants per MCU for register field values
- Bit field definitions with positions, widths, access modes
- Register access restrictions (read-only, write-only, read-write)

**This data is being parsed but not code-generated**, leaving 80% of SVD value unused.

## What Changes

Implement **CMSIS-equivalent register and bitfield generation** using modern C++20 with zero-overhead abstractions:

### 1. Typed Register Structures
Generate complete peripheral register maps as C++ structs:
```cpp
namespace stm32f103c8::peripherals::usart1 {
    struct Registers {
        volatile uint32_t SR;   // Status register @ +0x00
        volatile uint32_t DR;   // Data register @ +0x04
        volatile uint32_t BRR;  // Baud rate register @ +0x08
        volatile uint32_t CR1;  // Control register 1 @ +0x0C
        // ... all registers from SVD
    };

    constexpr Registers* regs = reinterpret_cast<Registers*>(0x40013800);
}
```

### 2. Bit Field Constants
Generate constexpr bit masks and positions:
```cpp
namespace stm32f103c8::usart::sr {
    constexpr uint32_t TXE_Pos  = 7;
    constexpr uint32_t TXE_Msk  = (1U << TXE_Pos);
    constexpr uint32_t RXNE_Pos = 5;
    constexpr uint32_t RXNE_Msk = (1U << RXNE_Pos);
}
```

### 3. Type-Safe Bit Field Accessors (Template-Based)
Zero-overhead compile-time bit manipulation:
```cpp
template<uint32_t Position, uint32_t Width>
struct BitField {
    static constexpr uint32_t mask = ((1U << Width) - 1) << Position;

    static constexpr uint32_t read(uint32_t reg) {
        return (reg & mask) >> Position;
    }

    static constexpr uint32_t write(uint32_t reg, uint32_t value) {
        return (reg & ~mask) | ((value << Position) & mask);
    }
};

// Usage in HAL:
using TXE = BitField<7, 1>;
if (TXE::read(USART1->SR)) { /* transmit empty */ }
```

### 4. Enumerated Register Values
Generate enum classes from SVD enumerations:
```cpp
enum class ADC_Resolution : uint8_t {
    BITS_12 = 0x0,  // 12-bit (15 ADCCLK cycles)
    BITS_10 = 0x1,  // 10-bit (13 ADCCLK cycles)
    BITS_8  = 0x2,  // 8-bit (11 ADCCLK cycles)
    BITS_6  = 0x3   // 6-bit (9 ADCCLK cycles)
};
```

### 5. Access Control Enforcement
Template-based read/write restrictions:
```cpp
template<typename T, AccessMode Mode>
struct Register {
    volatile T value;

    T read() requires (Mode == ReadOnly || Mode == ReadWrite) {
        return value;
    }

    void write(T v) requires (Mode == WriteOnly || Mode == ReadWrite) {
        value = v;
    }
};
```

### 6. Complete Pin Definitions
Extend current pin generation to include alternate functions from SVD:
```cpp
namespace stm32f103c8::pins {
    constexpr uint8_t PA9 = 9;

    namespace alternate_functions {
        template<> struct AF<PA9, USART1_TX> {
            static constexpr uint8_t value = 7;
        };
        template<> struct AF<PA9, TIM1_CH2> {
            static constexpr uint8_t value = 1;
        };
    }
}
```

### 7. Enhanced Code Generation Pipeline

**New Generators:**
- `generate_registers.py` - Register structures per peripheral
- `generate_bitfields.py` - Bit field constants and templates
- `generate_enums.py` - Enumerated values from SVD
- `generate_pin_functions.py` - Pin alternate function mappings

**Enhanced Parser:**
- Extract register definitions with offsets, sizes, reset values
- Parse bit field definitions with positions, widths, descriptions
- Extract enumerated values with symbolic names
- Parse access modes (read-only, write-only, read-write)

### Generated File Structure
```
src/hal/vendors/st/stm32f1/stm32f103c8/
├── peripherals.hpp         # (existing) Base addresses, memory map
├── pins.hpp               # (existing) Pin definitions
├── startup.cpp            # (existing) Vector table
├── registers/             # NEW: Complete register maps
│   ├── gpio_registers.hpp
│   ├── usart_registers.hpp
│   ├── timer_registers.hpp
│   ├── adc_registers.hpp
│   └── ...
├── bitfields/             # NEW: Bit field definitions
│   ├── gpio_bitfields.hpp
│   ├── usart_bitfields.hpp
│   └── ...
├── enums.hpp              # NEW: All enumerated constants
└── register_access.hpp    # NEW: Template utilities for type-safe access
```

### Benefits

**Professional C++ Features:**
- ✅ Zero runtime overhead (all `constexpr`, templates optimized away)
- ✅ Type safety at compile time (no magic numbers, enum classes)
- ✅ CMSIS-equivalent but with modern C++20 idioms
- ✅ Self-documenting (names and comments from SVD descriptions)
- ✅ IDE auto-complete friendly (IntelliSense shows all registers/fields)

**Maintainability:**
- ✅ Single source of truth (SVD files)
- ✅ Automatic updates when SVD changes
- ✅ Consistency across all vendors
- ✅ Eliminates manual register definition errors

**Developer Experience:**
```cpp
// Instead of:
*(volatile uint32_t*)(0x40013800 + 0x00) |= (1 << 7);  // ❌ Magic numbers

// Write:
using namespace stm32f103c8::usart1;
regs->SR |= sr::TXE_Msk;  // ✅ Type-safe, readable

// Or with templates (zero overhead):
BitField<7, 1>::set(regs->SR);  // ✅ Compile-time optimized
```

## Impact

### Affected Specs
- **NEW**: `codegen-svd-advanced` - Register and bitfield generation system
- **MODIFIED**: `enhance-svd-mcu-generation` - Extends pin generation with alternate functions

### Affected Code
- `tools/codegen/cli/parsers/generic_svd.py` - Enhanced register/bitfield parsing
- `tools/codegen/cli/generators/` - New register/bitfield/enum generators
- `tools/codegen/cli/core/config.py` - Add register generation templates
- `src/hal/vendors/**/` - New generated register files for all MCUs
- `examples/` - Update examples to use new type-safe register access

### Breaking Changes
**NONE** - This is additive. Existing code using base addresses continues to work. New code can opt into type-safe register access.

### Migration Path
Existing code works unchanged:
```cpp
// Old style still works:
*(volatile uint32_t*)0x40013800 = value;

// Migrate incrementally to:
stm32f103c8::usart1::regs->SR = value;
```

## Scope

### In Scope
- Register structure generation from SVD
- Bit field constant generation
- Type-safe bit field template utilities
- Enumerated value generation
- Pin alternate function mappings
- Access mode enforcement templates
- Documentation generation (comments from SVD descriptions)
- Examples demonstrating register access patterns

### Out of Scope (Future Enhancements)
- Hardware abstraction layer using generated registers (separate change)
- Peripheral driver templates (separate change)
- Runtime register validation/debugging tools
- SVD file editor/validator GUI
- Automatic linker script generation
- DMA request mapping generation

## Dependencies

- C++20 compiler (GCC 10+, Clang 12+)
- Python 3.9+ with `lxml` for SVD parsing
- Existing `generic_svd.py` parser infrastructure
- Jinja2 for template-based code generation

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| SVD register data incomplete/incorrect | Generated code missing registers | Validation tool to detect gaps, manual review process |
| Large generated files increase build time | Slower compilation | Header-only, lazy instantiation, use precompiled headers |
| Name collisions between vendors | Ambiguous register names | Namespace per vendor/family/MCU, strict naming conventions |
| Breaking changes if SVD schema evolves | Parser fails on new SVD versions | Version detection, backwards compatibility layer |
| Too many generated files clutter repo | Poor discoverability | Clear directory structure, index generation |

## Success Criteria

1. ✅ Generate complete register maps for STM32F1, SAMD21, RP2040, ESP32
2. ✅ Zero runtime overhead vs manual register access (assembly comparison)
3. ✅ Type-safe bit field access compiles to same code as direct bit manipulation
4. ✅ All 157+ enumerations per MCU generated as enum classes
5. ✅ Examples demonstrate register access replacing magic numbers
6. ✅ Documentation explains template usage and zero-cost abstractions
7. ✅ Validation tool confirms completeness of generated code
8. ✅ Build time increase < 10% for projects using generated registers

## Timeline Estimate

- **Phase 1** (Enhanced SVD parser): 2-3 days
- **Phase 2** (Register structure generator): 3-4 days
- **Phase 3** (Bitfield template utilities): 2-3 days
- **Phase 4** (Enumeration generator): 1-2 days
- **Phase 5** (Pin alternate function mappings): 2-3 days
- **Phase 6** (Documentation & examples): 2-3 days
- **Phase 7** (Validation & testing): 2-3 days
- **Total**: 2-3 weeks

## Open Questions

1. Should we generate separate headers per peripheral or one monolithic file?
   - **Recommendation**: Separate files for modularity, faster compilation
2. How to handle vendor-specific register naming conflicts?
   - **Recommendation**: Strict namespacing, vendor prefix for ambiguous names
3. Should bit field templates use operator overloading or explicit methods?
   - **Recommendation**: Explicit methods (clearer, no hidden costs)
4. Generate CMSIS-compatible names or modern C++ style?
   - **Recommendation**: Both via namespace aliases for compatibility
5. How to handle reserved/padding registers?
   - **Recommendation**: Generate as `uint32_t reserved[N]` with offset comments

## Related Work

- **CMSIS**: ARM's register definitions (C-style, macros, no type safety)
- **libopencm3**: Manual register definitions (outdated, incomplete)
- **Rust embedded-hal**: Type-safe register access (Rust-specific, good inspiration)
- **SVDConv**: ARM's SVD to header converter (generates C, not C++20)

This proposal brings CMSIS-level completeness with modern C++20 type safety and zero-overhead abstractions.
