# bme280

Bosch **BME280** combined temperature / pressure / humidity sensor, over I2C.

Portable by construction: the driver templates on the horizontal alloy concepts
[`I2cBus`](../../src/alloy/concepts.hpp) and `DelayNs` and names no chip, family,
or board. It works unchanged on every alloy board.

```cpp
#include <bme280.hpp>

alloy::lib::bme280 sensor{i2c, delay};   // default addr 0x76 (SDO -> GND)
if (sensor.init()) {                     // verifies chip-id 0x60, latches calibration
    auto r = sensor.measure();
    if (r.valid) {
        // r.temperature_c  (°C)
        // r.pressure_pa    (Pa)
        // r.humidity_pct   (%RH)
    }
}
```

## What it does

- **`init()`** reads the chip-id at `0xD0` (must be `0x60`) and latches the full
  factory trimming/calibration block (`0x88..0xA1` and `0xE1..0xE7`). Returns
  `false` on any NACK or a wrong id. Call it before `measure()`.
- **`measure()`** writes `ctrl_hum` (`0xF2`) and `ctrl_meas` (`0xF4`) to trigger
  one **forced-mode** conversion at oversampling x1 on all three axes, waits the
  worst-case conversion time (~9.3 ms) through the injected `DelayNs`, burst-reads
  the eight data bytes at `0xF7`, and applies the datasheet compensation.
- **`reset()`** issues a soft reset (`0xB6` -> `0xE0`); re-run `init()` after.

The alternate I2C address `0x77` (SDO -> VDD) is available as
`bme280<Bus,Delay>::addr_alt` and via the constructor's third argument.

## Numerics

Compensation uses the datasheet's **integer fixed-point** paths (`*_int32` for
temperature/humidity, `*_int64` for pressure), bit-exact to Bosch's reference
source — no floating point in the math. The `reading` struct converts to physical
float units only at the boundary:

| field           | fixed-point form        | conversion |
|-----------------|-------------------------|------------|
| `temperature_c` | °C × 100                | `/ 100`    |
| `pressure_pa`   | Pa in Q24.8 (Pa × 256)  | `/ 256`    |
| `humidity_pct`  | %RH in Q22.10 (× 1024)  | `/ 1024`   |

## Honest failure

Every method reports failure rather than returning garbage: a NACK, a wrong
chip-id, or calling `measure()` before a successful `init()` all yield
`reading{ valid = false }` (or `false` from the `bool` methods). Never read the
values when `valid` is false.

## Freestanding

Header-only C++23, no heap, no exceptions, no RTTI, fixed buffers only. Builds
under `-fno-exceptions -fno-rtti -Os -Wall -Wextra -Werror` and cross-compiles
for Cortex-M0+/M7. Tested off-target against the testkit `mock_i2c` / `mock_delay`
doubles — see [`tests/test_bme280.cpp`](tests/test_bme280.cpp).
