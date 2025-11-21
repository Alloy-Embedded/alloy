# ADC Usage Guide

**Version**: 1.0
**Date**: 2025-01-21
**Status**: Complete

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Single Channel Reading](#single-channel-reading)
4. [Multi-Channel Scanning](#multi-channel-scanning)
5. [Continuous Conversion](#continuous-conversion)
6. [DMA Integration](#dma-integration)
7. [Analog Watchdog](#analog-watchdog)
8. [Calibration](#calibration)
9. [Best Practices](#best-practices)
10. [Troubleshooting](#troubleshooting)

---

## Overview

The Analog-to-Digital Converter (ADC) converts analog voltages to digital values for processing.

### ADC Capabilities

| Platform | Channels | Resolution | Sample Rate | Features |
|----------|----------|------------|-------------|----------|
| **STM32F4** | 16 + 3 internal | 12-bit | 2.4 MSPS | DMA, Dual mode |
| **SAME70** | 12 | 12-16 bit | 1 MSPS | Offset/Gain correction |

### Key Features

- **Multiple resolutions**: 6-bit, 8-bit, 10-bit, 12-bit (up to 16-bit on SAME70)
- **Flexible sampling**: Single, continuous, or scan mode
- **DMA support**: Zero-CPU sampling for high throughput
- **Analog watchdog**: Automatic threshold monitoring
- **Internal channels**: Temperature sensor, VREF, VBAT monitoring

---

## Quick Start

### STM32F4 Basic Example

```cpp
#include "hal/vendors/st/stm32f4/generated/platform/adc.hpp"

using namespace alloy::hal::st::stm32f4;

int main() {
    // 1. Enable ADC clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // 2. Initialize ADC
    ADC1Hardware::enable();
    ADC1Hardware::set_resolution(ADC1Hardware::RES_12BIT);
    ADC1Hardware::set_alignment(false);  // Right alignment

    // 3. Configure channel
    ADC1Hardware::set_sample_time(0, 3);       // Channel 0, 84 cycles
    ADC1Hardware::set_sequence_length(1);      // Single channel
    ADC1Hardware::set_regular_sequence(1, 0);  // Rank 1 = Channel 0

    // 4. Read value
    ADC1Hardware::start_conversion();
    while (!ADC1Hardware::is_conversion_complete());
    u16 raw_value = ADC1Hardware::read_data();

    // 5. Convert to voltage (assuming 3.3V reference)
    float voltage = (raw_value * 3.3f) / 4095.0f;
}
```

### SAME70 Basic Example

```cpp
#include "hal/vendors/microchip/same70/generated/platform/adc.hpp"

using namespace alloy::hal::microchip::same70;

int main() {
    // 1. Enable AFEC0 clock
    PMC->PMC_PCER1 |= (1 << 8);  // AFEC0 peripheral ID

    // 2. Reset and configure
    AFEC0Hardware::reset();
    AFEC0Hardware::enable_channel(0);  // Enable channel 0

    // 3. Read value
    AFEC0Hardware::start_conversion();
    // Wait for conversion (check status register in production code)
    u16 raw_value = AFEC0Hardware::read_channel_data(0);

    // 4. Convert to voltage (12-bit, 3.3V reference)
    float voltage = (raw_value * 3.3f) / 4095.0f;
}
```

---

## Single Channel Reading

### Blocking Read

```cpp
Result<u16, ErrorCode> read_adc_blocking(u8 channel) {
    // Configure channel
    ADC1Hardware::set_regular_sequence(1, channel);

    // Start conversion
    ADC1Hardware::start_conversion();

    // Wait with timeout
    u32 timeout = 1000;  // cycles
    while (!ADC1Hardware::is_conversion_complete() && --timeout);

    if (timeout == 0) {
        return Err(ErrorCode::TIMEOUT);
    }

    // Read and return
    return Ok(ADC1Hardware::read_data());
}

// Usage
auto result = read_adc_blocking(0);
if (result.is_ok()) {
    u16 value = result.unwrap();
}
```

### Non-Blocking Read

```cpp
class AdcReader {
public:
    void start_read(u8 channel) {
        channel_ = channel;
        state_ = State::BUSY;

        ADC1Hardware::set_regular_sequence(1, channel);
        ADC1Hardware::start_conversion();
    }

    Result<u16, ErrorCode> get_result() {
        if (state_ == State::IDLE) {
            return Err(ErrorCode::NOT_READY);
        }

        if (ADC1Hardware::is_conversion_complete()) {
            state_ = State::IDLE;
            return Ok(ADC1Hardware::read_data());
        }

        return Err(ErrorCode::BUSY);
    }

private:
    enum class State { IDLE, BUSY };
    State state_ = State::IDLE;
    u8 channel_;
};

// Usage
AdcReader adc;
adc.start_read(0);

// Later, in main loop
auto result = adc.get_result();
if (result.is_ok()) {
    u16 value = result.unwrap();
}
```

### Voltage Conversion

```cpp
class VoltageReader {
public:
    VoltageReader(float vref = 3.3f, u8 resolution_bits = 12)
        : vref_(vref)
        , max_value_((1 << resolution_bits) - 1) {}

    float raw_to_voltage(u16 raw_value) const {
        return (static_cast<float>(raw_value) * vref_) / max_value_;
    }

    u16 voltage_to_raw(float voltage) const {
        return static_cast<u16>((voltage * max_value_) / vref_);
    }

private:
    float vref_;
    float max_value_;
};

// Usage
VoltageReader reader(3.3f, 12);  // 3.3V, 12-bit
float voltage = reader.raw_to_voltage(2048);  // ~1.65V
```

---

## Multi-Channel Scanning

### Scan Mode (Sequential Channels)

```cpp
void setup_scan_mode(const u8* channels, u8 count) {
    // Enable scan mode
    ADC1Hardware::enable_scan_mode();
    ADC1Hardware::set_sequence_length(count);

    // Configure sequence
    for (u8 i = 0; i < count; i++) {
        ADC1Hardware::set_regular_sequence(i + 1, channels[i]);
        ADC1Hardware::set_sample_time(channels[i], 3);
    }
}

Result<void, ErrorCode> read_multiple_channels(
    const u8* channels,
    u8 count,
    u16* values
) {
    setup_scan_mode(channels, count);

    // Start conversion
    ADC1Hardware::start_conversion();

    // Read all channels
    for (u8 i = 0; i < count; i++) {
        // Wait for conversion
        u32 timeout = 1000;
        while (!ADC1Hardware::is_conversion_complete() && --timeout);

        if (timeout == 0) {
            return Err(ErrorCode::TIMEOUT);
        }

        values[i] = ADC1Hardware::read_data();

        // Start next conversion if not last
        if (i < count - 1) {
            ADC1Hardware::start_conversion();
        }
    }

    return Ok();
}

// Usage
u8 channels[] = {0, 1, 2, 3};
u16 values[4];
auto result = read_multiple_channels(channels, 4, values);
```

### Round-Robin Sampling

```cpp
class MultiChannelAdc {
public:
    MultiChannelAdc(const u8* channels, u8 count)
        : channels_(channels)
        , count_(count)
        , current_(0) {

        for (u8 i = 0; i < count; i++) {
            values_[i] = 0;
        }
    }

    void update() {
        // Start conversion for current channel
        ADC1Hardware::set_regular_sequence(1, channels_[current_]);
        ADC1Hardware::start_conversion();

        // Check if previous conversion done
        if (ADC1Hardware::is_conversion_complete()) {
            values_[current_] = ADC1Hardware::read_data();

            // Move to next channel
            current_ = (current_ + 1) % count_;
        }
    }

    u16 get_value(u8 channel_index) const {
        return (channel_index < count_) ? values_[channel_index] : 0;
    }

private:
    const u8* channels_;
    u8 count_;
    u8 current_;
    u16 values_[16];
};

// Usage
u8 channels[] = {0, 1, 2, 3};
MultiChannelAdc adc(channels, 4);

void main_loop() {
    while (true) {
        adc.update();  // Call frequently

        // Read latest values
        u16 temp = adc.get_value(0);
        u16 light = adc.get_value(1);
        u16 voltage = adc.get_value(2);
    }
}
```

---

## Continuous Conversion

### Free-Running Mode

```cpp
void setup_continuous_mode(u8 channel) {
    // Configure channel
    ADC1Hardware::set_regular_sequence(1, channel);
    ADC1Hardware::set_sample_time(channel, 3);

    // Enable continuous mode
    ADC1Hardware::enable_continuous_mode();

    // Start conversions
    ADC1Hardware::start_conversion();
    // ADC will now continuously convert!
}

u16 read_latest() {
    // Just read the latest value
    // (conversion happens in background)
    return ADC1Hardware::read_data();
}

// Usage
setup_continuous_mode(0);

void main_loop() {
    while (true) {
        u16 value = read_latest();
        process(value);

        delay_ms(100);  // Sample at 10 Hz
    }
}
```

### Circular Buffer Pattern

```cpp
class ContinuousAdc {
public:
    ContinuousAdc(u8 channel, u16 buffer_size = 32)
        : channel_(channel)
        , buffer_size_(buffer_size)
        , write_index_(0) {}

    void start() {
        ADC1Hardware::set_regular_sequence(1, channel_);
        ADC1Hardware::enable_continuous_mode();
        ADC1Hardware::enable_eoc_interrupt();
        ADC1Hardware::start_conversion();
    }

    void on_conversion_complete() {
        buffer_[write_index_] = ADC1Hardware::read_data();
        write_index_ = (write_index_ + 1) % buffer_size_;
    }

    u16 get_average() const {
        u32 sum = 0;
        for (u16 i = 0; i < buffer_size_; i++) {
            sum += buffer_[i];
        }
        return sum / buffer_size_;
    }

private:
    u8 channel_;
    u16 buffer_size_;
    u16 write_index_;
    u16 buffer_[32];
};

// In interrupt handler
ContinuousAdc* g_adc = nullptr;

extern "C" void ADC_IRQHandler() {
    if (g_adc) {
        g_adc->on_conversion_complete();
    }
}
```

---

## DMA Integration

### DMA Continuous Sampling

```cpp
// Note: Full DMA template coming soon
// This shows the concept with direct register access

class AdcDma {
public:
    AdcDma(u8 channel, u16* buffer, u16 buffer_size)
        : buffer_(buffer)
        , buffer_size_(buffer_size) {

        // Enable DMA clock
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

        // Configure DMA stream for ADC1 (DMA2 Stream 0)
        auto* dma = reinterpret_cast<DMA_Stream_TypeDef*>(0x40026400);

        // Disable stream
        dma->CR &= ~DMA_SxCR_EN;
        while (dma->CR & DMA_SxCR_EN);

        // Configure DMA
        dma->PAR = reinterpret_cast<u32>(&ADC1->DR);  // Peripheral address
        dma->M0AR = reinterpret_cast<u32>(buffer);     // Memory address
        dma->NDTR = buffer_size;                       // Number of transfers

        // DMA configuration
        dma->CR = (0 << 25) |  // Channel 0
                  (1 << 13) |  // Memory size: 16-bit
                  (1 << 11) |  // Peripheral size: 16-bit
                  (1 << 10) |  // Memory increment
                  (1 << 8) |   // Circular mode
                  (0 << 6);    // Peripheral-to-memory

        // Enable DMA
        dma->CR |= DMA_SxCR_EN;

        // Configure ADC
        ADC1Hardware::set_regular_sequence(1, channel);
        ADC1Hardware::enable_continuous_mode();
        ADC1Hardware::enable_dma();
        ADC1Hardware::enable_dma_continuous();

        // Start conversions
        ADC1Hardware::start_conversion();
    }

    u16 get_value(u16 index) const {
        return (index < buffer_size_) ? buffer_[index] : 0;
    }

    float get_average() const {
        u32 sum = 0;
        for (u16 i = 0; i < buffer_size_; i++) {
            sum += buffer_[i];
        }
        return static_cast<float>(sum) / buffer_size_;
    }

private:
    u16* buffer_;
    u16 buffer_size_;
};

// Usage
u16 adc_buffer[256];
AdcDma adc_dma(0, adc_buffer, 256);

// DMA continuously fills buffer in background
// Read values anytime:
float avg = adc_dma.get_average();
```

---

## Analog Watchdog

Monitor ADC values automatically with hardware.

### Setup Watchdog

```cpp
void setup_watchdog(u16 low_threshold, u16 high_threshold) {
    // Set thresholds (12-bit values)
    ADC1Hardware::set_watchdog_low_threshold(low_threshold);
    ADC1Hardware::set_watchdog_high_threshold(high_threshold);

    // Enable watchdog
    ADC1Hardware::enable_watchdog();
    ADC1Hardware::enable_watchdog_interrupt();

    // Configure NVIC
    NVIC_EnableIRQ(ADC_IRQn);
}

// Interrupt handler
extern "C" void ADC_IRQHandler() {
    if (ADC1Hardware::has_watchdog_event()) {
        ADC1Hardware::clear_watchdog_flag();

        // Handle out-of-range condition
        handle_voltage_alarm();
    }
}

// Usage - Alert if voltage outside 1.5V - 2.0V range
setup_watchdog(
    1860,  // 1.5V / 3.3V * 4095
    2482   // 2.0V / 3.3V * 4095
);
```

### Battery Monitor Example

```cpp
class BatteryMonitor {
public:
    BatteryMonitor(float min_voltage, float max_voltage) {
        VoltageReader reader(3.3f, 12);

        u16 low = reader.voltage_to_raw(min_voltage);
        u16 high = reader.voltage_to_raw(max_voltage);

        setup_watchdog(low, high);
    }

    static void on_alarm() {
        // Battery voltage out of range
        enter_low_power_mode();
        log_warning("Battery voltage alarm!");
    }
};

// Usage - Alert if battery < 3.0V or > 4.2V
BatteryMonitor battery(3.0f, 4.2f);
```

---

## Calibration

### Offset Calibration

```cpp
class AdcCalibration {
public:
    Result<void, ErrorCode> calibrate_offset(u8 channel) {
        constexpr u16 SAMPLES = 100;
        u32 sum = 0;

        // Read multiple samples with input grounded
        for (u16 i = 0; i < SAMPLES; i++) {
            auto result = read_adc_blocking(channel);
            if (!result.is_ok()) {
                return Err(result.error());
            }
            sum += result.unwrap();
        }

        offset_ = sum / SAMPLES;
        return Ok();
    }

    u16 apply_offset(u16 raw_value) const {
        i32 corrected = static_cast<i32>(raw_value) - static_cast<i32>(offset_);
        return (corrected < 0) ? 0 : static_cast<u16>(corrected);
    }

private:
    u16 offset_ = 0;
};
```

### Two-Point Calibration

```cpp
class TwoPointCalibration {
public:
    void calibrate(float known_v1, u16 raw1, float known_v2, u16 raw2) {
        // Calculate slope and offset
        // V = slope * raw + offset

        slope_ = (known_v2 - known_v1) / (raw2 - raw1);
        offset_ = known_v1 - (slope_ * raw1);
    }

    float raw_to_voltage(u16 raw_value) const {
        return (slope_ * raw_value) + offset_;
    }

private:
    float slope_ = 3.3f / 4095.0f;  // Default
    float offset_ = 0.0f;
};

// Usage - Calibrate with known voltages
TwoPointCalibration cal;

// Measure at 1.0V
u16 raw1 = read_adc_blocking(0).unwrap();
// Measure at 2.5V
u16 raw2 = read_adc_blocking(0).unwrap();

cal.calibrate(1.0f, raw1, 2.5f, raw2);

// Now use calibrated readings
float accurate_voltage = cal.raw_to_voltage(raw_value);
```

---

## Best Practices

### DO ✅

1. **Always configure sample time**:
```cpp
// Good - appropriate sample time
ADC1Hardware::set_sample_time(0, 3);  // 84 cycles for 100k source impedance

// Bad - may not fully charge sampling capacitor
ADC1Hardware::set_sample_time(0, 0);  // Only 3 cycles!
```

2. **Use averaging for noisy signals**:
```cpp
u16 read_averaged(u8 channel, u8 samples = 16) {
    u32 sum = 0;
    for (u8 i = 0; i < samples; i++) {
        sum += read_adc_blocking(channel).unwrap();
    }
    return sum / samples;
}
```

3. **Check for errors**:
```cpp
// Good
auto result = read_adc_blocking(0);
if (result.is_ok()) {
    process(result.unwrap());
} else {
    handle_error(result.error());
}

// Bad
u16 value = read_adc_blocking(0).unwrap();  // Panics on error!
```

### DON'T ❌

1. **Don't ignore source impedance**:
```cpp
// ❌ Fast sample time + high impedance = inaccurate
// 10k potentiometer with 3-cycle sample time
ADC1Hardware::set_sample_time(0, 0);  // TOO FAST!

// ✅ Match sample time to source impedance
// 10k source → use 84 cycles or more
ADC1Hardware::set_sample_time(0, 3);  // Better
```

2. **Don't forget reference voltage accuracy**:
```cpp
// ❌ Assumes perfect 3.3V
float voltage = (raw * 3.3f) / 4095.0f;

// ✅ Measure actual VREF with multimeter
const float VREF_MEASURED = 3.28f;  // Actual measurement
float voltage = (raw * VREF_MEASURED) / 4095.0f;
```

3. **Don't mix analog and digital**:
```cpp
// ❌ Digital noise couples to ADC
toggle_led();  // Creates noise
u16 value = read_adc_blocking(0);  // Corrupted!

// ✅ Separate in time
u16 value = read_adc_blocking(0);  // Read first
toggle_led();  // Then do digital operations
```

---

## Troubleshooting

### "Readings are noisy"

**Causes**:
- Source impedance too high
- No decoupling capacitors
- Digital noise coupling
- Sample time too short

**Solutions**:
```cpp
// 1. Increase sample time
ADC1Hardware::set_sample_time(channel, 7);  // Max: 480 cycles

// 2. Average multiple readings
u16 value = read_averaged(channel, 16);

// 3. Add 0.1uF capacitor at input
// 4. Use separate analog ground plane (hardware)
```

### "Readings are incorrect"

**Causes**:
- Wrong reference voltage
- Source impedance too high
- Not calibrated
- Wrong alignment setting

**Solutions**:
```cpp
// 1. Verify reference voltage
// Measure VREF with multimeter, update code

// 2. Verify alignment
ADC1Hardware::set_alignment(false);  // Right-aligned for 12-bit

// 3. Calibrate
calibration.calibrate_offset(channel);
u16 corrected = calibration.apply_offset(raw);

// 4. Check source impedance
// Rule of thumb: R_source < 10k for fast sampling
```

### "Conversions timeout"

**Causes**:
- ADC clock not enabled
- Channel not configured
- Conversion not started

**Solutions**:
```cpp
// 1. Enable ADC clock
RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

// 2. Initialize ADC
ADC1Hardware::enable();

// 3. Configure channel BEFORE starting
ADC1Hardware::set_regular_sequence(1, 0);
ADC1Hardware::start_conversion();  // Now it will work
```

---

## Internal Channels

### Temperature Sensor

```cpp
// STM32F4 internal temperature sensor is on channel 16
float read_die_temperature() {
    // Enable temperature sensor
    ADC1->CCR |= ADC_CCR_TSVREFE;

    // Read raw value (channel 16)
    ADC1Hardware::set_regular_sequence(1, 16);
    ADC1Hardware::set_sample_time(16, 7);  // Max sample time
    ADC1Hardware::start_conversion();
    while (!ADC1Hardware::is_conversion_complete());
    u16 raw = ADC1Hardware::read_data();

    // Convert to temperature (STM32F4 specific)
    // Temp = (V25 - Vsense) / Avg_Slope + 25
    // V25 = 0.76V, Avg_Slope = 2.5mV/°C
    float vsense = (raw * 3.3f) / 4095.0f;
    float temp_c = ((0.76f - vsense) / 0.0025f) + 25.0f;

    return temp_c;
}
```

### VREF Monitoring

```cpp
// Monitor internal reference voltage
float read_vrefint() {
    // Channel 17 on STM32F4
    ADC1Hardware::set_regular_sequence(1, 17);
    ADC1Hardware::set_sample_time(17, 7);
    ADC1Hardware::start_conversion();
    while (!ADC1Hardware::is_conversion_complete());
    u16 raw = ADC1Hardware::read_data();

    // VREFINT is typically 1.21V ±3%
    float vrefint = (raw * 3.3f) / 4095.0f;
    return vrefint;
}
```

---

## Examples

Complete examples available in:
- `src/hal/vendors/st/stm32f4/generated/platform/adc.hpp` - STM32F4 API
- `src/hal/vendors/microchip/same70/generated/platform/adc.hpp` - SAME70 API

---

## Next Steps

- Learn about [Timer Usage](TIMER_USAGE_GUIDE.md)
- See [API Reference](API_REFERENCE.md) for complete API
- Check [Hardware Policy Guide](HARDWARE_POLICY_GUIDE.md) for policy-based design

---

**Updated**: 2025-01-21
**Version**: 1.0
