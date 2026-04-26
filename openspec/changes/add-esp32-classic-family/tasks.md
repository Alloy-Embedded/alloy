# Tasks: ESP32 Classic Family With Dual-Core Bring-Up

Tasks are ordered. Several phases are blocked on external prerequisites (descriptor
availability, bootloader strategy decision); each block is called out at the top of its
section. Do not start a phase before the previous one is reviewed.

## 0. Prerequisites
- [x] 0.1 `alloy-devices/espressif/esp32/` ships a `esp32` device descriptor
      with clock tree, GPIO, and UART runtime contracts. Dual-core start
      facts (APP_CPU start vector, DPORT release sequence, cache flush
      requirements) are NOT yet published; phase 4 is blocked on that.
- [x] 0.2 Bootloader strategy: provisional decision is direct-boot via
      `boards/esp32_devkit/startup.{S,cpp}` (no Espressif vendor blob,
      manual WDT disable + clock + IO_MUX setup). Documented in the
      committed board.cpp; no second-stage loader is needed for blink-class
      apps. PSRAM and complex peripheral bring-up are gated on a richer
      bootloader / IDF integration -- separate proposal.
- [x] 0.3 `add-custom-board-bringup` is merged; the arch enum migration
      below applies cleanly against the live `_ALLOY_VALID_ARCHES`.

## 1. Architecture enum migration
- [x] 1.1 `_ALLOY_VALID_ARCHES` in `cmake/board_manifest.cmake` is now
      `cortex-m0plus;cortex-m4;cortex-m7;riscv32;xtensa-lx6;xtensa-lx7;avr;native`.
- [x] 1.2 `esp32s3_devkitc` manifest entry uses `xtensa-lx7`;
      `esp32_devkit` and `esp_wrover_kit` use `xtensa-lx6`.
- [x] 1.3 `_boards.toml` entries for the three ESP32 boards reflect the new
      arch values transitively (the catalog stores `device`/`mcu`; arch is
      derived from the runtime manifest the scaffolder reads).
- [x] 1.4 `scaffold.py` has the split: `VALID_ARCHES`, `_cpu_flags_for_arch`,
      `_toolchain_file_for_arch`, `_toolchain_for_arch`, and
      `_ARCH_BY_BOARD_FALLBACK` all distinguish lx6 from lx7. The unified
      `xtensa-esp-elf-gcc` toolchain serves both.
- [x] 1.5 `test_scaffold.py` updated; both `xtensa-lx6` and `xtensa-lx7`
      exercised via the toolchain mapping test. 65/65 CLI tests pass.
- [x] 1.6 `docs/CUSTOM_BOARDS.md` accepted-arch table cites `xtensa-lx6`
      (ESP32 classic) and `xtensa-lx7` (ESP32-S2/S3) explicitly.
- [x] 1.7 Migration note recorded in this tasks.md and the SUPPORT_MATRIX:
      consuming projects that pinned `arch=xtensa` must move to
      `xtensa-lx7` (the only previous user) -- the change is mechanical and
      hits zero downstream consumers today.

## 2. Toolchain
- [x] 2.1 The unified `xtensa-esp-elf-gcc` pin already shipped with
      `add-esp-toolchain-pins`. It produces code for both LX6 (ESP32) and
      LX7 (ESP32-S2/S3) since Espressif's crosstool-NG 13.x consolidated
      both behind one driver. No separate `xtensa-esp32-elf-gcc` pin is
      needed; `_toolchain_for_arch("xtensa-lx6")` resolves to the same
      binary as `_toolchain_for_arch("xtensa-lx7")`.
- [x] 2.2 `cmake/toolchains/xtensa-esp32-elf.cmake` exists from the
      maintainer's earlier ESP32 work and resolves the binary through the
      alloy toolchain manager's standard search paths.
- [x] 2.3 `test_toolchains.py` covers the synthetic install path; the
      mapping is exercised twice (once per arch variant) by the scaffold
      test rather than the toolchain test.

## 3. Build system: family and platform
- [x] 3.1 `cmake/platforms/esp32.cmake` exists with LX6 compiler flags.
- [x] 3.2 `esp32` family branch is in `cmake/board_manifest.cmake` for both
      `esp32_devkit` and `esp_wrover_kit`.

## 4. Dual-core startup (BLOCKED on alloy-devices)
- [ ] 4.1 Bootloader strategy decided (direct-boot, no second-stage loader)
      and applied to `esp32_devkit` and `esp_wrover_kit` for blink-class
      apps. Direct-boot does not enable PSRAM or the second core; richer
      boot is a separate proposal.
- [ ] 4.2 APP_CPU bring-up: BLOCKED. The
      `alloy-devices/espressif/esp32/.../esp32/` descriptor does not yet
      publish dual-core start facts (APP_CPU start vector, DPORT release
      sequence, cache flush requirements). Implementing this now would
      mean synthesizing facts in alloy, which the project explicitly
      forbids. File a request against alloy-devices for these facts;
      this OpenSpec change tracks them as the only remaining blocker
      for the dual-core requirement.
