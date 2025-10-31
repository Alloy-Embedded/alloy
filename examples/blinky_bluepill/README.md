# Blinky Example - Blue Pill (STM32F103C8T6)

Minimal LED blink example for the Blue Pill development board.

## Hardware Requirements

- **Blue Pill board** (STM32F103C8T6)
  - Flash: 64KB (or 128KB for CB variant)
  - RAM: 20KB
  - Onboard LED: PC13 (active-low)

- **Programmer**: One of:
  - ST-Link V2 (recommended)
  - USB bootloader (built-in DFU mode)
  - Serial bootloader (UART1)

## What It Does

Blinks the onboard LED (PC13) with a 1-second period (500ms ON, 500ms OFF).

## Building

### Configure for Blue Pill

```bash
cmake -B build -DALLOY_BOARD=bluepill
cmake --build build --target blinky_bluepill
```

This will:
1. Use the ARM Cortex-M3 toolchain
2. Generate startup code from STM32F103C8 database
3. Build with STM32F1 HAL implementation
4. Produce `build/blinky_bluepill.elf` and `blinky_bluepill.bin`

## Flashing

### Using ST-Link

```bash
# Flash with st-flash (part of stlink tools)
st-flash write build/blinky_bluepill.bin 0x8000000

# Or use OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
    -c "program build/blinky_bluepill.elf verify reset exit"
```

### Using USB Bootloader (DFU)

1. Enter DFU mode:
   - Set BOOT0 jumper to 1 (3.3V)
   - Press RESET button
   - Set BOOT0 jumper back to 0 (GND)

2. Flash:
```bash
dfu-util -a 0 -s 0x08000000:leave -D build/blinky_bluepill.bin
```

### Using Serial Bootloader

```bash
stm32flash -w build/blinky_bluepill.bin -v /dev/ttyUSB0
```

## Expected Behavior

- **Onboard LED (PC13)**: Blinks at 1 Hz
  - Note: PC13 is active-low, so LED is ON when output is LOW

## Code Walkthrough

```cpp
// PC13 = Port C (2), Pin 13 = (2 * 16) + 13 = 45
constexpr uint8_t LED_PIN = 45;

// Create GPIO pin
stm32f1::GpioPin<LED_PIN> led;

// Configure as output (uses CRH register, bits 20-23)
led.configure(PinMode::Output);

// Blink loop
while (true) {
    led.set_low();   // LED ON (active-low)
    stm32f1::delay_ms(500);

    led.set_high();  // LED OFF
    stm32f1::delay_ms(500);
}
```

## Troubleshooting

### LED doesn't blink

1. **Check power**: Ensure USB cable is connected and power LED is on
2. **Check flash**: Verify binary was flashed successfully
3. **Check BOOT0**: Should be set to 0 (GND) for normal operation
4. **Try debugger**: Connect ST-Link and check if program is running

### Build errors

- **Toolchain not found**: Install `arm-none-eabi-gcc`
  ```bash
  # Ubuntu/Debian
  sudo apt install gcc-arm-none-eabi

  # macOS
  brew install --cask gcc-arm-embedded
  ```

- **Code generation failed**: Run manually:
  ```bash
  cd tools/codegen
  python3 generator.py --mcu STM32F103C8 \
      --database database/families/stm32f1xx_from_svd.json \
      --output ../../build/generated/STM32F103C8
  ```

## Pin Mapping Reference

| Pin | Function | Note |
|-----|----------|------|
| PC13 | LED | Active-low (LOW = ON) |
| PA9 | USART1_TX | For uart_echo example |
| PA10 | USART1_RX | For uart_echo example |

## Next Steps

- Try **uart_echo_bluepill** example for UART communication
- Modify blink rate by changing `delay_ms()` values
- Add more LEDs on other GPIO pins
