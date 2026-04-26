# Alloy Filesystem HAL

`alloy::hal::filesystem` ships an allocation-free, `noexcept`, virtual-
dispatch-free filesystem layer that sits on top of any block-granular
storage device (NOR flash, SD card, future eMMC, RAM disk for tests).
The two backends are **littlefs** (recommended for NOR flash) and
**FatFS** (required for host-PC interop on SD cards and USB MSC).

This document is the user-facing guide. The contract is in
`openspec/specs/filesystem-hal/spec.md`.

---

## TL;DR

```cpp
#include "drivers/memory/w25q/w25q_block_device.hpp"
#include "drivers/filesystem/littlefs/littlefs_backend.hpp"
#include "hal/filesystem/filesystem.hpp"

using namespace alloy;
using Spi    = my_board::SpiBus;
using Flash  = drivers::memory::w25q::BlockDevice<Spi, /*BlockCount=*/4096>;
using Backend = drivers::filesystem::littlefs::Backend<
    Flash, /*Read=*/16, /*Prog=*/16, /*Cache=*/16, /*LookAhead=*/16>;
using FS = hal::filesystem::Filesystem<Backend>;

Spi   spi;
Flash flash{spi};
FS    fs{flash};

if (fs.mount().is_err()) (void)fs.format();
auto file = fs.open("/counter.txt", hal::filesystem::OpenMode::ReadWriteCreate);
```

Every method returns `core::Result`. No exceptions, no heap, no globals.

---

## When to use which backend

| Property | littlefs | FatFS |
| --- | --- | --- |
| Power-loss safety | strong (CoW design, journaled) | weak (FAT updates not atomic) |
| Wear leveling | built in (dynamic + static) | none (host-OS responsibility) |
| RAM footprint | bounded by template params | bounded by FatFS config |
| Host-PC interop | no (alloy-only) | yes (FAT12/16/32/exFAT) |
| Best for | NOR flash, internal data | SD cards, USB MSC, swap with PC |
| License | BSD-3-Clause | BSD-1-Clause |

**Default to littlefs** for NOR flash. Only pick FatFS when a host PC
must read the same medium.

---

## The BlockDevice concept

```cpp
template <typename T>
concept BlockDevice = requires(T& d, std::size_t block,
    std::span<std::byte> buf, std::span<const std::byte> data) {
    { d.read(block, buf) }            -> core::ResultLike;
    { d.write(block, data) }          -> core::ResultLike;
    { d.erase(block, std::size_t{}) } -> core::ResultLike;
    { d.block_size() }                -> std::same_as<std::size_t>;
    { d.block_count() }               -> std::same_as<std::size_t>;
};
```

Every backend in the runtime accepts any type satisfying this concept.
The runtime ships two concrete adapters:

- **`drivers::memory::w25q::BlockDevice<Spi, BlockCount, CsPolicy>`** —
  W25Q-family SPI NOR flash. 4 KB sector erase, 256 B page program,
  JEDEC-ID read on `init()`. `CsPolicy` toggles between hardware NPCS
  and software GPIO bit-bang.

- **`drivers::memory::sdcard::SdCard<Spi, CsPolicy>`** — SD card in
  SPI mode (CMD0 / CMD8 / ACMD41 / CMD58 init; CMD17 read; CMD24
  write). SDHC / SDXC supported. SDSC rejected with `NotSupported` —
  the driver does NOT silently produce garbage on byte-addressed
  cards.

For test suites, an in-memory RAM disk is straightforward: just
implement the five concept methods backed by `std::array`. See
`tests/compile_tests/test_filesystem_api.cpp` for the pattern.

---

## Buffer sizing

Both backends size their internal buffers at instantiation. There is no
runtime allocation.

### littlefs

Five template parameters:

| Param | Meaning | Typical |
| --- | --- | --- |
| `Read` | minimum read size in bytes | 16 (NOR), 512 (SD) |
| `Prog` | minimum program (write) size | 16 (NOR), 512 (SD) |
| `Cache` | block cache buffer size | 16 - 256 |
| `LookAhead` | block allocation lookahead | 16 - 256 |

