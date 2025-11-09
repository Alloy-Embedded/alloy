# SAME70 Xplained Ultra - Board Support Package

Complete board support package for the **Atmel SAME70 Xplained Ultra** evaluation board.

## Board Specifications

- **MCU**: ATSAME70Q21B (ARM Cortex-M7 @ 300 MHz)
- **Flash**: 2 MB
- **RAM**: 384 KB
- **FPU**: Yes (FPv5-D16, double-precision)
- **LEDs**: 2x user LEDs (LED0, LED1)
- **Buttons**: 2x user buttons (SW0, SW1)
- **USB**: Full-speed USB device + host
- **Ethernet**: 10/100 Mbps
- **Camera Interface**: Yes
- **SD Card**: Yes

## Quick Start

### 1. Include the Board Header

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;
```

### 2. Initialize the Board

```cpp
int main() {
    // Initialize board with 300 MHz clock
    board::init(ClockConfig::Clock300MHz);

    // Your code here...
}
```

## Clock Configuration Options

The board supports multiple clock frequencies:

| Option | Frequency | Use Case |
|--------|-----------|----------|
| `Clock300MHz` | 300 MHz | Maximum performance |
| `Clock150MHz` | 150 MHz | Balanced performance/power |
| `Clock120MHz` | 120 MHz | USB compatible |
| `Clock48MHz` | 48 MHz | Low power |
| `Clock12MHz` | 12 MHz | Ultra-low power |

**Example:**
```cpp
// Initialize with 150 MHz for balanced performance
board::init(ClockConfig::Clock150MHz);
```

## LEDs

The board has 2 user LEDs (active low):

- **LED0** (Green): Pin PC8
- **LED1** (Green): Pin PC9

### LED Control

```cpp
// Direct pin control
board::led0.set();      // Turn OFF (active low)
board::led0.clear();    // Turn ON (active low)
board::led0.toggle();   // Toggle state

// Intuitive helper functions
board::led0_on();       // Turn ON
board::led0_off();      // Turn OFF
board::led1_on();       // Turn ON
board::led1_off();      // Turn OFF
```

### LED Blink Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    while (true) {
        board::led0_on();
        board::delay_ms(500);

        board::led0_off();
        board::delay_ms(500);
    }
}
```

## Buttons

The board has 2 user buttons (active low with pull-ups):

- **SW0**: Pin PA11
- **SW1**: Pin PB9

### Button Reading

```cpp
// Direct pin reading (returns false when pressed)
bool state = board::sw0.read();

// Intuitive helper functions
if (board::sw0_pressed()) {
    // Button SW0 is pressed
}

if (board::sw1_pressed()) {
    // Button SW1 is pressed
}
```

### Button Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    while (true) {
        if (board::sw0_pressed()) {
            board::led0_on();
        } else {
            board::led0_off();
        }

        if (board::sw1_pressed()) {
            board::led1_on();
        } else {
            board::led1_off();
        }
    }
}
```

## UART/USART

### UART0 (Debug Console)

UART0 is connected to the on-board EDBG debugger, providing a USB-UART bridge for debugging.

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Initialize UART0 at 115200 baud
    board::uart0.init(115200);

    // Send data
    board::uart0.write("Hello, SAME70!\r\n");

    // Read data
    uint8_t byte;
    if (board::uart0.read(&byte, 1).is_ok()) {
        // Process received byte
    }
}
```

### USART Instances

The board provides 3 USART instances:

```cpp
board::usart0.init(115200);
board::usart1.init(9600);
board::usart2.init(57600);
```

## I2C (TWIHS)

The board provides 3 I2C interfaces:

- **I2C0**: Pins PA3 (SDA), PA4 (SCL)
- **I2C1**: Pins PB4 (SDA), PB5 (SCL)
- **I2C2**: Pins PD27 (SDA), PD28 (SCL)

### I2C Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Initialize I2C0 at 100 kHz
    board::i2c0.init(100000);

    // Write to I2C device at address 0x50
    uint8_t data[] = {0x00, 0x01, 0x02};
    board::i2c0.write(0x50, data, sizeof(data));

    // Read from I2C device
    uint8_t buffer[16];
    board::i2c0.read(0x50, buffer, sizeof(buffer));
}
```

## SPI

The board provides 2 SPI interfaces:

- **SPI0**: Pins PD20 (MISO), PD21 (MOSI), PD22 (SCK), PB2 (CS0)
- **SPI1**: Pins PC26 (MISO), PC27 (MOSI), PC24 (SCK), PC25 (CS0)

### SPI Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Initialize SPI0 at 1 MHz
    board::spi0.init(1000000);

    // Write and read simultaneously
    uint8_t tx_data[] = {0x01, 0x02, 0x03};
    uint8_t rx_data[3];
    board::spi0.transfer(tx_data, rx_data, sizeof(tx_data));
}
```

## ADC

The board has a 12-bit ADC with 12 channels.

