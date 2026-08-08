// NetDevice driver for the SAM E70 GMAC (Cadence GEM, 10/100).
//
// Satisfies BOTH the Mdio concept (clause-22 PHY access via MAN) and the
// NetDevice concept (RX/TX of whole Ethernet frames). Descriptor rings and
// frame buffers are static, 8-byte aligned, in .bss — the GMAC DMA reads
// them directly and, since alloy leaves the M7 caches off, plain SRAM is
// coherent (no clean/invalidate needed in this bring-up).
//
// Descriptor format (DS60001527 §38): 8-byte {word0, word1}.
//   RX word0: [0]=OWNERSHIP (1 = SW owns / filled), [1]=WRAP, [31:2]=buf addr.
//      word1: [12:0] = received length.
//   TX word0: full buffer address.  word1: [31]=USED, [30]=WRAP, [15]=LAST,
//      [13:0] = length.
//
// Enable order (§38): NCFGR (speed/duplex/MDC-div), UR (RMII), DCFGR (buffer
// size), SA1B then SA1T (MAC — SA1T write arms the filter), RBQB/TBQB (ring
// bases), then NCR.RXEN|TXEN. MPE set early for MDIO. Not silicon-validated
// (Ethernet cable pending) — the MDIO ID read is the cable-free checkpoint.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/ip/microchip/gmac_v1.hpp"

namespace alloy::hal {

template <class Inst, unsigned RxN = 8, unsigned TxN = 4>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::gmac_v1>
class gmac {
public:
    using IP = typename Inst::ip;
    static constexpr std::uint32_t kBuf = 1536;  // one frame per buffer

    struct alignas(8) desc {
        std::uint32_t word0;
        std::uint32_t word1;
    };

    static IP::regs& r() { return *reinterpret_cast<typename IP::regs*>(Inst::base); }

    // ---- Mdio (clause-22) ----
    static std::uint16_t mdio_read(std::uint8_t phy, std::uint8_t reg) {
        r().MAN = kManClttoWtn | (0b10u << IP::op.pos) |
                  (static_cast<std::uint32_t>(phy) << IP::phya.pos) |
                  (static_cast<std::uint32_t>(reg) << IP::rega.pos);
        while (IP::idle.read(r()) == 0u) {
        }
        return static_cast<std::uint16_t>(r().MAN);
    }
    static void mdio_write(std::uint8_t phy, std::uint8_t reg, std::uint16_t val) {
        r().MAN = kManClttoWtn | (0b01u << IP::op.pos) |
                  (static_cast<std::uint32_t>(phy) << IP::phya.pos) |
                  (static_cast<std::uint32_t>(reg) << IP::rega.pos) | val;
        while (IP::idle.read(r()) == 0u) {
        }
    }
    // NetDevice/driver-facing wrappers (satisfy the Mdio concept surface).
    std::uint16_t read(std::uint8_t phy, std::uint8_t reg) const { return mdio_read(phy, reg); }
    void write(std::uint8_t phy, std::uint8_t reg, std::uint16_t val) const {
        mdio_write(phy, reg, val);
    }

    // ---- bring-up ----
    // Enable clocks + MDIO so the PHY can be probed. RMII, MDC = MCK/64.
    void begin_mdio() {
        using ncfgr = typename IP::ncfgr;
        alloy::gate_on(Inst::gate);
        r().NCR = 0;
        r().NCFGR = flags{ncfgr::clk_mck_64};  // MDC divider /64 (<=2.5 MHz)
        // Select RMII. UR.RMII polarity is INVERTED vs the field name on the
        // SAME70: 0 = RMII, 1 = MII. Writing 1 puts the MAC in MII mode on this
        // RMII-wired board — MDIO/refclk still work but the RMII data path is
        // dead both ways (silent: no RX, TX never leaves the MAC).
        r().UR = 0;
        IP::mpe.set(r());                      // management port for MDIO
    }

