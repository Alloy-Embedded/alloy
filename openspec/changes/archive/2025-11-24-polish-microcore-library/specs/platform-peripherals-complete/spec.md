# Spec: Platform Peripherals Completeness

## MODIFIED Requirements

### Requirement: UART implemented across all platforms
UART hardware policy MUST exist for STM32F1, STM32F4, STM32F7, STM32G0, and SAME70.

#### Scenario: Build UART example for all boards
```bash
$ ./ucore build nucleo_f401re uart_logger  # STM32F4 ✓
$ ./ucore build nucleo_f722ze uart_logger  # STM32F7 ✓
$ ./ucore build nucleo_g071rb uart_logger  # STM32G0 ✓
$ ./ucore build same70_xplained uart_logger # SAME70 ✓
```

**Expected**: All builds succeed with same source code
**Rationale**: Core peripheral must work everywhere

#### Scenario: UART hardware policy interface
```cpp
// src/hal/st/stm32f4/uart_policy.hpp
template<UartInstance Instance, uint32_t Baudrate>
class Stm32f4UartHardwarePolicy {
public:
    static void configure() { /* ... */ }
    static void write_byte(uint8_t byte) { /* ... */ }
    static uint8_t read_byte() { /* ... */ }
    static bool tx_ready() { /* ... */ }
    static bool rx_ready() { /* ... */ }
};
```

**Expected**: Consistent interface across all platforms
**Rationale**: Platform abstraction layer working correctly

## ADDED Requirements

### Requirement: SPI implemented for all platforms
SPI hardware policy MUST support master mode on all platforms.

#### Scenario: SPI master configuration
```cpp
using Spi = platform::SpiMaster<
    platform::Spi::Spi1,
    platform::SpiMode::Mode0,      // CPOL=0, CPHA=0
    platform::SpiBitOrder::MsbFirst,
    platform::SpiSpeed::_1MHz
>;

Spi::configure();
uint8_t response = Spi::transfer(0x42);
```

**Expected**: Works on STM32F4, STM32F7, STM32G0, SAME70
**Rationale**: Essential peripheral for sensors, displays, SD cards

#### Scenario: SPI hardware policy
```cpp
// Platform-specific implementation
template<SpiInstance Instance, SpiMode Mode, uint32_t Speed>
class Stm32f4SpiHardwarePolicy {
public:
    static void configure() {
        // Configure SPI peripheral registers
    }

    static uint8_t transfer(uint8_t data) {
        // Write to SPI->DR, read response
        return response;
    }

    static void write_buffer(const uint8_t* data, size_t len) {
        // Bulk transfer (optional DMA)
    }
};
```

**Expected**: Hardware-specific implementation, common interface
**Rationale**: Zero-overhead abstraction pattern

### Requirement: I2C implemented for all platforms
I2C hardware policy MUST support master mode with 7-bit addressing.

#### Scenario: I2C master communication
```cpp
using I2c = platform::I2cMaster<
    platform::I2c::I2c1,
    platform::I2cSpeed::_100kHz
>;

I2c::configure();

// Write to device
uint8_t data[] = {0x10, 0x20};
I2c::write(device_addr, data, sizeof(data));

// Read from device
uint8_t buffer[4];
I2c::read(device_addr, buffer, sizeof(buffer));
```

**Expected**: Works reliably on all platforms
**Rationale**: Critical for sensor communication

#### Scenario: I2C error handling
```cpp
auto result = I2c::write(device_addr, data, len);

if (!result) {
    switch(result.error()) {
        case I2cError::Nack:
            // Device not responding
            break;
        case I2cError::BusError:
            // Bus arbitration lost
            break;
        case I2cError::Timeout:
            // Transaction timeout
            break;
    }
}
```

**Expected**: Proper error detection and reporting
**Rationale**: I2C is error-prone, must handle gracefully

### Requirement: ADC implemented for all platforms
ADC hardware policy MUST support single-channel and multi-channel reads.

#### Scenario: Single ADC read
```cpp
using Adc = platform::AdcSingle<
    platform::Adc::Adc1,
    platform::AdcChannel::_0,
    platform::AdcResolution::_12bit
>;

Adc::configure();
uint16_t value = Adc::read();  // 0-4095
float voltage = (value / 4095.0f) * 3.3f;
```

**Expected**: Simple one-shot ADC reads
**Rationale**: Basic analog input functionality

#### Scenario: Multi-channel ADC with DMA
```cpp
using Adc = platform::AdcMulti<
    platform::Adc::Adc1,
    platform::AdcChannels<0, 1, 2, 3>,
    platform::AdcResolution::_12bit
>;

Adc::configure_dma(buffer, buffer_size);
Adc::start_continuous();

// DMA fills buffer automatically
uint16_t channel0 = buffer[0];
uint16_t channel1 = buffer[1];
```

**Expected**: High-performance multi-channel acquisition
**Rationale**: Common use case for data acquisition

### Requirement: Peripheral examples for each implementation
Each peripheral MUST have working example on real hardware.

#### Scenario: SPI example with OLED display
```cpp
// examples/spi_oled/main.cpp
using Spi = platform::SpiMaster<...>;
using Cs = platform::Gpio::Output<board::spi_cs_pin>;

void write_command(uint8_t cmd) {
    Cs::set_low();
    Spi::transfer(cmd);
    Cs::set_high();
}

int main() {
    Spi::configure();
    // Initialize display
    write_command(0xAE);  // Display off
    write_command(0xD5);  // Set clock
    // ...
}
```

**Expected**: Demonstrates real-world peripheral usage
**Rationale**: Examples validate implementation

## REMOVED Requirements

None. This completes existing peripheral support.

## Cross-Platform Compatibility Matrix

After implementation, all peripherals must work across all platforms:

| Peripheral | STM32F4 | STM32F7 | STM32G0 | SAME70 |
|------------|---------|---------|---------|--------|
| GPIO       | ✅      | ✅      | ✅      | ✅     |
| UART       | ✅      | ✅      | ⚠️      | ⚠️     |
| SPI        | ❌      | ❌      | ❌      | ❌     |
| I2C        | ❌      | ❌      | ❌      | ❌     |
| ADC        | ❌      | ❌      | ❌      | ❌     |

**Target after this change:** All ✅

**Rationale**: Complete platform support for common peripherals
