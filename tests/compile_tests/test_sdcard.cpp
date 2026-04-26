// Compile test: SdCard driver satisfies BlockDevice concept and exposes
// the expected init / read / write / erase / block_size / block_count API.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/memory/sdcard/sdcard.hpp"
#include "hal/filesystem/block_device.hpp"

namespace {

struct MockSpi {
    mutable std::size_t rx_byte_index{0};

    [[nodiscard]] auto transmit(std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto receive(std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        // Fill with 0xFF (idle line) — most SD commands will time-out in
        // a real scenario, but for a compile test we just verify the API.
        for (auto& b : rx) b = 0xFF;
        return alloy::core::Ok();
    }

    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0xFF;
        return alloy::core::Ok();
    }
};

using Sd = alloy::drivers::memory::sdcard::SdCard<MockSpi>;

static_assert(alloy::hal::filesystem::BlockDevice<Sd>,
              "SdCard must satisfy BlockDevice concept");

[[maybe_unused]] void compile_sdcard_api() {
    MockSpi spi;
    Sd sd{spi};

    // init() may return Timeout with the mock — we just verify it compiles.
    (void)sd.init();
    sd.set_block_count(8192);

    static_assert(Sd::block_size() == 512);
    // block_count() is runtime after set_block_count().
    (void)sd.block_count();

    std::array<std::byte, 512> buf{};
    (void)sd.read(0, buf);
    (void)sd.write(0, std::span<const std::byte>{buf});
    (void)sd.erase(0, 1);
}

}  // namespace
