// Compile test: SX1276 seed driver instantiates against the documented SPI bus
// surface and all four public methods (init, transmit, receive, rssi) remain
// callable. The MockSpiBus zero-fills rx, so init() will fail the RegVersion
// check at runtime; the purpose of this file is compile-time type-safety, not
// functional correctness.
//
// No main() — included via CMake add_library(OBJECT) or similar compile-only target.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/net/sx1276/sx1276.hpp"

namespace {

/// Minimal SPI mock that satisfies the bus surface contract.
/// transfer() zero-fills the rx buffer so reads return 0x00 for all registers.
struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> /*tx*/,
                                std::span<std::uint8_t>       rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        for (auto& b : rx) { b = 0u; }
        return alloy::core::Ok();
    }
};

/// Populate rx so that RegVersion (0x42) reads back 0x12 and the FIFO reads
/// return a plausible length / pointer for receive().
struct MockSpiBusWithVersion {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t> tx,
                                std::span<std::uint8_t>       rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        for (auto& b : rx) { b = 0u; }

        if (!tx.empty() && rx.size() >= 2u) {
            const std::uint8_t reg = static_cast<std::uint8_t>(tx[0] & 0x7Fu);

            if (reg == 0x42u) {
                // RegVersion — respond with 0x12 so init() succeeds.
                rx[1] = 0x12u;
            } else if (reg == 0x12u) {
                // RegIrqFlags — TxDone | RxDone | ValidHeader set so polling
                // busy_wait_irq() terminates immediately in test.
                rx[1] = alloy::drivers::net::sx1276::irq::kTxDone    |
                        alloy::drivers::net::sx1276::irq::kRxDone    |
                        alloy::drivers::net::sx1276::irq::kValidHeader;
            } else if (reg == 0x13u) {
                // RegRxNbBytes — report 4 bytes received.
                rx[1] = 4u;
            } else if (reg == 0x10u) {
                // RegFifoRxCurrentAddr — start at 0x00.
                rx[1] = 0x00u;
            }
        }
        return alloy::core::Ok();
    }
};

/// Verifies that Device<MockSpiBus> (NoOpCsPolicy default) compiles and all
/// four public methods are invocable with the correct signatures.
[[maybe_unused]] void compile_sx1276_default_cs() {
    MockSpiBus bus;

    // Default construction: NoOpCsPolicy, default Config (915 MHz, SF7, BW125).
    alloy::drivers::net::sx1276::Device<MockSpiBus> dev{bus};

    // init() — will fail at runtime (version = 0x00 != 0x12), but must compile.
    (void)dev.init();

    // transmit() — valid 5-byte payload.
    const std::array<std::uint8_t, 5u> hello{0x48u, 0x65u, 0x6Cu, 0x6Cu, 0x6Fu};
    (void)dev.transmit(std::span<const std::uint8_t>{hello});

    // receive() — 64-byte receive buffer.
    std::array<std::uint8_t, 64u> rx_buf{};
    (void)dev.receive(std::span<std::uint8_t>{rx_buf});

    // rssi() — read last-packet RSSI.
    (void)dev.rssi();
}

/// Verifies that Device<MockSpiBusWithVersion> succeeds the full init()
/// sequence and that transmit/receive/rssi can be chained.
[[maybe_unused]] void compile_sx1276_with_version_mock() {
    MockSpiBusWithVersion bus;

    alloy::drivers::net::sx1276::Config cfg{};
    cfg.frequency_hz = 868'000'000u;
    cfg.bandwidth    = alloy::drivers::net::sx1276::Bandwidth::kHz250;
    cfg.sf           = alloy::drivers::net::sx1276::SpreadingFactor::SF9;
    cfg.cr           = alloy::drivers::net::sx1276::CodingRate::CR4_6;
    cfg.tx_power_dbm = 14u;
    cfg.crc_enable   = false;

    alloy::drivers::net::sx1276::Device<MockSpiBusWithVersion> dev{bus, {}, cfg};

    auto ir = dev.init();
    (void)ir.is_ok();

    const std::array<std::uint8_t, 3u> payload{0x01u, 0x02u, 0x03u};
    auto tr = dev.transmit(std::span<const std::uint8_t>{payload});
    (void)tr.is_ok();

    std::array<std::uint8_t, 32u> rx_buf{};
    auto rr = dev.receive(std::span<std::uint8_t>{rx_buf});
    (void)rr.is_ok();

    auto rssi = dev.rssi();
    (void)rssi.is_ok();
}

/// Verifies high-power (>17 dBm) configuration path compiles.
[[maybe_unused]] void compile_sx1276_high_power() {
    MockSpiBus bus;
    alloy::drivers::net::sx1276::Config cfg{};
    cfg.tx_power_dbm = 20u;  // triggers RegPaDac = 0x87

    alloy::drivers::net::sx1276::Device<MockSpiBus> dev{bus, {}, cfg};
    (void)dev.init();
}

/// Verifies SF11 + 125 kHz path that sets LowDataRateOptimize.
[[maybe_unused]] void compile_sx1276_low_datarate_opt() {
    MockSpiBus bus;
    alloy::drivers::net::sx1276::Config cfg{};
    cfg.sf        = alloy::drivers::net::sx1276::SpreadingFactor::SF11;
    cfg.bandwidth = alloy::drivers::net::sx1276::Bandwidth::kHz125;

    alloy::drivers::net::sx1276::Device<MockSpiBus> dev{bus, {}, cfg};
    (void)dev.init();
}

}  // namespace
