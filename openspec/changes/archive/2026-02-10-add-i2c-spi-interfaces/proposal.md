## Why

I2C and SPI are essential for communicating with sensors, displays, and other peripherals. Need platform-agnostic interfaces.

## What Changes

- Create I2C interface in `src/hal/interface/i2c.hpp`
- Create SPI interface in `src/hal/interface/spi.hpp`
- Define I2cMaster concept (read, write, scan)
- Define SpiMaster concept (transfer, configure)
- Add error handling for bus errors (NACK, timeout, etc.)

## Impact

- Affected specs: hal-i2c-spi (new capability)
- Affected code: src/hal/interface/i2c.hpp, src/hal/interface/spi.hpp
- Foundation for peripheral drivers (sensors, displays)