    // Configure the DMA rings + MAC address and go live. Call after the PHY
    // has negotiated; pass its result so NCFGR speed/duplex match.
    void start(const std::uint8_t mac[6], bool speed_100, bool full_duplex) {
        for (unsigned i = 0; i < RxN; ++i) {
            rx_desc_[i].word0 = reinterpret_cast<std::uint32_t>(&rx_buf_[i][0]) &
                                ~0x3u;  // OWNERSHIP=0 (MAC owns)
            rx_desc_[i].word1 = 0;
        }
        rx_desc_[RxN - 1].word0 |= 0x2u;  // WRAP on the last RX descriptor
        for (unsigned i = 0; i < TxN; ++i) {
            tx_desc_[i].word0 = reinterpret_cast<std::uint32_t>(&tx_buf_[i][0]);
            tx_desc_[i].word1 = kTxUsed;  // USED=1 (SW owns, ring idle)
        }
        tx_desc_[TxN - 1].word1 |= kTxWrap;
        rx_next_ = tx_next_ = 0;

        r().SA1B = static_cast<std::uint32_t>(mac[0]) | (mac[1] << 8) |
                   (mac[2] << 16) | (mac[3] << 24);
        r().SA1T = static_cast<std::uint32_t>(mac[4]) | (mac[5] << 8);  // arms filter
        r().RBQB = reinterpret_cast<std::uint32_t>(&rx_desc_[0]);
        r().TBQB = reinterpret_cast<std::uint32_t>(&tx_desc_[0]);
        // Park priority queues 1..5 on dummy descriptors (TX marked USED so the
        // DMA finds no work; RX a 1-entry WRAP ring into a throwaway buffer), so
        // the GMAC never fetches from a null base pointer and faults. Base regs:
        // TBQBAPQ = 0x0440..0x0450, RBQBAPQ = 0x0480..0x0490 (uncurated).
        dummy_tx_.word0 = reinterpret_cast<std::uint32_t>(&dummy_buf_[0]);
        dummy_tx_.word1 = kTxUsed | kTxWrap;
        dummy_rx_.word0 = (reinterpret_cast<std::uint32_t>(&dummy_buf_[0]) & ~0x3u) | 0x2u;
        dummy_rx_.word1 = 0;
        for (std::uint32_t q = 0; q < 5; ++q) {
            auto reg = [&](std::uint32_t off) -> volatile std::uint32_t& {
                return *reinterpret_cast<volatile std::uint32_t*>(Inst::base + off);
            };
            reg(0x440u + q * 4u) = reinterpret_cast<std::uint32_t>(&dummy_tx_);
            reg(0x480u + q * 4u) = reinterpret_cast<std::uint32_t>(&dummy_rx_);
        }
        // 1536 B RX bufs (DRBS=24*64) + INCR4 AHB bursts. FBLDO must be a valid
        // burst length: 0 (0b00000) is RESERVED and stalls the RX/TX DMA — the
        // receiver then never fills a descriptor even with the link up.
        r().DCFGR = (24u << IP::drbs.pos) | (0b11u << IP::rxbms.pos) |
                    (0b00100u << IP::fbldo.pos);
        IP::spd.write(r(), speed_100 ? 1u : 0u);
        IP::fd.write(r(), full_duplex ? 1u : 0u);
        // The rings + buffers must be in SRAM before the DMA masters go live.
        __asm__ volatile("dsb 0xF" ::: "memory");
        IP::rxen.set(r());
        IP::txen.set(r());
    }

