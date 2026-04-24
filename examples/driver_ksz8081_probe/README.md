# KSZ8081 Driver Probe — SAME70 Xplained Ultra

Hardware smoke test for the `alloy::drivers::net::ksz8081` seed driver on the
Microchip KSZ8081RNACA (U5) on the SAME70 Xplained Ultra.

## What This Example Verifies

1. PMC peripheral clocks come up for PIOD (PID 16) and GMAC (PID 39).
2. `PD8` (GMDC) and `PD9` (GMDIO) mux to peripheral A.
3. The GMAC management port drives MDIO transactions with MDC under the
   KSZ8081's 2.5 MHz ceiling (MCK=150 MHz → NCFGR.CLK=4 → MDC ≈ 2.34 MHz).
4. PHY_ID1 reads back `0x0022` and PHY_ID2 reads back `0x156x` (low nibble
   is silicon revision). These are fixed in the KSZ8081 datasheet §5.1.5.
5. `Device::init()` soft-resets the PHY, loads the advertisement register,
   and restarts auto-negotiation without timing out.
6. `read_link_status()` reports link up + speed + duplex when a cable is
   plugged into a working partner. No cable is still a PASS (MDIO proved).

## Build

```bash
cmake --preset same70-analysis-debug
cmake --build build/presets/same70-analysis-debug --target driver_ksz8081_probe
```

Output artefacts:

```
build/presets/same70-analysis-debug/examples/driver_ksz8081_probe/
  driver_ksz8081_probe.elf
  driver_ksz8081_probe.hex
  driver_ksz8081_probe.bin
```

## Flash + Monitor (one-liner)

From the repo root, with the SAME70 Xplained connected on the **DEBUG** USB
port:

```bash
python3 scripts/alloyctl.py run --board same70_xplained \
    --target driver_ksz8081_probe
```

This builds, flashes via **openocd** (`board/atmel_same70_xplained.cfg`),
resets, and opens the serial monitor at `115200 8N1`. `Ctrl+C` exits the
monitor.

Short-cut for SAME70:

```bash
./scripts/same70_run_example.sh driver_ksz8081_probe
```

Prereqs: `arm-none-eabi-gcc`, `openocd`, `python3 -m pip install pyserial`.
No bossac required.

### Manual monitor

```bash
python3 scripts/alloyctl.py monitor --board same70_xplained
```

## Expected Output

With a live 100BASE-TX full-duplex partner:

```
ksz8081 probe: ready
ksz8081: PHY_ID1 = 0022
ksz8081: PHY_ID2 = 1561
ksz8081: init ok
ksz8081: link UP, 100 Mbps, full duplex
ksz8081: PROBE PASS
```

No cable plugged in (still PASS — proves MDIO is healthy):

```
ksz8081 probe: ready
ksz8081: PHY_ID1 = 0022
ksz8081: PHY_ID2 = 1561
ksz8081: init ok
ksz8081: link DOWN (no partner) — MDIO path OK
ksz8081: PROBE PASS
```

The user LED blinks slowly (500 ms) on pass and rapidly (100 ms) on failure.

## Failure Interpretation

| UART line | Meaning | Typical cause |
| --- | --- | --- |
| `FAIL (MDIO read PHY_ID1)` | GMAC management IDLE never asserted | PMC gate off, NCR.MPE not set, NCFGR.CLK too fast for the part |
| `PHY_ID1 = FFFF` | MDIO floating | PD8/PD9 still on GPIO, pull-ups missing, PHY in reset |
| `PHY_ID1 = 0000` | MDIO tied low | Wrong pin mux (peripheral B/C/D instead of A) |
| `FAIL (init — PHY ID mismatch or MDIO timeout)` | IDs didn't match KSZ8081 | Wrong strap PHY address, talking to the wrong PHY, or non-KSZ8081 carrier |
| `FAIL (read_link_status)` | MDIO broke after init | Clock glitch, hot-unplugged jumper |

## Notes

- The probe does **not** bring up the GMAC DMA or queues — it only exercises
  the management interface. You will not see network traffic; that belongs
  to a higher-level example.
- If your board strap puts the PHY at a non-zero address, pass
  `{.phy_address = N}` to `Device{mdio, {...}}` and update the `Same70GmacMdio`
  call sites accordingly.
- The adapter polls `GMAC_NSR.IDLE` without a timeout source other than a
  loop counter. That is intentional — this example is a bring-up probe, not
  a production driver — but it means a hardware-wedged GMAC shows up as a
  hang. Reset the board.
