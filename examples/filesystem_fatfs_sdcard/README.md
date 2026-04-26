# filesystem_fatfs_sdcard

Boot logger stored in [FatFS](http://elm-chan.org/fsw/ff/) on an SD card
(SPI mode) via SPI0 on the **SAME70 Xplained Ultra**.

## What it does

On every boot:
1. Initialises the SD card (SDHC/SDXC only).
2. Mounts the FAT32 volume (the card must be pre-formatted on a host).
3. Reads `/boot.txt` for the persistent boot count, increments it, writes back.
4. Appends one timestamped entry to `/log.txt`.
5. Reads the last entry back and prints it for verification.

UART output (115200, 8N1):

```
[sd] booting
[sd] card ok
[sd] mounted
[sd] wrote: boot=3 tick=47
[sd] last log: boot=3 tick=47
[sd] done
```

## Hardware wiring (SD card → SPI0 EXT1)

| SD card pin | SAME70 pin | EXT1 label |
|-------------|-----------|------------|
| /CS         | PD26      | PIN 14     |
| CLK         | PD22      | PIN 9 (SPCK) |
| MISO (DO)   | PD20      | PIN 5 (MISO) |
| MOSI (DI)   | PD21      | PIN 7 (MOSI) |
| VCC         | 3V3       | PIN 20     |
| GND         | GND       | PIN 19     |

> **Note**: PD25 (EXT1 pin 15) is reserved for the W25Q128 /CS used in the
> `filesystem_littlefs` example.  Both devices can share the SPI0 bus
> simultaneously using their respective CS pins.

## Card requirements

- SDHC or SDXC only (SDSC / legacy SD is rejected).
- Pre-formatted FAT32 on the host before first use.

```bash
# Linux/macOS — replace /dev/sdX with your card device
mkfs.fat -F 32 /dev/sdX
```

## Build

```bash
alloy bundle --board same70_xplained examples/filesystem_fatfs_sdcard
alloy flash --board same70_xplained filesystem_fatfs_sdcard.bin
```