### ADC Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Initialize ADC
    board::adc0.init();

    // Read channel 0
    uint16_t adc_value = board::adc0.read_channel(0);

    // Convert to voltage (assuming 3.3V reference)
    float voltage = (adc_value / 4096.0f) * 3.3f;
}
```

## PWM

The board provides 4 PWM channels.

### PWM Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Initialize PWM0 with 1 kHz frequency
    board::pwm0.init(1000);

    // Set 50% duty cycle
    board::pwm0.set_duty_cycle(50);

    // Start PWM
    board::pwm0.start();
}
```

## Timers

The board provides 3 timer instances for precise timing.

### Timer Example

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Initialize timer0
    board::timer0.init();

    // Start counting
    board::timer0.start();

    // Wait for specific time
    while (board::timer0.get_count() < 1000000) {
        // Wait 1 second (assuming 1 MHz timer clock)
    }
}
```

## DMA

Direct Memory Access for high-speed data transfers.

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init(ClockConfig::Clock300MHz);

    // Configure DMA transfer
    uint8_t src_buffer[256];
    uint8_t dst_buffer[256];

    board::dma0.configure_transfer(src_buffer, dst_buffer, 256);
    board::dma0.start();
}
```

## Utility Functions

### Delay Functions

```cpp
// Millisecond delay
board::delay_ms(1000);  // Wait 1 second

// Microsecond delay
board::delay_us(500);   // Wait 500 microseconds
```

### Board Information

```cpp
// Get board name
const char* name = board::get_board_name();
// Returns: "Atmel SAME70 Xplained Ultra"

// Get MCU name
const char* mcu = board::get_mcu_name();
// Returns: "ATSAME70Q21B"

// Get current clock frequency
uint32_t freq_hz = board::get_system_clock_hz();
```

## Complete Application Example

```cpp
/**
 * Complete SAME70 Xplained application example
 * - Blinks LED0 at 1 Hz
 * - Toggles LED1 when SW0 is pressed
 * - Sends periodic messages over UART
 * - Reads ADC value every second
 */

#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    // Initialize board with 300 MHz clock
    auto result = board::init(ClockConfig::Clock300MHz);
    if (result.is_error()) {
        // Handle initialization error
        while (true) {
            // Error indication: fast blink
            board::led0_on();
            board::delay_ms(100);
            board::led0_off();
            board::delay_ms(100);
        }
    }

    // Initialize UART for debug output
    board::uart0.init(115200);
    board::uart0.write("SAME70 Xplained - System Initialized\r\n");

    // Initialize ADC
    board::adc0.init();

    uint32_t counter = 0;

    while (true) {
        // Blink LED0
        board::led0.toggle();

        // Check button and control LED1
        if (board::sw0_pressed()) {
            board::led1_on();
        } else {
            board::led1_off();
        }

        // Every second (2 iterations of 500ms delay)
        if (counter % 2 == 0) {
            // Read ADC
            uint16_t adc_value = board::adc0.read_channel(0);
            float voltage = (adc_value / 4096.0f) * 3.3f;

            // Send status over UART
            char buffer[64];
            snprintf(buffer, sizeof(buffer),
                     "Counter: %lu, ADC: %u (%.2fV)\r\n",
                     counter / 2, adc_value, voltage);
            board::uart0.write(buffer);
        }

        counter++;
        board::delay_ms(500);
    }
}
```

## Pin Mapping Reference

### LEDs and Buttons

| Component | Pin | Description |
|-----------|-----|-------------|
| LED0 | PC8 | User LED 0 (Green) |
| LED1 | PC9 | User LED 1 (Green) |
| SW0 | PA11 | User Button 0 |
| SW1 | PB9 | User Button 1 |

### UART/USART

| Peripheral | TX Pin | RX Pin |
|------------|--------|--------|
| UART0 (Debug) | PA10 | PA9 |
| USART0 | PB1 | PB0 |
| USART1 | PA22 | PA21 |

### I2C (TWIHS)

| Peripheral | SDA Pin | SCL Pin |
|------------|---------|---------|
| I2C0 | PA3 | PA4 |
| I2C1 | PB4 | PB5 |
| I2C2 | PD27 | PD28 |

### SPI

| Peripheral | MISO | MOSI | SCK | CS0 |
|------------|------|------|-----|-----|
| SPI0 | PD20 | PD21 | PD22 | PB2 |
| SPI1 | PC26 | PC27 | PC24 | PC25 |

## Building

To build for SAME70 Xplained:

```bash
cmake -B build -DALLOY_BOARD=same70_xpld
cmake --build build
```

## Flashing

Flash via OpenOCD:

```bash
openocd -f board/atmel_same70_xplained.cfg \
        -c "program build/your_app.elf verify reset exit"
```

Or use the Makefile target:

```bash
make same70-flash
```

## Additional Resources

- [SAME70 Xplained Ultra User Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-44050-Cortex-M7-Microcontroller-SAM-E70-SAME70-Datasheet.pdf)
- [ATSAME70Q21 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-E70-S70-V70-V71-Family-Data-Sheet-DS60001527D.pdf)
- [Alloy Framework Documentation](../../README.md)

## License

Part of the Alloy Framework - MIT License
