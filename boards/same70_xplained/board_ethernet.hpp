#pragma once

// board_ethernet.hpp — SAME70 Xplained Ethernet factory helpers.
//
// Provides make_mdio(), make_gmac(), make_phy(), and make_ethernet_interface()
// that wire together Same70Mdio + Same70Gmac + KSZ8081 on the SAME70 Xplained
// Ultra board.
//
// Hardware layout (SAME70 Xplained Ultra, J6 connector):
//   GMAC base:     0x40050000
//   KSZ8081 PHY:   phy_address = 0x00
//   MDC:           PD8 (PERIPH_A)
//   MDIO:          PD9 (PERIPH_A)
//   ETH_TX_CLK:    PD0,  ETH_TX_EN:   PD1
//   ETH_TXD[0:3]:  PD2–PD5
//   ETH_RX_CLK:    PD14, ETH_RX_DV:   PD6
//   ETH_RXD[0:3]:  PD7,PD11–PD13
//   ETH_NRST:      PC10 (active LOW — driven high to release PHY)
//
// Prerequisites (call before make_ethernet_interface):
//   1. board::init() — clocks, MPU, and PLL must be up.
//   2. Call enable_ethernet_clocks() to gate GMAC in PMC.
//   3. Call mux_ethernet_pins() to connect GMAC signals to the pads.
//   4. Call release_phy_reset() to bring KSZ8081 out of hardware reset.
//
// Usage:
//   #include BOARD_ETHERNET_HEADER  // or include directly
//   board::enable_ethernet_clocks();
//   board::mux_ethernet_pins();
//   board::release_phy_reset();
//   auto mac_bytes = board::mac_from_eui48();  // or hard-code
//   auto mdio = board::make_mdio();
//   mdio.enable_management();
//   auto gmac = board::make_gmac(mac_bytes);
//   auto phy  = board::make_phy(mdio);
//   phy.init();  // PHY soft-reset + auto-neg
//   gmac.init(); // MAC DMA setup
//   auto eth  = board::make_ethernet_interface(gmac, phy);

#include <array>
#include <cstdint>
#include <span>

#include "drivers/net/ksz8081/ksz8081.hpp"
#include "drivers/net/ethernet_interface.hpp"
#include "src/hal/ethernet/same70_gmac.hpp"
#include "src/hal/mdio/same70_mdio.hpp"

namespace board {

// ---- Type aliases for this board's Ethernet stack -------------------------

using BoardMdio  = alloy::hal::mdio::Same70Mdio;
using BoardGmac  = alloy::hal::ethernet::Same70Gmac<4u, 8u>;
using BoardPhy   = alloy::drivers::net::ksz8081::Device<BoardMdio>;
using BoardEth   = alloy::net::EthernetInterface<BoardGmac, BoardPhy>;

// ---- MMIO helpers ----------------------------------------------------------

namespace _eth_detail {
inline volatile std::uint32_t* reg32(std::uint32_t addr) noexcept {
    return reinterpret_cast<volatile std::uint32_t*>(addr);
}
}

// ---- Clock gating ----------------------------------------------------------

// Enable GMAC peripheral clock (PMC_PCER1 bit 5 = ID 37 for ATSAME70Q21).
inline void enable_ethernet_clocks() noexcept {
    constexpr std::uint32_t kPmc    = 0x400E0600u;
    constexpr std::uint32_t kPcer1  = 0x100u;
    constexpr std::uint32_t kGmacId = 1u << 5u;   // peripheral ID 37 → PCER1 bit 5
    *_eth_detail::reg32(kPmc + kPcer1) = kGmacId;
}

// ---- Pin mux ---------------------------------------------------------------

// Configure GMAC pads on PIOD (PERIPH_A selection).
// PD0–PD9, PD11–PD14 are Ethernet signals.
inline void mux_ethernet_pins() noexcept {
    constexpr std::uint32_t kPiodBase = 0x400E1400u;
    constexpr std::uint32_t kMasked   =
        (1u <<  0) | (1u <<  1) | (1u <<  2) | (1u <<  3) |   // TX_CLK,TX_EN,TXD0,TXD1
        (1u <<  4) | (1u <<  5) | (1u <<  6) | (1u <<  7) |   // TXD2,TXD3,RX_DV,RXD0
        (1u <<  8) | (1u <<  9) |                              // MDC, MDIO
        (1u << 11) | (1u << 12) | (1u << 13) | (1u << 14);    // RXD1,RXD2,RXD3,RX_CLK

    // Disable PIO control (hand to peripheral A).
    *_eth_detail::reg32(kPiodBase + 0x04u) = kMasked;  // PDR
    // Select peripheral A (ABCDSR1 = 0, ABCDSR2 = 0 for periph A).
    *_eth_detail::reg32(kPiodBase + 0x70u) &= ~kMasked; // ABCDSR1
    *_eth_detail::reg32(kPiodBase + 0x74u) &= ~kMasked; // ABCDSR2
}

// ---- PHY reset -------------------------------------------------------------

// Release KSZ8081 hardware reset on PC10 (active LOW, drive HIGH to operate).
inline void release_phy_reset() noexcept {
    constexpr std::uint32_t kPiocBase  = 0x400E1200u;
    constexpr std::uint32_t kPc10Mask  = 1u << 10u;
    *_eth_detail::reg32(kPiocBase + 0x10u) = kPc10Mask;  // SODR: set output high
    *_eth_detail::reg32(kPiocBase + 0x04u) = kPc10Mask;  // PDR: disable PIO, let OE drive
    *_eth_detail::reg32(kPiocBase + 0x00u) = kPc10Mask;  // PER: re-enable PIO (GPIO high)
    *_eth_detail::reg32(kPiocBase + 0x10u) = kPc10Mask;  // SODR: output data register high
    *_eth_detail::reg32(kPiocBase + 0x14u) = kPc10Mask;  // PUDR: disable pull-up
    // Direction: output
    *_eth_detail::reg32(kPiocBase + 0x10u) = kPc10Mask;  // SODR (drive high = released)
}

// ---- MAC address -----------------------------------------------------------

// Returns a locally-administered unicast MAC address derived from the SoC
// unique ID (read from GPBR or AT24MAC402 if available).
// Fallback: fixed address — replace in production with AT24MAC402 read.
inline std::array<std::uint8_t, 6u> mac_from_eui48() noexcept {
    return {0x02u, 0xAEu, 0x70u, 0x00u, 0x00u, 0x01u};
}

// ---- Factory helpers -------------------------------------------------------

[[nodiscard]] inline BoardMdio make_mdio() noexcept {
    return BoardMdio{/* mdc_divider_bits = MCK/64 */ 0b100u};
}

[[nodiscard]] inline BoardGmac make_gmac(
    std::span<const std::uint8_t, 6u> mac_address) noexcept {
    return BoardGmac{mac_address};
}

[[nodiscard]] inline BoardPhy make_phy(BoardMdio& mdio) noexcept {
    return BoardPhy{mdio, {.phy_address = 0x00u}};
}

[[nodiscard]] inline BoardEth make_ethernet_interface(
    BoardGmac& gmac, BoardPhy& phy) noexcept {
    return BoardEth{gmac, phy};
}

}  // namespace board
