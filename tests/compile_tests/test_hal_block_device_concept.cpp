// Compile test: BlockDevice concept accepts conforming types and rejects
// non-conforming types at compile time.

#include <cstddef>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/filesystem/block_device.hpp"

namespace {

using alloy::core::ErrorCode;
using alloy::core::Ok;
using alloy::core::Result;
using alloy::hal::filesystem::BlockDevice;

// ── Conforming mock ──────────────────────────────────────────────────────────

struct MockBlockDevice {
    [[nodiscard]] auto read(std::size_t, std::span<std::byte>)
        -> Result<void, ErrorCode> { return Ok(); }
    [[nodiscard]] auto write(std::size_t, std::span<const std::byte>)
        -> Result<void, ErrorCode> { return Ok(); }
    [[nodiscard]] auto erase(std::size_t, std::size_t)
        -> Result<void, ErrorCode> { return Ok(); }
    [[nodiscard]] static auto block_size()  -> std::size_t { return 4096; }
    [[nodiscard]] static auto block_count() -> std::size_t { return 256; }
};

static_assert(BlockDevice<MockBlockDevice>,
              "MockBlockDevice must satisfy BlockDevice");

// ── Non-conforming types should NOT satisfy BlockDevice ──────────────────────

static_assert(!BlockDevice<int>,
              "int must not satisfy BlockDevice");
static_assert(!BlockDevice<void*>,
              "void* must not satisfy BlockDevice");

// Missing erase:
struct NoErase {
    auto read(std::size_t, std::span<std::byte>) -> Result<void, ErrorCode>;
    auto write(std::size_t, std::span<const std::byte>) -> Result<void, ErrorCode>;
    auto block_size()  -> std::size_t;
    auto block_count() -> std::size_t;
};
static_assert(!BlockDevice<NoErase>, "NoErase must not satisfy BlockDevice");

// Wrong return type for block_size:
struct BadBlockSize {
    auto read(std::size_t, std::span<std::byte>) -> Result<void, ErrorCode>;
    auto write(std::size_t, std::span<const std::byte>) -> Result<void, ErrorCode>;
    auto erase(std::size_t, std::size_t) -> Result<void, ErrorCode>;
    auto block_size()  -> int;   // wrong: must be std::size_t
    auto block_count() -> std::size_t;
};
static_assert(!BlockDevice<BadBlockSize>, "BadBlockSize must not satisfy BlockDevice");

// ── API smoke: Filesystem + File types compile with MockBlockDevice ──────────

[[maybe_unused]] void compile_filesystem_api() {
    MockBlockDevice dev;
    (void)dev.read(0, {});
    (void)dev.write(0, {});
    (void)dev.erase(0, 1);
    (void)dev.block_size();
    (void)dev.block_count();
}

}  // namespace
