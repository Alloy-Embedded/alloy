# UART Echo Example - Blue Pill (STM32F103C8T6)

Serial communication example that echoes back received characters via USART1.

## Hardware Requirements

- **Blue Pill board** (STM32F103C8T6)
- **USB-to-Serial adapter** (3.3V or 5V with level shifter)
  - Common options: CP2102, FT232, CH340

## Wiring

| Blue Pill Pin | Function | USB-Serial |
|--------------|----------|------------|
| PA9 | USART1_TX | RX |
| PA10 | USART1_RX | TX |
| GND | Ground | GND |

**Important**: Do NOT connect 5V from USB-Serial to Blue Pill. Power the Blue Pill via USB or use a 3.3V regulator.

## What It Does

1. Configures USART1 at 115200 baud, 8N1
2. Sends a welcome message
3. Echoes back every character received

## Building

```bash
cmake -B build -DALLOY_BOARD=bluepill
cmake --build build --target uart_echo_bluepill
```

## Flashing

```bash
# Using ST-Link
st-flash write build/uart_echo_bluepill.bin 0x8000000

# Using USB bootloader (DFU)
dfu-util -a 0 -s 0x08000000:leave -D build/uart_echo_bluepill.bin
```

## Testing

### Using screen (Linux/macOS)

```bash
screen /dev/ttyUSB0 115200
```

Press Ctrl+A, K to exit.

### Using minicom

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### Using PuTTY (Windows)

1. Select "Serial"
2. COM port: (your USB-Serial port, e.g., COM3)
3. Speed: 115200
4. Click "Open"

## Expected Output

```
Alloy UART Echo - Blue Pill
Type something (it will be echoed back):
Hello World!
Hello World!
```

Every character you type should be echoed back immediately.

## Code Walkthrough

### Configure GPIO for USART1

```cpp
// PA9 (TX) = (0 * 16) + 9 = 9
stm32f1::GpioPin<9> tx_pin;
tx_pin.configure(PinMode::Alternate);  // Alternate function (USART1_TX)

// PA10 (RX) = (0 * 16) + 10 = 10
stm32f1::GpioPin<10> rx_pin;
rx_pin.configure(PinMode::Input);  // Input mode
```

### Create and configure UART

```cpp
stm32f1::UartDevice<stm32f1::UsartId::USART1> uart;

UartConfig config{baud_rates::Baud115200};
uart.configure(config);
```

### Echo loop

```cpp
while (true) {
    if (uart.available() > 0) {
        auto result = uart.read_byte();
        if (result.is_ok()) {
            u8 received = result.value();

            // Echo it back
            while (uart.write_byte(received).is_error()) {
                stm32f1::delay_ms(1);
            }
        }
    }
}
```

## Troubleshooting

### No output on serial terminal

1. **Check wiring**: Ensure TX/RX are not swapped
2. **Check GND**: Ground must be connected
3. **Check baud rate**: Terminal must be set to 115200
4. **Check voltage**: USB-Serial should be 3.3V or have level shifter

### Garbled text

- **Wrong baud rate**: Set terminal to 115200
- **Wrong configuration**: Should be 8N1 (8 data bits, no parity, 1 stop bit)

### Characters not echoed

- **Check RX connection**: PA10 (RX) should connect to TX of USB-Serial
- **Check UART initialization**: Ensure `configure()` succeeds

## USART1 Details

- **APB2 bus** (up to 72 MHz)
- **Default baud rate**: 115200
- **BRR calculation**: `BRR = PCLK / (16 * baud_rate)`
  - At 72 MHz: `BRR = 72000000 / (16 * 115200) = 39.0625 ≈ 0x271`

## Next Steps

- Modify to send formatted strings
- Add line buffering (store characters until Enter is pressed)
- Implement a simple command parser
- Try different baud rates (9600, 38400, etc.)
