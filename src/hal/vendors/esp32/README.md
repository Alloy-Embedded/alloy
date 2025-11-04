# ESP32 HAL Backend - ESP-IDF Drivers

Optimized hardware abstraction layer implementations for ESP32 using ESP-IDF drivers.

## Features

### GPIO (`gpio.hpp`)
- ✅ Compile-time pin configuration with type safety
- ✅ Zero-cost abstraction over ESP-IDF `driver/gpio.h`
- ✅ Interrupt support with C++ callbacks
- ✅ Pull-up/pull-down resistor configuration
- ✅ Direct register access for performance

### UART (`uart.hpp`)
- ✅ Hardware FIFO and DMA support
- ✅ Buffered I/O with configurable sizes
- ✅ All standard baud rates and configurations (5-8 data bits, parity, stop bits)
- ✅ Hardware flow control (RTS/CTS)
- ✅ Event-driven operation (optional)

### SPI (`spi.hpp`)
- ✅ Hardware SPI with DMA support
- ✅ Up to 80 MHz clock speed
- ✅ All 4 SPI modes (Mode 0-3)
- ✅ Transaction queuing for efficiency
- ✅ Full-duplex and half-duplex modes
- ✅ Multiple devices per bus

### I2C (`i2c.hpp`)
- ✅ Hardware I2C master mode
- ✅ 7-bit and 10-bit addressing
- ✅ Standard (100 kHz) and Fast (400 kHz) modes
- ✅ Clock stretching support
- ✅ Repeated start conditions
- ✅ Timeout handling
- ✅ Bus scanning utility

## Usage

### GPIO Example

```cpp
#include "hal/esp32/gpio.hpp"

using namespace alloy::hal::esp32;

// LED on GPIO 2
GpioOutput<2> led;

void setup() {
    led.set_high();
    led.toggle();
}

// Button with interrupt
GpioInputPullUp<0> button;

void setup_button() {
    button.attach_interrupt(GPIO_INTR_NEGEDGE, []() {
        // Button pressed
        led.toggle();
    });
}
```

### UART Example

```cpp
#include "hal/esp32/uart.hpp"

using namespace alloy::hal::esp32;

Uart0 serial;

void setup() {
    // Initialize UART0 on default pins (TX=1, RX=3)
    serial.init(GPIO_NUM_1, GPIO_NUM_3);

    // Configure 115200 baud, 8N1
    serial.configure(UartConfig{115200_baud});

    // Send data
    serial.write_string("Hello, World!\r\n");

    // Read data
    auto result = serial.read_byte();
    if (result.is_ok()) {
        uint8_t byte = result.value();
    }
}
```

### SPI Example

```cpp
#include "hal/esp32/spi.hpp"

using namespace alloy::hal::esp32;

Spi2 spi;

void setup() {
    // Initialize SPI2 (CLK=14, MISO=12, MOSI=13)
    spi.init(GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13);

    // Add device with CS on GPIO 15
    auto device_result = spi.add_device(GPIO_NUM_15,
        SpiConfig{SpiMode::Mode0, 1000000});  // 1 MHz

    if (device_result.is_ok()) {
        auto device = device_result.value();

        // Transfer data
        uint8_t tx_data[] = {0x01, 0x02, 0x03};
        uint8_t rx_data[3];
        device.transfer(tx_data, rx_data);
    }
}
```

### I2C Example

```cpp
#include "hal/esp32/i2c.hpp"

using namespace alloy::hal::esp32;

I2c0 i2c;

void setup() {
    // Initialize I2C0 (SDA=21, SCL=22)
    i2c.init(GPIO_NUM_21, GPIO_NUM_22);

    // Configure for Fast mode (400 kHz)
    i2c.configure(I2cConfig{I2cSpeed::Fast});

    // Scan for devices
    auto scan_result = i2c.scan();
    if (scan_result.is_ok()) {
        for (uint8_t addr : scan_result.value()) {
            printf("Found device at 0x%02X\n", addr);
        }
    }

    // Read from device
    uint8_t reg_addr = 0x00;
    uint8_t data[2];
    i2c.write_read(0x50, {&reg_addr, 1}, data);
}
```

## Advantages over Direct ESP-IDF

| Feature | ESP-IDF Direct | CoreZero HAL |
|---------|---------------|--------------|
| **Type Safety** | C APIs, easy to misuse | C++ templates, compile-time checks |
| **Error Handling** | esp_err_t codes | Result<T> monadic operations |
| **Pin Configuration** | Runtime strings | Compile-time validation |
| **Resource Management** | Manual cleanup | RAII automatic cleanup |
| **Interrupt Callbacks** | C function pointers | C++ std::function, lambdas |
| **Code Size** | Depends on usage | Zero-cost abstractions |

## Performance

All HAL implementations use zero-cost abstractions:
- No virtual functions or runtime polymorphism
- Compile-time configuration eliminates runtime checks
- Direct inline calls to ESP-IDF drivers
- Same performance as hand-written ESP-IDF code

## Compatibility

- **Minimum ESP-IDF**: v5.0 or later
- **Supported Chips**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6
- **Build System**: ESP-IDF CMake (component system)

## Integration

To use ESP32 HAL backends in your project:

```cmake
# In your component's CMakeLists.txt
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES hal esp32  # Depend on HAL component
)
```

Then include the appropriate headers:

```cpp
#include "hal/esp32/gpio.hpp"
#include "hal/esp32/uart.hpp"
#include "hal/esp32/spi.hpp"
#include "hal/esp32/i2c.hpp"
```

## Notes

### Limitations

1. **UART**: Changing baud rate after initialization requires reinitialization
2. **SPI**: Clock speed configured per device, not per transaction
3. **I2C**: Clock speed set during init, requires reinitialization to change
4. **GPIO**: Interrupt callbacks stored per instance (use static/global carefully)

### Best Practices

1. **Use RAII**: Let destructors handle cleanup automatically
2. **Check Results**: Always handle Result<T> return values
3. **Compile-time Configuration**: Use templates where possible for performance
4. **Interrupts**: Keep ISR handlers short and fast
5. **DMA**: Enable for SPI/UART when transferring large amounts of data

## Examples

See `examples/` directory for complete working examples:
- `examples/esp32_gpio_blink/` - GPIO output with LED blink
- `examples/esp32_uart_echo/` - UART echo server
- `examples/esp32_spi_sd/` - SPI with SD card
- `examples/esp32_i2c_scan/` - I2C bus scanner

## See Also

- [ESP-IDF GPIO Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [ESP-IDF UART Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html)
- [ESP-IDF SPI Master Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html)
- [ESP-IDF I2C Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)
