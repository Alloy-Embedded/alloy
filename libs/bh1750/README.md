# bh1750

ROHM **BH1750FVI** ambient-light sensor driver (I2C), portable across any MCU
that provides an `alloy::I2cBus` and an `alloy::DelayNs`.

## Concepts

`template <class Bus, class Delay> requires alloy::I2cBus<Bus> && alloy::DelayNs<Delay>`

The driver names no chip, family, or board — it holds the bus by `const&` and
takes a delay source, exactly like every other first-party alloy driver.

## Use

```cpp
#include "bh1750.hpp"

alloy::lib::bh1750 light{i2c, delay};          // default addr 0x23
auto r = light.measure();
if (r.valid) {
    // r.lux is illuminance in lux
}
```

- `measure()` powers the part on (`0x01`), starts a continuous
  high-resolution measurement (`0x10`), waits 180 ms (datasheet max), reads the
  two-byte count MSB-first, and returns `{lux, valid}` where
  `lux = raw / 1.2`. Any NACK yields `valid == false`.
- `power_down()` issues the power-down opcode (`0x00`); returns `false` on NACK.

## Addresses

| ADDR pin | 7-bit address |
| -------- | ------------- |
| low (default) | `0x23` |
| high          | `0x5C` |

Pass the address as the third constructor argument, e.g.
`bh1750 light{i2c, delay, bh1750<Bus, Delay>::addr_high};`.

## Source

Behavior follows the ROHM BH1750FVI datasheet (Rev.D, Nov 2011): 1.2
counts-per-lux in H-resolution mode; typ 120 ms / max 180 ms conversion.
