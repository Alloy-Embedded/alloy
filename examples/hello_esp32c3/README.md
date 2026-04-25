# hello_esp32c3

Bare-metal bring-up example for ESP32-C3-DevKitM-1.
Uses direct boot (no second-stage bootloader), ROM-configured UART, raw GPIO.

## What it does

- Prints `[alloy] hello from ESP32-C3` then `[alloy] loop=N` every ~1 s
- Toggles GPIO8 (WS2812 data pin) at ~1 Hz as heartbeat

## Build

```sh
cmake --preset hw-esp32c3
cmake --build build/hw/esp32c3 --target hello_esp32c3
```

Output: `build/hw/esp32c3/examples/hello_esp32c3/hello_esp32c3.bin`

## Flash

```sh
# Install esptool if needed: pip install esptool
esptool.py --chip esp32c3 --port /dev/cu.usbserial-* write_flash 0x0 \
    build/hw/esp32c3/examples/hello_esp32c3/hello_esp32c3.bin
```

Or via alloyctl (auto-detects port):

```sh
python scripts/alloyctl.py flash --board esp32c3_devkitm --target hello_esp32c3
```

## Monitor

```sh
# macOS
screen /dev/cu.usbserial-* 115200

# or
python scripts/alloyctl.py monitor --board esp32c3_devkitm
```

## Wiring

| ESP32-C3 pin | Signal |
|-------------|--------|
| GPIO21      | UART TX → USB-serial RXD |
| GPIO20      | UART RX ← USB-serial TXD |
| GND         | GND |

The on-board USB port (USB_D+/USB_D-) enumerates as CDC/JTAG — use it with
`esptool.py` for flashing. A separate USB-serial adapter on GPIO21/20 is
needed for the debug UART.

## Direct boot

The binary starts with magic word `0xaebd041d` at byte 0. The ROM bootloader
detects this and jumps directly to byte 4 (`_start`) without loading a
second-stage bootloader. No partition table is needed.
