# alloy libraries

Third-party drivers for sensors, displays, and other parts — **grown entirely outside the core**.
A library never names a chip, a family, or a board. It templates on the horizontal
[concepts](../src/alloy/concepts.hpp) (`I2cBus`, `SpiBus`, `OutputPin`, `DelayNs`, …), so the same
driver works on every alloy board and every future one, with no change to the framework.

That is the whole bet: **the ecosystem can grow without touching `src/`.** These reference drivers
live in-tree only to seed the registry; nothing about the format requires it. An external driver in
its own git repo is identical in shape, and the registry references it the same way.

## What a library is

A self-contained directory:

```
mylib/
  alloy.lib.toml        # manifest: name, version, category, required concepts
  include/mylib.hpp     # header-only C++23, templated on concepts — the driver
  tests/test_mylib.cpp  # host test against the testkit mock bus (no hardware)
  README.md
```

### The manifest (`alloy.lib.toml`)

```toml
[lib]
name = "sht31"
version = "0.1.0"
description = "Sensirion SHT3x temperature + humidity sensor (I2C)"
category = "sensor"          # sensor | display | actuator | storage | io | comms
license = "MIT"
homepage = "https://sensirion.com/sht3x"

[requires]
concepts = ["I2cBus", "DelayNs"]   # the contracts the driver constrains on
alloy = ">=0.1"                     # minimum framework version

[headers]
include = ["include"]               # dirs added to the compile include path
namespace = "alloy::lib"            # where the driver's symbols live
```

### The driver

Constrained ONLY on concepts — this is what makes it portable:

```cpp
template <class Bus, class Delay>
    requires alloy::I2cBus<Bus> && alloy::DelayNs<Delay>
class sht31 { /* ... talks to `const Bus&` and `Delay&`, never a register address ... */ };
```

## Using a library

```
alloy lib list                 # browse the registry
alloy lib search humidity      # filter by name/summary/category
alloy lib info sht31           # manifest + required concepts
alloy lib add sht31            # vendor it into ./libs/sht31 and wire the include path
```

`add` records the library in your project's `alloy.toml` under `[libs]` and the build picks up its
include directory automatically. Then:

```cpp
#include <sht31.hpp>
auto sensor = alloy::lib::sht31{i2c, delay};
auto s = sensor.measure();     // s.temperature_c, s.humidity_pct
```

## Testing a library off-target

Every reference library ships a host test that exercises the driver against
[`testkit/mock_bus.hpp`](testkit/mock_bus.hpp) — scriptable `mock_i2c` / `mock_spi` / `mock_delay`
doubles that satisfy the same concepts. No board required, and CI compiles + runs every one, so a
library that regresses can't ship (NORTH_STAR: "no feature ships unless CI compiles what it emits").
