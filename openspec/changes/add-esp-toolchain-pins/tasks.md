# Tasks: Add ESP Toolchain Pins And ESP32 Catalog Entries

## 1. Catalog and pins
- [ ] 1.1 Add `esp32c3_devkitm` and `esp32s3_devkitc` entries to
      `tools/alloy-cli/src/alloy_cli/_boards.toml` matching
      `cmake/board_manifest.cmake` (vendor, family, device, arch, mcu, toolchain).
      Use OpenOCD config files where the runtime already declares them; otherwise
      omit the OpenOCD block for now.
- [ ] 1.2 Add `xtensa-esp32s3-elf-gcc` and `riscv32-esp-elf-gcc` entries to
      `tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml`:
      - `default_version` matching a current Espressif release (e.g. `esp-13.2.0_20240530`)
      - per-platform URLs from `dl.espressif.com/dl/...` for `darwin-arm64`,
        `darwin-x64`, `linux-x64`, `linux-arm64`
      - `sha256 = "TODO"` until validated; install refuses to run with TODO pins
      - `archive = "tar.xz"`, `strip_components = 1`, `bin_subdir = "bin"`
- [ ] 1.3 Extend `_toolchain_for_arch` in `tools/alloy-cli/src/alloy_cli/scaffold.py`
      so `arch=xtensa` resolves to `xtensa-esp32s3-elf-gcc` and `arch=riscv32`
      resolves to `riscv32-esp-elf-gcc`.

## 2. Tests
- [ ] 2.1 Extend `tests/test_toolchains.py` with a synthetic `xtensa-esp32s3-elf-gcc`
      install scenario (file:// URL, computed sha256) so the install/list/which path
      is exercised for the new toolchain class.
- [ ] 2.2 Extend `tests/test_scaffold.py` with `--board esp32c3_devkitm` and
      `--mcu ESP32-S3` cases. Both must produce a project with the expected device
      tuple and pick up the right toolchain name in the preflight output.
- [ ] 2.3 Confirm the new ESP boards do not break `alloy boards`; the listing
      command includes them with the right MCU annotation.

## 3. Documentation
- [ ] 3.1 Update `docs/SUPPORT_MATRIX.md` to add `esp32c3_devkitm` and
      `esp32s3_devkitc` at a `compile-only` tier (a new tier to introduce alongside
      the entry, scoped to "buildable in CI; no hardware validation"). Cross-link
      to this change in the maintenance notes.
- [ ] 3.2 Update `docs/CLI.md` to mention `xtensa-esp32s3-elf-gcc` and
      `riscv32-esp-elf-gcc` in the toolchain section, with a note that ESP32 ESP-IDF
      framework integration is a separate (future) proposal.
- [ ] 3.3 Cross-link this change from `docs/SUPPORT_MATRIX.md`'s ESP32 row so
      readers can see why the boards moved from "not listed" to "compile-only".

## 4. Pin validation (deferred)
- [ ] 4.1 Validate the sha256 of each `dl.espressif.com` archive on the maintainer's
      machine and replace `TODO` placeholders. This is a mechanical follow-up that
      does not require a code review pass; it lands in the same release that
      introduces the pins.
