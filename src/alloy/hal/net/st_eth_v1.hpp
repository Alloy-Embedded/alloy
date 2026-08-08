// NetDevice driver for the STM32 Ethernet MAC (Synopsys DWC_ether_mac, 10/100).
//
// Satisfies BOTH the Mdio concept (clause-22 PHY access via MACMIIAR/MACMIIDR)
// and the NetDevice concept (RX/TX of whole Ethernet frames). Descriptor rings
// and frame buffers are static, in .bss — the ETH DMA reads them directly.
//
// Descriptor format (RM0410, normal 4-word / EDFE=0), CHAINED (RCH/TCH=1, one
// buffer per descriptor, word 3 = next-descriptor pointer):
//   DES0: [31]=OWN (1 = DMA owns, 0 = CPU owns — OPPOSITE sense from the GEM).
//     RX status: [29:16]=FL frame length (incl. FCS), [9]=FS, [8]=LS.
//     TX control: [29]=LS, [28]=FS, [20]=TCH.
//   RDES1: [14]=RCH (chained), [12:0]=RBS1 buffer-1 size.
//   TDES1: [12:0]=TBS1 bytes to transmit.
//   DES2 = buffer-1 address; DES3 = next-descriptor address (chained).
//
// Bring-up order (RM0410 §Ethernet): SYSCFG.MII_RMII_SEL BEFORE the ETH MAC
// clocks (it latches at MAC clock-on); ETHMACEN + ETHMACTXEN + ETHMACRXEN;
// DMABMR.SWR soft reset (poll to 0); MACMIIAR.CR for the HCLK; MDIO PHY probe;
// MACA0L/H; MACCR.FES/DM; rings; DMABMR burst; DMARDLAR/DMATDLAR; DMAOMR
// store-and-forward + FTF flush; MACCR.TE|RE; DMAOMR.ST|SR. Not silicon-
// validated (Ethernet cable pending) — the MDIO ID read is the cable-free
// checkpoint, exactly as for the SAME70 gmac.
//
// RISK / silicon-check items (compile-verified, HW-pending):
//  * MDC divider MACMIIAR.CR = 0b100 (HCLK/102) assumes HCLK in 150-216 MHz
//    (Nucleo-F767ZI pll_180mhz). A slower profile needs a different CR (RM0410
//    table): 60-100->0b000, 100-150->0b001, 20-35->0b010, 35-60->0b011.
//  * SYSCFG_PMC / RCC_APB2ENR absolute addresses below are F7-family (RM0410).
//    F4/H7 differ (H7 has no SYSCFG_PMC; RMII is selected in SYSCFG_PMCR).
//  * D-cache: this driver assumes the rings/buffers are coherent (caches off,
//    like the M7 gmac note). With D-cache ON, add clean (TX) / invalidate (RX)
//    or place the rings in a non-cacheable MPU region.
//  * The ETH DMA is an AHB master and CANNOT reach the CPU's tightly-coupled
//    DTCM SRAM. The static rings/buffers must link into a DMA-reachable AHB-SRAM
//    bank (SRAM1); if the linker places .bss in DTCM the DMA silently does nothing.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/ip/st/eth_v1.hpp"

namespace alloy::hal {

template <class Inst, unsigned RxN = 8, unsigned TxN = 4>
    requires std::same_as<typename Inst::ip, alloy::ip::st::eth_v1>
class eth {
public:
    using IP = typename Inst::ip;
    static constexpr std::uint32_t kBuf = 1536;  // one frame per buffer

    // Normal 4-word descriptor (16 B). STM32's own struct — NOT the GEM pair.
    struct alignas(4) desc {
        std::uint32_t des0;
        std::uint32_t des1;
        std::uint32_t des2;
        std::uint32_t des3;
    };

    static IP::regs& r() { return *reinterpret_cast<typename IP::regs*>(Inst::base); }

    // ---- Mdio (clause-22) ----
    static std::uint16_t mdio_read(std::uint8_t phy, std::uint8_t reg) {
        r().MACMIIAR = (static_cast<std::uint32_t>(phy) << IP::pa.pos) |
                       (static_cast<std::uint32_t>(reg) << IP::mr.pos) |
                       (kMdcCr << IP::cr.pos) |
                       (std::uint32_t{1} << IP::mb.pos);  // MW=0 (read), MB=1 (start)
        while (IP::mb.read(r()) != 0u) {
        }
        return static_cast<std::uint16_t>(r().MACMIIDR & 0xFFFFu);
    }
    static void mdio_write(std::uint8_t phy, std::uint8_t reg, std::uint16_t val) {
        r().MACMIIDR = val;
        r().MACMIIAR = (static_cast<std::uint32_t>(phy) << IP::pa.pos) |
                       (static_cast<std::uint32_t>(reg) << IP::mr.pos) |
                       (kMdcCr << IP::cr.pos) |
                       (std::uint32_t{1} << IP::mw.pos) | (std::uint32_t{1} << IP::mb.pos);  // MW=1 (write), MB=1
        while (IP::mb.read(r()) != 0u) {
        }
    }
    // NetDevice/driver-facing wrappers (satisfy the Mdio concept surface).
    std::uint16_t read(std::uint8_t phy, std::uint8_t reg) const { return mdio_read(phy, reg); }
    void write(std::uint8_t phy, std::uint8_t reg, std::uint16_t val) const {
        mdio_write(phy, reg, val);
    }

