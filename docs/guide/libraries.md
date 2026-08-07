# Driver libraries

Sensors, displays, clocks and other parts live **outside the framework core**, as small
header-only libraries you vendor into your project:

```console
$ alloy lib search humidity
sht31    sensor    Sensirion SHT3x temperature + humidity sensor (I2C)

$ alloy lib add sht31
added sht31 0.1.0 -> ./libs/sht31
  include it: #include <sht31.hpp>   (namespace alloy::lib)
  the build now compiles ./libs/*/include onto the include path
```

```cpp
#include <sht31.hpp>

auto bus = board::i2c::open({.speed_hz = 100'000});
alloy::delay d;
alloy::lib::sht31 sensor{bus, d};

const auto r = sensor.measure();
if (r.valid) { /* r.temperature_c, r.humidity_pct */ }
```

## Why they are outside the core

A driver never names a chip, a family, or a board. It is templated on the framework's horizontal
**concepts** — `I2cBus`, `SpiBus`, `OutputPin`, `DelayNs` — so it compiles against any board that
satisfies them, including boards that do not exist yet:

```cpp
template <class Bus, class Delay>
    requires alloy::I2cBus<Bus> && alloy::DelayNs<Delay>
class sht31 { /* talks to a `const Bus&`, never to a register address */ };
```

That is the whole bet: **the ecosystem grows without touching `src/`.** The reference drivers
happen to live in-tree to seed the registry, but nothing about the format requires it — a driver
in its own git repository is identical in shape.

## What ships today

| Library | Category | Part |
| --- | --- | --- |
| `sht31` | sensor | Sensirion SHT3x temperature + humidity (I²C) |
| `bme280` | sensor | Bosch BME280 temperature + pressure + humidity (I²C) |
| `bh1750` | sensor | ROHM BH1750FVI ambient light (I²C) |
| `mpu6050` | sensor | InvenSense MPU-6050 6-axis IMU (I²C) |
| `ds3231` | rtc | Maxim DS3231 high-accuracy RTC (I²C) |
| `ssd1306` | display | Solomon Systech SSD1306 128×64 OLED (I²C) |

```console
$ alloy lib list                 # browse the registry
$ alloy lib info sht31           # manifest + the concepts it requires
$ alloy lib add sht31            # vendor into ./libs and wire the build
```

`add` copies the library into `./libs/<name>`, records it in your `alloy.toml` under `[libs]`,
and the build picks up the include directory. It is *vendored*, not fetched at build time: your
project keeps building when a registry entry moves.

## Writing one

A library is a directory with four things:

```
mylib/
├── alloy.lib.toml        # manifest
├── include/mylib.hpp     # header-only C++23, templated on concepts
├── tests/test_mylib.cpp  # host test against the mock buses
└── README.md
```

```toml title="alloy.lib.toml"
[lib]
name = "sht31"
version = "0.1.0"
description = "Sensirion SHT3x temperature + humidity sensor (I2C)"
category = "sensor"          # sensor | display | actuator | storage | io | comms | rtc
license = "MIT"

[requires]
concepts = ["I2cBus", "DelayNs"]   # the contracts the driver constrains on
alloy = ">=0.1"

[headers]
include = ["include"]
namespace = "alloy::lib"
```

Three habits separate a good alloy driver from a port of vendor sample code:

- **Constrain on concepts, not on types.** If the driver mentions a peripheral name, it is no
  longer portable.
- **Never return a value you did not verify.** The SHT31 driver checks the sensor's CRC-8 on each
  pair and marks the reading `valid = false` on mismatch, rather than handing back plausible
  garbage.
- **Cite the datasheet in the code.** Every magic number in these drivers carries the document
  and revision it came from — the same rule the framework applies to its own register work.

## Testing without hardware

Each library ships a host test that runs the driver against the scriptable doubles in
`libs/testkit/mock_bus.hpp` — `mock_i2c`, `mock_spi`, `mock_delay`, which satisfy the same
concepts a real bus does. You queue the bytes the part would answer with, run the driver, and
assert on what it did.

```console
$ alloy test
```

CI compiles and runs every library's tests, so a driver that regresses cannot ship. This is also
the fastest way to develop one: the sensor's protocol is a pure function of bytes in and bytes
out, and you can iterate on it on your laptop long before the part arrives.

## Publishing

The registry (`libs/registry.toml`) is generated from the manifests, so the index cannot drift
from what the libraries actually declare. An in-tree entry points at a path; an external one
points at a repository and a ref:

```toml
[sht31]
version = "0.1.0"
category = "sensor"
summary = "Sensirion SHT3x temperature + humidity sensor (I2C)"
concepts = ["I2cBus", "DelayNs"]
source = "git:https://github.com/you/alloy-sht31@v0.1.0"
```

Same manifest, same `alloy lib add`, no framework change — which is the point.

!!! tip "Drivers and coroutines mix"
    A blocking driver call inside an async task blocks *that task only*.
    [`examples/async_sensor`](https://github.com/Alloy-Embedded/alloy/tree/main/examples/async_sensor)
    reads an SHT31 once a second in one task while another keeps an LED blinking on its own
    cadence — no RTOS, no heap. See [Async tasks](async.md).
