# IS42S16100F SDRAM Probe — SAME70 Xplained Ultra

Hardware smoke test for the `alloy::drivers::memory::is42s16100f` chip
descriptor on the ISSI IS42S16100F-5B SDRAM (U6) on the SAME70 Xplained
Ultra.

## What This Example Verifies

1. The descriptor's constexpr geometry (2 banks × 2048 rows × 256 columns ×
   16 bits) matches the 16 Mbit capacity.
2. `timings_for_bus(MCK)` produces plausible bus-clock values (printed so
   you can eyeball them against the datasheet for your MCK preset).
3. PMC peripheral clocks for PIOA/PIOC/PIOD and SDRAMC (PID 62) come up.
4. Every SDRAM pin muxes to its peripheral-A assignment.
5. `MATRIX.CCFG_SMCNFCS.SDRAMEN` routes the `0x70000000` window to the
   SDRAMC chip-select.
6. The JEDEC init sequence (NOP → PALL → 8× REFRESH → LMR → NORMAL) runs to
   completion without wedging the controller.
7. An 8 KiB address-XOR pattern written at `0x70000000` reads back
   byte-for-byte.

## Build

```bash
cmake --preset same70-analysis-debug
cmake --build build/presets/same70-analysis-debug --target driver_is42s16100f_probe
```

Output artefacts:

```
build/presets/same70-analysis-debug/examples/driver_is42s16100f_probe/
  driver_is42s16100f_probe.elf
  driver_is42s16100f_probe.hex
  driver_is42s16100f_probe.bin
```

## Flash + Monitor (one-liner)

From the repo root, with the SAME70 Xplained connected on the **DEBUG** USB
port:

```bash
python3 scripts/alloyctl.py run --board same70_xplained \
    --target driver_is42s16100f_probe
```

This builds, flashes via **openocd** (`board/atmel_same70_xplained.cfg`),
resets, and opens the serial monitor at `115200 8N1`. `Ctrl+C` exits the
monitor.

Short-cut for SAME70:

```bash
./scripts/same70_run_example.sh driver_is42s16100f_probe
```

Prereqs: `arm-none-eabi-gcc`, `openocd`, `python3 -m pip install pyserial`.
No bossac required.

### Manual monitor

```bash
python3 scripts/alloyctl.py monitor --board same70_xplained
```

## Expected Output

```
is42s16100f probe: ready
is42s16100f: capacity = 2097152 bytes (16 Mbit), x16 bus, CAS=3
is42s16100f: timings @ MCK = t_rc=9 t_rcd=3 t_rp=3 t_ras=6 t_wr=3 t_rfc=9 refresh=4688
is42s16100f: SDRAMC init ok
is42s16100f: 8 KiB pattern PASS
is42s16100f: PROBE PASS
```

The exact timing numbers scale with your MCK: at 150 MHz MCK you should see
`t_rc = ceil(60 ns × 150e6) = 9`, `t_ras = ceil(37 × 150e6) = 6`, and
`refresh = ceil(31'250 × 150e6) = 4688`. Any significant deviation means
either the board clock preset changed or the descriptor was edited.

LED blinks slowly (500 ms) on pass and rapidly (100 ms) on failure.

## Failure Interpretation

| UART line | Meaning | Typical cause |
| --- | --- | --- |
| (probe hangs before `SDRAMC init ok`) | SDRAMC or a PIO didn't come out of reset | PMC gate off, `SDRAMEN` not set, board PSU not delivering 3.3 V rail to U6 |
| `FAIL (pattern mismatch @ 0x70000XXX: wrote=YY read=ZZ)` | Bytes read don't match bytes written | Stuck data bit (diff between wrote/read isolates it), missed pin mux, wrong CAS latency, or timings too tight for the MCK preset |

## Notes

- The probe walks only the first **8 KiB** for speed. A healthy SDRAM will
  either pass this check or fail in the first few hundred bytes — a full-
  surface test is a separate exercise and adds seconds of run time.
- `busy_wait_us` is a dumb cycle-counting loop so the probe has no
  dependency on SysTick ordering relative to SDRAMC bring-up. It overshoots
  at `-O2`; that's fine, datasheet delays all have floors, no ceilings.
- The probe does **not** touch the SDRAM after the check — any subsequent
  stack/heap placement into SDRAM would collide with the pattern, so the
  linker script still points RAM at on-chip SRAM. If you want to migrate
  sections into SDRAM, fork the linker script, don't just rerun the probe.
