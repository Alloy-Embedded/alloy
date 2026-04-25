// examples/driver_ksz8081_probe/main.cpp
//
// SAME70 Xplained Ultra — KSZ8081RNA Ethernet PHY driver probe.
//
// Target: the Microchip KSZ8081RNACA (U5) soldered on the SAME70 Xplained
// Ultra board, wired to the SAME70 GMAC peripheral in RMII mode with MDC/MDIO
// on PD8/PD9. The PHY address strap on this carrier is 0, RMII is selected,
// and the reference clock is fed by GTXCK.
//
// Since alloy does not yet ship an in-tree MDIO HAL, this example embeds a
// small `Same70GmacMdio` adapter that drives the SAME70 GMAC's management
// interface directly (GMAC_NCR / GMAC_NCFGR / GMAC_NSR / GMAC_MAN). That
// adapter satisfies the MdioBus contract that the PHY driver is templated on:
//
//   auto read(uint8_t phy, uint8_t reg) const
//       -> Result<uint16_t, ErrorCode>;
//   auto write(uint8_t phy, uint8_t reg, uint16_t value) const
//       -> Result<void, ErrorCode>;
//
// What the probe does, in order:
//   1. Initialises the board debug UART (115200-8-N-1).
//   2. Enables PMC peripheral clocks for PIOD (PID 16) and GMAC (PID 39) and
//      switches PD8/PD9 to peripheral A so GMAC owns the MDIO pins.
//   3. Enables the GMAC management port (NCR.MPE) and programs the MDC clock
//      divider from MCK so MDC stays inside the KSZ8081's 2.5 MHz envelope.
//   4. Reads PHY ID1 (expected 0x0022) and PHY ID2 (expected 0x156x — low
//      nibble is silicon revision). The driver's `init()` performs the same
//      check internally; this probe prints the raw values first so a bad
//      wiring or strap still produces a useful UART line.
//   5. Calls `Device::init()` which soft-resets the PHY, programs the
//      auto-negotiation advertisement, and kicks off auto-negotiation.
//   6. Polls `read_link_status()` for up to ~5 s. If the cable is plugged
//      into a working partner the link comes up and we report speed +
//      duplex; if not we still report auto-neg state so the user knows the
//      MDIO path works even with no cable.
//
// Expected UART output with a cable connected to a 100BASE-TX full-duplex
// partner:
//
//   ksz8081 probe: ready
//   ksz8081: PHY_ID1 = 0022
//   ksz8081: PHY_ID2 = 1561
//   ksz8081: init ok
//   ksz8081: link UP, 100 Mbps, full duplex
//   ksz8081: PROBE PASS
//
// Expected UART output with no cable plugged in (still a PASS — MDIO works):
//
//   ksz8081 probe: ready
//   ksz8081: PHY_ID1 = 0022
//   ksz8081: PHY_ID2 = 1561
//   ksz8081: init ok
//   ksz8081: link DOWN (no partner) — MDIO path OK
//   ksz8081: PROBE PASS
//
// On failure the line preceding FAIL tells you which step broke. LED blinks
// slowly (500 ms) on pass, rapidly (100 ms) on any failure.

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "driver_ksz8081_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "drivers/net/ksz8081/ksz8081.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

using namespace alloy::examples::uart_console;

// ---- MMIO helpers ----------------------------------------------------------

inline volatile std::uint32_t& reg32(std::uintptr_t addr) {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}

// PIOC — host for the KSZ8081 nRESET strap (PC10 on SAME70 Xplained Ultra).
constexpr std::uintptr_t kPiocBase =
    alloy::device::base<alloy::device::PeripheralId::GPIOC>();
// PIOD — peripheral multiplex for PD0 (GTXCK), PD8 (GMDC), PD9 (GMDIO).
constexpr std::uintptr_t kPiodBase =
    alloy::device::base<alloy::device::PeripheralId::GPIOD>();
constexpr std::uintptr_t kPioPdr = kPiodBase + 0x04;     // peripheral disable PIO
constexpr std::uintptr_t kPioAbcdsr1 = kPiodBase + 0x70;
constexpr std::uintptr_t kPioAbcdsr2 = kPiodBase + 0x74;

// PIO register offsets reused across ports.
inline std::uintptr_t pio_per(std::uintptr_t base)  { return base + 0x00; }
inline std::uintptr_t pio_oer(std::uintptr_t base)  { return base + 0x10; }
inline std::uintptr_t pio_sodr(std::uintptr_t base) { return base + 0x30; }
inline std::uintptr_t pio_codr(std::uintptr_t base) { return base + 0x34; }

// GMAC — management interface. Base address comes from the generated device
// contract so a probe author can't type the wrong literal. (An earlier hand-
// coded version of this probe had 0x40034000u, which is XDMAC, not GMAC.)
constexpr std::uintptr_t kGmacBase =
    alloy::device::base<alloy::device::PeripheralId::GMAC>();
