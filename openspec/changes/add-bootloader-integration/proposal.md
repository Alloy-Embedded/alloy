# Add Bootloader Integration — MCUboot + OTA

## Why

A bare-metal HAL without a firmware update path is a prototype tool, not a production
platform. Every shipping embedded product needs:

1. **Secure bootloader** — verifies image signature before booting. Prevents bricked
   devices from accepting corrupted firmware.
2. **OTA (Over-the-Air) update** — downloads a new firmware image and instructs the
   bootloader to apply it on next boot. Essential for field-deployed devices.
3. **Recovery mode** — if the new image is corrupt or fails startup, the bootloader
   rolls back to the last known-good image.

MCUboot is the industry standard for this: it is used by Zephyr, Apache Mynewt, Amazon
FreeRTOS, and NuttX. It is written in C, BSD-licensed, and well-audited. The flash image
format (`MCUboot image header`, swap/overwrite upgrade modes) is well-documented.

This change integrates MCUboot as an optional bootloader for Alloy projects:

1. **`alloyctl bundle --with-mcuboot`** — wraps a firmware ELF with the MCUboot image
   header, signs it, and produces a ready-to-flash `.bin`.
2. **`drivers/ota/`** — an OTA update client that downloads a firmware image via the
   network (Ethernet/Wi-Fi), writes it to the secondary slot in flash, and sets the
   MCUboot `pending` flag.
3. **Board partition layouts** — predefined MCUboot-compatible partition tables for
   each supported board (primary slot, secondary slot, scratch area, settings partition).

## What Changes

### `tools/alloyctl.py` — MCUboot image wrapping

`alloy bundle --with-mcuboot --key <signing.pem> --version <x.y.z>`:
1. Runs `imgtool sign` (MCUboot's Python tool, installed via pip) on the input ELF.
2. Produces `<target>-<version>-signed.bin` with the MCUboot header.
3. If `--key` is omitted, produces an unsigned image (development mode only, warned).

`alloy flash --mcuboot`: flashes the MCUboot bootloader binary to offset 0x0 and the
application to the primary slot offset (board-dependent).

`alloy recover --slot secondary`: flashes a recovery image to the secondary slot,
triggering a swap on next boot. Useful when OTA went wrong and a JTAG probe is available.

### MCUboot bootloader binaries

Pre-built MCUboot binaries (with Alloy-compatible configurations) for each supported
board are published as GitHub Release assets alongside the `alloy-devices` artifacts.
The `alloyctl doctor` command verifies the pinned MCUboot version matches the installed
`imgtool`.

Board-specific MCUboot configurations live in `boards/<board>/mcuboot/`:
- `mcuboot.conf` — MCUboot Kconfig fragment (signature algorithm, upgrade mode,
  slot sizes)
- `partitions.csv` (ESP32) or `memory_map.cmake` (ARM) — partition layout with
  MCUboot-compatible offsets

### `drivers/ota/` — OTA update client

`ota_client.hpp`: `OtaClient<NetworkInterface, FlashBackend>` (depends on `add-network-hal`
and `add-filesystem-hal`'s `FlashBackend` concept).

```cpp
template <NetworkInterface Net, FlashBackend Flash>
class OtaClient {
public:
    // Download image from URL, write to secondary flash slot, verify hash.
    Result<void> download(std::string_view url, std::span<const std::byte> expected_sha256);
    // Set MCUboot pending flag in the secondary slot trailer.
    Result<void> request_update();
    // Clear pending flag (abort OTA before reboot).
    Result<void> cancel_update();
};
```

`ota_client.cpp`: HTTP GET download using `TcpStream`, chunked write to the secondary
slot via `FlashBackend`, SHA-256 hash verification on the complete image.

### Partition layouts per board

| Board | Bootloader | Primary Slot | Secondary Slot | Scratch | Settings |
|-------|-----------|--------------|----------------|---------|----------|
| nucleo_g071rb | 0x0800_0000, 32K | 0x0800_8000, 96K | 0x0801_8000, 96K | 0x0802_C000, 32K | 0x0803_4000 |
| nucleo_f401re | 0x0800_0000, 32K | 0x0800_8000, 192K | 0x0803_8000, 192K | 0x0806_8000, 32K | 0x0807_0000 |
| same70_xplained | External W25Q | W25Q primary | W25Q secondary | W25Q scratch | Internal NVM |
| esp32_devkit | 0x1000, 60K | 0x10000, 1536K | 0x190000, 1536K | 0x310000, 256K | 0x9000 |
| nrf52840_dk | 0x0, 48K | 0xC000, 472K | 0x82000, 472K | External | 0xF8000 |

### Examples

`examples/mcuboot_blink/`: a blink firmware packaged with MCUboot header and signed.
Flash bootloader + primary slot. Verify bootloader starts the application. On STM32G0
Nucleo.

`examples/ota_update/`: full OTA flow. Device boots, connects to Ethernet (SAME70
Xplained), polls an HTTP server for a firmware update, downloads to secondary slot,
verifies hash, requests swap. On next boot, MCUboot swaps slots and the new firmware
runs.

`examples/ota_rollback/`: OTA update that intentionally has a bad header. MCUboot
detects the bad image and rolls back to the primary slot. Demonstrates the safety
guarantee.

### Documentation

`docs/OTA.md`: MCUboot integration guide, partition layout rationale, signing workflow,
OTA client usage, rollback mechanics, recovery procedure.

## What Does NOT Change

- Applications that do not use MCUboot — no change to existing examples or build flow.
  MCUboot integration is fully opt-in (`--with-mcuboot` flag).
- The `alloy-devices` contract — OTA is an application-layer concern.

## Alternatives Considered

**Custom bootloader:** Writing a custom bootloader from scratch duplicates effort and
lacks the security review MCUboot has received. MCUboot is the correct choice.

**USB DFU only (no OTA):** DFU requires physical access to the device. OTA is essential
for field-deployed devices. Both DFU (from `add-usb-hal`) and OTA are needed.

**Zephyr's OTA (SUIT / MCUmgr):** Zephyr's OTA stack is entangled with Zephyr's OS
concepts and MCUmgr protocol. MCUboot as a standalone bootloader works independently
of any RTOS.
