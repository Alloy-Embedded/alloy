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
- [ ] 2.4 `tests/unit/test_w25q_block_device.cpp`: host unit test with a fake SPI
      that intercepts read/write/erase commands. Verify command sequences match W25Q
      datasheet.

## 3. SD card SPI driver

- [x] 3.1 Create `drivers/memory/sdcard/sdcard.hpp`: `SdCard<Spi, CsPolicy>` implementing
      SD SPI mode. Init sequence: CMD0, CMD8, ACMD41, CMD58. Read: CMD17. Write: CMD24.
      GpioCsPolicy<Pin> supported for software GPIO CS.
- [x] 3.2 Support SDHC/SDXC (block addressing), reject SDSC with `NotSupported`.
- [x] 3.3 `static_assert(BlockDevice<SdCard<FakeSpi>>)`.
- [x] 3.4 `tests/compile_tests/test_sdcard.cpp`: compile check against mock SPI.
- [ ] 3.5 `tests/unit/test_sdcard.cpp`: host unit test with fake SPI simulating SD
      init sequence and CMD17/CMD24 responses.

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
- [ ] 4.5 `tests/unit/test_littlefs_host.cpp`: host unit test using an in-memory RAM
      disk as `BlockDevice`. Write/read/delete files. Power-loss simulation.

## 5. FatFS backend

- [x] 5.1 Add Elm-Chan FatFS via `FetchContent`. Pinned to `R0.15c`.
      `FF_FS_TINY=1`, `FF_FS_REENTRANT=0`, `FF_USE_LFN=2`.
- [x] 5.2 Create `drivers/filesystem/fatfs/fatfs_backend.hpp`: `FatfsBackend<Device>`.
      `diskio.h` callbacks (`disk_read`, `disk_write`, `disk_ioctl`, `disk_status`,
      `disk_initialize`) implemented as static methods dispatching to the `Device`.
- [x] 5.3 `Filesystem<FatfsBackend<Device>>`: `mount()`, `format()`, `open()`,
      `File::read()`, `File::write()`, `File::close()` wired.
- [ ] 5.4 `tests/unit/test_fatfs_host.cpp`: RAM disk, create FAT32, write/read files.

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
- [ ] 6.2 Hardware spot-check: power-cut test — pull power mid-write, verify filesystem
      mountable after reconnect (littlefs power-loss guarantee).
- [x] 6.3 `examples/filesystem_fatfs_sdcard/` targeting `same70_xplained` + SD card over SPI0.
      Appends timestamped line to `/log.txt` on each boot.
      **⚠ HW VALIDATION PENDING** — SD card module not available for bench test.
- [ ] 6.4 Hardware spot-check: write 100 MB via 4 KB blocks, verify CRC32, measure
      throughput (target: >1 MB/s at SPI 20 MHz).

## 7. USB Mass Storage (depends on add-usb-hal)

- [ ] 7.1 Create `drivers/usb/msc/msc.hpp`: `MassStorage<Controller, Device>` with
      Bulk-Only Transport (BOT) state machine. CBW/CSW, SCSI READ(10)/WRITE(10).
- [ ] 7.2 `examples/usb_mass_storage/` targeting `nucleo_g071rb`.
- [ ] 7.3 Hardware spot-check: copy 10 MB file to/from USB drive, verify md5sum.

## 8. Documentation

- [ ] 8.1 `docs/FILESYSTEM.md`: BlockDevice concept guide, littlefs vs FatFS trade-offs,
      mount/format workflow, buffer sizing, power-loss safety, USB MSC integration.
- [ ] 8.2 `docs/SUPPORT_MATRIX.md`: add `filesystem` row.
