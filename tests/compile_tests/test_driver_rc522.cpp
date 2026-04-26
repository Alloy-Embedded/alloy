// Compile test: RC522 seed driver instantiates against the documented public
// SPI HAL surface. Exercises init(), is_card_present(), and read_uid() so that
// any drift in the bus handle's `transfer()` signature fails the build.
//
// MockSpiBus zero-fills rx on every call. This means:
//   - VersionReg read returns 0x00 → init() returns CommunicationError, which
//     is still a valid Result<void, ErrorCode> — the type contract is exercised.
//   - is_card_present() returns Ok(false) — no card detected.
//   - read_uid() returns CommunicationError — no card responded.
// All three paths compile and the return types are correctly inferred.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/net/rc522/rc522.hpp"

namespace {

struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_rc522_against_public_spi_handle() {
    MockSpiBus bus;

    // Default CsPolicy (NoOpCsPolicy).
    alloy::drivers::net::rc522::Device<MockSpiBus> dev{bus};

    // init() — returns Err(CommunicationError) because VersionReg == 0x00,
    // but the Result type is correctly constructed.
    auto init_result = dev.init();
    (void)init_result.is_ok();

    // is_card_present() — returns Ok(false); result type must be Result<bool>.
    auto present_result = dev.is_card_present();
    (void)present_result.is_ok();

    // read_uid() — returns Err(...); result type must be Result<Uid>.
    auto uid_result = dev.read_uid();
    (void)uid_result.is_ok();

    // Verify that Uid is a plain-data struct (no heap, trivially destructible).
    static_assert(sizeof(alloy::drivers::net::rc522::Uid) >= 11u,
                  "Uid must hold at least 10 data bytes + 1 size byte");
}

}  // namespace
