# Tasks: Bootloader Integration — MCUboot + OTA

Phases 1–2 are tooling and do not require hardware. Phases 3–5 require hardware.
Depends on: `add-network-hal` (OTA client), `add-filesystem-hal` (FlashBackend for OTA).
USB DFU integration requires `add-usb-hal`.

## 1. alloyctl MCUboot image wrapping

- [ ] 1.1 Add `imgtool` as an optional dependency in `alloyctl`: detect via
      `pip show imgtool`; `alloy doctor` warns if missing when `--with-mcuboot` is used.
- [ ] 1.2 Implement `alloy bundle --with-mcuboot [--key <pem>] [--version <x.y.z>]`:
      runs `imgtool sign` on the input ELF. If `--key` absent, use `--pad-header` only
      (dev mode, unsigned). Output: `<target>-<version>-signed.bin`.
- [ ] 1.3 Implement `alloy flash --mcuboot`: flashes bootloader binary (fetched from
      GitHub Release assets) to 0x0 and app to primary slot offset per board config.
- [ ] 1.4 Implement `alloy recover --slot secondary`: flashes a user-supplied image to
      the secondary slot, sets pending flag via `imgtool` trailer.
- [ ] 1.5 `alloy doctor --check mcuboot`: verify `imgtool` installed and version matches
      pinned MCUboot version.

## 2. Partition layouts and MCUboot configs

- [ ] 2.1 Create `boards/nucleo_g071rb/mcuboot/memory_map.cmake`: defines
      `ALLOY_BOOT_OFFSET`, `ALLOY_PRIMARY_OFFSET`, `ALLOY_SECONDARY_OFFSET`,
      `ALLOY_SCRATCH_OFFSET` and their sizes. CMakeLists.txt uses these when
      `ALLOY_MCUBOOT=ON`.
- [ ] 2.2 Create `boards/nucleo_f401re/mcuboot/memory_map.cmake`: same for STM32F4.
- [ ] 2.3 Create `boards/same70_xplained/mcuboot/memory_map.cmake`: primary + secondary
      on W25Q external flash via the W25Q FlashBackend. Scratch on internal SRAM.
- [ ] 2.4 Update `boards/esp32_devkit/partitions.csv`: add MCUboot-compatible partition
      layout (bootloader + otadata + ota_0 + ota_1). Update `alloyctl.py` to use the
      MCUboot layout when `ALLOY_MCUBOOT=ON`.
- [ ] 2.5 Create `boards/nrf52840_dk/mcuboot/memory_map.cmake` (deferred until
      nRF52840 bring-up is complete; add placeholder).
- [ ] 2.6 Publish MCUboot pre-built binaries for `nucleo_g071rb`, `nucleo_f401re`,
      `same70_xplained`, and `esp32_devkit` as GitHub Release assets. Pin version
      in `CMakeLists.txt` as `ALLOY_MCUBOOT_VERSION`.

## 3. MCUboot hardware validation

- [ ] 3.1 `examples/mcuboot_blink/` targeting `nucleo_g071rb`:
      a. Flash MCUboot bootloader to 0x08000000.
      b. Bundle blink firmware with `alloy bundle --with-mcuboot`.
      c. Flash signed image to primary slot.
      d. Verify: bootloader starts, jumps to application, LED blinks.
- [ ] 3.2 Hardware spot-check — swap test: write a second (v2) blink image to secondary
      slot, set pending. On next boot, MCUboot swaps slots. Verify v2 runs (different
      blink rate). On second reboot, swap confirmed — v2 is now primary.
- [ ] 3.3 Hardware spot-check — rollback test: write a corrupt (truncated) image to
      secondary slot, set pending. MCUboot detects bad header, does not boot corrupt
      image, stays on primary slot. Verify v1 still runs.
- [ ] 3.4 Same flow for `esp32_devkit` with ESP32 OTA partition layout.

## 4. OTA client driver

- [ ] 4.1 Create `drivers/ota/ota_client.hpp`: `OtaClient<Net, Flash>` template.
      `download(url, expected_sha256)`, `request_update()`, `cancel_update()`.
      Depends on `TcpStream` (from `add-network-hal`) and `FlashBackend`
      (from `add-filesystem-hal`).
- [ ] 4.2 `drivers/ota/ota_client.cpp`: HTTP GET with chunked download. SHA-256
      computed incrementally via a built-in SHA-256 implementation (no OpenSSL;
      pure C++ with no heap). MCUboot image trailer written on download completion.
- [ ] 4.3 `tests/compile_tests/test_ota_client.cpp`: compile check with fake net +
      fake flash.
- [ ] 4.4 `tests/unit/test_ota_client.cpp`: host-level unit test. Mock HTTP server
      (loopback socket), fake flash backend recording writes. Verify: correct block
      sequence written, SHA-256 verified, `request_update` sets trailer correctly,
      `cancel_update` clears it.

## 5. OTA examples (require SAME70 + Ethernet)

- [ ] 5.1 `examples/ota_update/` targeting `same70_xplained`:
      a. Device boots, connects Ethernet, obtains DHCP address.
      b. Polls `http://<server>/firmware.bin` every 30 seconds.
      c. On new firmware detected (version header check), downloads to W25Q secondary.
      d. Verifies SHA-256 against `http://<server>/firmware.sha256`.
      e. Sets pending flag, reboots.
      f. MCUboot swaps and runs new firmware.
- [ ] 5.2 `examples/ota_rollback/` targeting `same70_xplained`:
      Same flow but server serves a corrupt image (truncated). Verify MCUboot rollback.
- [ ] 5.3 Hardware spot-check: end-to-end OTA update measured: time from poll to reboot
      into new firmware target < 60 seconds for a 256 KB image on 100 Mbps Ethernet.

## 6. Documentation

- [ ] 6.1 Create `docs/OTA.md`: MCUboot integration guide, partition layout rationale
      for each board, signing workflow, `alloy bundle/flash/recover` CLI reference,
      OTA client usage, rollback mechanics, security considerations (key management,
      unsigned dev builds).
- [ ] 6.2 `docs/SUPPORT_MATRIX.md`: add `mcuboot` and `ota` rows. STM32G0 + STM32F4
      mcuboot = `hardware`. SAME70 OTA = `hardware`. ESP32 mcuboot = `hardware`.
      nRF52840 = `compile-review`.
- [ ] 6.3 Update `docs/CLI.md`: `alloy bundle`, `alloy flash --mcuboot`,
      `alloy recover` command documentation.
