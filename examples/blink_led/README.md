# LED Blink Example - SAME70 Xplained Ultra

Este exemplo demonstra o uso do SysTick timer para delays precisos.

## Arquivos Disponíveis

### ✅ main.cpp (HAL Direto) - TESTADO E FUNCIONANDO
Usa HAL diretamente sem board abstraction.

### main_board_abstraction.cpp (Board Abstraction)
Usa board::init() com toda a inicialização automática.

## Supported Boards

- SAME70 Xplained Ultra
- Arduino Zero
- Waveshare RP2040 Zero
- Any board implementing the standard board interface

## Hardware Requirements

- One of the supported boards
- USB cable for programming and power
- The board must have at least one user LED

## Expected Behavior

The LED blinks with the following pattern:
- LED ON for 500ms
- LED OFF for 500ms
- Repeats indefinitely

## Building

### SAME70 Xplained Ultra

```bash
mkdir build-same70
cd build-same70
cmake .. -DALLOY_BOARD=same70_xplained -DCMAKE_BUILD_TYPE=Release
make blink_led
```

### Arduino Zero

```bash
mkdir build-arduino
cd build-arduino
cmake .. -DALLOY_BOARD=arduino_zero -DCMAKE_BUILD_TYPE=Release
make blink_led
```

### Waveshare RP2040 Zero

```bash
mkdir build-rp2040
cd build-rp2040
cmake .. -DALLOY_BOARD=waveshare_rp2040_zero -DCMAKE_BUILD_TYPE=Release
make blink_led
```

## Flashing

### SAME70 Xplained Ultra

Using OpenOCD:
```bash
make flash-blink_led
```

Or using BOSSA:
```bash
bossac -e -w -v -b -R blink_led.bin
```

### Arduino Zero

Using BOSSA:
```bash
bossac -i -d --port=/dev/ttyACM0 -U -e -w -v blink_led.bin -R
```

### Waveshare RP2040 Zero

Using picotool:
```bash
picotool load blink_led.uf2
picotool reboot
```

## Source Code

```cpp
#include BOARD_HEADER

int main() {
    // Initialize the board
    board::init();

    // Blink LED forever
    while (true) {
        board::led::on();
        board::delay_ms(500);
        board::led::off();
        board::delay_ms(500);
    }
}
```

## Key Features

### Portability

The same code works on all boards because it uses the standard `board::` interface:
- `board::init()` - Initialize hardware
- `board::led::on()` / `board::led::off()` - LED control
- `board::delay_ms()` - Millisecond delay

### Build Configuration

The board is selected at build time via CMake:
- `-DALLOY_BOARD=same70_xplained`
- `-DALLOY_BOARD=arduino_zero`
- `-DALLOY_BOARD=waveshare_rp2040_zero`

CMake automatically:
- Sets the correct `BOARD_HEADER` macro
- Links the appropriate startup code
- Configures the linker script
- Sets up the toolchain

### No Board-Specific Code

Notice that the source code contains:
- No `#ifdef` for different boards
- No direct hardware register access
- No platform-specific includes
- No pin number references

Everything is abstracted through the `board::` interface.

## Troubleshooting

### LED doesn't blink

1. Check that the board is powered
2. Verify the correct board was selected in CMake
3. Check that the flash operation completed successfully
4. Some boards have active-low LEDs (LED is on when pin is low)

### Build errors

1. Make sure you specified `-DALLOY_BOARD=<board_name>`
2. Verify the board name is correct
3. Check that the board's `board.hpp` exists
4. Ensure the toolchain is properly configured

### Flash errors

1. Check that the board is connected via USB
2. Verify you have the correct flash tool installed
3. Some boards require a bootloader (Arduino Zero, RP2040)
4. Check that you have permissions to access the USB device

## See Also

- `examples/systick_test/` - Tests timing precision
- `examples/button_led/` - Interactive LED control
- `docs/BOARD_ABSTRACTION_DESIGN.md` - Board abstraction design
- `boards/common/board_interface.hpp` - Standard interface definition
