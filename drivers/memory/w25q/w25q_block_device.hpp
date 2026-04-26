#pragma once

// W25Q BlockDevice adapter
//
// Adapts `alloy::drivers::memory::w25q::Device<Spi>` to the
// `alloy::hal::filesystem::BlockDevice` concept. One erase block = one W25Q
// 4 KiB sector. One write block = one W25Q page (256 B), but the adapter
// accepts full-sector writes and splits them into pages internally.
//
// Template parameters:
//   Spi        — SPI bus handle (same type as passed to w25q::Device).
//   BlockCount — number of 4 KiB sectors to expose. Must not exceed the
//                physical capacity. Caller is responsible for sizing this
//                correctly for the flash part (e.g. W25Q128 = 4096 sectors).
//   CsPolicy   — chip-select policy (default: NoOpCsPolicy).
//                Use GpioCsPolicy<Pin> for software GPIO CS.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/memory/w25q/w25q.hpp"
#include "hal/filesystem/block_device.hpp"

namespace alloy::drivers::memory::w25q {

template <typename Spi, std::size_t BlockCount, typename CsPolicy = NoOpCsPolicy>
class BlockDevice {
public:
    static constexpr std::size_t kBlockSize  = kSectorSizeBytes;  // 4096
    static constexpr std::size_t kBlockCount = BlockCount;

    explicit BlockDevice(Spi& spi, CsPolicy cs = {}, Config cfg = {})
        : device_{spi, cs, cfg} {}

    [[nodiscard]] auto init() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return device_.init();
    }

    // ── BlockDevice concept methods ──────────────────────────────────────────

    [[nodiscard]] auto read(std::size_t block, std::span<std::byte> buf)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (block >= kBlockCount || buf.size() != kBlockSize) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        const auto addr = static_cast<std::uint32_t>(block * kBlockSize);
        return device_.read(addr, to_u8_span(buf));
    }

    // Accepts a full 4 KiB sector. Splits into 256 B page programs internally.
    [[nodiscard]] auto write(std::size_t block, std::span<const std::byte> data)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (block >= kBlockCount || data.size() != kBlockSize) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        const auto base_addr = static_cast<std::uint32_t>(block * kBlockSize);
        constexpr std::size_t kPagesPerSector = kBlockSize / kPageSizeBytes;
        for (std::size_t p = 0; p < kPagesPerSector; ++p) {
            const auto page_addr = base_addr + static_cast<std::uint32_t>(p * kPageSizeBytes);
            auto page_data = data.subspan(p * kPageSizeBytes, kPageSizeBytes);
            if (auto r = device_.page_program(page_addr, to_const_u8_span(page_data));
                r.is_err()) {
                return r;
            }
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto erase(std::size_t block, std::size_t count)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (block + count > kBlockCount) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        for (std::size_t i = 0; i < count; ++i) {
            const auto addr = static_cast<std::uint32_t>((block + i) * kBlockSize);
            if (auto r = device_.sector_erase(addr); r.is_err()) {
                return r;
            }
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] static constexpr auto block_size() -> std::size_t { return kBlockSize; }
    [[nodiscard]] static constexpr auto block_count() -> std::size_t { return kBlockCount; }

    [[nodiscard]] auto jedec_id() const -> JedecId { return device_.jedec_id(); }

private:
    static auto to_u8_span(std::span<std::byte> s) -> std::span<std::uint8_t> {
        return {reinterpret_cast<std::uint8_t*>(s.data()), s.size()};
    }
    static auto to_const_u8_span(std::span<const std::byte> s)
        -> std::span<const std::uint8_t> {
        return {reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
    }

    Device<Spi, CsPolicy> device_;
};

}  // namespace alloy::drivers::memory::w25q

// Concept gate — fails at include time if the adapter no longer satisfies BlockDevice.
namespace {
struct _MockSpiForW25qGate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};
static_assert(
    alloy::hal::filesystem::BlockDevice<
        alloy::drivers::memory::w25q::BlockDevice<_MockSpiForW25qGate, 1>>,
    "W25qBlockDevice must satisfy alloy::hal::filesystem::BlockDevice");
}  // namespace
