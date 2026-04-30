# Tasks: Device HAL Coverage Report

All phases are host-only.

## 1. Field contracts

- [ ] 1.1 Create `cmake/hal-contracts/` directory.
      Write field contract JSON for: `gpio`, `uart`, `spi`, `i2c`, `timer_pwm`,
      `sdmmc`, `usb`, `eth`, `can`, `adc`, `dac`.
      Format: `{ "module": "...", "required": [...], "optional": [...] }`.
- [ ] 1.2 Add JSON Schema for contract files in
      `cmake/hal-contracts/contract-schema.json`.
      Validate all contract files in CI via `scripts/validate_contracts.py`.
- [ ] 1.3 Keep contracts versioned: add `"contract_version"` field.
      HAL contract version bumps trigger re-score of all devices.

## 2. Coverage computation script

- [ ] 2.1 Write `scripts/hal_coverage.py`:
      - `--device <id>`: load IR JSON for device; run contract scoring.
      - `--all`: score all devices in `alloy-devices/`.
      - `--output json|text|markdown`: choose output format.
      - `--assert-no-regression <baseline.json>`: exit 1 if any score drops.
- [ ] 2.2 Implement scoring formula:
      `score = (req_present + 0.5 * opt_present) / (req_total + 0.5 * opt_total)`.
      Status thresholds: `full ≥ 0.95`, `partial ≥ 0.01`, `absent = 0.0`.
- [ ] 2.3 Add unit tests `tests/scripts/test_hal_coverage.py`:
      construct a fake IR JSON with known fields; assert score matches expected value.
- [ ] 2.4 Test with real `stm32g071rb` IR: verify gpio=full, sdmmc=absent.

## 3. `alloy device info` integration

- [ ] 3.1 Add coverage section to `alloy device info <id>` output.
      Reads `~/.alloy/devices/<family>/coverage/<device>.json` if present;
      falls back to computing from local IR JSON.
- [ ] 3.2 Add `--coverage-only` flag to `alloy device info` for scripting.

## 4. `alloy doctor --coverage` integration

- [ ] 4.1 Add `--coverage [device]` flag to `alloy doctor`.
      Prints per-module coverage table for the current board's device.
- [ ] 4.2 `alloy doctor` without flags: print a one-line summary,
      e.g. `alloy device info: gpio 100%, uart 75%, ...` after existing checks.

## 5. CI publishing

- [ ] 5.1 Add `alloy-devices/.github/workflows/coverage.yml`:
      runs `hal_coverage.py --all --output json` after regen step.
      Writes `coverage/<device>.json` to `gh-pages` branch.
- [ ] 5.2 Implement `--assert-no-regression` check in publish workflow:
      compare against previous release baseline; fail on score drop.
- [ ] 5.3 Generate shields.io endpoint JSON per device:
      `{ "schemaVersion": 1, "label": "HAL coverage", "message": "uart 75%", "color": "yellow" }`.
      Publish to `alloy-rs.dev/coverage/<device>/badge.json`.

## 6. Documentation

- [ ] 6.1 `docs/HAL_COVERAGE.md`: how coverage scores are computed, how to read
      the field contracts, how to improve coverage for a device.
- [ ] 6.2 `docs/PORTING_NEW_PLATFORM.md`: add coverage report as final
      check in the porting checklist.
- [ ] 6.3 Add coverage badges for tier-1 boards to main `README.md`.
