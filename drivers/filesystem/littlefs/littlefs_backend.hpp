#pragma once

// LittleFS backend for alloy::hal::filesystem::Filesystem<>.
//
// Wraps the littlefs C library (https://github.com/littlefs-project/littlefs,
// MIT) behind the Backend interface expected by Filesystem<Backend> and
// File<Backend>. No heap is used: all buffers are std::array members.
//
// Template parameters:
//   Device      — must satisfy BlockDevice concept.
//   ReadSz      — read cache size in bytes. Must be >= device.block_size()
//                 when used as the read granularity. Typically 256 or 512.
//   ProgSz      — prog (write) cache size. Same constraints as ReadSz.
//   CacheSz     — per-operation transfer cache (read + prog copy in lfs_config).
//                 Must be a multiple of ReadSz and ProgSz. Typically CacheSz = ReadSz.
//   LookaheadSz — lookahead buffer in bytes. Must be a multiple of 8.
//                 Controls how many blocks littlefs scans ahead for free space.
//                 Larger = fewer passes, more RAM. Minimum 8.
//
// Usage:
//   using Flash = w25q::BlockDevice<SpiHandle, 4096>;   // 4096 × 4 KiB = 16 MB
//   using Lfs   = LittlefsBackend<Flash, 256, 256, 256, 16>;
//   Filesystem<Lfs> fs{Lfs{flash_device}};
//   fs.mount().unwrap();
//
// Requires:
//   CMake target alloy-littlefs (from drivers/filesystem/CMakeLists.txt) must
//   be linked. It compiles lfs.c and lfs_util.c from the fetched source tree
//   with LFS_NO_MALLOC=1.

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

// littlefs C header — available after alloy-littlefs FetchContent.
#include <lfs.h>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/filesystem/block_device.hpp"
#include "hal/filesystem/filesystem.hpp"

namespace alloy::drivers::filesystem::littlefs {

namespace detail {

inline auto lfs_err_to_error_code(int err) -> alloy::core::ErrorCode {
    switch (err) {
        case LFS_ERR_IO:          return alloy::core::ErrorCode::CommunicationError;
        case LFS_ERR_CORRUPT:     return alloy::core::ErrorCode::ChecksumError;
        case LFS_ERR_NOENT:       return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_EXIST:       return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_NOTDIR:      return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_ISDIR:       return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_NOTEMPTY:    return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_BADF:        return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_FBIG:        return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_INVAL:       return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_NOSPC:       return alloy::core::ErrorCode::HardwareError;
        case LFS_ERR_NOMEM:       return alloy::core::ErrorCode::HardwareError;
        case LFS_ERR_NOATTR:      return alloy::core::ErrorCode::InvalidParameter;
        case LFS_ERR_NAMETOOLONG: return alloy::core::ErrorCode::InvalidParameter;
        default:                  return alloy::core::ErrorCode::HardwareError;
    }
}

inline auto open_flags(alloy::hal::filesystem::OpenMode mode) -> int {
    using OpenMode = alloy::hal::filesystem::OpenMode;
    switch (mode) {
        case OpenMode::ReadOnly:  return LFS_O_RDONLY;
        case OpenMode::WriteOnly: return LFS_O_WRONLY | LFS_O_CREAT;
        case OpenMode::ReadWrite: return LFS_O_RDWR  | LFS_O_CREAT;
        case OpenMode::Append:    return LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND;
        case OpenMode::Truncate:  return LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
        default:                  return LFS_O_RDONLY;
    }
}

inline auto seek_whence(alloy::hal::filesystem::SeekOrigin origin) -> int {
    using SeekOrigin = alloy::hal::filesystem::SeekOrigin;
    switch (origin) {
        case SeekOrigin::Begin:   return LFS_SEEK_SET;
        case SeekOrigin::Current: return LFS_SEEK_CUR;
        case SeekOrigin::End:     return LFS_SEEK_END;
        default:                  return LFS_SEEK_SET;
    }
}

}  // namespace detail

template <hal::filesystem::BlockDevice Device,
          std::size_t ReadSz      = 256,
          std::size_t ProgSz      = 256,
          std::size_t CacheSz     = 256,
          std::size_t LookaheadSz = 16>
class LittlefsBackend {
    static_assert(LookaheadSz % 8 == 0, "LookaheadSz must be a multiple of 8");
    static_assert(CacheSz >= ReadSz && CacheSz >= ProgSz,
                  "CacheSz must be >= ReadSz and ProgSz");

public:
    // FileHandle holds lfs_file_t + its private cache buffer.
    // No heap: the buffer lives inside FileHandle (on the caller's stack or as
    // a static/member variable).
    struct FileHandle {
        lfs_file_t                     file{};
        alignas(4) std::array<std::uint8_t, CacheSz> cache{};
        lfs_file_config                cfg{.buffer = cache.data(), .attrs = nullptr, .attr_count = 0};
        bool                           open{false};
    };

