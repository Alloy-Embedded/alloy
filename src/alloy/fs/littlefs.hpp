// A small persistent filesystem, backed by the vendored littlefs — a
// power-loss-resilient, wear-levelling log filesystem (fs/vendor/LICENSE.md).
// Where alloy/util/nvm_kv.hpp gives a flat key/value log over one flash page,
// this gives real named files and directories that survive reset.
//
// Portable by construction: written against the alloy::BlockDevice concept, so
// the SAME filesystem runs on any backing store — internal MCU flash or an
// external SPI-NOR — and unit-tests on the host against a RAM block device.
//
// Usage (see examples/fs; zero #ifdef — the null stub + caps gate carry it):
//   if constexpr (board::caps::fs) {
//       board::fs.mount_or_format();
//       board::fs.write_file("/boot", bytes);
//       int n = board::fs.read_file("/boot", buf);   // -1 on error
//   }
//
// Facts (region base/size, block/prog geometry) come from generated board data
// and the flash HAL — this header carries no chip addresses (guard #1).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/concepts.hpp"
#include "alloy/flash.hpp"
#include "alloy/fs/vendor/lfs.h"

namespace alloy::fs {

// ── Block-device adapters ────────────────────────────────────────────────

// Adapts an alloy::FlashController (internal MCU flash: memory-mapped reads,
// erase-by-page, program in 64-bit double-words) to the BlockDevice contract.
// One erase page = one littlefs block. `base`/`size` describe the reserved
// region (absolute flash address + byte length), both from generated board
// data; the flash HAL turns the absolute address into an erase page/bank.
template <alloy::FlashController Flash>
class flash_block_device {
    std::uintptr_t base_;
    std::uint32_t  size_;
    Flash          flash_{};

public:
    constexpr flash_block_device(std::uintptr_t base, std::uint32_t size)
        : base_(base), size_(size) {}

    // Reads are memory-mapped: the flash array is directly addressable.
    [[nodiscard]] bool bd_read(std::uint32_t block, std::uint32_t off,
                               void* buffer, std::size_t n) const {
        const auto* src =
            reinterpret_cast<const std::uint8_t*>(base_ + block * block_size() + off);
        std::memcpy(buffer, src, n);
        return true;
    }

    // Program `n` bytes (a multiple of prog_size = 8) as 64-bit double-words.
    // Each dword is copied through an aligned scalar so an unaligned littlefs
    // cache is safe, then handed to the HAL one at a time (its program unit).
    [[nodiscard]] bool bd_prog(std::uint32_t block, std::uint32_t off,
                               const void* buffer, std::size_t n) const {
        std::uintptr_t addr = base_ + block * block_size() + off;
        const auto* p = static_cast<const std::uint8_t*>(buffer);
        for (std::size_t i = 0; i < n; i += 8u) {
            std::uint64_t word;
            std::memcpy(&word, p + i, 8u);
            if (!flash_.program(addr, &word, 1u)) return false;
            addr += 8u;
        }
        return true;
    }

    [[nodiscard]] bool bd_erase(std::uint32_t block) const {
        return flash_.erase_page(base_ + block * block_size());
    }

    // program/erase busy-wait to completion in the HAL, so nothing is buffered.
    [[nodiscard]] bool bd_sync() const { return true; }

    [[nodiscard]] std::uint32_t block_size() const { return Flash::page_size; }
    [[nodiscard]] std::uint32_t block_count() const { return size_ / block_size(); }
    // Reads are memory-mapped (any granularity); 8 keeps caches dword-aligned so
    // the flash HAL's 64-bit program unit never straddles a cache boundary.
    [[nodiscard]] std::uint32_t read_size() const { return 8u; }
    [[nodiscard]] std::uint32_t prog_size() const { return 8u; }
};

// Adapts the SPI-NOR driver (alloy::drivers::spi_flash: 4 KiB erase sectors,
// 256-byte program pages, explicit reads) to the BlockDevice contract.
// `capacity` is the chip size in bytes (jedec_id().capacity_bytes()).
template <class SpiFlash>
class spi_block_device {
    SpiFlash      flash_;
    std::uint32_t capacity_;

public:
    static constexpr std::uint32_t block_bytes = 4096u;  // 4 KiB sector erase
    static constexpr std::uint32_t prog_bytes  = 256u;   // 256-byte page program

    constexpr spi_block_device(SpiFlash flash, std::uint32_t capacity)
        : flash_(flash), capacity_(capacity) {}

    [[nodiscard]] bool bd_read(std::uint32_t block, std::uint32_t off,
                               void* buffer, std::size_t n) const {
        return flash_.read(block * block_bytes + off,
                           std::span<std::uint8_t>(static_cast<std::uint8_t*>(buffer), n));
    }

