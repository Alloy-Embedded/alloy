# Blink Example for SAME70 Xplained

This example demonstrates basic GPIO usage on the SAME70 Xplained board by blinking the onboard LED.

## Hardware Requirements

- **Board**: Atmel SAME70 Xplained
- **MCU**: ATSAME70Q21 (ARM Cortex-M7 @ 300MHz)
- **LED**: LED0 connected to PC8 (onboard)

## Features Demonstrated

- GPIO configuration for SAME70/SAMV71
- PIO (Parallel I/O) controller usage
- Basic delay functionality

## Building

```bash
# Configure for SAME70 Xplained board
cmake -B build -DALLOY_BOARD=same70_xpld

# Build the example
cmake --build build --target blink_same70
```

## Flashing

```bash
# Flash to the board (requires OpenOCD or similar)
cmake --build build --target flash-blink_same70
```

## Expected Behavior

The onboard LED (LED0) should blink at 1 Hz (500ms on, 500ms off).

## Code Overview

```cpp
// Create GPIO pin for LED0 (PC8)
GPIOPin<pins::PC8> led;

// Configure as output
led.configureOutput();

// Blink forever
while (true) {
    led.set();          // Turn LED on
    delay_ms(500);      // Wait 500ms
    
    led.clear();        // Turn LED off
    delay_ms(500);      // Wait 500ms
}
```

## Notes

- This example uses the generated GPIO HAL for SAME70
- The PIO controller provides low-level GPIO access
- LED0 on SAME70 Xplained is active-low (set() turns it on)
