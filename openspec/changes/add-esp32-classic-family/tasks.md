# Tasks: ESP32 Classic Family With Dual-Core Bring-Up

Tasks are ordered. Several phases are blocked on external prerequisites (descriptor
availability, bootloader strategy decision); each block is called out at the top of its
section. Do not start a phase before the previous one is reviewed.

## 0. Prerequisites (must clear before phase 1)
- [ ] 0.1 Confirm `alloy-devices` publishes `espressif/esp32/esp32/` runtime
      descriptors covering at minimum: clock tree, system clock, dual-core startup
      facts, GPIO, UART. If absent, file a request against alloy-devices and pause.
- [ ] 0.2 Decide the second-stage-bootloader strategy (option (a) alloy second-stage
      loader, or option (b) vendored Espressif blob). Record the decision in
      `design.md` under "Resolved Questions".
- [ ] 0.3 Confirm the `add-custom-board-bringup` change is merged so the arch enum
      migration has a stable origin. If it has not merged yet, amend it instead of
      writing a migration in this change.

## 1. Architecture enum migration
- [ ] 1.1 Rename `xtensa` to `xtensa-lx7` in `_ALLOY_VALID_ARCHES` and add
      `xtensa-lx6` (`cmake/board_manifest.cmake`).
- [ ] 1.2 Update `boards/esp32s3_devkitc/` arch references to `xtensa-lx7`
      (manifest entry only; the board files themselves should not need changes).
- [ ] 1.3 Update `tools/alloy-cli/src/alloy_cli/_boards.toml` to switch the S3 entry
      to `xtensa-lx7`.
- [ ] 1.4 Update `tools/alloy-cli/src/alloy_cli/scaffold.py`:
      `VALID_ARCHES` and `_toolchain_for_arch`.
- [ ] 1.5 Update `tools/alloy-cli/tests/test_scaffold.py` and any other tests pinning
      the old `xtensa` value.
- [ ] 1.6 Update `docs/CUSTOM_BOARDS.md` accepted-arch table.
- [ ] 1.7 Add a one-paragraph migration note to the change's description for users
      who already wired a custom board against `arch=xtensa`.

## 2. Toolchain
- [ ] 2.1 Add `xtensa-esp32-elf-gcc` to
      `tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml` (skeleton entries with
      `sha256 = "TODO"`; the maintainer fills SHAs after downloading).
- [ ] 2.2 Add `cmake/toolchains/xtensa-esp32-elf.cmake`, modelled after
      `xtensa-esp32s3-elf.cmake` but without IDF_TOOLS_PATH fallback (the alloy
      toolchain manager owns the install location).
- [ ] 2.3 Extend `tests/test_toolchains.py` with a synthetic
      `xtensa-esp32-elf-gcc` install case.

## 3. Build system: family and platform
- [ ] 3.1 Add `cmake/platforms/esp32.cmake` with the LX6 compiler flags (`-mlongcalls`,
      no-exceptions/rtti, dead-code elimination). Mirror the structure of
      `esp32s3.cmake`.
- [ ] 3.2 Add an `esp32` family branch in `cmake/board_manifest.cmake` for boards we
      add in phase 6.

## 4. Dual-core startup (blocked on 0.1, 0.2)
- [ ] 4.1 Implement second-stage bootloader behaviour per the strategy decided in 0.2:
      either an alloy minimal loader linked into the image, or a vendored Espressif
      blob with a documented build-time fetch step.
- [ ] 4.2 Implement APP_CPU bring-up in the descriptor-driven startup runtime, fed by
      data from `alloy-devices`. PRO_CPU completes existing startup, then triggers
      APP_CPU via the documented sync sequence.
- [ ] 4.3 Add `alloy::runtime::Core`, `current_core()`, and `launch_on(Core, fn)` as
      public surface in the runtime header; arch-specific implementation behind it.
- [ ] 4.4 Cover dual-core startup with a host-MMIO-style smoke test (where feasible)
      and a focused unit test on the public `current_core` / `launch_on` API.

## 5. Descriptor consumption (blocked on 0.1)
- [ ] 5.1 Verify the `espressif/esp32/esp32` descriptor flows through
      `alloy_devices.cmake` without per-vendor branches; add any missing
      family-specific check the existing pipeline needs.
- [ ] 5.2 Add a compile smoke test that exercises descriptor consumption for the new
      family, mirroring the SAME70/STM32G0 smokes.

## 6. Boards
- [ ] 6.1 Add `boards/esp_wrover_kit/` with board.hpp, board_config.hpp, board.cpp,
      board_uart.hpp. Pin assignments per WROVER-KIT v4.1 schematic:
      RGB LED on GPIO0/2/4, debug UART through FT2232HL B channel, JTAG via FT2232HL
      A channel. PSRAM, LCD, microSD, camera left as TODO with explicit comments.
- [ ] 6.2 Add `boards/esp32_devkitc/` with the same file shape. ESP-WROOM-32 module,
      LED on GPIO2, debug UART through CP210x bridge.
- [ ] 6.3 Add the two boards to `cmake/board_manifest.cmake`, vendor=espressif,
      family=esp32, device=esp32, arch=xtensa-lx6.

## 7. CLI catalog
- [ ] 7.1 Add `esp_wrover_kit` and `esp32_devkitc` entries to
      `tools/alloy-cli/src/alloy_cli/_boards.toml` matching the manifest.
- [ ] 7.2 Cover both boards with scaffolding tests in
      `tools/alloy-cli/tests/test_scaffold.py` (catalog match for each board, plus
      the WROVER-KIT MCU `ESP32-WROVER-B` resolving via `--mcu`).

## 8. Documentation
- [ ] 8.1 Update `docs/SUPPORT_MATRIX.md` to add both boards at `compile-only` tier.
- [ ] 8.2 Update `docs/BOARD_TOOLING.md` with WROVER-KIT JTAG flow (uses onboard
      FT2232HL via OpenOCD) and DevKitC v4 (external probe required).
- [ ] 8.3 Update `docs/CLI.md` with `xtensa-esp32-elf-gcc` toolchain entry.
- [ ] 8.4 Update `docs/CUSTOM_BOARDS.md` to reflect the migrated arch enum.

## 9. Hardware bring-up runbook (deferred)
- [ ] 9.1 Once the boards arrive, write a hardware bring-up runbook documenting
      flashing, blink validation, and a `time_probe`-equivalent that exercises both
      cores. The runbook lands in a follow-up change that then promotes both boards
      from `compile-only` to `representative` in `SUPPORT_MATRIX.md`.
