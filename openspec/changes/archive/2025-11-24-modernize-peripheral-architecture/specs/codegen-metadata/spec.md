# Spec: Codegen Signal Metadata

## Overview

Extend SVD codegen to generate signal routing tables and compatibility matrices.

## ADDED Requirements

### Requirement: Pin Alternate Function Tables
The system SHALL generate pin→signal mappings from SVD.

#### Scenario: USART1 Signal Table
```cpp
// Generated from SVD
namespace generated::same70 {
    struct Usart1Signals {
        struct Tx {
            static constexpr std::array pins = {
                PinDef{PinId::PA9, AlternateFunction::AF7},
                PinDef{PinId::PA15, AlternateFunction::AF7},
                PinDef{PinId::PB6, AlternateFunction::AF7}
            };
        };
        struct Rx {
            static constexpr std::array pins = {
                PinDef{PinId::PA10, AlternateFunction::AF7},
                PinDef{PinId::PB7, AlternateFunction::AF7}
            };
        };
    };
}
```

**Success Criteria**:
- ✅ All peripherals have signal tables
- ✅ Pin IDs and AF numbers accurate
- ✅ Generated from SVD, not hardcoded

---

### Requirement: DMA Compatibility Matrix
The system SHALL generate DMA channel→peripheral mappings.

#### Scenario: DMA1 Stream Compatibility
```cpp
// Generated
namespace generated::same70 {
    struct Dma1Stream7 {
        static constexpr std::array supported_peripherals = {
            PeripheralDef{PeripheralId::USART1, DmaChannel::TX},
            PeripheralDef{PeripheralId::SPI1, DmaChannel::TX},
            PeripheralDef{PeripheralId::ADC1, DmaChannel::DATA}
        };
    };
}
```

**Success Criteria**:
- ✅ All DMA streams have compatibility lists
- ✅ Request types included (TX, RX, DATA)
- ✅ Platform-specific variations handled

## Dependencies

- SVD parser enhancements
- Template generation system
- Platform-specific metadata sources (datasheets)
