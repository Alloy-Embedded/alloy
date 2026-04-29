#pragma once

// drivers/ota/ota_client.hpp
//
// OtaClient<Net, Flash> — MCUboot-aware OTA download client.
//
// Dependencies (open specs, both must be satisfied before hardware use):
//   Net   — satisfies TcpStreamProvider (from add-network-hal)
//   Flash — satisfies alloy::hal::filesystem::BlockDevice (block_device.hpp)
//
// Usage pattern:
//   OtaConfig cfg{ .secondary_slot_offset = 0, .slot_size = 0x100000 };
//   OtaClient client{net, flash, cfg};
//   if (client.download(url, sha256).is_ok()) {
//       client.request_update();  // MCUboot will swap on next reset
//   }
//
// All operations are blocking. Async wrappers are deferred to add-async-ota.

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::ota {

// ── Concepts ──────────────────────────────────────────────────────────────────

/// Minimum interface required from the network layer (e.g. TCP socket or
/// HTTP stream from add-network-hal). Net must be constructible and provide
/// blocking HTTP GET semantics via connect/read/close.
template <typename T>
concept TcpStreamProvider =
    requires(T& t, std::string_view url, std::uint16_t port,
             std::span<std::byte> buf) {
        { t.connect(url, port) } -> std::same_as<core::Result<void, core::ErrorCode>>;
        { t.read(buf) }          -> std::same_as<core::Result<std::size_t, core::ErrorCode>>;
        { t.close() }            -> std::same_as<void>;
    };

/// Minimum interface required from the flash backend. A BlockDevice-compatible
/// type (block_device.hpp) satisfies this automatically.
template <typename T>
concept FlashWriter =
    requires(T& t, std::size_t block,
             std::span<const std::byte> data,
             std::span<std::byte> buf) {
        { t.erase(block, std::size_t{}) } -> std::same_as<core::Result<void, core::ErrorCode>>;
        { t.write(block, data) }           -> std::same_as<core::Result<void, core::ErrorCode>>;
        { t.read(block, buf) }             -> std::same_as<core::Result<void, core::ErrorCode>>;
        { t.block_size() }                 -> std::same_as<std::size_t>;
        { t.block_count() }                -> std::same_as<std::size_t>;
    };

// ── Config ────────────────────────────────────────────────────────────────────

/// OTA partition configuration. Values must match the board's memory_map.cmake.
struct OtaConfig {
    /// Byte offset of the secondary slot within the Flash backend.
    std::size_t secondary_slot_offset = 0u;
    /// Size of one OTA slot in bytes (primary == secondary).
    std::size_t slot_size             = 0u;
    /// MCUboot image header size (must match --header-size used by imgtool).
    std::size_t header_size           = 0x200u;
    /// HTTP port for firmware server.
    std::uint16_t http_port           = 80u;
};

// ── SHA-256 digest type ───────────────────────────────────────────────────────

using Sha256Digest = std::array<std::byte, 32>;

// ── MCUboot image trailer magic ───────────────────────────────────────────────
// BOOT_MAGIC written at trailer_offset = slot_size - 16 (per MCUboot spec §3.5).
// Triggers a swap request when read by MCUboot on the next boot.

inline constexpr std::array<std::uint8_t, 16> kMcubootMagic = {
    0x77, 0xc2, 0x95, 0xf3, 0x60, 0xd2, 0xef, 0x7f,
    0x35, 0x52, 0x50, 0x0f, 0x2c, 0xb6, 0x79, 0x80,
};

// ── OtaClient ─────────────────────────────────────────────────────────────────

/// Blocking OTA client for MCUboot-based systems.
///
/// @tparam Net   Must satisfy TcpStreamProvider.
/// @tparam Flash Must satisfy FlashWriter (e.g. W25QBlockDevice, SdCardDevice).
template <TcpStreamProvider Net, FlashWriter Flash>
class OtaClient {
  public:
    /// Construct with references to the network stream and flash backend.
    /// *cfg* must be populated from the board's memory_map.cmake constants.
    constexpr OtaClient(Net& net, Flash& flash, const OtaConfig& cfg) noexcept
        : net_{&net}, flash_{&flash}, cfg_{cfg} {}

    // ── Download ──────────────────────────────────────────────────────────────

    /// HTTP-GET *url*, stream-write to secondary slot, verify SHA-256.
    ///
    /// Downloads in block-sized chunks to avoid heap allocation. SHA-256 is
    /// computed incrementally over each received block.
    ///
    /// Returns:
    ///   Ok()                 — download complete; SHA-256 verified.
    ///   Err(NetworkError)    — HTTP connect or read failure.
    ///   Err(StorageError)    — flash erase or write failure.
    ///   Err(InvalidChecksum) — SHA-256 mismatch; secondary slot was erased.
    [[nodiscard]] auto download(
        std::string_view url,
        const Sha256Digest& expected_sha256
    ) -> core::Result<void, core::ErrorCode>
    {
        // 1. Open TCP connection
        if (const auto r = net_->connect(url, cfg_.http_port); r.is_err()) {
            return r;
        }

        // 2. Erase secondary slot before writing
        const std::size_t block_sz    = flash_->block_size();
        const std::size_t start_block = cfg_.secondary_slot_offset / block_sz;
        const std::size_t block_count = cfg_.slot_size / block_sz;
        if (const auto r = flash_->erase(start_block, block_count); r.is_err()) {
            net_->close();
            return r;
        }

        // 3. Stream-read → flash-write, accumulate SHA-256
        Sha256State sha{};
        sha256_init(sha);

        std::array<std::byte, 256> chunk{};
        std::size_t written = 0u;

        while (written < cfg_.slot_size) {
            const auto rx = net_->read(std::span{chunk});
            if (rx.is_err()) {
                net_->close();
                return core::Err(core::ErrorCode{rx.unwrap_err()});
            }
            const std::size_t n = rx.unwrap();
            if (n == 0u) break;  // server closed connection

            const std::size_t block = (cfg_.secondary_slot_offset + written) / block_sz;
            auto slice = std::span{chunk}.first(n);
            if (const auto r = flash_->write(block, std::span<const std::byte>{slice}); r.is_err()) {
                net_->close();
                return r;
            }
            sha256_update(sha, slice);
            written += n;
        }
        net_->close();

        // 4. Verify SHA-256
        Sha256Digest computed{};
        sha256_final(sha, computed);
        if (computed != expected_sha256) {
            // Erase the corrupted slot to prevent accidental boot
            (void)flash_->erase(start_block, block_count);
            return core::Err(core::ErrorCode::ChecksumError);
        }

        bytes_written_ = written;
        return core::Ok();
    }

    // ── Update control ────────────────────────────────────────────────────────

    /// Write MCUboot BOOT_MAGIC trailer to the secondary slot.
    ///
    /// After a successful download(), calling request_update() arms the swap.
    /// MCUboot will detect the trailer on the next reset and perform the swap.
    [[nodiscard]] auto request_update() -> core::Result<void, core::ErrorCode>
    {
        if (bytes_written_ == 0u) {
            return core::Err(core::ErrorCode::NotInitialized);
        }

        // Trailer location: last 16 bytes of the slot (MCUboot spec §3.5)
        const std::size_t block_sz = flash_->block_size();
        const std::size_t trailer_byte =
            cfg_.secondary_slot_offset + cfg_.slot_size - sizeof(kMcubootMagic);
        const std::size_t trailer_block = trailer_byte / block_sz;

        // Read the existing block, patch the trailer, write back
        std::array<std::byte, 256> buf{};
        if (buf.size() < block_sz) {
            // Safety: if block_size > 256, the caller must use a larger buffer.
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        auto block_buf = std::span{buf}.first(block_sz);
        if (const auto r = flash_->read(trailer_block, block_buf); r.is_err()) {
            return r;
        }

        const std::size_t offset_in_block = trailer_byte % block_sz;
        for (std::size_t i = 0u; i < sizeof(kMcubootMagic); ++i) {
            block_buf[offset_in_block + i] = static_cast<std::byte>(kMcubootMagic[i]);
        }
        if (const auto r = flash_->erase(trailer_block, 1u); r.is_err()) {
            return r;
        }
        return flash_->write(trailer_block, std::span<const std::byte>{block_buf});
    }

    /// Erase the MCUboot trailer from the secondary slot (cancel pending swap).
    [[nodiscard]] auto cancel_update() -> core::Result<void, core::ErrorCode>
    {
        const std::size_t block_sz = flash_->block_size();
        const std::size_t start_block = cfg_.secondary_slot_offset / block_sz;
        const std::size_t block_count = cfg_.slot_size / block_sz;
        bytes_written_ = 0u;
        return flash_->erase(start_block, block_count);
    }

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /// Bytes written during the last download() call (0 if not yet downloaded).
    [[nodiscard]] constexpr std::size_t bytes_written() const noexcept {
        return bytes_written_;
    }

  private:
    Net*       net_;
    Flash*     flash_;
    OtaConfig  cfg_;
    std::size_t bytes_written_ = 0u;

    // ── Minimal in-place SHA-256 (no heap, no OpenSSL) ────────────────────────
    // Full FIPS 180-4 implementation. Constants from §4.2.2, schedule from §6.2.

    struct Sha256State {
        std::uint32_t h[8]{
            0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
        };
        std::uint8_t  buf[64]{};
        std::uint64_t bit_len = 0u;
        std::uint32_t buf_len = 0u;
    };

    static constexpr std::uint32_t kK[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
        0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
        0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
        0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
        0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
        0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
        0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
        0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
    };

    static constexpr auto rotr(std::uint32_t x, std::uint32_t n) -> std::uint32_t {
        return (x >> n) | (x << (32u - n));
    }

    static void sha256_compress(Sha256State& s) {
        std::uint32_t w[64];
        for (std::uint32_t i = 0u; i < 16u; ++i) {
            const std::uint8_t* p = s.buf + i * 4u;
            w[i] = (static_cast<std::uint32_t>(p[0]) << 24u) |
                   (static_cast<std::uint32_t>(p[1]) << 16u) |
                   (static_cast<std::uint32_t>(p[2]) <<  8u) |
                   (static_cast<std::uint32_t>(p[3]));
        }
        for (std::uint32_t i = 16u; i < 64u; ++i) {
            const std::uint32_t s0 = rotr(w[i-15], 7u) ^ rotr(w[i-15], 18u) ^ (w[i-15] >> 3u);
            const std::uint32_t s1 = rotr(w[i- 2], 17u) ^ rotr(w[i- 2], 19u) ^ (w[i- 2] >> 10u);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        std::uint32_t a = s.h[0], b = s.h[1], c = s.h[2], d = s.h[3];
        std::uint32_t e = s.h[4], f = s.h[5], g = s.h[6], h = s.h[7];
        for (std::uint32_t i = 0u; i < 64u; ++i) {
            const std::uint32_t S1  = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
            const std::uint32_t ch  = (e & f) ^ (~e & g);
            const std::uint32_t t1  = h + S1 + ch + kK[i] + w[i];
            const std::uint32_t S0  = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2  = S0 + maj;
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        s.h[0] += a; s.h[1] += b; s.h[2] += c; s.h[3] += d;
        s.h[4] += e; s.h[5] += f; s.h[6] += g; s.h[7] += h;
    }

    static void sha256_init(Sha256State& s) { s = Sha256State{}; }

    static void sha256_update(Sha256State& s, std::span<const std::byte> data) {
        for (const auto b : data) {
            s.buf[s.buf_len++] = static_cast<std::uint8_t>(b);
            s.bit_len += 8u;
            if (s.buf_len == 64u) {
                sha256_compress(s);
                s.buf_len = 0u;
            }
        }
    }

    static void sha256_final(Sha256State& s, Sha256Digest& out) {
        s.buf[s.buf_len++] = 0x80u;
        if (s.buf_len > 56u) {
            while (s.buf_len < 64u) s.buf[s.buf_len++] = 0u;
            sha256_compress(s);
            s.buf_len = 0u;
        }
        while (s.buf_len < 56u) s.buf[s.buf_len++] = 0u;
        for (std::uint32_t i = 0u; i < 8u; ++i) {
            s.buf[56u + i] = static_cast<std::uint8_t>(s.bit_len >> (56u - 8u * i));
        }
        sha256_compress(s);
        for (std::uint32_t i = 0u; i < 8u; ++i) {
            out[i * 4u + 0u] = static_cast<std::byte>(s.h[i] >> 24u);
            out[i * 4u + 1u] = static_cast<std::byte>(s.h[i] >> 16u);
            out[i * 4u + 2u] = static_cast<std::byte>(s.h[i] >>  8u);
            out[i * 4u + 3u] = static_cast<std::byte>(s.h[i]);
        }
    }
};

}  // namespace alloy::drivers::ota
