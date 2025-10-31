# Pin Mappings

This document provides pin mapping information for all supported boards.

## Overview

Each board has different GPIO pin numbering schemes. Alloy uses the board's native pin numbering for simplicity.

## Pin Numbering Systems

| Board | System | Example |
|-------|--------|---------|
| Blue Pill (STM32F103) | Port + Pin Number | `PA0`, `PC13` → Pin = Port × 16 + Number |
| STM32F4 Discovery | Port + Pin Number | `PD12`, `PA5` → Pin = Port × 16 + Number |
| Raspberry Pi Pico | Direct GPIO | `GPIO0`, `GPIO25` → Pin = GPIO number |
| Arduino Zero | Arduino Pin Number | `D0-D13`, `A0-A5` → See mapping table |
| ESP32 DevKit | Direct GPIO | `GPIO0`, `GPIO2` → Pin = GPIO number |

---

## Blue Pill (STM32F103C8)

### Port to Pin Number Conversion
```
Pin = (Port × 16) + Pin Number

Where Port:
  PA = 0, PB = 1, PC = 2, PD = 3

Examples:
  PA0  = 0×16 + 0  = 0
  PA5  = 0×16 + 5  = 5
  PB12 = 1×16 + 12 = 28
  PC13 = 2×16 + 13 = 45 (on-board LED)
```

### Key Pins
- **On-board LED**: PC13 (Pin 45, active LOW)
- **USB D+**: PA12
- **USB D-**: PA11
- **BOOT0**: Pull-up resistor, jumper selectable
- **SWD Debug**: PA13 (SWDIO), PA14 (SWCLK)

### Pin Table (Selected)
| Pin # | Port Pin | 5V Tolerant | Default Function |
|-------|----------|-------------|------------------|
| 0-15  | PA0-PA15 | Most Yes | GPIO, USART1/2, SPI1, I2C1, TIM1-4, ADC1/2 |
| 16-31 | PB0-PB15 | Most Yes | GPIO, USART3, SPI1/2, I2C1/2, TIM1-4, ADC1/2 |
| 32-47 | PC0-PC15 | Yes | GPIO, ADC1/2 (PC0-5) |

