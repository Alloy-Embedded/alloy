// Host-level unit tests for OtaClient<Net, Flash>.
//
// Test vectors: NIST SHA-256 standard test vectors used where SHA-256
// verification must pass (test fixture data is chosen to match known digests).
//
// Ref: openspec/changes/add-bootloader-integration/tasks.md §4.4

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include "drivers/ota/ota_client.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

using alloy::core::ErrorCode;
using alloy::core::Ok;
using alloy::core::Err;
using alloy::core::Result;
using alloy::drivers::ota::OtaClient;
using alloy::drivers::ota::OtaConfig;
using alloy::drivers::ota::Sha256Digest;
using alloy::drivers::ota::kMcubootMagic;

// ── Mock infrastructure ───────────────────────────────────────────────────────

/// TCP stream that returns a fixed byte sequence then EOF.
struct FixedTcpStream {
    std::vector<std::byte> payload;
    std::size_t pos = 0u;
    bool closed     = false;

    [[nodiscard]] auto connect(std::string_view, std::uint16_t)
        -> Result<void, ErrorCode>
    {
        pos = 0u;
        return Ok();
    }

    [[nodiscard]] auto read(std::span<std::byte> buf)
        -> Result<std::size_t, ErrorCode>
    {
        if (pos >= payload.size()) return Ok(std::size_t{0});  // EOF
        const std::size_t n = std::min(buf.size(), payload.size() - pos);
        std::copy_n(payload.begin() + static_cast<std::ptrdiff_t>(pos), n, buf.begin());
        pos += n;
        return Ok(static_cast<std::size_t>(n));
    }

    void close() noexcept { closed = true; }
};

/// In-memory flash backend — records erases/writes in a linear byte buffer.
struct RecordingFlash {
    static constexpr std::size_t kBlockSize  = 256u;
    static constexpr std::size_t kBlockCount = 64u;  // 16 KB total

    std::vector<std::byte> mem = std::vector<std::byte>(kBlockSize * kBlockCount, std::byte{0xFF});

    std::size_t erase_calls = 0u;
    std::size_t write_calls = 0u;

    [[nodiscard]] auto erase(std::size_t block, std::size_t count)
        -> Result<void, ErrorCode>
    {
        ++erase_calls;
        const std::size_t start = block * kBlockSize;
        const std::size_t len   = count * kBlockSize;
        if (start + len > mem.size()) return Err(ErrorCode::InvalidParameter);
        std::fill(mem.begin() + static_cast<std::ptrdiff_t>(start),
                  mem.begin() + static_cast<std::ptrdiff_t>(start + len),
                  std::byte{0xFF});
        return Ok();
    }

    [[nodiscard]] auto write(std::size_t block, std::span<const std::byte> data)
        -> Result<void, ErrorCode>
    {
        ++write_calls;
        const std::size_t start = block * kBlockSize;
        if (start + data.size() > mem.size()) return Err(ErrorCode::InvalidParameter);
        std::copy(data.begin(), data.end(),
                  mem.begin() + static_cast<std::ptrdiff_t>(start));
        return Ok();
    }

    [[nodiscard]] auto read(std::size_t block, std::span<std::byte> buf)
        -> Result<void, ErrorCode>
    {
        const std::size_t start = block * kBlockSize;
        if (start + buf.size() > mem.size()) return Err(ErrorCode::InvalidParameter);
        std::copy_n(mem.begin() + static_cast<std::ptrdiff_t>(start), buf.size(), buf.begin());
        return Ok();
    }

    [[nodiscard]] constexpr std::size_t block_size()  const noexcept { return kBlockSize;  }
    [[nodiscard]] constexpr std::size_t block_count() const noexcept { return kBlockCount; }
};

// ── Test helpers ──────────────────────────────────────────────────────────────

/// Make OtaConfig for a RecordingFlash that starts at block 0.
inline OtaConfig make_config() {
    return OtaConfig{
        .secondary_slot_offset = 0u,
        .slot_size             = RecordingFlash::kBlockSize * RecordingFlash::kBlockCount,
        .header_size           = 0x200u,
        .http_port             = 80u,
    };
}

/// NIST SHA-256 test vector: SHA-256 of the empty byte sequence.
/// Used to validate download() with a zero-byte payload.
inline constexpr Sha256Digest kSha256Empty = {{
    std::byte{0xe3}, std::byte{0xb0}, std::byte{0xc4}, std::byte{0x42},
    std::byte{0x98}, std::byte{0xfc}, std::byte{0x1c}, std::byte{0x14},
    std::byte{0x9a}, std::byte{0xfb}, std::byte{0xf4}, std::byte{0xc8},
    std::byte{0x99}, std::byte{0x6f}, std::byte{0xb9}, std::byte{0x24},
    std::byte{0x27}, std::byte{0xae}, std::byte{0x41}, std::byte{0xe4},
    std::byte{0x64}, std::byte{0x9b}, std::byte{0x93}, std::byte{0x4c},
    std::byte{0xa4}, std::byte{0x95}, std::byte{0x99}, std::byte{0x1b},
    std::byte{0x78}, std::byte{0x52}, std::byte{0xb8}, std::byte{0x55},
}};

