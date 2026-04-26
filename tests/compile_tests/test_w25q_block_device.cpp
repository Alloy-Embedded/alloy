// Compile test: W25qBlockDevice adapter satisfies BlockDevice and correctly
// wraps the W25Q driver's transfer() signature.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/memory/w25q/w25q_block_device.hpp"
#include "hal/filesystem/block_device.hpp"

namespace {

struct MockSpi {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> tx,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        // Simulate JEDEC-ID response for init().
        for (auto& b : rx) b = 0;
        if (!tx.empty() && tx[0] == 0x9F && rx.size() >= 4) {
            rx[1] = 0xEF; rx[2] = 0x40; rx[3] = 0x18;
        }
        // Simulate WIP=0 (not busy) for status poll.
        if (!tx.empty() && tx[0] == 0x05 && rx.size() >= 2) {
            rx[1] = 0x00;
        }
        return alloy::core::Ok();
    }
};

using W25q = alloy::drivers::memory::w25q::BlockDevice<MockSpi, /*BlockCount=*/16>;

static_assert(alloy::hal::filesystem::BlockDevice<W25q>,
              "W25qBlockDevice must satisfy BlockDevice concept");

[[maybe_unused]] void compile_w25q_block_device_api() {
    MockSpi spi;
    W25q dev{spi};

    (void)dev.init();

    // block_size() and block_count() are constexpr.
    static_assert(W25q::kBlockSize  == 4096);
    static_assert(W25q::kBlockCount == 16);
    static_assert(dev.block_size()  == 4096);
    static_assert(dev.block_count() == 16);

    std::array<std::byte, 4096> buf{};
    (void)dev.read(0, buf);
    (void)dev.write(0, std::span<const std::byte>{buf});
    (void)dev.erase(0, 1);
    (void)dev.jedec_id();
}

}  // namespace
