# ds3231

Portable driver for the Maxim/Analog **DS3231** high-accuracy real-time clock
(TCXO) over I2C. Constrained only on the `alloy::I2cBus` concept — no chip,
family, or board is named.

## Concepts

| Template param | Concept        |
| -------------- | -------------- |
| `Bus`          | `alloy::I2cBus` |

The driver holds the bus by `const&`. Fixed I2C address `0x68`.

## API

```cpp
#include "ds3231.hpp"

alloy::lib::ds3231 rtc{i2c};          // any alloy::I2cBus

// Read the wall clock (plain-binary alloy::datetime — BCD handled internally).
alloy::datetime dt;
if (rtc.now(dt)) { /* dt.hour, dt.minute, ... */ }

// Or the convenience form (returns a default datetime on bus error):
alloy::datetime t = rtc.now();

// Set the clock (forces 24-hour mode, century 20xx).
rtc.set(dt);

// Die temperature, 0.25 C resolution.
if (auto tc = rtc.read_temperature_c(); tc.valid) { /* tc.celsius */ }
```

## Behavior

- Registers `0x00..0x06` hold seconds/minutes/hours/day-of-week/date/month/year
  as **BCD**. The driver converts BCD⇄binary at the register edge, so user code
  only sees the plain-binary `alloy::datetime`. `now()` reads all seven with a
  single `write_read`; `set()` writes them in one transaction.
- Hours are always written and read in **24-hour** mode (register bit 6 = 0).
  `alloy::datetime` has no weekday field, so the day-of-week register is written
  to `1` and ignored on read.
- `read_temperature_c()` decodes the 10-bit two's-complement value from
  `0x11`/`0x12` (upper two bits of `0x12` are the quarter-degree fraction),
  correctly handling negative temperatures.
- Every method reports failure honestly: `now(dt)`/`set()` return `false` and
  `read_temperature_c().valid` is `false` on a NACK or bus error.

## Freestanding

No heap, no exceptions, no RTTI. Fixed stack buffers only. Builds clean under
`-fno-exceptions -fno-rtti -Os -Wall -Wextra -Werror` for Cortex-M0+ and M7.
