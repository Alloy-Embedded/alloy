// Host tests for the littlefs filesystem facade (alloy/fs/littlefs.hpp). A RAM
// block device stands in for flash, so the exact portable code that runs on
// silicon is exercised here under ASan/UBSan (littlefs asserts stay ON). Two
// geometries are covered — internal-flash-like (2 KB block, 8 B program) and
// SPI-NOR-like (4 KB block, 256 B program) — to catch geometry-specific bugs.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/fs/littlefs.hpp"
#include "alloy_test.hpp"

namespace {
// A RAM-backed BlockDevice: erase fills 0xFF, program clears bits (NOR AND
// semantics — so a stray double-program would corrupt, like real flash), read
// is a memcpy. A shared static store lets a fresh filesystem instance re-open
// the same bytes (the persistence-across-remount test).
template <std::uint32_t BlockSize, std::uint32_t BlockCount,
          std::uint32_t ReadSize, std::uint32_t ProgSize>
struct ram_bd {
    static inline std::uint8_t store[BlockSize * BlockCount];

    [[nodiscard]] bool bd_read(std::uint32_t b, std::uint32_t off, void* buf,
                               std::size_t n) const {
        std::memcpy(buf, store + b * BlockSize + off, n);
        return true;
    }
    [[nodiscard]] bool bd_prog(std::uint32_t b, std::uint32_t off, const void* buf,
                               std::size_t n) const {
        std::uint8_t* dst = store + b * BlockSize + off;
        const auto* src = static_cast<const std::uint8_t*>(buf);
        for (std::size_t i = 0; i < n; ++i) dst[i] &= src[i];  // NOR: bits go 1->0 only
        return true;
    }
    [[nodiscard]] bool bd_erase(std::uint32_t b) const {
        std::memset(store + b * BlockSize, 0xFF, BlockSize);
        return true;
    }
    [[nodiscard]] bool bd_sync() const { return true; }
    [[nodiscard]] std::uint32_t block_size() const { return BlockSize; }
    [[nodiscard]] std::uint32_t block_count() const { return BlockCount; }
    [[nodiscard]] std::uint32_t read_size() const { return ReadSize; }
    [[nodiscard]] std::uint32_t prog_size() const { return ProgSize; }
};

using flash_bd = ram_bd<2048, 16, 8, 8>;   // internal-flash-like
using spi_bd = ram_bd<4096, 8, 1, 256>;     // SPI-NOR-like

static_assert(alloy::BlockDevice<flash_bd>);
static_assert(alloy::BlockDevice<spi_bd>);

template <class Bd>
void wipe() {
    std::memset(Bd::store, 0xFF, sizeof(Bd::store));
}

std::span<const std::uint8_t> bytes(const char* s) {
    return {reinterpret_cast<const std::uint8_t*>(s), std::strlen(s)};
}

// A FlashController (internal-flash contract: erase whole pages, program 64-bit
// double-words, reads memory-mapped) over a static RAM region — drives the REAL
// alloy::fs::flash_block_device so its 8-byte dword bd_prog loop and block_count
// math run under ASan, not just ram_bd.
struct fake_flash_ctrl {
    static constexpr std::uint32_t page_size = 2048;
    alignas(std::uint64_t) static inline std::uint8_t region[32768];
    [[nodiscard]] bool erase_page(std::uintptr_t addr) const {
        std::memset(reinterpret_cast<void*>(addr), 0xFF, page_size);
        return true;
    }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* data,
                               std::size_t n) const {
        auto* w = reinterpret_cast<std::uint64_t*>(addr);
        for (std::size_t i = 0; i < n; ++i) w[i] &= data[i];  // NOR: bits 1->0 only
        return true;
    }
};