    explicit LittlefsBackend(Device& device) : device_(&device) {
        build_config();
    }

    // ── Backend interface (called by Filesystem<> and File<>) ────────────────

    [[nodiscard]] auto mount() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (int err = lfs_mount(&lfs_, &cfg_); err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto format() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (int err = lfs_format(&lfs_, &cfg_); err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto open(std::string_view path,
                            hal::filesystem::OpenMode mode)
        -> alloy::core::Result<FileHandle, alloy::core::ErrorCode> {
        FileHandle fh{};
        // lfs_file_opencfg requires a null-terminated path.
        char path_buf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        const std::size_t n = std::min(path.size(), sizeof(path_buf) - 1);
        std::memcpy(path_buf, path.data(), n);
        path_buf[n] = '\0';

        if (int err = lfs_file_opencfg(&lfs_, &fh.file, path_buf,
                                        detail::open_flags(mode), &fh.cfg);
            err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        fh.open = true;
        return alloy::core::Ok(std::move(fh));
    }

    [[nodiscard]] auto read(FileHandle& fh, std::span<std::byte> buf)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        const lfs_ssize_t n = lfs_file_read(&lfs_, &fh.file,
                                             buf.data(),
                                             static_cast<lfs_size_t>(buf.size()));
        if (n < 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(static_cast<int>(n)));
        }
        return alloy::core::Ok(static_cast<std::size_t>(n));
    }

    [[nodiscard]] auto write(FileHandle& fh, std::span<const std::byte> data)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        const lfs_ssize_t n = lfs_file_write(&lfs_, &fh.file,
                                              data.data(),
                                              static_cast<lfs_size_t>(data.size()));
        if (n < 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(static_cast<int>(n)));
        }
        return alloy::core::Ok(static_cast<std::size_t>(n));
    }