- [ ] 4.3 Public surface (`alloy::runtime::Core`, `current_core`,
      `launch_on`): waits on 4.2 so the surface lands with a real
      implementation rather than a single-core stub that misleads users.
- [ ] 4.4 Tests: deferred to land with 4.3.

## 5. Descriptor consumption (PARTIAL)
- [x] 5.1 The `espressif/esp32/esp32` descriptor flows through
      `alloy_devices.cmake` for the runtime (clock, GPIO, UART pieces).
- [ ] 5.2 Compile smoke test pending a board-level workaround: the
      descriptor currently lacks `signal_cts` / `signal_rts` enumerators
      that `src/device/dev.hpp` references unconditionally when
      `ALLOY_DEVICE_RUNTIME_AVAILABLE` is on. Same gap exists on
      esp32c3/esp32s3 and is tracked separately as a runtime-side
      conditional or alloy-devices descriptor extension.

## 6. Boards
- [x] 6.1 `boards/esp_wrover_kit/` shipped with board.hpp, board.cpp,
      board_uart_raw.hpp, esp32.ld, partitions.csv, startup.{S,cpp},
      syscalls.cpp. Mirrors DevKitC v4 for LED + UART direct-boot;
      WROVER-specific peripherals (PSRAM, LCD, microSD, full RGB,
      FT2232HL JTAG) explicitly out of scope and called out in the
      board.hpp docstring.
- [x] 6.2 `boards/esp32_devkit/` shipped earlier with the same file shape.
- [x] 6.3 Both boards in `cmake/board_manifest.cmake`. WROVER uses
      `device=esp32-wroom32` so the descriptor pipeline picks up the
      WROVER-B-specific facts when alloy-devices ships them; today both
      boards effectively consume the shared `esp32` runtime surface.

## 7. CLI catalog
- [x] 7.1 Both `esp32_devkit` and `esp_wrover_kit` are in
      `tools/alloy-cli/src/alloy_cli/_boards.toml` with the right MCU
      strings (`ESP32` and `ESP32-WROVER-B`) and `xtensa-esp-elf-gcc`
      toolchain. WROVER additionally records `openocd_config =
      "board/esp32-wrover-kit-3.3v.cfg"` for the on-board FT2232HL probe.
- [x] 7.2 `alloy boards` lists the new entries; scaffold tests cover the
      catalog match path against the synthetic SDK.

## 8. Documentation
- [x] 8.1 Both boards are in `docs/SUPPORT_MATRIX.md` at `compile-only`
      tier with the dual-core gap and follow-ups called out.
- [x] 8.2 BOARD_TOOLING.md: WROVER's `board/esp32-wrover-kit-3.3v.cfg`
      OpenOCD path is recorded in the catalog entry; further runbook
      content lands with the hardware bring-up follow-up (phase 9).
- [x] 8.3 `docs/CLI.md` already mentions `xtensa-esp-elf-gcc` covering
      both LX6 and LX7 (the unified driver post 13.x).
- [x] 8.4 `docs/CUSTOM_BOARDS.md` accepted-arch table now lists
      `xtensa-lx6` (ESP32) and `xtensa-lx7` (ESP32-S2/S3).

## 9. Hardware bring-up runbook (deferred)
- [ ] 9.1 Once the boards arrive, write a hardware bring-up runbook
      documenting flashing, blink validation, and a `time_probe`-equivalent
      that exercises both cores. The runbook lands in a follow-up change
      that promotes the boards from `compile-only` to `representative` in
      `SUPPORT_MATRIX.md`. Single-core blink will land first; dual-core
      validation depends on phase 4.

## Status summary (post-implementation)

Phases 0, 1, 2, 3, 6, 7, 8 are complete. Phase 5 is partial. Phases 4
and 9 are gated on external work that does not belong in this change:

- **Phase 4 (dual-core)** -- blocked on `alloy-devices` publishing
  APP_CPU start vector, DPORT release sequence, and cache-flush
  requirements for the `espressif/esp32/esp32` descriptor.
- **Phase 5.2 (compile smoke)** -- blocked on either runtime-side
  conditional usage of `signal_cts` / `signal_rts` in `src/device/dev.hpp`
  or alloy-devices adding those enumerators to the espressif descriptors
  (same gap exists on esp32c3 / esp32s3, not LX6-specific).
- **Phase 9 (hardware runbook)** -- blocked on physical hardware in the
  maintainer's hands.

The deliverable shape after this change: `alloy new --board esp32_devkit`
and `alloy new --board esp_wrover_kit` produce projects with the right
device tuple and toolchain wiring; the build will compile up to the
descriptor gap, which is the same gap esp32c3/esp32s3 hit today.
