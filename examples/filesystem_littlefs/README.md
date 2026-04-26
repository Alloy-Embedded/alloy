# filesystem_littlefs

Persistent boot counter stored in [LittleFS](https://github.com/littlefs-project/littlefs)
on a Winbond W25Q128 NOR flash (16 MiB) via SPI0 on the **SAME70 Xplained Ultra**.

## What it does

| Boot | Action |
|------|--------|
| First | Formats filesystem, writes `/counter.txt` = `1` |
| Subsequent | Reads counter, increments, writes back |

UART output (115200, 8N1):

```
[fs] booting
[fs] flash ok
[fs] mounted          ← or "formatting and mounted" on first boot
[fs] counter=42
[fs] done
```

## Hardware wiring (W25Q128 → EXT1 header)

| W25Q128 pin | SAME70 pin | EXT1 label |
|-------------|-----------|------------|
| /CS         | PD25      | PIN 15     |
| CLK         | PD22      | PIN 9 (SPCK) |
| MISO (DO)   | PD20      | PIN 5 (MISO) |
| MOSI (DI)   | PD21      | PIN 7 (MOSI) |
| VCC         | 3V3       | PIN 20     |
| GND         | GND       | PIN 19     |

> PD26 (EXT1 pin 14) is reserved for the SD card /CS in the
> `filesystem_fatfs_sdcard` example — both devices share the SPI0 bus.

## Power-loss safety

LittleFS guarantees filesystem consistency across power cuts.  Pull power
mid-write — on the next boot the old counter value is recovered intact.

## Build

```bash
alloy bundle --board same70_xplained examples/filesystem_littlefs
alloy flash  --board same70_xplained filesystem_littlefs.bin
```
