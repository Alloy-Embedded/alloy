# Blinky Example

Classic "Hello World" for embedded systems - blinks an LED using GPIO.

## Description

This example demonstrates:
- Basic GPIO pin configuration
- Digital output control (HIGH/LOW)
- Simple delay functionality
- Platform abstraction (runs on host and embedded targets)

On **host** platforms (PC), GPIO operations are printed to the console.
On **embedded** targets, a physical LED will blink.

## Hardware Setup

### Host (PC)
No hardware needed - runs natively and prints GPIO state to console.

### Embedded Targets

**Common LED pins:**
- **Raspberry Pi Pico (RP2040)**: GPIO 25 (onboard LED)
- **STM32 BluePill**: PC13 (onboard LED)
- **ESP32 DevKit**: GPIO 2 (onboard LED)
- **Renesas RL78**: Configure per your board

Modify pin number in `main.cpp` if needed:
```cpp
host::GpioPin<25> led;  // Change 25 to your LED pin
```

## Building

### Host Platform

```bash
# Configure for host
cmake -B build -S . -DALLOY_BOARD=host

# Build
cmake --build build

# Run
./build/examples/blinky/blinky
```

Expected output:
```
==================================
Alloy Blinky Example (Host Mode)
==================================

[GPIO Mock] Pin 25 configured as Output
[GPIO Mock] Pin 25 set HIGH

Starting blink loop (Ctrl+C to stop)...

[GPIO Mock] Pin 25 toggled to LOW
[GPIO Mock] Pin 25 toggled to HIGH
[GPIO Mock] Pin 25 toggled to LOW
...
```

Press Ctrl+C to stop.

### Embedded Targets (Future)

```bash
# Configure for RP2040 (Raspberry Pi Pico)
cmake -B build -S . -DALLOY_BOARD=rp_pico

# Build
cmake --build build

# Flash (using picotool)
picotool load build/examples/blinky/blinky.uf2
```

Similar commands for other boards (STM32, ESP32, RL78).

## Code Overview

```cpp
#include "hal/host/gpio.hpp"
#include "platform/delay.hpp"

using namespace alloy::hal;
using namespace alloy::platform;

int main() {
    // Create GPIO pin (pin 25)
    host::GpioPin<25> led;

    // Configure as output
    led.configure(PinMode::Output);

    // Set initial state
    led.set_high();

    // Blink forever
    while (true) {
        delay_ms(500);  // Wait 500ms
        led.toggle();   // Toggle LED state
    }

    return 0;
}
```

## Key Concepts

### GPIO Pin Configuration
```cpp
host::GpioPin<25> led;
led.configure(PinMode::Output);
```

Pin number is a template parameter for zero-cost abstraction.

### Digital Output
```cpp
led.set_high();  // Turn LED on
led.set_low();   // Turn LED off
led.toggle();    // Flip current state
```

### Platform Delays
```cpp
delay_ms(500);  // Wait 500 milliseconds
```

Implementation varies by platform:
- **Host**: `std::this_thread::sleep_for()`
- **Embedded**: Hardware timers or busy-wait loops

## Next Steps

- Modify blink rate by changing `delay_ms()` parameter
- Try different pin numbers (if supported by your board)
- Add multiple LEDs
- Explore other examples (UART, I2C, etc)

## Troubleshooting

**Build fails:**
- Ensure CMake 3.25+ is installed
- Verify C++20 compiler (GCC 11+ or Clang 13+)

**No output on embedded target:**
- Check LED pin number matches your board
- Verify LED polarity (some boards have inverted LEDs)
- Check power supply

**"Command not found" on host:**
- Run from repository root, not inside build/
- Use correct path: `./build/examples/blinky/blinky`
