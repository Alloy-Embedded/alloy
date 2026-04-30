# Tasks: Declarative Board Manifest

All phases are host-only (no hardware required).

## 1. JSON schema and tooling

- [ ] 1.1 Define `cmake/schemas/board-manifest/v1.json` (JSON Schema Draft 7).
      Required fields: board_id, vendor, family, device, arch, linker_script,
      board_header, toolchain, debug, uart.debug, serial_globs, firmware_targets,
      bundle_target, build_dir_suffix, tier.
      Optional: leds, buttons, clock_profiles, mcuboot, notes.
- [ ] 1.2 Write Python validator `scripts/validate_board_manifest.py`:
      loads all `boards/**/board.json`, validates against schema, reports errors.
      Used in CI and by `alloy doctor`.
- [ ] 1.3 Write `alloy doctor` check: validate all board.json files at doctor-run time.
      Report `[ok]` or `[FAIL]` per board.

## 2. Create board.json for all existing boards

- [ ] 2.1 `boards/nucleo_g071rb/board.json`
- [ ] 2.2 `boards/nucleo_g0b1re/board.json`
- [ ] 2.3 `boards/nucleo_f401re/board.json`
- [ ] 2.4 `boards/same70_xplained/board.json`
- [ ] 2.5 `boards/raspberry_pi_pico/board.json`
- [ ] 2.6 `boards/avr128da32_curiosity_nano/board.json`
- [ ] 2.7 `boards/esp32c3_devkitm/board.json`
- [ ] 2.8 `boards/esp32s3_devkitc/board.json`
- [ ] 2.9 `boards/esp32_devkit/board.json`
- [ ] 2.10 `boards/esp_wrover_kit/board.json`
- [ ] 2.11 `boards/nrf52840_dk/board.json` (placeholder; nRF52840 bring-up pending)

## 3. CMake reader

- [ ] 3.1 Rewrite `cmake/board_manifest.cmake`: replace if/elseif chain with
      `file(GLOB_RECURSE ...)` + CMake 3.25 JSON parsing. Expose same output variables
      (ALLOY_DEVICE_SELECTED_VENDOR etc.) for backward compatibility.
- [ ] 3.2 Support `ALLOY_CUSTOM_BOARD_DIR`: scan this path before built-in boards.
      If `board.json` found there, use it; otherwise fall back to built-in scan.
- [ ] 3.3 Add schema version check: read `$schema` field; warn if schema version
      newer than CMake reader version; fail if incompatible (major version mismatch).
- [ ] 3.4 Cache board manifest resolution: write resolved variables to a stamp file
      in `${CMAKE_BINARY_DIR}` so re-configure avoids re-scanning all board.json files.

## 4. alloyctl.py migration

- [ ] 4.1 Add `_discover_boards(root)` function: scans `boards/**/board.json`,
      parses each, constructs `BoardConfig`. Replace the hardcoded `BOARDS` dict with
      `BOARDS = _discover_boards(ROOT)`.
- [ ] 4.2 Add `_discover_board_insights(root)`: similarly auto-discover and construct
      `BoardInsight` from `board.json` connector/clock/uart fields. Eliminate
      hardcoded `BOARD_INSIGHTS` dict.
- [ ] 4.3 Handle boards without `board.json` gracefully: log a warning, skip. Keeps
      alloyctl functional during incremental migration.
- [ ] 4.4 Verify `alloyctl explain/diff/info` still work after migration using
      auto-discovered data.

## 5. alloy new --board integration

- [ ] 5.1 `alloy new --board <name>`: read `board.json` to scaffold linker script path,
      default clock profile, and debug UART config into the generated `CMakeLists.txt`.
- [ ] 5.2 `alloy boards`: list all boards discovered from `board.json` files,
      with tier, family, and arch columns. Replace current hardcoded list.

## 6. Documentation

- [ ] 6.1 `docs/CUSTOM_BOARDS.md`: update to use `board.json` instead of editing
      board_manifest.cmake. Include full field reference with examples.
- [ ] 6.2 `docs/PORTING_NEW_BOARD.md`: replace CMake-centric instructions with
      "create board.json + board.hpp + linker script" workflow.
- [ ] 6.3 `cmake/schemas/board-manifest/v1.json`: add `description` fields to every
      property so IDEs show hover documentation when editing board.json.
