# Tasks: Add ESP Toolchain Pins And ESP32 Catalog Entries

## 1. Catalog and pins
- [x] 1.1 Add `esp32c3_devkitm` and `esp32s3_devkitc` entries to
      `tools/alloy-cli/src/alloy_cli/_boards.toml` matching
      `cmake/board_manifest.cmake` (vendor, family, device, arch, mcu, toolchain).
      Boards use the ESP32 builtin USB-JTAG so no openocd_config_files entries are
      needed yet (handled at flash time by alloyctl).
- [x] 1.2 Add `xtensa-esp32s3-elf-gcc` and `riscv32-esp-elf-gcc` entries to
      `tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml`. Default version
      `esp-13.2.0_20240530`, per-platform URLs from
      `github.com/espressif/crosstool-NG/releases/download/...`, archive `tar.xz`,
      `sha256 = "TODO"` until validated.
- [x] 1.3 Extend `_toolchain_for_arch` in `tools/alloy-cli/src/alloy_cli/scaffold.py`
      so `arch=xtensa` resolves to `xtensa-esp32s3-elf-gcc` and `arch=riscv32`
      resolves to `riscv32-esp-elf-gcc`. Comment notes the rename to xtensa-lx6 /
      xtensa-lx7 expected with `add-esp32-classic-family`.
- [x] 1.4 Handle the no-linker-script case in `_layer_from_in_tree_board` -- when
      an in-tree board ships no `.ld` file (ESP32 boards today), render a TODO
      placeholder using the existing `board_skeleton/linker.ld.j2` template and
      surface a one-line warning. This was not in the original task list; it
      surfaced during 1.1 testing because ESP32 boards rely on a vendor-supplied
      linker fragment that does not yet have a bare-metal substitute.

## 2. Tests
- [x] 2.1 Extend `tests/test_toolchains.py` with a synthetic
      `xtensa-esp32s3-elf-gcc` install scenario (file:// URL, computed sha256) so
      install/list/which is exercised for the new toolchain class.
- [x] 2.2 Extend `tests/test_scaffold.py` with `--board esp32c3_devkitm`,
      `--board esp32s3_devkitc`, `--mcu ESP32-C3`, and `--mcu ESP32-S3` cases plus
      a direct `_toolchain_for_arch` mapping check.
- [x] 2.3 `alloy boards` lists both ESP entries with their MCU annotation
      (covered by `test_cli_boards_lists_esp_targets`).

## 3. Documentation
- [x] 3.1 Update `docs/SUPPORT_MATRIX.md` to add `esp32c3_devkitm` and
      `esp32s3_devkitc` at a `compile-only` tier; introduce the new tier definition
      alongside.
- [x] 3.2 Update `docs/CLI.md` to mention `xtensa-esp32s3-elf-gcc` and
      `riscv32-esp-elf-gcc` in the toolchain section, with the explicit non-goal
      that pinning the toolchain does not imply ESP-IDF framework integration.
- [x] 3.3 Cross-link `add-esp-toolchain-pins` from the new ESP32 rows in
      `docs/SUPPORT_MATRIX.md`.

## 4. Pin validation (deferred)
- [ ] 4.1 Validate the sha256 of each Espressif archive on the maintainer's machine
      and replace `TODO` placeholders. Mechanical follow-up that lands in the same
      release.
