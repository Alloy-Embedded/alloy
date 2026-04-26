# Tasks: Expand Chip Coverage

Phases 1–3 are tooling and do not require hardware. Phases 4–6 add chip families.
Phase 7 adds CI coverage. Each phase is independently mergeable.

## 1. Open codegen — spec and scaffolding

- [ ] 1.1 Create `tools/alloy-codegen/` Python package with `pyproject.toml`,
      `README.md`, and `CONTRIBUTING_DEVICES.md`. MIT license.
- [ ] 1.2 Implement `alloy-codegen generate --svd <file> --overlay <yaml> --out <dir>`:
      ingests CMSIS SVD; emits `alloy/device/semantics.hpp`,
      `alloy/device/descriptors.hpp`, `alloy/device/bindings.hpp` stubs.
- [ ] 1.3 Implement SVD → peripheral semantics extraction: GPIO, UART, SPI, I2C, DMA,
      ADC, DAC, PWM, Timer, CAN, RTC, Watchdog. Each peripheral becomes a typed
      semantic struct matching the existing `alloy-devices` contract.
- [ ] 1.4 Implement `alloy-codegen validate --artifact <dir> --board <cmake-preset>`:
      runs a compile-only smoke test (includes `alloy/device/semantics.hpp`, checks
      `static_assert` gates pass). Wraps `cmake --build` under the hood.
- [ ] 1.5 Implement YAML overlay schema for DMA request IDs, pin alternate function
      tables, and clock domain names (data not present in SVD). Document schema in
      `CONTRIBUTING_DEVICES.md`.
- [ ] 1.6 Add `alloy doctor --check devices` listing locally pinned families vs.
      upstream published artifacts.
- [ ] 1.7 Publish `alloy-codegen` as `alloy-codegen` on PyPI (same version as alloy
      library). CI gate: `pip install alloy-codegen && alloy-codegen --version` in
      the host-validation workflow.

## 2. codegen self-test on existing families

- [ ] 2.1 Run `alloy-codegen generate` on the SAME70, STM32F4, and STM32G0 SVDs.
      Output must round-trip: diff against the current hand-authored `alloy-devices`
      artifacts. Acceptable delta: whitespace, ordering, comments.
- [ ] 2.2 Fix any semantic gaps exposed by the round-trip diff. Document each gap
      as a known overlay requirement.
- [ ] 2.3 Add CI job `codegen-roundtrip` that runs this check on every PR touching
      `tools/alloy-codegen/` or `openspec/`.

## 3. ATDF and ESP-IDF header ingestion

- [ ] 3.1 Implement `alloy-codegen generate --atdf <file>` backend for Microchip ATDF
      format. Validate against SAMD21 ATDF.
- [ ] 3.2 Implement `alloy-codegen generate --esp-headers <dir>` backend for ESP-IDF
      register headers. Validate against ESP32-S3 headers.
- [ ] 3.3 Document both backends in `CONTRIBUTING_DEVICES.md`.

## 4. STM32 family expansion (Batch 1)

- [ ] 4.1 Generate `alloy-devices` artifacts for: STM32H743, STM32L476, STM32U575,
      STM32WB55. Use SVD from STMicroelectronics CMSIS Device packs.
- [ ] 4.2 Add `cmake/platforms/stm32h7.cmake`, `cmake/platforms/stm32l4.cmake`,
      `cmake/platforms/stm32u5.cmake`, `cmake/platforms/stm32wb.cmake`.
- [ ] 4.3 Add reference board: `boards/nucleo_h743zi/` (NUCLEO-H743ZI), blink example,
      linker script, startup. Compile-review preset in `CMakePresets.json`.
- [ ] 4.4 Add reference board: `boards/nucleo_l476rg/` (NUCLEO-L476RG), same baseline.
- [ ] 4.5 Compile-review presets for STM32U5 (NUCLEO-U575ZI-Q) and STM32WB55
      (P-NUCLEO-WB55). No bring-up code required at this phase — platform + toolchain
      only.
- [ ] 4.6 `docs/SUPPORT_MATRIX.md` updated: STM32H7, L4, U5, WB55 at `compile-review`.

## 5. Nordic nRF52 family

- [ ] 5.1 Generate `alloy-devices` artifact for nRF52840. Overlay file for UART
      (UARTE vs UART), SPI (SPIM), I2C (TWIM), PPI (Programmable Peripheral
      Interconnect, maps to DMA bindings).
- [ ] 5.2 Add `cmake/toolchains/arm-none-eabi.cmake` already covers nRF52 (Cortex-M4F).
      Add `cmake/platforms/nrf52.cmake`.
- [ ] 5.3 Add `boards/nrf52840_dk/` board bring-up (Nordic DK). GPIO + UART baseline.
      Blink + UART hello example. Compile-review preset.
- [ ] 5.4 Generate artifact for nRF52833. Compile-review only (no dedicated board).
- [ ] 5.5 `docs/SUPPORT_MATRIX.md`: nRF52840 at `compile-review`, nRF52833 at
      `codegen-only`.

## 6. Remaining families (Batch 2)

- [ ] 6.1 RP2350: generate artifact from RP2350 SVD. Add `cmake/platforms/rp2350.cmake`
      and `boards/raspberry_pi_pico2/`. Compile-review preset.
- [ ] 6.2 SAMD21: generate artifact from ATDF. Add `cmake/platforms/samd21.cmake`
      and `boards/arduino_mkrzero/`. Compile-review preset.
- [ ] 6.3 SAMD51: generate artifact from ATDF. Add `cmake/platforms/samd51.cmake`
      and `boards/adafruit_feather_m4/`. Compile-review preset.
- [ ] 6.4 NXP LPC55S69: generate artifact from SVD. Add `cmake/platforms/lpc55s6x.cmake`.
      Compile-review preset only (no board).
- [ ] 6.5 NXP MIMXRT1062: generate artifact from SVD. Add
      `cmake/platforms/imxrt1062.cmake` and `boards/teensy41/`. Compile-review preset.
- [ ] 6.6 ESP32-S3: generate artifact from ESP-IDF headers. Add
      `boards/esp32s3_devkitc/` full bring-up (currently compile-review; promote to
      hardware build after HW validation).
- [ ] 6.7 WCH CH32V307: generate artifact from vendor headers. RISC-V RV32IMAC toolchain
      reference. Compile-review preset.
- [ ] 6.8 GD32F450: generate artifact from SVD (GigaDevice publishes CMSIS packs).
      STM32-compatible layout expected. Compile-review preset.
- [ ] 6.9 TI TM4C123: generate artifact from CMSIS SVD. Add
      `cmake/platforms/tm4c.cmake`. Compile-review preset.

## 7. CI and quality gates

- [ ] 7.1 Add `compile-review-all` CI job: builds all `review-*` CMakePresets in
      parallel. Fails on any compile error. Runs on every PR.
- [ ] 7.2 Add `codegen-batch` CI job: runs `alloy-codegen generate + validate` for
      all SVDs in `tools/alloy-codegen/testdata/`. Verifies no regressions in the
      generator itself.
- [ ] 7.3 Add `support-matrix-lint` CI job: verifies every entry in
      `docs/SUPPORT_MATRIX.md` has a matching CMakePreset and vice versa. Prevents
      doc/code drift.
- [ ] 7.4 Publish `alloy-devices` artifacts for all supported families as versioned
      GitHub Release assets. `alloyctl doctor --check devices` fetches the manifest.
