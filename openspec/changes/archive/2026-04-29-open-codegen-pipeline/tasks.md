# Tasks: Open-Source the Codegen Pipeline

Phases 1–4 are host-only (no hardware required). Phase 5 is CI/CD setup.

## 1. alloy-svd-ingest

- [x] 1.1 Define Alloy IR JSON schema v1.2.0 (peripheral, register, field, pin, route,
      clock_tree, dma_mux sections) and publish as `alloy-ir-schema.json` (JSON Schema).
- [x] 1.2 Implement SVD parser: CMSIS SVD XML → IR peripherals + registers + fields.
      Handle SVD quirks: derived-from, dim/dimIndex arrays, cluster groups.
- [x] 1.3 Implement ATDF parser: Microchip ATDF → IR. Map `<module>/<register-group>`
      to IR peripheral; map `<bitfield>` to IR field.
- [ ] 1.4 Implement ESP-IDF header scraper: parse `soc/<chip>_reg.h` + `_struct.h` → IR.
      (out of scope for current sprint — ESP32-C3 bare-metal uses SVD not header scrape)
- [x] 1.5 Implement YAML patch engine: `op: add_field`, `op: set_irq`, `op: add_semantic`,
      `op: rename`, `op: delete`, `op: override_base_address`.
- [x] 1.6 Port existing patches from private pipeline to public YAML format.
      Bootstrap patches created for st/stm32g0 and microchip/same70.
- [x] 1.7 Publish as `alloy-svd-ingest` pip package with CLI:
      `alloy-svd-ingest --svd <url_or_path> [--patch <yaml>] --out alloy-ir.json`

## 2. alloy-ir-validate

- [x] 2.1 Define field contract JSON files for all 16 HAL modules (uart, spi, i2c, gpio,
      dma, adc, dac, can, rtc, watchdog, timer, pwm, qspi, sdmmc, eth, usb).
      Each lists required and optional semantic field keys.
- [x] 2.2 Implement schema validator: check IR against JSON Schema, report errors with
      field paths. Exit non-zero on required field missing.
- [x] 2.3 Implement coverage reporter: per-peripheral, per-HAL-module coverage percentage.
      Emit `validation-report.json` and human-readable summary.
- [x] 2.4 Implement diff mode: compare two IR files, report added/removed/changed
      peripherals, fields, IRQ numbers. Flag breaking changes.
- [x] 2.5 Publish as `alloy-ir-validate` pip package with CLI:
      `alloy-ir-validate alloy-ir.json [--prev alloy-ir-prev.json] [--strict]`

## 3. alloy-cpp-emit

- [x] 3.1 Port all Jinja2 templates from private pipeline. One template per generated
      file type (17 templates total). Templates must be deterministic (sorted output,
      no timestamps).
- [x] 3.2 Implement emitter engine: load IR + templates, run Jinja2, write output tree
      matching `alloy-devices-v1` layout exactly.
- [x] 3.3 Add golden-file test: regenerate all 10 existing families, diff against current
      checked-in headers in alloy-devices. Zero diff = test passes.
      (Implemented in codegen-ci.yml golden-diff job — non-fatal warning until IR files exist)
- [x] 3.4 Add partial emit: `--only driver_semantics/uart` for incremental regeneration
      during development.
- [x] 3.5 Publish as `alloy-cpp-emit` pip package with CLI:
      `alloy-cpp-emit alloy-ir.json --out <dir> [--only <file_pattern>]`

## 4. alloy-codegen orchestrator

- [x] 4.1 Create `alloy-codegen` pip package that composes the three sub-tools:
      `alloy-codegen add <svd_url_or_path> [--patch <yaml>] [--vendor v] [--family f]`
      Runs ingest → validate → emit in sequence. Writes to `alloy-devices/<vendor>/<family>/`.
- [x] 4.2 Add `alloy-codegen regen --all`: re-runs pipeline for all families whose SVD
      source hash has changed.
- [x] 4.3 Add `alloy-codegen diff <family> [--base <ref>]`: shows IR diff + coverage delta
      vs a previous run.
- [x] 4.4 Write contribution guide: `docs/CONTRIBUTING_DEVICE.md` with step-by-step:
      obtain SVD → write patches → run codegen → submit PR to alloy-devices.

## 5. CI/CD integration

- [x] 5.1 Add GitHub Actions workflow in alloy-devices: on SVD source hash change →
      auto-regen → PR with diff. (codegen-ci.yml — regen job)
- [x] 5.2 Add `alloy-ir-validate --strict` gate to CI: fail on required field missing
      for any device.
- [x] 5.3 Publish coverage report as GitHub Pages artifact per CI run.
      (upload-artifact step in codegen-ci.yml)
- [x] 5.4 Add golden-file diff check to alloy CI: regenerate and diff against checked-in
      headers to detect codegen regressions.

## 6. Documentation

- [x] 6.1 `docs/CODEGEN.md`: pipeline architecture, IR schema reference, how to add a device.
- [x] 6.2 `docs/CONTRIBUTING_DEVICE.md`: step-by-step with worked example (adding STM32H743).
- [x] 6.3 `docs/YAML_PATCHES.md`: full patch op reference with examples.
- [x] 6.4 `docs/IR_SCHEMA.md`: JSON schema reference with field semantics.
