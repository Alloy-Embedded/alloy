// Compile test: FM25V10 seed driver instantiates against the documented public
// SPI HAL surface. Exercises init(), read(), and write() so that any drift in
// the bus handle's `transfer()` signature fails the build.
//
// MockSpiBus zero-fills rx on every call. This means:
//   - RDID byte[7] (manufacturer) returns 0x00 → init() returns
//     CommunicationError. The Result type contract is still exercised.
//   - read() and write() invoke the bus transfer surface correctly.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/memory/fm25v10/fm25v10.hpp"

namespace {

struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_fm25v10_against_public_spi_handle() {
    MockSpiBus bus;

    // Default CsPolicy (NoOpCsPolicy).
    alloy::drivers::memory::fm25v10::Device<MockSpiBus> dev{bus};

    // init() — returns Err(CommunicationError) because manufacturer byte == 0x00.
    auto init_result = dev.init();
    (void)init_result.is_ok();

    // read() — exercises two-transfer path; returns Ok even with mock data.
    std::array<std::uint8_t, 16> read_buf{};
    auto read_result = dev.read(0x00000u, read_buf);
    (void)read_result.is_ok();

    // write() — exercises WREN + header + chunked data path.
    const std::array<std::uint8_t, 16> write_buf{
        0xAB, 0xCD, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    };
    auto write_result = dev.write(0x00000u, write_buf);
    (void)write_result.is_ok();

    // Boundary: address at end of address space.
    std::array<std::uint8_t, 1> one_byte{};
    auto boundary_result = dev.read(0x1FFFFu, one_byte);
    (void)boundary_result.is_ok();
}

}  // namespace
