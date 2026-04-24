# Alloy Driver Seed Library

Header-only device drivers built on the public `alloy` HAL surface.

Drivers live under this directory so they are separate from the HAL (`src/hal/**`), the
device descriptor layer (`src/device/**`), the board helpers (`boards/**`), and the examples
(`examples/**`). Drivers never modify any of those trees — they are pure consumers of the
public HAL handle surface.

## What Ships Today

| Driver | Bus | Status |
| --- | --- | --- |
| [SSD1306](display/ssd1306/) | I2C | seed — init, clear, pixel, text, flush |
| [BME280](sensor/bme280/) | I2C | seed — chip-ID check, calibration load, compensated read |
| [W25Q](memory/w25q/) | SPI | seed — JEDEC ID, read, page program, sector erase |
| [AT24MAC402](memory/at24mac402/) | I2C | seed — paged EEPROM read/write, EUI-48/64 + 128-bit serial |
| [KSZ8081RNACA](net/ksz8081/) | MDIO | seed — PHY-ID, soft reset, auto-neg, link status (templated on user MDIO handle) |
| [IS42S16100F-5B](memory/is42s16100f/) | SDRAMC | chip descriptor — geometry + ns→cycles timings for SAME70 SDRAMC init |

These are seed drivers: they cover the most common bring-up surface for their device class.
Deeper coverage (scroll, IIR tuning, chip erase, security registers) is left for follow-up
changes per device.

## Convention

Every driver under this tree follows the same rules:

1. **Header-only.** Drivers ship as a single `.hpp` (plus helper headers like fonts or
   calibration-math inlines).
2. **Templated over the HAL handle.** The driver takes the bus handle type as a template
   parameter, so the same driver compiles against any board whose helper layer exposes the
   bus through the public HAL surface (e.g. `board::make_i2c()`, `board::make_spi()`).
3. **`core::Result<T, core::ErrorCode>` everywhere.** Every fallible operation returns a
   `Result`. Timeout, transport failures, and device-specific errors all surface as
   `ErrorCode` values — no exceptions, no global `errno`.
4. **No dynamic allocation.** No `new`, no `malloc`, no STL containers that allocate. All
   buffers are user-provided (passed in) or fixed-size members.
5. **No ownership of the bus.** The driver stores a reference or pointer to a
   user-configured bus handle. The application configures the bus once and shares it across
   drivers if it wants to.
6. **Documented device and datasheet reference.** Each driver header opens with the device
   class, the datasheet revision it was written against, and any deviations.

## Template

Use this skeleton when adding a new driver:

```cpp
#pragma once

// drivers/<class>/<device>/<device>.hpp
//
// Driver for <vendor> <device> over <bus>.
// Written against datasheet revision <rev>.
// Seed driver: covers init + <primary operation>. See drivers/README.md.

#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace alloy::drivers::<class>::<device> {

struct Config {
    // device-level config (e.g. I2C address, chip-select line, page size)
};

template <typename BusHandle>
class Device {
public:
    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    [[nodiscard]] auto init() -> alloy::core::Result<void, alloy::core::ErrorCode>;
    // ...primary operations...

private:
    BusHandle* bus_;
    Config cfg_;
};

}  // namespace alloy::drivers::<class>::<device>
```

## Using A Seed Driver

```cpp
#include BOARD_I2C_HEADER
#include "drivers/display/ssd1306/ssd1306.hpp"

int main() {
    board::init();

    auto i2c = board::make_i2c();
    if (i2c.configure().is_err()) { /* handle */ }

    alloy::drivers::display::ssd1306::Device display{i2c, {.address = 0x3C}};
    if (display.init().is_err()) { /* handle */ }

    display.clear();
    display.draw_text(0, 0, "hello alloy");
    (void)display.flush();
}
```

## Testing

Each seed driver has a compile test under `tests/compile_tests/test_driver_seed_*.cpp` that
instantiates the driver against a minimal mock bus handle and proves every required method
compiles. If the public HAL surface drifts in a way the driver depends on, those tests fail
to build and the descriptor-contract-smoke release gate catches the regression.