/// NIST SHA-256 test vector: SHA-256 of bytes {0x61, 0x62, 0x63} = "abc".
/// Used to validate download() with a 3-byte payload.
inline constexpr Sha256Digest kSha256Abc = {{
    std::byte{0xba}, std::byte{0x78}, std::byte{0x16}, std::byte{0xbf},
    std::byte{0x8f}, std::byte{0x01}, std::byte{0xcf}, std::byte{0xea},
    std::byte{0x41}, std::byte{0x41}, std::byte{0x40}, std::byte{0xde},
    std::byte{0x5d}, std::byte{0xae}, std::byte{0x2e}, std::byte{0xc7},
    std::byte{0x3b}, std::byte{0x00}, std::byte{0x36}, std::byte{0x1b},
    std::byte{0xbe}, std::byte{0xf0}, std::byte{0x46}, std::byte{0x9a},
    std::byte{0x82}, std::byte{0xd8}, std::byte{0xc3}, std::byte{0x6c},
    std::byte{0x68}, std::byte{0xac}, std::byte{0x79}, std::byte{0xeb},
}};

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("OtaClient: download empty payload succeeds when SHA-256 matches",
          "[ota][unit]")
{
    FixedTcpStream  net{};   // empty payload → EOF on first read
    RecordingFlash  flash{};
    auto            cfg = make_config();

    OtaClient<FixedTcpStream, RecordingFlash> client{net, flash, cfg};

    const auto result = client.download("http://example.local/fw.bin", kSha256Empty);
    REQUIRE(result.is_ok());
    REQUIRE(client.bytes_written() == 0u);
    CHECK(net.closed);
}

TEST_CASE("OtaClient: download returns ChecksumError when SHA-256 mismatches",
          "[ota][unit]")
{
    // Payload: 3 bytes "abc"
    FixedTcpStream net{};
    net.payload = {std::byte{0x61}, std::byte{0x62}, std::byte{0x63}};
    RecordingFlash flash{};
    auto cfg = make_config();

    OtaClient<FixedTcpStream, RecordingFlash> client{net, flash, cfg};

    // Pass the wrong SHA-256 (zero digest) → should return ChecksumError
    constexpr Sha256Digest wrong_sha{};
    const auto result = client.download("http://example.local/fw.bin", wrong_sha);
    REQUIRE(result.is_err());
    CHECK(result.err() == ErrorCode::ChecksumError);
}

TEST_CASE("OtaClient: download 3-byte payload ('abc') verifies SHA-256 correctly",
          "[ota][unit]")
{
    FixedTcpStream net{};
    net.payload = {std::byte{0x61}, std::byte{0x62}, std::byte{0x63}};  // "abc"
    RecordingFlash flash{};
    auto cfg = make_config();

    OtaClient<FixedTcpStream, RecordingFlash> client{net, flash, cfg};

    const auto result = client.download("http://example.local/fw.bin", kSha256Abc);
    REQUIRE(result.is_ok());
    REQUIRE(client.bytes_written() == 3u);

    // Verify the data was written to flash at offset 0 (block 0).
    REQUIRE(flash.write_calls >= 1u);
    CHECK(flash.mem[0] == std::byte{0x61});
    CHECK(flash.mem[1] == std::byte{0x62});
    CHECK(flash.mem[2] == std::byte{0x63});
}

TEST_CASE("OtaClient: request_update writes MCUboot magic at trailer offset",
          "[ota][unit]")
{
    FixedTcpStream net{};
    net.payload = {std::byte{0x61}, std::byte{0x62}, std::byte{0x63}};
    RecordingFlash flash{};
    auto cfg = make_config();

    OtaClient<FixedTcpStream, RecordingFlash> client{net, flash, cfg};

    // First download to set bytes_written_ > 0.
    REQUIRE(client.download("http://example.local/fw.bin", kSha256Abc).is_ok());

    // Apply swap request.
    const auto upd = client.request_update();
    REQUIRE(upd.is_ok());

    // MCUboot magic must appear at slot_size - 16 bytes.
    const std::size_t trailer = cfg.slot_size - kMcubootMagic.size();
    for (std::size_t i = 0u; i < kMcubootMagic.size(); ++i) {
        CHECK(flash.mem[trailer + i] == static_cast<std::byte>(kMcubootMagic[i]));
    }
}

TEST_CASE("OtaClient: request_update before download returns NotInitialized",
          "[ota][unit]")
{
    FixedTcpStream net{};
    RecordingFlash flash{};
    auto cfg = make_config();

    OtaClient<FixedTcpStream, RecordingFlash> client{net, flash, cfg};

    const auto result = client.request_update();
    REQUIRE(result.is_err());
    CHECK(result.err() == ErrorCode::NotInitialized);
}

TEST_CASE("OtaClient: cancel_update erases secondary slot",
          "[ota][unit]")
{
    FixedTcpStream net{};
    net.payload = {std::byte{0x61}, std::byte{0x62}, std::byte{0x63}};
    RecordingFlash flash{};
    auto cfg = make_config();

    OtaClient<FixedTcpStream, RecordingFlash> client{net, flash, cfg};

    REQUIRE(client.download("http://example.local/fw.bin", kSha256Abc).is_ok());
    REQUIRE(client.request_update().is_ok());

    // After cancel, bytes_written should be reset.
    const auto cancel = client.cancel_update();
    REQUIRE(cancel.is_ok());
    CHECK(client.bytes_written() == 0u);

    // Secondary slot should be erased (0xFF filled).
    for (std::size_t i = 0u; i < cfg.slot_size; ++i) {
        REQUIRE(flash.mem[i] == std::byte{0xFF});
    }
}