**Full pinout**: See STM32F103C8 datasheet or [STM32duino Wiki](https://wiki.stm32duino.com/index.php?title=Blue_Pill)

---

## STM32F4 Discovery (STM32F407VG)

### Port to Pin Number Conversion
```
Pin = (Port × 16) + Pin Number

Where Port:
  PA = 0, PB = 1, PC = 2, PD = 3, PE = 4, PF = 5, PG = 6, PH = 7, PI = 8

Examples:
  PD12 = 3×16 + 12 = 60 (Green LED)
  PD13 = 3×16 + 13 = 61 (Orange LED)
  PD14 = 3×16 + 14 = 62 (Red LED)
  PD15 = 3×16 + 15 = 63 (Blue LED)
```

### Key Pins
- **LEDs**:
  - Green: PD12 (Pin 60)
  - Orange: PD13 (Pin 61)
  - Red: PD14 (Pin 62)
  - Blue: PD15 (Pin 63)
- **User Button**: PA0 (active HIGH)
- **USB OTG FS**: PA11 (D-), PA12 (D+)
- **MEMS Accelerometer**: PE0 (CS), PE1 (INT)
- **Audio**: PC7 (MCLK), PC10 (SCLK), PC12 (SDIN)

**Full pinout**: See STM32F407VG Discovery user manual [UM1472](https://www.st.com/resource/en/user_manual/dm00039084.pdf)

---

## Raspberry Pi Pico (RP2040)

### GPIO Numbering
Direct GPIO numbering - no conversion needed.

```
Pin = GPIO number
Example: GPIO25 = Pin 25
```

### Key Pins
- **On-board LED**: GPIO25 (active HIGH, green)
- **ADC Pins**: GPIO26-29 (ADC0-ADC3)
- **I2C0 Default**: GPIO4 (SDA), GPIO5 (SCL)
- **I2C1 Default**: GPIO6 (SDA), GPIO7 (SCL)
- **SPI0 Default**: GPIO16 (MISO), GPIO17 (CS), GPIO18 (SCK), GPIO19 (MOSI)
- **UART0 Default**: GPIO0 (TX), GPIO1 (RX)

### Pin Table
| GPIO | Physical Pin | Functions | Notes |
|------|--------------|-----------|-------|
| 0-22 | 1-29 | All: GPIO, PWM; Some: SPI, I2C, UART, PIO | Configurable |
| 23-25 | - | GPIO only | 23/24: Debug, 25: LED |
| 26-29 | 31-34 | GPIO, ADC0-3 | ADC capable (3.3V max!) |

**Full pinout**: See [Raspberry Pi Pico datasheet](https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf)

---

## Arduino Zero (ATSAMD21G18)

### Arduino Pin to GPIO Conversion
Arduino Zero uses Arduino-style pin numbering (D0-D19, A0-A5).

| Arduino Pin | SAMD21 Port | Alloy Pin # | Function |
|-------------|-------------|-------------|----------|
| D0 | PA11 | 11 | UART RX |
| D1 | PA10 | 10 | UART TX |
| D2-D12 | Various | See table | Digital I/O |
| D13/LED | PA17 | 17 | Built-in LED, SPI SCK |
| A0 | PA02 | 2 | ADC, DAC |
| A1-A5 | PA04-PB03 | See table | ADC |

### Key Pins
- **On-board LED**: D13 / PA17 (Pin 17, active HIGH, orange)
- **USB**: PA24 (D-), PA25 (D+)
- **I2C**: PA22 (SDA), PA23 (SCL)
- **SPI**: PA17 (SCK), PA18 (MOSI), PA19 (MISO)
- **DAC**: PA02 (A0)

### Complete Mapping
```
D0  → PA11 (Pin 11)    D10 → PA18 (Pin 18)
D1  → PA10 (Pin 10)    D11 → PA16 (Pin 16)
D2  → PA14 (Pin 14)    D12 → PA19 (Pin 19)
D3  → PA09 (Pin 9)     D13 → PA17 (Pin 17) ← LED
D4  → PA08 (Pin 8)     A0  → PA02 (Pin 2)
D5  → PA15 (Pin 15)    A1  → PB08 (Pin 24)
D6  → PA20 (Pin 20)    A2  → PB09 (Pin 25)
D7  → PA21 (Pin 21)    A3  → PA04 (Pin 4)
D8  → PA06 (Pin 6)     A4  → PA05 (Pin 5)
D9  → PA07 (Pin 7)     A5  → PB02 (Pin 18)
```

**Full pinout**: See [Arduino Zero schematic](https://www.arduino.cc/en/uploads/Main/Arduino-Zero-schematic.pdf)

---

## ESP32 DevKit (ESP32-WROOM-32)

### GPIO Numbering
Direct GPIO numbering - use as-is.

```
Pin = GPIO number
Example: GPIO2 = Pin 2
```

### Key Pins
- **On-board LED**: GPIO2 (active HIGH, blue - board dependent)
- **BOOT Button**: GPIO0 (active LOW)
- **EN (Reset) Button**: External reset
- **Strapping Pins** (special boot mode control):
  - GPIO0, GPIO2, GPIO5, GPIO12, GPIO15

### Special Notes
- **Input-only pins**: GPIO34-39 (no pull-up/down, no output)
- **RTC GPIO**: GPIO0, GPIO2, GPIO4, GPIO12-15, GPIO25-27, GPIO32-39
- **Touch-capable**: GPIO0, GPIO2, GPIO4, GPIO12-15, GPIO27, GPIO32-33

### Pin Table (Selected)
| GPIO | Input | Output | ADC | Touch | Notes |
|------|-------|--------|-----|-------|-------|
| 0 | ✓ | ✓ | ADC2_1 | T1 | BOOT button, Strapping |
| 1 | ✓ | ✓ | - | - | UART0 TX (debug) |
| 2 | ✓ | ✓ | ADC2_2 | T2 | LED, Strapping |
| 3 | ✓ | ✓ | - | - | UART0 RX (debug) |
| 4 | ✓ | ✓ | ADC2_0 | T0 | - |
| 5 | ✓ | ✓ | - | - | Strapping, SPI CS |
| 12-15 | ✓ | ✓ | Various | Various | Strapping pins |
| 16-17 | - | - | - | - | Reserved (PSRAM) |
| 18-19 | ✓ | ✓ | - | - | SPI |
| 21-22 | ✓ | ✓ | - | - | I2C default |
| 25-27 | ✓ | ✓ | ADC2 | Touch | DAC1, DAC2 |
| 32-33 | ✓ | ✓ | ADC1 | Touch | - |
| 34-39 | ✓ | ✗ | ADC1 | Some | Input only! |

**Full pinout**: See [ESP32 DevKit pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html)

---

## Usage in Code

### STM32 Example (Port-based)
```cpp
// Blue Pill - LED on PC13
constexpr uint8_t LED_PIN = (2 * 16) + 13;  // Port C (2), Pin 13
Board::LedBlue led;  // Uses pin PC13
led.on();
```

### RP2040/ESP32 Example (Direct GPIO)
```cpp
// Pico - LED on GPIO25
constexpr uint8_t LED_PIN = 25;
Board::Led::on();  // Uses GPIO25

// ESP32 - LED on GPIO2
constexpr uint8_t LED_PIN = 2;
Board::Led::on();  // Uses GPIO2
```

## See Also

- [Boards Documentation](boards.md) - Board specifications
- [Board Header Files](../boards/) - Pre-defined pin constants
- Vendor datasheets for complete pin multiplexing tables