    // littlefs writes in prog_size (256 B) chunks aligned to the page; split any
    // larger flush into single-page programs (the driver rejects page crossings).
    [[nodiscard]] bool bd_prog(std::uint32_t block, std::uint32_t off,
                               const void* buffer, std::size_t n) const {
        const auto* p = static_cast<const std::uint8_t*>(buffer);
        const std::uint32_t start = block * block_bytes + off;
        for (std::size_t i = 0; i < n; i += prog_bytes) {
            const std::size_t chunk = (n - i < prog_bytes) ? n - i : prog_bytes;
            if (!flash_.page_program(
                    start + static_cast<std::uint32_t>(i),
                    std::span<const std::uint8_t>(p + i, chunk))) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool bd_erase(std::uint32_t block) const {
        return flash_.sector_erase_4k(block * block_bytes);
    }

    [[nodiscard]] bool bd_sync() const { return true; }

    [[nodiscard]] std::uint32_t block_size() const { return block_bytes; }
    [[nodiscard]] std::uint32_t block_count() const { return capacity_ / block_bytes; }
    [[nodiscard]] std::uint32_t read_size() const { return 1u; }
    [[nodiscard]] std::uint32_t prog_size() const { return prog_bytes; }
};

// ── The filesystem ───────────────────────────────────────────────────────

// A littlefs instance bound to a BlockDevice. Non-copyable and non-movable:
// littlefs keeps a back-pointer to this object (cfg_.context) for its I/O
// callbacks, so the object must not relocate — board::fs is a stable inline
// global. CacheSize must be a multiple of the device's read/prog size and a
// divisor of its block size (littlefs asserts this at mount/format).
template <alloy::BlockDevice Dev, std::uint32_t CacheSize = 256u,
          std::uint32_t LookaheadSize = 16u>
class filesystem {
    Dev        dev_;
    lfs_t      lfs_{};
    lfs_config cfg_{};
    bool       mounted_{false};

    alignas(std::uint64_t) std::uint8_t read_buf_[CacheSize];
    alignas(std::uint64_t) std::uint8_t prog_buf_[CacheSize];
    alignas(std::uint64_t) std::uint8_t lookahead_buf_[LookaheadSize];
    // One reusable per-file cache — file operations here are sequential
    // (open → I/O → close), never overlapping, so a single buffer suffices.
    alignas(std::uint64_t) std::uint8_t file_buf_[CacheSize];

    // littlefs I/O trampolines: recover `this` from cfg_.context and forward to
    // the block device. 0 = ok, LFS_ERR_IO = failure (littlefs treats it as a
    // media error and reports it up through mount/read/write).
    static int c_read(const lfs_config* c, lfs_block_t block, lfs_off_t off,
                      void* buffer, lfs_size_t size) {
        auto* self = static_cast<filesystem*>(c->context);
        return self->dev_.bd_read(block, off, buffer, size) ? 0 : LFS_ERR_IO;
    }
    static int c_prog(const lfs_config* c, lfs_block_t block, lfs_off_t off,
                      const void* buffer, lfs_size_t size) {
        auto* self = static_cast<filesystem*>(c->context);
        return self->dev_.bd_prog(block, off, buffer, size) ? 0 : LFS_ERR_IO;
    }
    static int c_erase(const lfs_config* c, lfs_block_t block) {
        auto* self = static_cast<filesystem*>(c->context);
        return self->dev_.bd_erase(block) ? 0 : LFS_ERR_IO;
    }
    static int c_sync(const lfs_config* c) {
        auto* self = static_cast<filesystem*>(c->context);
        return self->dev_.bd_sync() ? 0 : LFS_ERR_IO;
    }

    void build_config() {
        cfg_.context = this;
        cfg_.read = &c_read;
        cfg_.prog = &c_prog;
        cfg_.erase = &c_erase;
        cfg_.sync = &c_sync;
        cfg_.read_size = dev_.read_size();
        cfg_.prog_size = dev_.prog_size();
        cfg_.block_size = dev_.block_size();
        cfg_.block_count = dev_.block_count();
        // Recycle a block for wear-levelling after ~500 erase cycles.
        cfg_.block_cycles = 500;
        cfg_.cache_size = CacheSize;
        cfg_.lookahead_size = LookaheadSize;
        cfg_.read_buffer = read_buf_;
        cfg_.prog_buffer = prog_buf_;
        cfg_.lookahead_buffer = lookahead_buf_;
    }

    // Shared body for write_file/append_file (differ only in open flags).
    [[nodiscard]] bool write_impl(const char* path,
                                  std::span<const std::uint8_t> data, int flags) {
        if (!mounted_) return false;
        lfs_file_t f;
        lfs_file_config fcfg{};
        fcfg.buffer = file_buf_;
        if (lfs_file_opencfg(&lfs_, &f, path, flags, &fcfg) != 0) return false;
        const lfs_ssize_t wrote =
            lfs_file_write(&lfs_, &f, data.data(), static_cast<lfs_size_t>(data.size()));
        const int closed = lfs_file_close(&lfs_, &f);
        return wrote == static_cast<lfs_ssize_t>(data.size()) && closed == 0;
    }

public:
    explicit filesystem(Dev dev) : dev_(dev) { build_config(); }

    filesystem(const filesystem&) = delete;
    filesystem& operator=(const filesystem&) = delete;

    // Mount an existing filesystem. False if the region isn't a valid littlefs
    // (blank flash, corruption, or geometry mismatch).
    [[nodiscard]] bool mount() {
        if (mounted_) return true;
        mounted_ = (lfs_mount(&lfs_, &cfg_) == 0);
        return mounted_;
    }
    // Erase + lay down a fresh (empty) filesystem. Discards all contents.
    // littlefs leaves the fs UNMOUNTED after formatting (lfs_format ends in
    // lfs_deinit), so clear mounted_ — a caller must mount() before using it,
    // and is_mounted() stays honest.
    [[nodiscard]] bool format() {
        mounted_ = false;
        return lfs_format(&lfs_, &cfg_) == 0;
    }
    // The usual boot path: mount, and if that fails (first boot / blank flash)
    // format once and mount the fresh filesystem.
    [[nodiscard]] bool mount_or_format() {
        if (mount()) return true;
        if (!format()) return false;
        mounted_ = (lfs_mount(&lfs_, &cfg_) == 0);
        return mounted_;
    }
    void unmount() {
        if (mounted_) {
            lfs_unmount(&lfs_);
            mounted_ = false;
        }
    }
    [[nodiscard]] bool is_mounted() const { return mounted_; }

    // Whole-file write, creating or truncating. False on any I/O error.
    [[nodiscard]] bool write_file(const char* path, std::span<const std::uint8_t> data) {
        return write_impl(path, data, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    }
    // Append to a file, creating it if absent.
    [[nodiscard]] bool append_file(const char* path, std::span<const std::uint8_t> data) {
        return write_impl(path, data, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    }
    // Read up to out.size() bytes. Returns the byte count, or -1 on error.
    [[nodiscard]] int read_file(const char* path, std::span<std::uint8_t> out) {
        if (!mounted_) return -1;
        lfs_file_t f;
        lfs_file_config fcfg{};
        fcfg.buffer = file_buf_;
        if (lfs_file_opencfg(&lfs_, &f, path, LFS_O_RDONLY, &fcfg) != 0) return -1;
        const lfs_ssize_t n =
            lfs_file_read(&lfs_, &f, out.data(), static_cast<lfs_size_t>(out.size()));
        lfs_file_close(&lfs_, &f);
        return n < 0 ? -1 : static_cast<int>(n);
    }
    [[nodiscard]] bool remove(const char* path) {
        return mounted_ && lfs_remove(&lfs_, path) == 0;
    }
    [[nodiscard]] bool mkdir(const char* path) {
        if (!mounted_) return false;
        const int rc = lfs_mkdir(&lfs_, path);
        return rc == 0 || rc == LFS_ERR_EXIST;
    }
    [[nodiscard]] bool exists(const char* path) {
        if (!mounted_) return false;
        lfs_info info;
        return lfs_stat(&lfs_, path, &info) == 0;
    }
    // File size in bytes, or -1 if it doesn't exist / isn't a regular file.
    [[nodiscard]] std::int32_t file_size(const char* path) {
        if (!mounted_) return -1;
        lfs_info info;
        if (lfs_stat(&lfs_, path, &info) != 0) return -1;
        return static_cast<std::int32_t>(info.size);
    }
    // Invoke fn(name, size, is_dir) for each entry in `path` ("/" = root).
    // Returns false on a directory I/O error.
    template <class Fn>
    [[nodiscard]] bool for_each(const char* path, Fn&& fn) {
        if (!mounted_) return false;
        lfs_dir_t dir;
        if (lfs_dir_open(&lfs_, &dir, path) != 0) return false;
        lfs_info info;
        int rc;
        while ((rc = lfs_dir_read(&lfs_, &dir, &info)) > 0) {
            fn(static_cast<const char*>(info.name),
               static_cast<std::uint32_t>(info.size), info.type == LFS_TYPE_DIR);
        }
        lfs_dir_close(&lfs_, &dir);
        return rc == 0;
    }
};

// Convenience alias for the common case: littlefs over internal MCU flash.
// Generated board data constructs it as `fs{{base, size}}`.
template <class Flash>
using flash_filesystem = filesystem<flash_block_device<Flash>>;

// No-op stand-in for boards without an fs role — keeps caps-guarded code
// (`if constexpr (board::caps::fs)`) compiling everywhere, zero #ifdef.
struct null_filesystem {
    [[nodiscard]] bool mount() { return false; }
    [[nodiscard]] bool format() { return false; }
    [[nodiscard]] bool mount_or_format() { return false; }
    void unmount() {}
    [[nodiscard]] bool is_mounted() const { return false; }
    [[nodiscard]] bool write_file(const char*, std::span<const std::uint8_t>) { return false; }
    [[nodiscard]] bool append_file(const char*, std::span<const std::uint8_t>) { return false; }
    [[nodiscard]] int read_file(const char*, std::span<std::uint8_t>) { return -1; }
    [[nodiscard]] bool remove(const char*) { return false; }
    [[nodiscard]] bool mkdir(const char*) { return false; }
    [[nodiscard]] bool exists(const char*) { return false; }
    [[nodiscard]] std::int32_t file_size(const char*) { return -1; }
    template <class Fn>
    [[nodiscard]] bool for_each(const char*, Fn&&) { return false; }
};

}  // namespace alloy::fs
