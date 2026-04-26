# Add Filesystem HAL

## Why

Most embedded applications that go beyond "blink + UART" need persistent storage:
configuration files, data logs, firmware update images, certificates. Today Alloy has
a W25Q SPI NOR flash driver that exposes raw `read`, `write`, and `erase` — there is no
filesystem layer. Applications must implement wear leveling, bad block management, and
directory structures themselves.

The two standard bare-metal filesystems are:

- **littlefs** — designed for NOR flash with wear leveling, power-loss resilience, and
  bounded RAM. The de-facto standard for MCU filesystems.
- **FatFS (Elm-Chan)** — FAT12/16/32/exFAT, required for SD cards and USB mass storage
  interoperability with host PCs.

This change adds a `BlockDevice` concept that both filesystems consume, concrete block
device implementations for W25Q flash and SD cards over SPI, and a `filesystem` HAL
namespace with mount/open/read/write/close APIs that compile-time dispatch to either
filesystem backend.

## What Changes

### `src/hal/filesystem/` — filesystem abstraction

`block_device.hpp`:

```cpp
template <typename T>
concept BlockDevice = requires(T& d, std::size_t block,
    std::span<std::byte> buf, std::span<const std::byte> data) {
    { d.read(block, buf) }      -> core::ResultLike;
    { d.write(block, data) }    -> core::ResultLike;
    { d.erase(block, std::size_t{}) } -> core::ResultLike;
    { d.block_size() }          -> std::same_as<std::size_t>;
    { d.block_count() }         -> std::same_as<std::size_t>;
};
```

`filesystem.hpp`:

```cpp
template <typename Backend>
class File {
public:
    Result<std::size_t> read(std::span<std::byte>);
    Result<std::size_t> write(std::span<const std::byte>);
    Result<void>        seek(std::int64_t offset, SeekOrigin);
    Result<std::int64_t> tell();
    Result<void>        close();
};

template <typename Backend>
class Filesystem {
public:
    Result<void>      mount();
    Result<void>      format();
    Result<File<Backend>> open(std::string_view path, OpenMode);
    Result<void>      remove(std::string_view path);
    Result<void>      rename(std::string_view old_path, std::string_view new_path);
    Result<DirEntry>  stat(std::string_view path);
};
```

`Backend` is `LittlefsBackend<Device>` or `FatfsBackend<Device>`. Both are concept-
checked against `BlockDevice`. Zero virtual dispatch.

### `drivers/filesystem/` — filesystem backends

`littlefs/littlefs_backend.hpp`: `LittlefsBackend<Device>` wrapping the littlefs C
library (fetched via `FetchContent`, MIT license). Provides the `lfs_config` struct
populated from `Device::block_size()` and `Device::block_count()`. Read/write/erase
callbacks delegate to the device concept methods. Buffer sizes are template parameters
(no heap).

`fatfs/fatfs_backend.hpp`: `FatfsBackend<Device>` wrapping Elm-Chan FatFS (BSD license).
Implements `diskio.h` callbacks against the `BlockDevice`. Thread safety: FatFS's
`FF_FS_REENTRANT` is disabled (single-task access enforced by design).

### `drivers/memory/` — block device adapters

`w25q/w25q_block_device.hpp`: `W25qBlockDevice<SpiHandle>` adapting the existing W25Q
SPI flash driver to the `BlockDevice` concept. Adds `erase(block, count)` using W25Q
sector erase (4 KB sectors). `static_assert(BlockDevice<W25qBlockDevice<FakeSpi>>)`.

`sdcard/sdcard.hpp`: `SdCard<SpiHandle>` — SD card over SPI in SPI mode (compatible
with all SD/SDHC/SDXC cards). Init sequence (CMD0/CMD8/ACMD41/CMD58), block
read/write (CMD17/CMD24). `static_assert(BlockDevice<SdCard<FakeSpi>>)`.

### `drivers/usb/msc/` — USB Mass Storage class driver

`msc.hpp`: `MassStorage<Controller, Device>` implementing USB MSC Bulk-Only Transport
(BOT). Exposes the `BlockDevice` to the host PC as a USB storage device. Requires the
USB HAL from `add-usb-hal`. With this, an SD card over SPI appears as a USB drive on
the host without any host driver.

### Examples

`examples/filesystem_littlefs/`: create a littlefs filesystem on W25Q flash (128 Mbit).
Write a config file (`/config.txt`), read it back after reset (power-loss resilience
test). Builds for `nucleo_g071rb` + W25Q via SPI.

`examples/filesystem_fatfs_sdcard/`: mount FAT32 on SD card over SPI. List root
directory, create a log file, write timestamped entries. Builds for `same70_xplained`
(SD card slot on the board).

`examples/usb_mass_storage/`: SD card over SPI exposed as USB MSC to host PC. Files
written by the firmware are readable on the host. Combines USB HAL + FatFS + SdCard.
Builds for `nucleo_g071rb`.

### Documentation

`docs/FILESYSTEM.md`: BlockDevice concept, littlefs vs FatFS comparison, mount/format
workflow, configuration (block count, read/write sizes, cache sizes), power-loss safety
guarantees, USB MSC integration.

## What Does NOT Change

- W25Q driver public API — `W25qBlockDevice` is an additive adapter header.
- USB HAL — MSC is an additive class driver that depends on `add-usb-hal`.
- No heap allocation in the filesystem layer — all buffers are template parameters.

## Alternatives Considered

**Embedded VFS (virtual filesystem) with runtime dispatch:** Adding a virtual
`FileSystem` base class would allow runtime swapping between littlefs and FatFS. Rejected
— violates the zero-virtual-dispatch invariant. Compile-time Backend selection is
sufficient; most applications know their storage medium at build time.

**SPIFFS:** Superseded by littlefs in every respect. littlefs has better power-loss
safety, lower RAM usage, and active maintenance.
