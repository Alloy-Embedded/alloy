# Alloy Driver Catalog

> Auto-generated from [`drivers/MANIFEST.json`](../drivers/MANIFEST.json).  
> Run `alloyctl info --drivers` for a machine-readable summary.

Status legend: **compile-review** = instantiates against mock bus, not yet hardware-validated. **hardware-validated** = tested on real silicon.

---

## Display

| Driver | Chips | Interface | Status | Example |
|--------|-------|-----------|--------|---------|
| [ssd1306](../drivers/display/ssd1306/ssd1306.hpp) | SSD1306 | I2C + SPI | compile-review | `driver_ssd1306_probe` |
| [st7789](../drivers/display/st7789/st7789.hpp) | ST7789 | SPI | compile-review | `driver_st7789_probe` |
| [ili9341](../drivers/display/ili9341/ili9341.hpp) | ILI9341 | SPI | compile-review | `driver_ili9341_probe` |
| [uc8151](../drivers/display/uc8151/uc8151.hpp) | UC8151 | SPI | compile-review | `driver_uc8151_probe` |
| [tm1637](../drivers/display/tm1637/tm1637.hpp) | TM1637 | GPIO | compile-review | `driver_tm1637_probe` |

## Sensor

| Driver | Chips | Interface | Status | Example |
|--------|-------|-----------|--------|---------|
| [bme280](../drivers/sensor/bme280/bme280.hpp) | BME280 | I2C | compile-review | `driver_bme280_probe` |
| [sht4x](../drivers/sensor/sht4x/sht4x.hpp) | SHT40, SHT41 | I2C | compile-review | `driver_sht4x_probe` |
| [aht20](../drivers/sensor/aht20/aht20.hpp) | AHT20 | I2C | compile-review | `driver_aht20_probe` |
| [lps22hh](../drivers/sensor/lps22hh/lps22hh.hpp) | LPS22HH | I2C | compile-review | `driver_lps22hh_probe` |
| [icm42688p](../drivers/sensor/icm42688p/icm42688p.hpp) | ICM-42688-P | SPI | compile-review | `driver_icm42688p_probe` |
| [mpu6050](../drivers/sensor/mpu6050/mpu6050.hpp) | MPU-6050 | I2C | compile-review | `driver_mpu6050_probe` |
| [lsm6dsox](../drivers/sensor/lsm6dsox/lsm6dsox.hpp) | LSM6DSOX | I2C | compile-review | `driver_lsm6dsox_probe` |
| [lis3mdl](../drivers/sensor/lis3mdl/lis3mdl.hpp) | LIS3MDL | I2C | compile-review | `driver_lis3mdl_probe` |

## Memory

| Driver | Chips | Interface | Status | Example |
|--------|-------|-----------|--------|---------|
| [w25q](../drivers/memory/w25q/w25q.hpp) | W25Q16/32/64/128/256 | SPI | compile-review | `filesystem_littlefs` |
| [at24mac402](../drivers/memory/at24mac402/at24mac402.hpp) | AT24MAC402 | I2C | compile-review | `driver_at24mac402_probe` |
| [is42s16100f](../drivers/memory/is42s16100f/is42s16100f.hpp) | IS42S16100F | SDRAMC | compile-review | `driver_is42s16100f_probe` |
| [sdcard](../drivers/memory/sdcard/sdcard.hpp) | SD/SDHC/SDXC | SPI | compile-review | `filesystem_fatfs_sdcard` |
| [fm25v10](../drivers/memory/fm25v10/fm25v10.hpp) | FM25V10 | SPI | compile-review | `driver_fm25v10_probe` |

## Net / Connectivity

| Driver | Chips | Interface | Status | Example |
|--------|-------|-----------|--------|---------|
| [ksz8081](../drivers/net/ksz8081/ksz8081.hpp) | KSZ8081 | MDIO | compile-review | `driver_ksz8081_probe` |
| [nmea_parser](../drivers/net/nmea_parser/nmea_parser.hpp) | generic GPS | UART | compile-review | `driver_nmea_parser_probe` |
| [sx1276](../drivers/net/sx1276/sx1276.hpp) | SX1276 | SPI | compile-review | `driver_sx1276_probe` |
| [rc522](../drivers/net/rc522/rc522.hpp) | MFRC522 | SPI | compile-review | `driver_rc522_probe` |

## Power

| Driver | Chips | Interface | Status | Example |
|--------|-------|-----------|--------|---------|
| [max17048](../drivers/power/max17048/max17048.hpp) | MAX17048/49 | I2C | compile-review | `driver_max17048_probe` |
| [ina219](../drivers/power/ina219/ina219.hpp) | INA219 | I2C | compile-review | `driver_ina219_probe` |
| [ina3221](../drivers/power/ina3221/ina3221.hpp) | INA3221 | I2C | compile-review | `driver_ina3221_probe` |

---

## Adding a new driver

```bash
alloyctl new-driver --name <name> --interface <i2c|spi|uart|1wire> --category <sensor|display|memory|net|power>
```

Generates the three required files (driver header, compile test, probe example) following [`drivers/DRIVER_SPEC.md`](../drivers/DRIVER_SPEC.md).
After implementing, add an entry to [`drivers/MANIFEST.json`](../drivers/MANIFEST.json).