constexpr std::uintptr_t kGmacNcr = kGmacBase + 0x00;
constexpr std::uintptr_t kGmacNcfgr = kGmacBase + 0x04;
constexpr std::uintptr_t kGmacNsr = kGmacBase + 0x08;
constexpr std::uintptr_t kGmacMan = kGmacBase + 0x34;

constexpr std::uint32_t kNcrMpe = 1u << 4;  // GMAC_NCR.MPE (per SAM E70 datasheet)
constexpr std::uint32_t kNsrIdle = 1u << 2;
constexpr std::uint32_t kNcfgrClkShift = 18;
constexpr std::uint32_t kNcfgrClkMask = 0x7u << kNcfgrClkShift;

// Pick the NCFGR.CLK divider so MDC <= 2.5 MHz for the current MCK.
// SAME70 GMAC divides MCK by: 8 / 16 / 32 / 48 / 64 / 96 (CLK = 0..5).
[[nodiscard]] constexpr auto ncfgr_clk_for_mck(std::uint32_t mck_hz) -> std::uint32_t {
    constexpr std::uint32_t kMdcMaxHz = 2'500'000u;
    if (mck_hz <= 20'000'000u) return 0u;    // MCK/8
    if (mck_hz <= 40'000'000u) return 1u;    // MCK/16
    if (mck_hz <= 80'000'000u) return 2u;    // MCK/32
    if (mck_hz <= 120'000'000u) return 3u;   // MCK/48
    if (mck_hz <= 160'000'000u) return 4u;   // MCK/64  (150 MHz → 2.34 MHz)
    if (mck_hz <= 240'000'000u) return 5u;   // MCK/96
    // Fall back to the slowest available divider; still saturates at MCK/96.
    (void)kMdcMaxHz;
    return 5u;
}

// ---- SAME70 GMAC MDIO adapter (implements MdioBus contract) ---------------

struct Same70GmacMdio {
    using ResultU16 = alloy::core::Result<std::uint16_t, alloy::core::ErrorCode>;
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    static constexpr std::uint32_t kPollMax = 200'000u;

    [[nodiscard]] static auto wait_idle() -> bool {
        for (std::uint32_t i = 0; i < kPollMax; ++i) {
            if ((reg32(kGmacNsr) & kNsrIdle) != 0) return true;
        }
        return false;
    }

    [[nodiscard]] auto read(std::uint8_t phy, std::uint8_t reg) const -> ResultU16 {
        if (!wait_idle()) {
            return alloy::core::Err(alloy::core::ErrorCode::Timeout);
        }
        // SOF=01 (bit 30), OP=10=read (bits 29:28), PHYA (27:23), REGA (22:18),
        // CODE=10 (bits 17:16), DATA=0.
        const std::uint32_t man = (1u << 30) | (2u << 28) |
                                  ((phy & 0x1Fu) << 23) | ((reg & 0x1Fu) << 18) |
                                  (2u << 16);
        reg32(kGmacMan) = man;
        if (!wait_idle()) {
            return alloy::core::Err(alloy::core::ErrorCode::Timeout);
        }
        return alloy::core::Ok(static_cast<std::uint16_t>(reg32(kGmacMan) & 0xFFFFu));
    }

    [[nodiscard]] auto write(std::uint8_t phy, std::uint8_t reg, std::uint16_t value) const
        -> ResultVoid {
        if (!wait_idle()) {
            return alloy::core::Err(alloy::core::ErrorCode::Timeout);
        }
        // OP=01=write (bits 29:28), DATA = value (15:0).
        const std::uint32_t man = (1u << 30) | (1u << 28) |
                                  ((phy & 0x1Fu) << 23) | ((reg & 0x1Fu) << 18) |
                                  (2u << 16) | value;
        reg32(kGmacMan) = man;
        if (!wait_idle()) {
            return alloy::core::Err(alloy::core::ErrorCode::Timeout);
        }
        return alloy::core::Ok();
    }
};

// ---- Bring-up helpers ------------------------------------------------------

void enable_pmc_clocks() {
    alloy::clock::enable<alloy::device::PeripheralId::GMAC>();
    alloy::clock::enable<alloy::device::PeripheralId::GPIOC>();
    alloy::clock::enable<alloy::device::PeripheralId::GPIOD>();
}

void mux_mdio_pins() {
    using alloy::device::PeripheralId;
    using alloy::device::PinId;
    using alloy::device::SignalId;
    alloy::pinmux::route<PinId::PD0, PeripheralId::GMAC, SignalId::signal_gtxck>();
    alloy::pinmux::route<PinId::PD8, PeripheralId::GMAC, SignalId::signal_gmdc>();
    alloy::pinmux::route<PinId::PD9, PeripheralId::GMAC, SignalId::signal_gmdio>();
}

