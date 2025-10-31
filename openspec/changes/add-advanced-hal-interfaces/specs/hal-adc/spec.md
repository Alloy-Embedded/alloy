# Spec Delta: hal-adc

**Capability**: `hal-adc`
**Status**: NEW

## ADDED Requirements

### Requirement: ADC Configuration

The system SHALL provide a way to configure ADC parameters including resolution, reference voltage, and sample time.

**Rationale**: Different sensors require different ADC configurations for accuracy.

#### Scenario: Configure 12-bit ADC with external reference
```cpp
AdcConfig config{
    .resolution = AdcResolution::Bits12,
    .reference = AdcReference::External,
    .sample_time = AdcSampleTime::Cycles84
};
auto result = adc.configure(config);
assert(result.is_ok());
```

### Requirement: Single Channel Read

The system SHALL support reading a single ADC channel and returning the raw value.

**Rationale**: Most basic ADC operation - read one sensor.

#### Scenario: Read temperature sensor on channel 0
```cpp
auto result = adc.read_single(AdcChannel::Channel0);
assert(result.is_ok());
uint16_t raw_value = result.value();
float voltage = (raw_value / 4095.0f) * 3.3f;
```

### Requirement: Multi-Channel Scanning

The system SHALL support scanning multiple ADC channels in sequence.

**Rationale**: Efficient reading of multiple sensors without reconfiguration.

#### Scenario: Scan 4 analog sensors
```cpp
std::array<AdcChannel, 4> channels = {
    AdcChannel::Channel0,
    AdcChannel::Channel1,
    AdcChannel::Channel2,
    AdcChannel::Channel3
};
std::array<uint16_t, 4> values;
auto result = adc.read_multi_channel(channels, values);
assert(result.is_ok());
```

### Requirement: Continuous Conversion Mode

The system SHALL support continuous conversion mode for real-time monitoring.

**Rationale**: Some applications need constant monitoring (e.g., audio input).

#### Scenario: Start continuous conversion with callback
```cpp
adc.start_continuous(AdcChannel::Channel0, [](uint16_t value) {
    // Process each sample
    process_audio_sample(value);
});
// ... later
adc.stop_continuous();
```

### Requirement: DMA Support

The system SHALL support DMA transfers for efficient data acquisition.

**Rationale**: CPU-free data transfer for high-speed sampling.

#### Scenario: Capture 1000 samples using DMA
```cpp
std::array<uint16_t, 1000> buffer;
auto result = adc.start_dma(AdcChannel::Channel0, buffer);
assert(result.is_ok());
// Wait for completion or check status
while (!adc.is_dma_complete()) { }
```

### Requirement: Calibration

The system SHALL provide calibration functionality when supported by the hardware for improved accuracy.

**Rationale**: Some MCUs support self-calibration for better results.

#### Scenario: Calibrate ADC before use
```cpp
auto result = adc.calibrate();
if (result.is_ok()) {
    // ADC calibrated, ready for accurate measurements
}
```

### Requirement: Error Handling

The system SHALL return appropriate error codes for ADC failures (overrun, timeout, invalid channel).

**Rationale**: Robust error handling for production systems.

#### Scenario: Handle ADC overrun
```cpp
auto result = adc.read_single(AdcChannel::Channel0);
if (result.is_error() && result.error() == ErrorCode::AdcOverrun) {
    // Handle data loss
}
```

## Non-Functional Requirements

### Perf-ADC-001: Conversion Time
The ADC read operation SHOULD complete within the hardware-specified conversion time plus minimal overhead (< 10 CPU cycles).

### Safe-ADC-001: Thread Safety
ADC operations MUST be safe to call from interrupt contexts when documented as interrupt-safe.

### Port-ADC-001: Platform Independence
The ADC interface MUST work across ARM Cortex-M, Xtensa, and RISC-V architectures.
