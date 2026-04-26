## ADDED Requirements

### Requirement: Runtime SHALL Provide A BlockDevice Concept

The runtime MUST define `alloy::hal::filesystem::BlockDevice<T>` so any
storage medium that exposes block-granular `read`, `write`, and `erase`
plus introspectable `block_size()` / `block_count()` plugs into the
filesystem layer without virtual dispatch. All operations MUST be
`noexcept` and return typed `core::Result` values. Concrete block-device
adapters (NOR flash, SD card, RAM disk, future eMMC) MUST satisfy the
concept via `static_assert` so a regression breaks the build.

#### Scenario: Block-device adapters are concept-checked at compile time

- **WHEN** any header that ships an adapter (`W25qBlockDevice`,
  `SdCard`, future RAM-disk fixtures) is compiled
- **THEN** a `static_assert(BlockDevice<…>)` confirms concept conformance
- **AND** changes that break the contract fail to build, not silently
  drift

#### Scenario: BlockDevice operations return typed Results

- **WHEN** application code calls `read`, `write`, or `erase` on any
  conforming device
- **THEN** the call returns `core::Result<…, core::ErrorCode>` (or a
  device-specific error enum) rather than a raw int / errno
- **AND** every call site can branch on `is_ok()` / `is_err()` without
  consulting per-driver documentation

### Requirement: Runtime SHALL Provide A Templated Filesystem Surface

The runtime MUST expose `alloy::hal::filesystem::Filesystem<Backend>` and
`File<Backend>` templates whose `mount()`, `format()`, `open(path, mode)`,
`read`, `write`, `seek(offset, origin)`, `tell()`, `close()`,
`remove(path)`, `rename(old, new)`, and `stat(path)` operations all return
`core::Result`. The template MUST select the storage layout at compile
time via the `Backend` parameter — no virtual dispatch is permitted on
the hot path. `OpenMode`, `SeekOrigin`, and `DirEntry` MUST be typed
enums / structs, not magic integers.

#### Scenario: Backend selection is compile-time

- **WHEN** an application names `Filesystem<LittlefsBackend<Device>>` or
  `Filesystem<FatfsBackend<Device>>` at instantiation
- **THEN** the resulting type carries no virtual table
- **AND** the linker's image contains only the chosen backend's symbols

#### Scenario: All filesystem operations report typed errors

- **WHEN** any `Filesystem` or `File` method fails (NotMounted,
  PathNotFound, ReadOnly, NoSpace, IoError, …)
- **THEN** the failure is reported through `core::Result` with a typed
  error code
- **AND** no exception is thrown

### Requirement: Runtime SHALL Ship A littlefs Backend

The runtime MUST ship `alloy::drivers::filesystem::LittlefsBackend<
Device, ReadSz, ProgSz, CacheSz, LookAheadSz>` wrapping littlefs (vendored
via `FetchContent` at a pinned tag). All buffers MUST be `std::array`
members of the backend (no `malloc` / heap). The `lfs_config` block MUST
populate from template parameters and the `Device` concept methods.
Power-loss resilience MUST be inherited unmodified from the upstream
littlefs guarantees.

#### Scenario: littlefs runs without a heap

- **WHEN** any image links `LittlefsBackend`
- **THEN** no `malloc` / `new` symbols appear in the linked output from
  filesystem-HAL code
- **AND** every littlefs cache / lookahead buffer is sized at compile
  time from template parameters

#### Scenario: littlefs version is pinned

- **WHEN** the `drivers/filesystem/CMakeLists.txt` is reviewed
- **THEN** the littlefs `FetchContent` declaration carries an explicit
  version tag (e.g. `v2.8.1`), not a moving branch reference

### Requirement: Runtime SHALL Ship A FatFS Backend

The runtime MUST ship `alloy::drivers::filesystem::FatfsBackend<Device>`
wrapping Elm-Chan FatFS (vendored via `FetchContent` at a pinned tag).
The backend MUST implement the `diskio.h` callbacks (`disk_read`,
`disk_write`, `disk_ioctl`, `disk_status`, `disk_initialize`) as static
methods that dispatch to the supplied `Device`. FatFS MUST be configured
single-task (`FF_FS_REENTRANT = 0`); concurrency is enforced by design,
not by FatFS-internal locks.

#### Scenario: FatFS host-PC interoperability is preserved

- **WHEN** an SD card is formatted via the FatFS backend on the device
  and inserted into a host PC
- **THEN** the host reads the filesystem as FAT12/16/32 / exFAT per the
  upstream FatFS contract
- **AND** no MCU-side post-processing is required for host
  compatibility

#### Scenario: FatFS thread safety is documented as caller-enforced

- **WHEN** a build links `FatfsBackend`
- **THEN** `FF_FS_REENTRANT = 0` is set in the build configuration
- **AND** documentation states the application owns single-writer
  enforcement

### Requirement: Runtime SHALL Ship Concrete BlockDevice Adapters For NOR Flash And SD Card

The runtime MUST ship `alloy::drivers::memory::w25q::BlockDevice<Spi, N,
CsPolicy>` adapting the existing W25Q SPI NOR flash driver to the
`BlockDevice` concept (sector erase = 4 KB, page program = 256 B, JEDEC
read on init). The runtime MUST also ship
`alloy::drivers::memory::sdcard::SdCard<Spi, CsPolicy>` implementing SD
SPI mode (CMD0 / CMD8 / ACMD41 / CMD58 init, CMD17 read, CMD24 write).
SDHC / SDXC (block addressing) MUST be supported; SDSC (byte addressing)
MUST be rejected with a typed `NotSupported` error rather than silently
producing garbage. Both adapters MUST accept a `CsPolicy` template
parameter so the chip-select strategy (no-op, GPIO bit-bang) is
configurable without forking the driver.

#### Scenario: SDSC cards are rejected with a typed error

- **WHEN** an SDSC (byte-addressed) card is inserted and `init()` is
  called
- **THEN** the call returns the typed `NotSupported` error
- **AND** the driver does not enter any read or write code path

#### Scenario: Chip-select policy is per-instantiation

- **WHEN** a board uses a hardware NPCS line for chip select on one
  device and a software GPIO bit-bang on another
- **THEN** each device names its own `CsPolicy` template parameter
- **AND** no driver-internal `#ifdef` is required to switch between the
  two

### Requirement: Filesystem HAL SHALL Be Heap-Free, RTOS-Free, And Opt-In

Every filesystem-HAL layer MUST be heap-free, RTOS-free, and entirely
`noexcept`. This applies to the concept, the `Filesystem` / `File`
templates, the littlefs and FatFS backends, and every block-device
adapter alike. Builds
that do not include the filesystem headers MUST link no filesystem
symbols and pay no filesystem RAM cost. Buffer sizing MUST be expressed
as template parameters; no runtime allocation is permitted.

#### Scenario: Zero-overhead gate covers the filesystem HAL

- **WHEN** the zero-overhead release gate compiles a board that does
  NOT pull in any filesystem header
- **THEN** the linked image contains no filesystem-HAL symbols
- **AND** no filesystem-related `.bss` or `.data` cost is paid

#### Scenario: Buffer sizing is compile-time

- **WHEN** an application instantiates `LittlefsBackend<Device, R, P, C,
  L>` or `FatfsBackend<Device>`
- **THEN** every backing buffer (cache, lookahead, sector buffer) is
  sized at instantiation
- **AND** no `malloc` / `new` is called at runtime