    [[nodiscard]] auto seek(FileHandle& fh,
                            std::int64_t offset,
                            hal::filesystem::SeekOrigin origin)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        const lfs_soff_t pos = lfs_file_seek(&lfs_, &fh.file,
                                              static_cast<lfs_soff_t>(offset),
                                              detail::seek_whence(origin));
        if (pos < 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(static_cast<int>(pos)));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto tell(const FileHandle& fh) const
        -> alloy::core::Result<std::int64_t, alloy::core::ErrorCode> {
        const lfs_soff_t pos = lfs_file_tell(
            const_cast<lfs_t*>(&lfs_),
            const_cast<lfs_file_t*>(&fh.file));
        if (pos < 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(static_cast<int>(pos)));
        }
        return alloy::core::Ok(static_cast<std::int64_t>(pos));
    }

    [[nodiscard]] auto close(FileHandle& fh)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (!fh.open) return alloy::core::Ok();
        fh.open = false;
        if (int err = lfs_file_close(&lfs_, &fh.file); err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto remove(std::string_view path)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        char buf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        const std::size_t n = std::min(path.size(), sizeof(buf) - 1);
        std::memcpy(buf, path.data(), n);
        if (int err = lfs_remove(&lfs_, buf); err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto rename(std::string_view old_path, std::string_view new_path)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        char obuf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        char nbuf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        std::memcpy(obuf, old_path.data(), std::min(old_path.size(), sizeof(obuf) - 1));
        std::memcpy(nbuf, new_path.data(), std::min(new_path.size(), sizeof(nbuf) - 1));
        if (int err = lfs_rename(&lfs_, obuf, nbuf); err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto stat(std::string_view path)
        -> alloy::core::Result<hal::filesystem::DirEntry, alloy::core::ErrorCode> {
        char buf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        std::memcpy(buf, path.data(), std::min(path.size(), sizeof(buf) - 1));
        struct lfs_info info{};
        if (int err = lfs_stat(&lfs_, buf, &info); err != 0) {
            return alloy::core::Err(detail::lfs_err_to_error_code(err));
        }
        hal::filesystem::DirEntry entry{};
        std::memcpy(entry.name, info.name,
                    std::min(sizeof(info.name), sizeof(entry.name) - 1));
        entry.size   = info.size;
        entry.is_dir = (info.type == LFS_TYPE_DIR);
        return alloy::core::Ok(std::move(entry));
    }

private:
    // ── lfs_config callbacks (static, context = LittlefsBackend*) ────────────

    static auto lfs_read_cb(const struct lfs_config* c,
                             lfs_block_t block,
                             lfs_off_t off,
                             void* buffer,
                             lfs_size_t size) -> int {
        auto* self = static_cast<LittlefsBackend*>(c->context);
        // Forward as a block-addressed read; off is the intra-block byte offset.
        // Reuse read_buf_: do a full block read and memcpy the relevant slice.
        // Since the Device concept exposes block-level read, we need to read
        // the full block and memcpy the relevant slice.
        // For efficiency we could cache the block, but this simple path
        // reads the whole block each time (correct, slightly slow).
        const std::size_t blk_sz = self->device_->block_size();
        static_assert(sizeof(self->scratch_) >= 1,
                      "scratch_ must hold at least one byte");
        // lfs guarantees off + size <= block_size, so a single read suffices
        // if we read the whole block into a temp buffer. Here we use the
        // existing scratch_ (sized to block_size or ProgSz, whichever is
        // larger). Since LookaheadSz is typically much smaller, we read the
        // whole block into read_buf_ (a block-sized array member).
        auto span = std::span<std::byte>{self->read_buf_.data(), blk_sz};
        if (auto r = self->device_->read(static_cast<std::size_t>(block), span);
            r.is_err()) {
            return LFS_ERR_IO;
        }
        std::memcpy(buffer, self->read_buf_.data() + off, size);
        return 0;
    }

    static auto lfs_prog_cb(const struct lfs_config* c,
                             lfs_block_t block,
                             lfs_off_t off,
                             const void* buffer,
                             lfs_size_t size) -> int {
        auto* self = static_cast<LittlefsBackend*>(c->context);
        const std::size_t blk_sz = self->device_->block_size();
        // Read-modify-write: read current block, patch the [off, off+size) slice,
        // write back. (LFS guarantees it won't cross a block boundary.)
        auto span = std::span<std::byte>{self->read_buf_.data(), blk_sz};
        if (auto r = self->device_->read(static_cast<std::size_t>(block), span);
            r.is_err()) {
            return LFS_ERR_IO;
        }
        std::memcpy(self->read_buf_.data() + off, buffer, size);
        auto wspan = std::span<const std::byte>{self->read_buf_.data(), blk_sz};
        if (auto r = self->device_->write(static_cast<std::size_t>(block), wspan);
            r.is_err()) {
            return LFS_ERR_IO;
        }
        return 0;
    }

    static auto lfs_erase_cb(const struct lfs_config* c, lfs_block_t block) -> int {
        auto* self = static_cast<LittlefsBackend*>(c->context);
        if (auto r = self->device_->erase(static_cast<std::size_t>(block), 1);
            r.is_err()) {
            return LFS_ERR_IO;
        }
        return 0;
    }

    static auto lfs_sync_cb(const struct lfs_config* /*c*/) -> int { return 0; }

    void build_config() {
        cfg_.context = this;
        cfg_.read    = lfs_read_cb;
        cfg_.prog    = lfs_prog_cb;
        cfg_.erase   = lfs_erase_cb;
        cfg_.sync    = lfs_sync_cb;

        cfg_.read_size      = ReadSz;
        cfg_.prog_size      = ProgSz;
        cfg_.block_size     = device_->block_size();
        cfg_.block_count    = device_->block_count();
        cfg_.cache_size     = CacheSz;
        cfg_.lookahead_size = LookaheadSz;
        cfg_.block_cycles   = 500;  // wear leveling aggressiveness

        cfg_.read_buffer      = read_cache_.data();
        cfg_.prog_buffer      = prog_cache_.data();
        cfg_.lookahead_buffer = lookahead_buf_.data();
    }

    Device*                                         device_;
    lfs_t                                           lfs_{};
    lfs_config                                      cfg_{};

    // Block-sized scratch buffer for read-modify-write in prog callback.
    // Sized to the maximum block size we might encounter at compile time.
    // If the actual block_size() > this, the program will assert at runtime.
    // W25Q sectors are 4096 B; SD blocks are 512 B.
    static constexpr std::size_t kMaxBlockSize = 4096;
    alignas(4) std::array<std::byte, kMaxBlockSize>  read_buf_{};

    // lfs_config buffers — no heap.
    alignas(4) std::array<std::uint8_t, CacheSz>     read_cache_{};
    alignas(4) std::array<std::uint8_t, CacheSz>     prog_cache_{};
    alignas(4) std::array<std::uint8_t, LookaheadSz> lookahead_buf_{};
    alignas(4) std::array<std::uint8_t, 1>            scratch_{};  // placeholder
};

}  // namespace alloy::drivers::filesystem::littlefs
