# mpu6050

InvenSense **MPU-6050** 6-axis IMU (3-axis accelerometer + 3-axis gyroscope)
over I2C. Portable: templated on the `alloy::I2cBus` concept only — no chip,
family, or board named.

## Concepts

| Template param | Concept          |
|----------------|------------------|
| `Bus`          | `alloy::I2cBus`  |

The driver holds the bus by `const&`; the rvalue-bus constructor is deleted to
prevent dangling.

## API

```cpp
#include "mpu6050.hpp"
using alloy::lib::mpu6050;

mpu6050 imu{i2c};              // default address 0x68 (AD0 = GND); 0x69 = VDD

std::uint8_t id = 0;
if (imu.identify(id) && id == mpu6050<decltype(i2c)>::who_am_i) {
    imu.wake();                // clear PWR_MGMT_1 (0x6B) — leaves power-on sleep
    auto s = imu.read();       // burst accel (0x3B) + gyro (0x43)
    if (s.valid) {
        float g   = s.az / mpu6050<decltype(i2c)>::accel_lsb_per_g;   // +/-2 g
        float dps = s.gz / mpu6050<decltype(i2c)>::gyro_lsb_per_dps;  // +/-250 dps
    }
}
```

- `identify(id)` — reads `WHO_AM_I` (0x75); a healthy part returns `0x68`.
- `wake()` — writes `PWR_MGMT_1 = 0` to exit sleep and select the internal
  oscillator.
- `read()` — returns `{ax, ay, az, gx, gy, gz, valid}` as raw big-endian
  `int16` counts. `valid == false` on any bus NACK.

## Scaling

Raw counts use the power-on default full-scale ranges. Divide by the constexpr
sensitivities:

| Constant             | Value   | Range        |
|----------------------|---------|--------------|
| `accel_lsb_per_g`    | 16384.0 | +/-2 g       |
| `gyro_lsb_per_dps`   | 131.0   | +/-250 deg/s |

## Failure model

Every method reports honestly: a NACK or bus error yields `false` (or
`valid = false` from `read()`), and `identify()` leaves its out-param
untouched on failure. No values are fabricated.

Register/address facts are from the InvenSense MPU-6000/6050 Register Map
(RM-MPU-6000A-00, rev 4.2).
