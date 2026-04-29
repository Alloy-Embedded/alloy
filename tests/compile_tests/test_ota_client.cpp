// Compile test: OtaClient<Net, Flash> instantiates with mock types that
// satisfy TcpStreamProvider and FlashWriter concepts.
//
// Hardware dependency: none — this is a host-only compile-time check.
// The mock types are minimal concept satisfiers; no I/O occurs.
//
// Ref: openspec/changes/add-bootloader-integration/tasks.md §4.3

#include "drivers/ota/ota_client.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

using alloy::core::ErrorCode;
using alloy::core::Ok;
using alloy::core::Err;
using alloy::core::Result;

// ── Mock network stream ───────────────────────────────────────────────────────

struct FakeTcpStream {
    [[nodiscard]] auto connect(std::string_view /*url*/, std::uint16_t /*port*/)
        -> Result<void, ErrorCode>
    {
        return Ok();
    }

    [[nodiscard]] auto read(std::span<std::byte> buf)
        -> Result<std::size_t, ErrorCode>
    {
        // Simulate end-of-stream immediately (zero bytes available).
        for (auto& b : buf) b = std::byte{0xFF};
        return Ok(std::size_t{0});
    }

    void close() noexcept {}
};

static_assert(alloy::drivers::ota::TcpStreamProvider<FakeTcpStream>,
              "FakeTcpStream must satisfy TcpStreamProvider");

// ── Mock flash backend ────────────────────────────────────────────────────────

struct FakeFlash {
    static constexpr std::size_t kBlockSize  = 256u;
    static constexpr std::size_t kBlockCount = 0x1000u;  // 1 MB at 256-byte blocks

    [[nodiscard]] auto erase(std::size_t /*block*/, std::size_t /*count*/)
        -> Result<void, ErrorCode>
    {
        return Ok();
    }

    [[nodiscard]] auto write(std::size_t /*block*/, std::span<const std::byte> /*data*/)
        -> Result<void, ErrorCode>
    {
        return Ok();
    }

    [[nodiscard]] auto read(std::size_t /*block*/, std::span<std::byte> buf)
        -> Result<void, ErrorCode>
    {
        for (auto& b : buf) b = std::byte{0xFF};
        return Ok();
    }

    [[nodiscard]] constexpr std::size_t block_size()  const noexcept { return kBlockSize;  }
    [[nodiscard]] constexpr std::size_t block_count() const noexcept { return kBlockCount; }
};

static_assert(alloy::drivers::ota::FlashWriter<FakeFlash>,
              "FakeFlash must satisfy FlashWriter");

// ── Instantiation check ───────────────────────────────────────────────────────

namespace {

void exercise_ota_client() {
    using namespace alloy::drivers::ota;

    FakeTcpStream net{};
    FakeFlash flash{};

    OtaConfig cfg{
        .secondary_slot_offset = 0u,
        .slot_size             = FakeFlash::kBlockSize * FakeFlash::kBlockCount,
        .header_size           = 0x200u,
        .http_port             = 80u,
    };

    OtaClient<FakeTcpStream, FakeFlash> client{net, flash, cfg};

    // Compile-check: download() signature is callable
    constexpr Sha256Digest zero_sha256{};
    [[maybe_unused]] auto dl = client.download("http://example.local/fw.bin", zero_sha256);

    // Compile-check: request_update() and cancel_update() are callable
    [[maybe_unused]] auto req = client.request_update();
    [[maybe_unused]] auto can = client.cancel_update();

    // Compile-check: bytes_written() is a constexpr-compatible query
    [[maybe_unused]] std::size_t bw = client.bytes_written();
}

}  // namespace