// A SPI-NOR device (explicit read, 256-byte page program, 4 KiB sector erase)
// over a static RAM region — drives the REAL alloy::fs::spi_block_device so its
// per-256-byte page-split bd_prog loop runs under ASan.
struct fake_spi_nor {
    alignas(std::uint64_t) static inline std::uint8_t region[32768];
    [[nodiscard]] bool read(std::uint32_t addr, std::span<std::uint8_t> out) const {
        std::memcpy(out.data(), region + addr, out.size());
        return true;
    }
    [[nodiscard]] bool page_program(std::uint32_t addr,
                                    std::span<const std::uint8_t> data) const {
        for (std::size_t i = 0; i < data.size(); ++i) region[addr + i] &= data[i];  // NOR
        return true;
    }
    [[nodiscard]] bool sector_erase_4k(std::uint32_t addr) const {
        std::memset(region + addr, 0xFF, 4096);
        return true;
    }
};

// A deterministic multi-KB payload that spans several blocks and program pages.
void fill_pattern(std::span<std::uint8_t> buf, std::uint8_t seed) {
    for (std::size_t i = 0; i < buf.size(); ++i) {
        buf[i] = static_cast<std::uint8_t>(i * 7u + seed);
    }
}
}  // namespace

ALLOY_TEST(fs_format_then_mount) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(!fs.mount());  // blank region: nothing valid to mount
    ALLOY_CHECK(fs.format());
    ALLOY_CHECK(fs.mount());
    ALLOY_CHECK(fs.is_mounted());
}

ALLOY_TEST(fs_mount_or_format_on_blank) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());  // formats once, then mounts
    ALLOY_CHECK(fs.is_mounted());
}

ALLOY_TEST(fs_write_read_roundtrip) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.write_file("/hello.txt", bytes("alloy")));
    std::uint8_t rd[16] = {};
    const int n = fs.read_file("/hello.txt", rd);
    ALLOY_CHECK_EQ(n, 5);
    ALLOY_CHECK(std::memcmp(rd, "alloy", 5) == 0);
    ALLOY_CHECK_EQ(fs.file_size("/hello.txt"), 5);
    ALLOY_CHECK(fs.exists("/hello.txt"));
    ALLOY_CHECK(!fs.exists("/nope"));
}

ALLOY_TEST(fs_overwrite_truncates) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.write_file("/x", bytes("longer-value")));
    ALLOY_CHECK(fs.write_file("/x", bytes("hi")));  // create|trunc
    ALLOY_CHECK_EQ(fs.file_size("/x"), 2);
}

ALLOY_TEST(fs_persists_across_remount) {
    wipe<flash_bd>();
    {
        alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
        ALLOY_CHECK(fs.mount_or_format());
        ALLOY_CHECK(fs.write_file("/cfg", bytes("42")));
        fs.unmount();
    }
    // A brand-new instance over the same bytes must see the file — mount, not format.
    alloy::fs::filesystem<flash_bd> fs2{flash_bd{}};
    ALLOY_CHECK(fs2.mount());
    std::uint8_t rd[8] = {};
    const int n = fs2.read_file("/cfg", rd);
    ALLOY_CHECK_EQ(n, 2);
    ALLOY_CHECK(std::memcmp(rd, "42", 2) == 0);
}

ALLOY_TEST(fs_append_grows_file) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.append_file("/log", bytes("a")));
    ALLOY_CHECK(fs.append_file("/log", bytes("bc")));
    std::uint8_t rd[8] = {};
    const int n = fs.read_file("/log", rd);
    ALLOY_CHECK_EQ(n, 3);
    ALLOY_CHECK(std::memcmp(rd, "abc", 3) == 0);
}

ALLOY_TEST(fs_mkdir_and_list) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.mkdir("/d"));
    ALLOY_CHECK(fs.write_file("/d/a", bytes("xx")));
    int files = 0;
    std::uint32_t seen_size = 0;
    ALLOY_CHECK(fs.for_each("/d", [&](const char* name, std::uint32_t size, bool is_dir) {
        if (name[0] == '.') return;  // skip "." and ".."
        if (!is_dir) {
            ++files;
            seen_size = size;
        }
    }));
    ALLOY_CHECK_EQ(files, 1);
    ALLOY_CHECK_EQ(seen_size, 2u);
}