Bigger caches reduce flash wear but cost RAM. Start at 16/16/16/16,
profile, raise as needed.

### FatFS

`FF_FS_TINY = 1` and `FF_USE_LFN = 2` are pre-configured (small TFs,
LFN via heap-free static buffer). `FF_FS_REENTRANT = 0` — the
application owns single-writer enforcement (no FatFS-internal locks).

---

## Mount workflow

```cpp
FS fs{device};

// First boot or corruption: format() before mount() succeeds.
auto r = fs.mount();
if (r.is_err()) {
    if (auto fr = fs.format(); fr.is_err()) {
        // Hardware fault — escalate to your error handler.
    }
    (void)fs.mount();
}
```

`format()` MUST NOT be called on every boot — it erases user data.
Standard idiom: try `mount()`, format only on failure.

---

## File operations

```cpp
auto open = fs.open("/log.txt",
    hal::filesystem::OpenMode::WriteAppendCreate);
if (open.is_err()) return open.error();

auto& file = open.value();
(void)file.write(std::span<const std::byte>{payload, payload_n});
(void)file.close();
```

`OpenMode` enumerates `Read`, `ReadWrite`, `WriteAppendCreate`,
`ReadWriteCreate`, `WriteCreateTruncate`. `SeekOrigin` is
`Begin`, `Current`, `End`.

Closing always succeeds at the API level; underlying flush failures
surface as a typed error from `close()` itself, not silently dropped.

---

## Power-loss safety

littlefs is the only backend that guarantees mountable-after-power-loss
in the general case. The guarantee is: at any point during a write,
pulling power leaves the filesystem in a state where the next `mount()`
either sees the new data fully committed or sees the previous version.
There is no in-between corrupt state.

FatFS does NOT have this guarantee. Pulling power mid-write can leave
the FAT inconsistent. If interop with a host PC is your goal but power
loss is also a real risk, copy to FatFS only at known-safe boundaries
(end of session) and otherwise log into littlefs.

---

## Wiring on existing boards

Both filesystem demos target SAME70 Xplained Ultra:

- **`examples/filesystem_littlefs/`** — W25Q128 SPI NOR over SPI0.
  Wiring: /CS → PD25, CLK → PD22, DO → PD20, DI → PD21, /WP → VCC,
  /HOLD → VCC. JEDEC `EF 40 18`. Demo formats on first boot, increments
  `/counter.txt` on subsequent boots.

- **`examples/filesystem_fatfs_sdcard/`** — SD card over SPI0. Wiring:
  /CS → PD26, CLK → PD22, MISO → PD20, MOSI → PD21. Appends a
  timestamped line to `/log.txt` each boot.

> **HW validation status:** both examples build and link clean; on-bench
> validation pending physical W25Q128 + SD card module — see
> `docs/SUPPORT_MATRIX.md` for the current `filesystem` tier.

---

## USB Mass Storage (planned)

`drivers/usb/msc/MassStorage<Controller, Device>` will expose any
`BlockDevice` to a host PC as a USB drive. SD-card-via-USB without a
host driver, exactly the appliance pattern. Lands once `add-usb-hal`
ships.

---

## Constraints

- No heap (`new` / `malloc`) anywhere in the filesystem HAL or its
  backends.
- No RTOS dependency. Filesystem operations are synchronous and run on
  the calling task.
- `noexcept` throughout. Every error is a typed `core::Result`.
- No global mutable state; the `Filesystem<Backend>` instance owns
  every buffer.
- Buffer sizing is compile-time only — no runtime resizing.
- littlefs and FatFS are vendored at pinned tags via `FetchContent`;
  no system-installed copy is consulted.

---

## Follow-ups (not yet shipped)

- Host unit-test suite (`test_w25q_block_device.cpp`, `test_sdcard.cpp`,
  `test_littlefs_host.cpp`, `test_fatfs_host.cpp`) — tracked under
  `add-filesystem-host-test-suite`.
- W25Q power-cut bench test + SD-card 100 MB throughput test —
  tracked with the hardware-tier promotion.
- USB MSC class driver — gated on `add-usb-hal`.
- Cookbook recipe page in `docs/COOKBOOK.md`.
