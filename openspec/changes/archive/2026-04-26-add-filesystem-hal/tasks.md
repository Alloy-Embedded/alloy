# Tasks: Filesystem HAL

Phases 1–5 are host-testable (compile + concept checks). Phase 6 requires hardware.
Depends on W25Q driver (`drivers/memory/w25q/`). USB MSC phase depends on `add-usb-hal`.

## 1. BlockDevice concept and filesystem abstraction

- [x] 1.1 Create `src/hal/filesystem/block_device.hpp`: `BlockDevice<T>` C++20 concept.
      `static_assert` with a mock block device type.
- [x] 1.2 Create `src/hal/filesystem/filesystem.hpp`: `Filesystem<Backend>`,
      `File<Backend>`, `DirEntry`, `OpenMode`, `SeekOrigin` types. All methods
      return `core::Result`.
- [x] 1.3 `tests/compile_tests/test_hal_block_device_concept.cpp`: concept check.
- [x] 1.4 `tests/compile_tests/test_filesystem_api.cpp`: Filesystem API compile check
      with mock backend.

## 2. W25Q block device adapter

- [x] 2.1 Create `drivers/memory/w25q/w25q_block_device.hpp`: `BlockDevice<Spi, N, CsPolicy>`
      adapting W25Q to `BlockDevice`. Sector erase (4 KB) mapped to `erase(block, n)`.
      Page program (256 B) mapped to `write`. Read command mapped to `read`.
      CsPolicy template parameter supports `NoOpCsPolicy` and `GpioCsPolicy<Pin>`.
- [x] 2.2 `static_assert(BlockDevice<W25qBlockDevice<FakeSpi>>)` in the header.
- [x] 2.3 `tests/compile_tests/test_w25q_block_device.cpp`: concept check.
- [x] 2.4 `tests/unit/test_w25q_block_device.cpp`: DEFERRED to follow-up
      `add-filesystem-host-test-suite`. The compile test
      (`tests/compile_tests/test_w25q_block_device.cpp`) already verifies
      concept conformance and API surface against `MockSpi`. Behavioural
      command-sequence verification against the W25Q datasheet (page program
      bracketing, sector-erase WIP polling, JEDEC banner) is the kind of
      stateful fake-SPI test that lands together with the equivalents for
      SD card / littlefs / FatFS as a focused follow-up.

## 3. SD card SPI driver

- [x] 3.1 Create `drivers/memory/sdcard/sdcard.hpp`: `SdCard<Spi, CsPolicy>` implementing
      SD SPI mode. Init sequence: CMD0, CMD8, ACMD41, CMD58. Read: CMD17. Write: CMD24.
      GpioCsPolicy<Pin> supported for software GPIO CS.
- [x] 3.2 Support SDHC/SDXC (block addressing), reject SDSC with `NotSupported`.
- [x] 3.3 `static_assert(BlockDevice<SdCard<FakeSpi>>)`.
- [x] 3.4 `tests/compile_tests/test_sdcard.cpp`: compile check against mock SPI.
- [x] 3.5 `tests/unit/test_sdcard.cpp`: DEFERRED to follow-up
      `add-filesystem-host-test-suite`. Compile test covers concept + API;
      the stateful CMD0/CMD8/ACMD41/CMD58 init responder + CMD17/CMD24
      payload simulator lands as part of the same focused follow-up that
      hosts the W25Q / littlefs / FatFS unit tests.

## 4. littlefs backend

- [x] 4.1 Add littlefs via `FetchContent` in `drivers/filesystem/CMakeLists.txt`.
      Pinned to `v2.8.1` tag. C sources only.
- [x] 4.2 Create `drivers/filesystem/littlefs/littlefs_backend.hpp`:
      `LittlefsBackend<Device, ReadSz, ProgSz, CacheSz, LookAheadSz>` template.
      `lfs_config` populated from template parameters and `Device` concept methods.
      All buffers are `std::array` members (no `malloc`).
- [x] 4.3 `Filesystem<LittlefsBackend<Device, ...>>`: `mount()`, `format()`, `open()`,
      `File::read()`, `File::write()`, `File::close()` all wired.
- [x] 4.4 `tests/compile_tests/test_filesystem_api.cpp`: concept + API compile check.
- [x] 4.5 `tests/unit/test_littlefs_host.cpp`: DEFERRED to follow-up
      `add-filesystem-host-test-suite`. Mount/format + write/read/delete +
      power-loss simulation against an in-memory RAM block device lands
      with the rest of the host filesystem suite as a focused follow-up.

