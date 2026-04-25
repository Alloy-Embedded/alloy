# hello_esp32

Bare-metal bring-up for ESP32-DevKit (Xtensa LX6).
Runs on top of the ESP-IDF 2nd-stage bootloader already in flash.

## What it does

- Prints `[alloy] hello from ESP32` then `[alloy] loop=N` every ~1 s on UART0
- Blinks the built-in blue LED (GPIO2) at ~1 Hz

## Build

```sh
cmake --preset hw-esp32
cmake --build build/hw/esp32 --target hello_esp32
# builds hello_esp32 ELF + hello_esp32.bin (via esptool elf2image)
```

## Flash

```sh
esptool.py --chip esp32 --port /dev/cu.usbserial-* \
    write_flash 0x10000 build/hw/esp32/examples/hello_esp32/hello_esp32.bin
```

Or via alloyctl:

```sh
python scripts/alloyctl.py flash --board esp32_devkit --target hello_esp32
```

## Monitor

```sh
screen /dev/cu.usbserial-* 115200
# or
python scripts/alloyctl.py monitor --board esp32_devkit
```

## Why 0x10000?

The ESP32 partition table (default factory layout) maps the first app to flash
address 0x10000. The 2nd-stage bootloader reads this address from the partition
table and loads the image segments into IRAM/DRAM before jumping to entry.

We do **not** reflash the bootloader (0x1000) or partition table (0x8000).
The ones already in flash from the factory are compatible with our app image.

## Boot flow

```
ROM → bootloader (flash 0x1000) → reads partition table (flash 0x8000)
    → loads app image (flash 0x10000) → copies IRAM/DRAM segments
    → jumps to call_start_cpu0 (startup.S)
    → _alloy_startup (startup.cpp) — BSS/data/ctors
    → main()
```

## UART wiring

UART0 is on the USB-serial chip (CP2102) already on the DevKit board —
no external adapter needed. The `/dev/cu.usbserial-*` port is both flash
and debug UART.