    // ---- bring-up ----
    // Enable clocks + interface + MDIO so the PHY can be probed.
    void begin_mdio() {
        // RMII is already selected by board::eth_configure_pins() (an STM32 fact in
        // SYSCFG, OUTSIDE this MAC block — its address is chip data). That must run
        // before the MAC clock latches, which it does: the examples call
        // eth_configure_pins() before begin_mdio(). So this HAL touches no register
        // outside its own IP + the data-provided gate.
        //
        // (1) ETH MAC clocks: block enable from data (AHB1ENR.ETHMACEN); TX/RX
        // enable are an IP fact on the SAME AHB1ENR (Inst::gate.reg — a data
        // address), so no RCC base is hardcoded. Without TX/RX EN the RMII clocks
        // stay gated and traffic stalls silently.
        alloy::gate_on(Inst::gate);  // AHB1ENR.ETHMACEN (bit 25)
        auto& ahb1enr = *reinterpret_cast<rw32*>(Inst::gate.reg);
        ahb1enr = ahb1enr | kEthMacTxEn | kEthMacRxEn;  // bits 26/27

        // (2) DMA soft reset: resets MAC + DMA. Self-clearing — poll until 0.
        IP::swr.set(r());
        while (IP::swr.read(r()) != 0u) {
        }
        // (3) With the MAC clocked, MDIO is live (MACMIIAR.CR set per access).
    }

    // Configure the DMA rings + MAC address and go live. Call after the PHY has
    // negotiated; pass its result so MACCR speed/duplex match.
    void start(const std::uint8_t mac[6], bool speed_100, bool full_duplex) {
        // Chained RX/TX rings: one buffer per descriptor, word 3 -> next desc.
        for (unsigned i = 0; i < RxN; ++i) {
            rx_desc_[i].des0 = kOwn;                        // DMA owns (armed to fill)
            rx_desc_[i].des1 = kRxRch | (kBuf & 0x1FFFu);   // RCH + RBS1 buffer size
            rx_desc_[i].des2 = reinterpret_cast<std::uint32_t>(&rx_buf_[i][0]);
            rx_desc_[i].des3 = reinterpret_cast<std::uint32_t>(&rx_desc_[(i + 1) % RxN]);
        }
        for (unsigned i = 0; i < TxN; ++i) {
            tx_desc_[i].des0 = kTxTch;                      // chained, CPU owns (idle)
            tx_desc_[i].des1 = 0;
            tx_desc_[i].des2 = reinterpret_cast<std::uint32_t>(&tx_buf_[i][0]);
            tx_desc_[i].des3 = reinterpret_cast<std::uint32_t>(&tx_desc_[(i + 1) % TxN]);
        }
        rx_next_ = tx_next_ = 0;

        // Station MAC address: low word first, then high (RM0410).
        r().MACA0LR = static_cast<std::uint32_t>(mac[0]) | (mac[1] << 8) |
                      (mac[2] << 16) | (mac[3] << 24);
        r().MACA0HR = static_cast<std::uint32_t>(mac[4]) | (mac[5] << 8);

        // Perfect filter on MACA0, no flow control.
        r().MACFFR = 0;
        r().MACFCR = 0;

        // Speed/duplex from the PHY link result (FCS strip stays in software,
        // so leave APCS/CSTF off — the RX descriptor length includes the CRC).
        IP::fes.write(r(), speed_100 ? 1u : 0u);
        IP::dm.write(r(), full_duplex ? 1u : 0u);

        // DMA bus mode: 32-beat burst, address-aligned, fixed burst; EDFE stays
        // 0 (normal 4-word descriptors). Program before the descriptor bases.
        r().DMABMR = (0b100000u << IP::pbl.pos) | (std::uint32_t{1} << IP::fb.pos) | (std::uint32_t{1} << IP::aab.pos);

        // Descriptor list bases (DMA is stopped here).
        r().DMARDLAR = reinterpret_cast<std::uint32_t>(&rx_desc_[0]);
        r().DMATDLAR = reinterpret_cast<std::uint32_t>(&tx_desc_[0]);

        // Store-and-forward both ways; flush the TX FIFO once (self-clearing).
        using dmaomr = typename IP::dmaomr;
        r().DMAOMR = dmaomr::rsf | dmaomr::tsf;
        IP::ftf.set(r());
        while (IP::ftf.read(r()) != 0u) {
        }

        // Rings + buffers must be visible to the DMA masters before they go live.
        __asm__ volatile("dsb 0xF" ::: "memory");

        // Enable MAC TX+RX, then start the TX and RX DMA.
        IP::te.set(r());
        IP::re.set(r());
        IP::st.set(r());
        IP::sr.set(r());
    }