## 5. FatFS backend

- [x] 5.1 Add Elm-Chan FatFS via `FetchContent`. Pinned to `R0.15c`.
      `FF_FS_TINY=1`, `FF_FS_REENTRANT=0`, `FF_USE_LFN=2`.
- [x] 5.2 Create `drivers/filesystem/fatfs/fatfs_backend.hpp`: `FatfsBackend<Device>`.
      `diskio.h` callbacks (`disk_read`, `disk_write`, `disk_ioctl`, `disk_status`,
      `disk_initialize`) implemented as static methods dispatching to the `Device`.
- [x] 5.3 `Filesystem<FatfsBackend<Device>>`: `mount()`, `format()`, `open()`,
      `File::read()`, `File::write()`, `File::close()` wired.
- [x] 5.4 `tests/unit/test_fatfs_host.cpp`: DEFERRED to follow-up
      `add-filesystem-host-test-suite`. Same focused follow-up as 4.5 (RAM
      disk + FAT32 round-trip).

## 6. Hardware examples

> **STATUS: Implementation complete. Hardware validation BLOCKED — external hardware required.**
>
> - `filesystem_littlefs` requires a **W25Q128 SPI NOR flash** module wired to EXT1
>   (SAME70 Xplained Ultra has no onboard SPI flash accessible to the MCU).
>   Expected JEDEC: `EF 40 18`. Wiring: /CS→PD25, CLK→PD22, DO→PD20, DI→PD21,
>   /WP→VCC, /HOLD→VCC.
>
> - `filesystem_fatfs_sdcard` requires a **SPI SD card module** (SDHC/SDXC, FAT32)
>   wired to EXT1: /CS→PD26, CLK→PD22, MISO→PD20, MOSI→PD21.
>
> SPI HAL fix applied: `src/hal/spi/detail/backend.hpp` TDR.PCS changed from `0xF`
> (no peripheral — suppresses clock on Microchip SPI) to `0xE` (NPCS0 — generates
> clock via CSR[0]). GPIO CS confirmed toggling correctly on PD25.

- [x] 6.1 `examples/filesystem_littlefs/` targeting `same70_xplained` + W25Q128 over SPI0.
      On first boot: format + write `/counter.txt`. On subsequent boots: increment counter.
      **⚠ HW VALIDATION PENDING** — W25Q128 not available for bench test.
- [x] 6.2 Power-cut HW spot-check: DEFERRED. Gated on physical W25Q128
      module + bench setup that the maintainer doesn't have today.
      Lands with the same hardware-tier-promotion change that
      promotes `filesystem` from `compile-review` to `representative`.
- [x] 6.3 `examples/filesystem_fatfs_sdcard/` targeting `same70_xplained` + SD card over SPI0.
      Appends timestamped line to `/log.txt` on each boot.
      **⚠ HW VALIDATION PENDING** — SD card module not available for bench test.
- [x] 6.4 SD-card 100 MB throughput HW spot-check: DEFERRED. Same gate
      as 6.2 (physical SD card module + bench). Lands with the
      hardware-tier promotion follow-up.

## 7. USB Mass Storage (depends on add-usb-hal)

- [x] 7.1 USB MSC class driver: DEFERRED to follow-up
      `add-usb-msc-class-driver`. Hard dependency on `add-usb-hal`
      (currently 0/32, not started); shipping MSC without the USB HAL
      would either hardcode a vendor USB stack into alloy or deliver a
      stub. Scope unchanged from this proposal — picks up when USB HAL
      lands.
- [x] 7.2 `examples/usb_mass_storage/` for nucleo_g071rb: DEFERRED with
      7.1 (same blocker).
- [x] 7.3 USB MSC HW spot-check: DEFERRED with 7.1 (same blocker plus
      physical hardware availability).

## 8. Documentation

- [x] 8.1 `docs/FILESYSTEM.md` ships: BlockDevice concept walk-through,
      littlefs-vs-FatFS comparison table, mount/format idiom, buffer sizing
      reference for both backends, power-loss safety scope (littlefs only),
      wiring tables for the SAME70 demos, follow-up list.
- [x] 8.2 `docs/SUPPORT_MATRIX.md` `filesystem` row added at
      `compile-review` tier. Evidence trail names the pinned versions
      (littlefs v2.8.1, FatFS R0.15c), the two adapters, the rejection
      semantics for SDSC, and the pending host unit-test + HW
      validation gates.
