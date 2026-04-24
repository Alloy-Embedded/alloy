// Compile test: W25Q seed driver instantiates against the documented public
// SPI HAL surface. Exercises JEDEC ID, chunked read, page program, sector
// erase, and status poll so that any drift in the bus handle's `transfer()`
// signature fails the build.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/memory/w25q/w25q.hpp"

namespace {

struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> tx,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        // Make the JEDEC-ID probe succeed (non-all-0x00, non-all-0xFF) and the
        // status poll see WIP=0 quickly, so `init()` + `wait_while_busy()` both
        // return Ok during the compile test.
        for (auto& b : rx) b = 0;
        if (!tx.empty() && tx[0] == 0x9F && rx.size() >= 4) {
            rx[1] = 0xEF;  // Winbond
            rx[2] = 0x40;
            rx[3] = 0x18;
        }
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_w25q_against_public_spi_handle() {
    MockSpiBus bus;
    alloy::drivers::memory::w25q::Device flash{bus,
                                              {.status_poll_max_iterations = 4u}};

    (void)flash.init();
    auto id = flash.read_jedec_id();
    (void)id.is_ok();

    std::array<std::uint8_t, 512> read_buffer{};
    (void)flash.read(0x000000u, read_buffer);

    std::array<std::uint8_t, 32> payload{};
    (void)flash.page_program(0x000000u, payload);

    (void)flash.sector_erase(0x000000u);
    (void)flash.wait_while_busy();
}

}  // namespace
