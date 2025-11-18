# Spec Delta: hal-dma

**Capability**: `hal-dma`
**Status**: NEW

## ADDED Requirements

### Requirement: Memory-to-Memory Transfer

The system SHALL support DMA transfers between memory regions.

#### Scenario: Fast array copy
```cpp
uint8_t source[1024];
uint8_t dest[1024];

DmaConfig config{
    .direction = DmaDirection::MemoryToMemory,
    .mode = DmaMode::Normal
};
dma.configure(config);
auto result = dma.start_transfer(source, dest, 1024);
assert(result.is_ok());
```

### Requirement: Peripheral-to-Memory Transfer

The system SHALL support DMA transfers from peripheral to memory (e.g., ADC, UART RX).

#### Scenario: ADC sampling with DMA
```cpp
uint16_t adc_buffer[100];

DmaConfig config{
    .direction = DmaDirection::PeripheralToMemory,
    .mode = DmaMode::Normal,
    .peripheral_address = &ADC1->DR
};
dma.configure(config);
dma.start_transfer(nullptr, adc_buffer, 100);
```

### Requirement: Memory-to-Peripheral Transfer

The system SHALL support DMA transfers from memory to peripheral (e.g., UART TX, DAC).

#### Scenario: UART transmit with DMA
```cpp
uint8_t tx_data[] = "Hello DMA";

DmaConfig config{
    .direction = DmaDirection::MemoryToPeripheral,
    .peripheral_address = &UART1->DR
};
dma.configure(config);
dma.start_transfer(tx_data, nullptr, sizeof(tx_data));
```

### Requirement: Circular Mode

The system SHALL support circular buffer mode for continuous transfers.

#### Scenario: Audio output with circular buffer
```cpp
uint16_t audio_buffer[512];

DmaConfig config{
    .direction = DmaDirection::MemoryToPeripheral,
    .mode = DmaMode::Circular,
    .peripheral_address = &DAC->DR
};
dma.configure(config);
dma.start_transfer(audio_buffer, nullptr, 512);
// DMA automatically wraps to buffer start
```

### Requirement: Transfer Complete Callback

The system SHALL support callbacks for transfer completion and errors.

#### Scenario: Process data after DMA complete
```cpp
dma.set_complete_callback([]() {
    // Transfer complete, process data
    process_adc_data();
});

dma.set_error_callback([](ErrorCode error) {
    // Handle DMA error
});
```

### Requirement: Transfer Status

The system SHALL provide functions to query transfer status and remaining count.

#### Scenario: Monitor transfer progress
```cpp
dma.start_transfer(source, dest, 1000);
while (!dma.is_complete()) {
    uint32_t remaining = dma.get_remaining_count();
    // Update progress bar: (1000 - remaining) / 1000 * 100%
}
```

### Requirement: Priority Configuration

The system SHALL support configuring DMA channel priority.

#### Scenario: High-priority audio, low-priority logging
```cpp
DmaConfig audio_dma{
    .priority = DmaPriority::VeryHigh
};
DmaConfig log_dma{
    .priority = DmaPriority::Low
};
```

### Requirement: Data Width Configuration

The system SHALL support configuring transfer data width (8-bit, 16-bit, 32-bit).

#### Scenario: 16-bit ADC DMA
```cpp
DmaConfig config{
    .direction = DmaDirection::PeripheralToMemory,
    .data_width = DmaDataWidth::Bits16
};
```

## Non-Functional Requirements

### Perf-DMA-001: Zero CPU Overhead
DMA transfers MUST not consume CPU cycles during transfer (except callback).

### Safe-DMA-001: Concurrent Transfers
Multiple DMA channels MUST be usable concurrently without interference.

### Safe-DMA-002: Cache Coherency
The system SHOULD document cache coherency requirements for DMA buffers.
