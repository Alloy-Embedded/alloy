#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::filesystem {

// ── Supporting types ──────────────────────────────────────────────────────────

enum class OpenMode : std::uint8_t {
    ReadOnly  = 0x01,  // O_RDONLY
    WriteOnly = 0x02,  // O_WRONLY — implies Create
    ReadWrite = 0x03,  // O_RDWR
    Append    = 0x06,  // O_WRONLY | O_APPEND
    Truncate  = 0x0A,  // O_WRONLY | O_TRUNC — truncate on open
};

enum class SeekOrigin : std::uint8_t {
    Begin   = 0,
    Current = 1,
    End     = 2,
};

struct DirEntry {
    static constexpr std::size_t kMaxNameLen = 255;
    char   name[kMaxNameLen + 1]{};
    std::size_t size{};
    bool   is_dir{};
};

// ── File<Backend> ─────────────────────────────────────────────────────────────
//
// Returned by Filesystem<Backend>::open(). Non-copyable, moveable. The file
// is closed automatically when the object is destroyed.
//
// Backend requirements (internal contract, not enforced by concept here):
//   - Backend::FileHandle  — opaque file state type (moveable, default-constructible)
//   - backend.read(FileHandle&, span<byte>) -> Result<size_t>
//   - backend.write(FileHandle&, span<const byte>) -> Result<size_t>
//   - backend.seek(FileHandle&, int64_t, SeekOrigin) -> Result<void>
//   - backend.tell(const FileHandle&) -> Result<int64_t>
//   - backend.close(FileHandle&) -> Result<void>

template <typename Backend>
class Filesystem;  // forward declaration

template <typename Backend>
class File {
public:
    using FileHandle = typename Backend::FileHandle;

    File() = default;
    ~File() {
        if (valid_) {
            (void)close();
        }
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& o) noexcept
        : backend_(o.backend_), handle_(std::move(o.handle_)), valid_(o.valid_) {
        o.valid_   = false;
        o.backend_ = nullptr;
    }
    File& operator=(File&& o) noexcept {
        if (this != &o) {
            if (valid_) {
                (void)close();
            }
            backend_ = o.backend_;
            handle_  = std::move(o.handle_);
            valid_   = o.valid_;
            o.valid_   = false;
            o.backend_ = nullptr;
        }
        return *this;
    }

    [[nodiscard]] auto read(std::span<std::byte> buf)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        return backend_->read(handle_, buf);
    }

    [[nodiscard]] auto write(std::span<const std::byte> data)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        return backend_->write(handle_, data);
    }

    [[nodiscard]] auto seek(std::int64_t offset, SeekOrigin origin)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return backend_->seek(handle_, offset, origin);
    }

    [[nodiscard]] auto tell() const
        -> alloy::core::Result<std::int64_t, alloy::core::ErrorCode> {
        return backend_->tell(handle_);
    }

    [[nodiscard]] auto close()
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (!valid_) {
            return alloy::core::Ok();
        }
        valid_ = false;
        return backend_->close(handle_);
    }

    [[nodiscard]] auto is_open() const -> bool { return valid_; }

private:
    friend class Filesystem<Backend>;

    explicit File(Backend& b, FileHandle&& h)
        : backend_(&b), handle_(std::move(h)), valid_(true) {}

    Backend*   backend_{nullptr};
    FileHandle handle_{};
    bool       valid_{false};
};

// ── Filesystem<Backend> ───────────────────────────────────────────────────────
//
// Thin wrapper that exposes mount/format/open/remove/rename/stat via the Backend.
// Backend must be moveable. Filesystem is non-copyable (contains mutable state).
//
// Typical usage:
//   W25qBlockDevice<SpiHandle> dev{spi, {.block_count = 4096}};
//   LittlefsBackend<decltype(dev)> lfs_b{dev};
//   Filesystem fs{std::move(lfs_b)};
//   fs.mount();
//   auto file = fs.open("/cfg.txt", OpenMode::ReadWrite).unwrap();

template <typename Backend>
class Filesystem {
public:
    explicit Filesystem(Backend backend) : backend_(std::move(backend)) {}

    Filesystem(const Filesystem&) = delete;
    Filesystem& operator=(const Filesystem&) = delete;
    Filesystem(Filesystem&&) = default;
    Filesystem& operator=(Filesystem&&) = default;

    [[nodiscard]] auto mount()
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return backend_.mount();
    }

    [[nodiscard]] auto format()
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return backend_.format();
    }

    [[nodiscard]] auto open(std::string_view path, OpenMode mode)
        -> alloy::core::Result<File<Backend>, alloy::core::ErrorCode> {
        auto result = backend_.open(path, mode);
        if (result.is_err()) {
            return alloy::core::Err(std::move(result).err());
        }
        return alloy::core::Ok(File<Backend>{backend_, std::move(result).unwrap()});
    }

    [[nodiscard]] auto remove(std::string_view path)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return backend_.remove(path);
    }

    [[nodiscard]] auto rename(std::string_view old_path, std::string_view new_path)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return backend_.rename(old_path, new_path);
    }

    [[nodiscard]] auto stat(std::string_view path)
        -> alloy::core::Result<DirEntry, alloy::core::ErrorCode> {
        return backend_.stat(path);
    }

private:
    Backend backend_;
};

}  // namespace alloy::hal::filesystem