    // ---- NetDevice ----
    [[nodiscard]] std::uint32_t receive(std::span<std::uint8_t> out) {
        using rsr = typename IP::rsr;
        desc& d = rx_desc_[rx_next_];
        if ((d.word0 & 0x1u) == 0u) {  // MAC still owns: nothing received
            return 0;
        }
        // The reported length includes the 4-byte Ethernet FCS; strip it so
        // callers get just the frame (echoing the FCS back as data corrupts the
        // reply length + upper-layer checksums).
        const std::uint32_t raw = d.word1 & 0x1FFFu;
        const std::uint32_t len = raw > 4u ? raw - 4u : 0u;
        const std::uint32_t n = len < out.size() ? len : static_cast<std::uint32_t>(out.size());
        const auto* buf = reinterpret_cast<const std::uint8_t*>(d.word0 & ~0x3u);
        for (std::uint32_t i = 0; i < n; ++i) {
            out[i] = buf[i];
        }
        d.word0 &= ~0x1u;  // hand the descriptor back to the MAC
        rx_next_ = (rx_next_ + 1) % RxN;
        r().RSR = flags{rsr::rec};  // clear the receive-status flag
        return n;
    }

    [[nodiscard]] bool transmit(std::span<const std::uint8_t> frame) {
        if (frame.size() > kBuf) {
            return false;
        }
        desc& d = tx_desc_[tx_next_];
        if ((d.word1 & kTxUsed) == 0u) {  // MAC still owns this slot
            return false;
        }
        auto* buf = reinterpret_cast<std::uint8_t*>(d.word0);
        for (std::size_t i = 0; i < frame.size(); ++i) {
            buf[i] = frame[i];
        }
        const std::uint32_t wrap = (tx_next_ == TxN - 1) ? kTxWrap : 0u;
        // Clear USED, set LAST + length + wrap: hands the slot to the MAC.
        d.word1 = kTxLast | wrap | static_cast<std::uint32_t>(frame.size());
        tx_next_ = (tx_next_ + 1) % TxN;
        // Drain the M7 store buffer so the frame + descriptor are in SRAM before
        // the DMA master reads them (else the read can fault / see stale data).
        __asm__ volatile("dsb 0xF" ::: "memory");
        IP::starttx.set(r());  // kick the TX DMA
        return true;
    }

    [[nodiscard]] bool link_up() const { return link_; }
    void set_link(bool up) { link_ = up; }
    [[nodiscard]] std::uint32_t mtu() const { return 1500; }

private:
    static constexpr std::uint32_t kTxUsed = std::uint32_t{1} << 31;
    static constexpr std::uint32_t kTxWrap = std::uint32_t{1} << 30;
    static constexpr std::uint32_t kTxLast = std::uint32_t{1} << 15;
    // MAN CLTTO(30)=1 + WTN(17:16)=0b10 (clause-22 write-then-numeric).
    static constexpr std::uint32_t kManClttoWtn = (std::uint32_t{1} << 30) | (0b10u << 16);

    inline static desc rx_desc_[RxN]{};
    inline static desc tx_desc_[TxN]{};
    // 32-byte aligned: the DMA reads/writes frame data in fixed AHB bursts
    // (FBLDO=INCR4), and an unaligned burst that straddles a 1 KB AHB boundary
    // faults (TSR.TFC/HRESP). Alignment also keeps the RX descriptor's low-2-bit
    // OWNERSHIP/WRAP flags out of the buffer address. kBuf is a multiple of 32,
    // so every per-frame slice stays aligned too.
    alignas(32) inline static std::uint8_t rx_buf_[RxN][kBuf]{};
    alignas(32) inline static std::uint8_t tx_buf_[TxN][kBuf]{};
    // The GMAC has 5 priority queues beyond queue 0. We use only queue 0, but
    // their base pointers reset to 0 and the DMA faults (HRESP) fetching a
    // descriptor from the null pointer — park them all on these dummies.
    inline static desc dummy_tx_{};
    inline static desc dummy_rx_{};
    alignas(32) inline static std::uint8_t dummy_buf_[kBuf]{};
    unsigned rx_next_ = 0;
    unsigned tx_next_ = 0;
    bool link_ = false;
};

}  // namespace alloy::hal