ALLOY_TEST(fs_remove) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.write_file("/tmp", bytes("z")));
    ALLOY_CHECK(fs.exists("/tmp"));
    ALLOY_CHECK(fs.remove("/tmp"));
    ALLOY_CHECK(!fs.exists("/tmp"));
}

ALLOY_TEST(fs_spi_geometry_roundtrip) {
    wipe<spi_bd>();
    alloy::fs::filesystem<spi_bd> fs{spi_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.write_file("/big", bytes("the quick brown fox")));
    std::uint8_t rd[32] = {};
    const int n = fs.read_file("/big", rd);
    ALLOY_CHECK_EQ(n, 19);
    ALLOY_CHECK(std::memcmp(rd, "the quick brown fox", 19) == 0);
}

// Exercise the PRODUCTION flash_block_device adapter (not just ram_bd): its
// 8-byte dword bd_prog loop, memory-mapped bd_read, and block_count, with a
// 5000-byte file that spans >1 erase block (2048) and hundreds of program units.
ALLOY_TEST(fs_flash_adapter_multikb) {
    std::memset(fake_flash_ctrl::region, 0xFF, sizeof(fake_flash_ctrl::region));
    using bd = alloy::fs::flash_block_device<fake_flash_ctrl>;
    static_assert(alloy::BlockDevice<bd>);
    alloy::fs::filesystem<bd> fs{bd{reinterpret_cast<std::uintptr_t>(fake_flash_ctrl::region),
                                    sizeof(fake_flash_ctrl::region)}};
    ALLOY_CHECK(fs.mount_or_format());
    std::uint8_t big[5000];
    fill_pattern(big, 3);
    ALLOY_CHECK(fs.write_file("/big", big));
    std::uint8_t rd[5000] = {};
    const int n = fs.read_file("/big", rd);
    ALLOY_CHECK_EQ(n, 5000);
    ALLOY_CHECK(std::memcmp(rd, big, sizeof(big)) == 0);
}

// Exercise the PRODUCTION spi_block_device adapter: its per-256-byte page-split
// bd_prog loop and 4 KiB block geometry, with a 5000-byte file spanning >1
// sector (4096) and ~20 program pages.
ALLOY_TEST(fs_spi_adapter_multikb) {
    std::memset(fake_spi_nor::region, 0xFF, sizeof(fake_spi_nor::region));
    using bd = alloy::fs::spi_block_device<fake_spi_nor>;
    static_assert(alloy::BlockDevice<bd>);
    alloy::fs::filesystem<bd> fs{
        bd{fake_spi_nor{}, static_cast<std::uint32_t>(sizeof(fake_spi_nor::region))}};
    ALLOY_CHECK(fs.mount_or_format());
    std::uint8_t big[5000];
    fill_pattern(big, 1);
    ALLOY_CHECK(fs.write_file("/big", big));
    std::uint8_t rd[5000] = {};
    const int n = fs.read_file("/big", rd);
    ALLOY_CHECK_EQ(n, 5000);
    ALLOY_CHECK(std::memcmp(rd, big, sizeof(big)) == 0);
}

// format() must leave the fs unmounted (littlefs deinits on format) so a
// standalone factory-reset — mount_or_format(); format(); mount() — actually
// re-mounts instead of operating on stale state.
ALLOY_TEST(fs_format_resets_mounted) {
    wipe<flash_bd>();
    alloy::fs::filesystem<flash_bd> fs{flash_bd{}};
    ALLOY_CHECK(fs.mount_or_format());
    ALLOY_CHECK(fs.write_file("/gone", bytes("x")));
    ALLOY_CHECK(fs.format());          // wipe
    ALLOY_CHECK(!fs.is_mounted());     // format left it unmounted
    ALLOY_CHECK(fs.mount());           // must actually re-mount, not early-return
    ALLOY_CHECK(!fs.exists("/gone"));  // the wipe really happened
}