    // ---- NetDevice ----
    [[nodiscard]] std::uint32_t receive(std::span<std::uint8_t> out) {
        using dmasr = typename IP::dmasr;
        desc& d = rx_desc_[rx_next_];
        if ((d.des0 & kOwn) != 0u) {  // DMA still owns: nothing received
            return 0;
        }
        // RDES0.FL [29:16] is the frame length including the 4-byte FCS — strip
        // it (echoing the FCS back as data corrupts reply length + checksums).
        const std::uint32_t raw = (d.des0 >> 16) & 0x3FFFu;
        const std::uint32_t len = raw > 4u ? raw - 4u : 0u;
        const std::uint32_t n = len < out.size() ? len : static_cast<std::uint32_t>(out.size());
        const auto* buf = reinterpret_cast<const std::uint8_t*>(d.des2);
        for (std::uint32_t i = 0; i < n; ++i) {
            out[i] = buf[i];
        }
        d.des0 = kOwn;  // hand the descriptor back to the DMA (des1/des2/des3 persist)
        rx_next_ = (rx_next_ + 1) % RxN;
        r().DMASR = dmasr::rs | dmasr::nis;  // clear the RX status flags
        __asm__ volatile("dsb 0xF" ::: "memory");
        r().DMARPDR = 1u;  // poll demand: un-suspend the RX DMA if it stalled
        return n;
    }

    [[nodiscard]] bool transmit(std::span<const std::uint8_t> frame) {
        if (frame.size() > kBuf) {
            return false;
        }
        desc& d = tx_desc_[tx_next_];
        if ((d.des0 & kOwn) != 0u) {  // DMA still owns this slot
            return false;
        }
        auto* buf = reinterpret_cast<std::uint8_t*>(d.des2);
        for (std::size_t i = 0; i < frame.size(); ++i) {
            buf[i] = frame[i];
        }
        d.des1 = static_cast<std::uint32_t>(frame.size()) & 0x1FFFu;  // TBS1
        // OWN + chained + first+last segment: hands the slot to the DMA.
        d.des0 = kOwn | kTxTch | kTxFs | kTxLs;
        tx_next_ = (tx_next_ + 1) % TxN;
        // Drain the store buffer so the frame + descriptor are in SRAM before
        // the DMA master reads them.
        __asm__ volatile("dsb 0xF" ::: "memory");
        r().DMATPDR = 1u;  // transmit poll demand: kick the TX DMA
        return true;
    }

    [[nodiscard]] bool link_up() const { return link_; }
    void set_link(bool up) { link_ = up; }
    [[nodiscard]] std::uint32_t mtu() const { return 1500; }

private:
    // Descriptor bits (RM0410 normal descriptors).
    static constexpr std::uint32_t kOwn = std::uint32_t{1} << 31;    // DES0.OWN (1 = DMA owns)
    static constexpr std::uint32_t kRxRch = std::uint32_t{1} << 14;  // RDES1.RCH (chained)
    static constexpr std::uint32_t kTxLs = std::uint32_t{1} << 29;   // TDES0.LS (last segment)
    static constexpr std::uint32_t kTxFs = std::uint32_t{1} << 28;   // TDES0.FS (first segment)
    static constexpr std::uint32_t kTxTch = std::uint32_t{1} << 20;  // TDES0.TCH (chained)

    // MACMIIAR.CR clock range for the MDC divider (see RISK note above).
    static constexpr std::uint32_t kMdcCr = 0b100u;  // HCLK/102 (150-216 MHz)

    // TX/RX clock enables sit next to ETHMACEN in AHB1ENR (written via
    // Inst::gate.reg, a data address). Bit positions are IP facts this HAL owns —
    // no silicon address here. RMII selection (SYSCFG, outside this block) is a
    // chip fact done by the generated board::eth_configure_pins().
    static constexpr std::uint32_t kEthMacTxEn = std::uint32_t{1} << 26;  // AHB1ENR.ETHMACTXEN
    static constexpr std::uint32_t kEthMacRxEn = std::uint32_t{1} << 27;  // AHB1ENR.ETHMACRXEN

    inline static desc rx_desc_[RxN]{};
    inline static desc tx_desc_[TxN]{};
    // 32-byte aligned so the DMA's fixed AHB bursts never straddle a boundary,
    // and (with D-cache) each buffer sits on its own cache line. kBuf is a
    // multiple of 32 so every per-frame slice stays aligned.
    alignas(32) inline static std::uint8_t rx_buf_[RxN][kBuf]{};
    alignas(32) inline static std::uint8_t tx_buf_[TxN][kBuf]{};
    unsigned rx_next_ = 0;
    unsigned tx_next_ = 0;
    bool link_ = false;
};

}  // namespace alloy::hal
