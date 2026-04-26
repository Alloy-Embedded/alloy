// Compile test: UC8151 seed driver instantiates against the documented public
// SPI HAL surface. Exercises init(), update(), and sleep() so that any drift
// in the bus handle's transfer() signature or the driver's own public API
// fails the build instead of silently regressing.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/display/uc8151/uc8151.hpp"

namespace {

// ── Mock bus ──────────────────────────────────────────────────────────────────

struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t>       rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

// ── Mock GPIO pins ─────────────────────────────────────────────────────────────

struct MockDcPin {
    alloy::core::Result<void, alloy::core::ErrorCode> set_high() { return alloy::core::Ok(); }
    alloy::core::Result<void, alloy::core::ErrorCode> set_low()  { return alloy::core::Ok(); }
};

struct MockCsPin {
    alloy::core::Result<void, alloy::core::ErrorCode> set_high() { return alloy::core::Ok(); }
    alloy::core::Result<void, alloy::core::ErrorCode> set_low()  { return alloy::core::Ok(); }
};

struct MockBusyPin {
    // Always returns false (not busy) so wait_busy() returns immediately.
    [[nodiscard]] bool is_high() const noexcept { return false; }
};

// ── Compile exercises ─────────────────────────────────────────────────────────

[[maybe_unused]] void compile_uc8151_against_public_spi_handle() {
    using namespace alloy::drivers::display::uc8151;

    MockSpiBus  bus;
    MockDcPin   dc_pin;
    MockCsPin   cs_pin;
    MockBusyPin busy_pin;

    // Explicit GPIO policy objects — most representative of real usage.
    Device<MockSpiBus,
           GpioDcPolicy<MockDcPin>,
           GpioCsPolicy<MockCsPin>,
           GpioBusyPolicy<MockBusyPin>>
        epd{bus,
            GpioDcPolicy<MockDcPin>{dc_pin},
            GpioCsPolicy<MockCsPin>{cs_pin},
            GpioBusyPolicy<MockBusyPin>{busy_pin}};

    // init()
    (void)epd.init();

    // update() — framebuffer for the 212×104 default panel.
    constexpr std::size_t kFbBytes = (212u * 104u + 7u) / 8u;
    std::array<std::uint8_t, kFbBytes> fb{};
    fb.fill(0x00u);  // all white
    (void)epd.update(fb);

    // sleep()
    (void)epd.sleep();
}

// Verify default no-op policies also compile.
[[maybe_unused]] void compile_uc8151_noop_policies() {
    MockSpiBus bus;
    alloy::drivers::display::uc8151::Device<MockSpiBus> epd{bus};
    (void)epd.init();

    constexpr std::size_t kFbBytes = (212u * 104u + 7u) / 8u;
    std::array<std::uint8_t, kFbBytes> fb{};
    (void)epd.update(fb);
    (void)epd.sleep();
}

// Verify 2.9" panel variant (296×128) compiles.
[[maybe_unused]] void compile_uc8151_29inch() {
    MockSpiBus bus;
    alloy::drivers::display::uc8151::Device<MockSpiBus,
                                            alloy::drivers::display::uc8151::NoOpDcPolicy,
                                            alloy::drivers::display::uc8151::NoOpCsPolicy,
                                            alloy::drivers::display::uc8151::NoOpBusyPolicy,
                                            296u, 128u>
        epd{bus};

    constexpr std::size_t kFbBytes = (296u * 128u + 7u) / 8u;
    std::array<std::uint8_t, kFbBytes> fb{};
    (void)epd.init();
    (void)epd.update(fb);
    (void)epd.sleep();
}

}  // namespace
