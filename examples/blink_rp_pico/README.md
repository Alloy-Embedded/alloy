# Blink Example - Raspberry Pi Pico

This example demonstrates basic GPIO usage with the Alloy framework on the Raspberry Pi Pico board.

## Hardware

- **Board**: Raspberry Pi Pico
- **MCU**: RP2040 (dual ARM Cortex-M0+ @ 133MHz)
- **LED**: GPIO25 (on-board LED)

## Features Demonstrated

- GPIO pin configuration as output
- Digital output using RP2040 SIO (Single-cycle I/O)
- Basic delay functions

## Building

```bash
# Configure for Raspberry Pi Pico
cmake -B build -DALLOY_BOARD=rp_pico

# Build the example
cmake --build build --target blink_rp_pico

# Flash to board (requires picotool or UF2 bootloader)
cmake --build build --target flash_blink_rp_pico
```

## Expected Behavior

The on-board LED (GPIO25) should blink at 1 Hz:
- 500ms ON
- 500ms OFF

## Code Overview

```cpp
// Create GPIO pin instance for LED
GPIOPin<pins::LED> led;

// Configure as output
led.configureOutput();

// Blink loop
while (true) {
    led.set();      // Turn on
    delay_ms(500);
    led.clear();    // Turn off
    delay_ms(500);
}
```

## RP2040 SIO Architecture

The RP2040 uses SIO (Single-cycle I/O) for fast GPIO operations:
- **Single-cycle access**: GPIO reads/writes complete in one CPU cycle
- **Atomic operations**: Separate SET/CLR/XOR registers for thread-safe operations
- **Dual-core safe**: Each core has independent SIO access

## Pin Configuration

GPIO25 is configured with:
- **Function**: SIO (GPIO mode, function 5)
- **Direction**: Output
- **Pad**: Default drive strength (4mA)

## Resources

- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [Pico Getting Started](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
