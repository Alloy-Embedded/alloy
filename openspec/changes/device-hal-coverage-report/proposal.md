# Proposal: Device HAL Coverage Report

## Status
`open` — prerequisite for community contributions and device quality tracking.

## Problem

There is no way to know how complete the alloy HAL support is for a given device.
A user picking `stm32g071rb` cannot tell which HAL modules work, which are partial,
and which are absent. This blocks:

1. **Users**: cannot evaluate alloy for their project without building and
   discovering missing features at link time.
2. **Contributors**: no clear signal of which devices need work.
3. **CI**: no regression signal — a codegen change that silently drops UART
   semantic traits goes undetected.

## Proposed Solution

### Coverage defined by field contracts

Each HAL module (`uart`, `spi`, `i2c`, `gpio`, `sdmmc`, `usb`, etc.) has a
**field contract** — a JSON file listing the IR fields required for full
coverage and which are optional:

```json
// cmake/hal-contracts/uart.json
{
  "module": "uart",
  "required": ["baud_rate_reg", "tx_data_reg", "rx_data_reg",
               "tx_ready_flag", "rx_ready_flag", "enable_field"],
  "optional": ["timingr_field", "kernel_clock_mux", "hardware_flow_ctl",
               "half_duplex_field", "dma_rx_req", "dma_tx_req"]
}
```

### Coverage computation

`scripts/hal_coverage.py` reads the IR JSON for a device and scores each module:

```
coverage(module, device) =
    (required_present + 0.5 * optional_present) /
    (required_total   + 0.5 * optional_total)
```

Output formats:

**Terminal (alloy doctor --coverage)**:
```
Device: stm32g071rb

Module    Required  Optional  Score   Status
gpio      6/6       3/3       100%    FULL
uart      6/6       2/6       75%     PARTIAL
spi       6/6       0/4       67%     PARTIAL
i2c       5/6       0/5       55%     PARTIAL
timer     4/6       0/3       44%     PARTIAL
sdmmc     0/6       0/5       0%      ABSENT
usb       0/6       0/4       0%      ABSENT
```

**JSON (machine-readable)**:
```json
{
  "device": "stm32g071rb",
  "alloy_devices_version": "1.0.0",
  "modules": {
    "gpio":  { "score": 1.00, "status": "full",    "missing_required": [] },
    "uart":  { "score": 0.75, "status": "partial",
               "missing_optional": ["timingr_field","kernel_clock_mux","hardware_flow_ctl","dma_rx_req","dma_tx_req"] },
    "sdmmc": { "score": 0.00, "status": "absent",
               "missing_required": ["cmd_reg","data_reg","status_reg","clock_div","card_detect","block_size"] }
  }
}
```

**Markdown badge (for README)**:
```
[![stm32g071rb HAL](https://img.shields.io/endpoint?url=...)](https://alloy-rs.dev/coverage/stm32g071rb)
```

### Coverage published to alloy-rs.dev

CI pipeline:
1. After codegen regen, run `hal_coverage.py` for every device.
2. Write `coverage/<device>.json` to the GitHub Pages branch.
3. Serve via `alloy-rs.dev/coverage/<device>.json`.
4. Shields.io dynamic badge reads the endpoint.

### `alloy device info <id>` integration

```sh
alloy device info stm32g071rb
# ...
# HAL coverage:
#   gpio   100% (full)
#   uart    75% (partial) — missing: timingr, kernel_clock_mux, flow_ctl, dma_rx, dma_tx
#   sdmmc   0%  (absent)
```

### CI regression check

`alloy-devices` CI runs `hal_coverage.py --assert-no-regression` comparing the
new coverage JSON against the previous release. Any module that drops in score
(required field removed) is a build failure.

## Non-goals

- Runtime coverage (which code paths execute on hardware) — this is compile-time
  IR field presence only.
- Per-register bit coverage.
