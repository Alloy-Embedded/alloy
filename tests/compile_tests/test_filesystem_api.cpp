// Compile test: Filesystem<> and File<> API compiles against a mock backend
// without requiring littlefs or fatfs headers.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/filesystem/filesystem.hpp"

namespace {

using namespace alloy::hal::filesystem;
using alloy::core::ErrorCode;
using alloy::core::Ok;
using alloy::core::Err;
using alloy::core::Result;

// ── Minimal mock backend ─────────────────────────────────────────────────────

struct MockFileHandle {
    bool open{false};
};

struct MockBackend {
    using FileHandle = MockFileHandle;

    [[nodiscard]] auto mount()  -> Result<void, ErrorCode> { return Ok(); }
    [[nodiscard]] auto format() -> Result<void, ErrorCode> { return Ok(); }

    [[nodiscard]] auto open(std::string_view, OpenMode)
        -> Result<FileHandle, ErrorCode> {
        return Ok(FileHandle{.open = true});
    }

    [[nodiscard]] auto read(FileHandle&, std::span<std::byte>)
        -> Result<std::size_t, ErrorCode> { return Ok(std::size_t{0}); }

    [[nodiscard]] auto write(FileHandle&, std::span<const std::byte>)
        -> Result<std::size_t, ErrorCode> { return Ok(std::size_t{0}); }

    [[nodiscard]] auto seek(FileHandle&, std::int64_t, SeekOrigin)
        -> Result<void, ErrorCode> { return Ok(); }

    [[nodiscard]] auto tell(const FileHandle&) const
        -> Result<std::int64_t, ErrorCode> { return Ok(std::int64_t{0}); }

    [[nodiscard]] auto close(FileHandle&)
        -> Result<void, ErrorCode> { return Ok(); }

    [[nodiscard]] auto remove(std::string_view)
        -> Result<void, ErrorCode> { return Ok(); }

    [[nodiscard]] auto rename(std::string_view, std::string_view)
        -> Result<void, ErrorCode> { return Ok(); }

    [[nodiscard]] auto stat(std::string_view)
        -> Result<DirEntry, ErrorCode> {
        return Ok(DirEntry{});
    }
};

// ── Compile smoke ────────────────────────────────────────────────────────────

[[maybe_unused]] void compile_filesystem_and_file_api() {
    Filesystem<MockBackend> fs{MockBackend{}};

    (void)fs.mount();
    (void)fs.format();

    auto file_result = fs.open("/test.txt", OpenMode::ReadWrite);
    (void)file_result.is_ok();

    if (file_result.is_ok()) {
        auto file = std::move(file_result).unwrap();

        std::byte buf[64]{};
        (void)file.read(buf);
        (void)file.write(std::span<const std::byte>{buf});
        (void)file.seek(0, SeekOrigin::Begin);
        (void)file.tell();
        (void)file.is_open();
        (void)file.close();
    }

    (void)fs.remove("/test.txt");
    (void)fs.rename("/a.txt", "/b.txt");
    (void)fs.stat("/b.txt");
}

// File<> destructor auto-closes — verify it compiles cleanly.
[[maybe_unused]] void compile_file_auto_close() {
    Filesystem<MockBackend> fs{MockBackend{}};
    {
        auto f = fs.open("/x", OpenMode::ReadOnly);
        (void)f.is_ok();
        // f destroyed here → close() called automatically
    }
}

// File<> move semantics.
[[maybe_unused]] void compile_file_move() {
    Filesystem<MockBackend> fs{MockBackend{}};
    auto r = fs.open("/a", OpenMode::WriteOnly);
    if (r.is_ok()) {
        auto f1 = std::move(r).unwrap();
        auto f2 = std::move(f1);  // move construct
        (void)f2.is_open();
    }
}

}  // namespace
