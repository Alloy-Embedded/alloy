# AT24MAC402 Driver Probe — SAME70 Xplained Ultra

Hardware smoke test for the `alloy::drivers::memory::at24mac402` seed driver on the
Microchip AT24MAC402 (U9) soldered on the SAME70 Xplained Ultra board.

## What This Example Verifies

1. TWIHS0 brings up on `PA3` (TWD0) / `PA4` (TWCK0).
2. The AT24MAC402 ACKs on both logical I2C addresses:
   - `0x57` — EEPROM block
   - `0x5F` — protected block (EUI-48 / EUI-64 / 128-bit serial)
3. The factory-programmed EUI-48 starts with the Microchip OUI `FC:C2:3D`.
4. A 32-byte pattern written at EEPROM offset `0x08` (crossing the 16-byte page
   boundary at offset `0x10`) reads back byte-for-byte, i.e. the driver splits
   the write into per-page transactions correctly.

## Build

```bash
cmake --preset same70-analysis-debug
cmake --build build/presets/same70-analysis-debug --target driver_at24mac402_probe
```

Output artefacts:

```
build/presets/same70-analysis-debug/examples/driver_at24mac402_probe/
  driver_at24mac402_probe.elf
  driver_at24mac402_probe.hex
  driver_at24mac402_probe.bin
```

## Flash + Monitor (one-liner)

From the repo root, with the SAME70 Xplained connected on the **DEBUG** USB
port:

```bash
python3 scripts/alloyctl.py run --board same70_xplained \
    --target driver_at24mac402_probe
```

This configures the build dir (if needed), builds the target, flashes via
**openocd** using `board/atmel_same70_xplained.cfg`, resets, and opens the
serial monitor at `115200 8N1`. `Ctrl+C` exits the monitor; the board keeps
running.

Short-cut for SAME70:

```bash
./scripts/same70_run_example.sh driver_at24mac402_probe
```

Prereqs: `arm-none-eabi-gcc`, `openocd`, `python3 -m pip install pyserial`.
No bossac required.

If openocd can't attach, press **RESET** on the board and rerun. For a full
chip recover (erases GPNVM1 too), use
`python3 scripts/alloyctl.py recover --board same70_xplained --target driver_at24mac402_probe`.

### Manual monitor

If you already flashed by other means and only want the serial output:

```bash
python3 scripts/alloyctl.py monitor --board same70_xplained
# or
python3 scripts/uart_monitor.py
```

## Expected Output

```
at24mac402 probe: ready
at24mac402: init ok
at24mac402: EUI-48 = FC:C2:3D:XX:XX:XX
at24mac402: serial = 00 FC C2 3D XX XX XX XX XX XX XX XX XX XX XX XX
at24mac402: write + readback PASS
at24mac402: PROBE PASS
```

`XX` values are chip-unique. The common prefix `FC:C2:3D` is the Microchip IEEE
OUI — every genuine AT24MAC402 reports it. A different prefix usually means the
probe is talking to the wrong I2C address block.

The user LED blinks slowly (500 ms) on pass and rapidly (100 ms) on any failure.

## Failure Interpretation

| UART line | Meaning | Typical cause |
| --- | --- | --- |
| `FAIL (i2c configure)` | TWIHS0 refused to come up | PMC gate off, or board clock preset mismatch |
| `FAIL (init / device not ACKing)` | I2C transaction never got ACK | Wrong strap address, bad pull-ups, U9 depopulated |
| `FAIL (read_eui48)` / `FAIL (read_serial_number)` | Protected block read failed | Strap does not put the protected block at `0x5F`; override `Config::protected_address` |
| `FAIL (write)` | Page write NAKed | `WP` pin tied high on the carrier — this example assumes `WP = GND` |
| `FAIL (readback)` | Readback transaction failed | Same causes as `init` |
| `FAIL (pattern mismatch)` | Bytes came back different | Usually means the page-split logic is broken — file a bug |

## Notes

- The probe rewrites EEPROM bytes `0x08..0x27` with the pattern `0xA0..0xBF`.
  If you care about preserving application data there, power-cycle instead of
  re-running the probe once you see PASS.
- The driver does **not** drive the `WP` pin; the caller owns it. The SAME70
  Xplained Ultra hard-wires `WP = GND` so writes are always permitted.