// SAME70 Xplained Ultra ties the KSZ8081 nRESET to PC10. Pulse low → high so
// the PHY leaves reset before we try MDIO. Datasheet requires >=10 us low and
// >=100 us high before management access; we use 10 ms each for margin.
void release_phy_reset() {
    constexpr std::uint32_t kPc10 = 1u << 10;
    reg32(pio_per(kPiocBase))  = kPc10;   // PIO control (not peripheral)
    reg32(pio_oer(kPiocBase))  = kPc10;   // output enable
    reg32(pio_codr(kPiocBase)) = kPc10;   // drive low
    for (volatile std::uint32_t i = 0; i < 200'000u; ++i) { __asm__ volatile("nop"); }
    reg32(pio_sodr(kPiocBase)) = kPc10;   // drive high
    for (volatile std::uint32_t i = 0; i < 2'000'000u; ++i) { __asm__ volatile("nop"); }
}

void gmac_enable_management() {
    const std::uint32_t clk = ncfgr_clk_for_mck(
        board::same70_xplained::ClockConfig::pclk_freq_hz);
    std::uint32_t ncfgr = reg32(kGmacNcfgr);
    ncfgr = (ncfgr & ~kNcfgrClkMask) | (clk << kNcfgrClkShift);
    reg32(kGmacNcfgr) = ncfgr;
    reg32(kGmacNcr) = reg32(kGmacNcr) | kNcrMpe;
}

template <typename Uart>
void print_hex16(const Uart& uart, std::uint16_t v) {
    constexpr auto kHex = "0123456789ABCDEF";
    const char buf[5] = {kHex[(v >> 12) & 0xF], kHex[(v >> 8) & 0xF],
                         kHex[(v >> 4) & 0xF], kHex[v & 0xF], '\0'};
    write_text(uart, std::string_view{buf, 4});
}

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

[[noreturn]] void blink_ok() {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        blink_error(100);
    }
    write_line(uart, "ksz8081 probe: ready");

    enable_pmc_clocks();
    mux_mdio_pins();
    release_phy_reset();
    gmac_enable_management();

    Same70GmacMdio mdio{};

    // Pre-read PHY_ID so a bad wiring shows up on UART before Device::init()
    // short-circuits the run.
    auto id1 = mdio.read(0, alloy::drivers::net::ksz8081::reg::kPhyId1);
    if (id1.is_err()) {
        write_line(uart, "ksz8081: FAIL (MDIO read PHY_ID1)");
        blink_error(100);
    }
    write_text(uart, "ksz8081: PHY_ID1 = ");
    print_hex16(uart, id1.unwrap());
    write_text(uart, "\r\n");

    auto id2 = mdio.read(0, alloy::drivers::net::ksz8081::reg::kPhyId2);
    if (id2.is_err()) {
        write_line(uart, "ksz8081: FAIL (MDIO read PHY_ID2)");
        blink_error(100);
    }
    write_text(uart, "ksz8081: PHY_ID2 = ");
    print_hex16(uart, id2.unwrap());
    write_text(uart, "\r\n");

    alloy::drivers::net::ksz8081::Device phy{mdio, {.phy_address = 0}};

    if (phy.init().is_err()) {
        write_line(uart, "ksz8081: FAIL (init — PHY ID mismatch or MDIO timeout)");
        blink_error(100);
    }
    write_line(uart, "ksz8081: init ok");

    // Poll up to ~5 s for auto-neg to complete. No-cable is still a PASS: we
    // validated the MDIO path by reading PHY_ID and completing init().
    alloy::drivers::net::ksz8081::LinkStatus status{};
    bool link_came_up = false;
    for (std::uint32_t i = 0; i < 50; ++i) {
        auto s = phy.read_link_status();
        if (s.is_err()) {
            write_line(uart, "ksz8081: FAIL (read_link_status)");
            blink_error(100);
        }
        status = s.unwrap();
        if (status.up && status.auto_negotiation_complete) {
            link_came_up = true;
            break;
        }
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(100);
    }

    if (link_came_up) {
        write_text(uart, "ksz8081: link UP, ");
        if (status.speed_mbps == 100) {
            write_text(uart, "100 Mbps, ");
        } else if (status.speed_mbps == 10) {
            write_text(uart, "10 Mbps, ");
        } else {
            write_text(uart, "?? Mbps, ");
        }
        write_line(uart, status.full_duplex ? "full duplex" : "half duplex");
    } else {
        write_line(uart, "ksz8081: link DOWN (no partner) — MDIO path OK");
    }

    write_line(uart, "ksz8081: PROBE PASS");
    blink_ok();
}
