# Spec: DMA Integration System

## Overview

Type-safe DMA channel allocation and connection validation.

## ADDED Requirements

### Requirement: DMA Channel Registry
The system SHALL track DMA channel allocations at compile-time.

#### Scenario: Prevent Double Allocation
```cpp
using UartTxDma = DmaConnection<Usart1::TxData, Dma1::Stream7>;
using SpiTxDma = DmaConnection<Spi1::TxData, Dma1::Stream7>;  // Same stream!

// Compile error:
// error: DMA Stream 7 already allocated to USART1
// note: Choose a different stream for SPI1
// note: Available streams for SPI1 TX: Stream 3, Stream 5
```

**Success Criteria**:
- ✅ Detects conflicting allocations
- ✅ Suggests available alternatives
- ✅ Zero runtime overhead

---

### Requirement: Type-Safe DMA Configuration
DMA transfers SHALL be validated for peripheral compatibility.

#### Scenario: UART DMA Transfer
```cpp
// Connection defines source/destination types
UartTxDma::transfer({
    .source = tx_buffer,        // Must be memory address
    .destination = Usart1::TxData,  // Peripheral register (auto)
    .size = buffer_size,
    .mode = DmaMode::Circular
});

// Type error if wrong:
// error: Cannot transfer from UART to UART (expected memory source)
```

**Success Criteria**:
- ✅ Source/destination types checked
- ✅ Peripheral register address automatic
- ✅ Transfer size validated

## Dependencies

- Signal routing for peripheral→DMA connections
- SVD codegen for DMA compatibility tables
