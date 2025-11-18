# Blinky Example Specification

Simple LED blink example demonstrating basic GPIO usage.

## ADDED Requirements

### Requirement: EXAMPLE-BLINK-001 - Basic LED Blink

The example SHALL demonstrate basic GPIO output by blinking an LED.

#### Scenario: LED blinks at 1Hz

```cpp
#include "board.hpp"

int main() {
    Board::initialize();
    Board::Led::init();

    while (true) {
        Board::Led::toggle();
        delay_ms(500);
    }
}
```

**Output**: LED blinks on/off every 500ms (1Hz)

---

### Requirement: EXAMPLE-BLINK-002 - Multiple Boards

The example SHALL work on multiple target boards without code changes.

**Supported boards**:
- STM32F103C8 (Blue Pill)
- STM32F407VG (Discovery)
- ESP32
- RP2040
- SAMD21

---

## Related Specifications

- GPIO Interface
- Board Abstraction
