#pragma once

// alloy::hal::ethernet::Same70Gmac — SAME70 GMAC Ethernet MAC driver.
//
// Implements EthernetMac using SAME70 GMAC hardware DMA descriptors.
// No heap allocation: TX/RX descriptor rings and frame buffers are stored
// inline as template-size arrays.
//
// Template parameters:
//   TxN — number of TX descriptors (= TX buffers). Default 4.
//   RxN — number of RX descriptors (= RX buffers). Default 8.
//
// RAM footprint: (TxN + RxN) * (8 B descriptor + 1536 B buffer)
//   Default (4+8): 12 * 1544 ≈ 18 KB.
//
// Prerequisites (caller's responsibility before calling init()):
//   1. Enable GMAC clock:   PMC_PCER1 |= (1u << 5)
//   2. Mux TX/RX pins:      PD0-PD7 + PD13/14 as PERIPH_A (ETH_TX/RX)
//   3. Initialize MDIO bus, PHY, and wait for link up.
//   4. Call Same70Gmac::init() — after PHY is up.
//
// Usage:
//   std::array<uint8_t,6> mac = board::read_mac_from_at24mac();
//   Same70Gmac<4,8> gmac{mac};
//   gmac.init();

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "src/hal/ethernet/ethernet_mac.hpp"

namespace alloy::hal::ethernet {

// ============================================================================
// GMAC descriptor structures (ATSAME70 datasheet §37)
// ============================================================================

struct GmacTxDescriptor {
    std::uint32_t addr;
    // status bits:
    //   bit31: used (1 = owned by CPU/software, 0 = DMA may send it)
    //   bit30: wrap (last descriptor in ring — next wraps to [0])
    //   bit16: no_crc (don't append FCS)
    //   bits[13:0]: length
    std::uint32_t status;
};

struct GmacRxDescriptor {
    // addr bits:
    //   bits[31:2]: buffer address (4-byte aligned)
    //   bit1: wrap (last descriptor in ring)
    //   bit0: ownership (0 = GMAC owns it and may write; 1 = CPU owns it)
    std::uint32_t addr;
    // status bits[12:0]: frame length (valid when ownership bit = 1)
    std::uint32_t status;
};

// ============================================================================
// Same70Gmac
// ============================================================================

template <std::size_t TxN = 4u, std::size_t RxN = 8u>
class Same70Gmac {
   public:
    explicit Same70Gmac(std::span<const std::uint8_t, 6u> mac_address) noexcept {
        for (std::size_t i = 0u; i < 6u; ++i) mac_address_[i] = mac_address[i];
    }

    // Configures GMAC DMA, descriptor rings, and MAC address.
    // Leaves TX/RX enabled (NCR.TE | NCR.RE).
    void init() noexcept {
        // Disable TX/RX first.
        *ncr() &= ~(kNcrTe | kNcrRe);

        // Program MAC address into SA1B/SA1T registers.
        *sa1b() = (std::uint32_t{mac_address_[0]}       )
                | (std::uint32_t{mac_address_[1]} <<  8u)
                | (std::uint32_t{mac_address_[2]} << 16u)
                | (std::uint32_t{mac_address_[3]} << 24u);
        *sa1t() = (std::uint32_t{mac_address_[4]}      )
                | (std::uint32_t{mac_address_[5]} << 8u);

        // DMA config: FBLDO=4-beat, RX buffer size = 1536/64 = 24 units.
        constexpr std::uint32_t kRxBufUnits = 1536u / 64u;  // 24
        *dcfgr() = (0x1u)                        // FBLDO = 4-beat AHB burst
                 | (kRxBufUnits << 16u);         // RXBMS

        // Initialise TX descriptors: all owned by CPU (used=1), last wraps.
        for (std::size_t i = 0u; i < TxN; ++i) {
            tx_desc_[i].addr   = reinterpret_cast<std::uint32_t>(tx_buf_[i].data());
            tx_desc_[i].status = kTxUsed;
        }
        tx_desc_[TxN - 1u].status |= kTxWrap;
        tx_head_ = 0u;

        // Initialise RX descriptors: owned by GMAC (ownership=0), last wraps.
        for (std::size_t i = 0u; i < RxN; ++i) {
            rx_desc_[i].addr   = reinterpret_cast<std::uint32_t>(rx_buf_[i].data());
            rx_desc_[i].status = 0u;
        }
        rx_desc_[RxN - 1u].addr |= kRxWrap;
        rx_head_ = 0u;

        // Write descriptor ring base addresses.
        *rbqb() = reinterpret_cast<std::uint32_t>(rx_desc_.data());
        *tbqb() = reinterpret_cast<std::uint32_t>(tx_desc_.data());

        // Enable RX + TX.
        *ncr() |= (kNcrRe | kNcrTe);
    }

    // Queue one raw Ethernet frame for transmission.
    // buf must contain a complete Ethernet II frame (no FCS).
    [[nodiscard]] core::Result<void, MacError> send_frame(
        std::span<const std::byte> buf) noexcept {
        if (buf.size() > kMaxFrameBytes) return core::Err(MacError::FrameTooLarge);

        GmacTxDescriptor& d = tx_desc_[tx_head_];
        if (!(d.status & kTxUsed)) return core::Err(MacError::Busy);

        // Copy frame into the TX buffer.
        for (std::size_t i = 0u; i < buf.size(); ++i) {
            tx_buf_[tx_head_][i] = buf[i];
        }

        // Build descriptor status: clear used, set length, preserve wrap.
        const bool is_last = (tx_head_ == TxN - 1u);
        d.status = static_cast<std::uint32_t>(buf.size()) & 0x3FFFu;
        if (is_last) d.status |= kTxWrap;
        // Leaving used=0 gives descriptor to DMA.

        // Kick the transmitter.
        *ncr() |= kNcrTstart;

        tx_head_ = (tx_head_ + 1u) % TxN;
        return core::Ok();
    }

    // Receive one raw Ethernet frame from the RX ring.
    // buf must be at least 1518 bytes. Returns the frame length on success.
    [[nodiscard]] core::Result<std::size_t, MacError> recv_frame(
        std::span<std::byte> buf) noexcept {
        GmacRxDescriptor& d = rx_desc_[rx_head_];
        // Ownership bit set → GMAC wrote a frame here.
        if (!(d.addr & kRxOwnership)) return core::Err(MacError::NoFrame);

        const std::size_t length = d.status & 0x1FFFu;
        const std::size_t copy_len = (length < buf.size()) ? length : buf.size();
        for (std::size_t i = 0u; i < copy_len; ++i) {
            buf[i] = rx_buf_[rx_head_][i];
        }

        // Return descriptor to GMAC: clear ownership bit (keep wrap bit).
        d.addr &= ~kRxOwnership;

        rx_head_ = (rx_head_ + 1u) % RxN;
        return core::Ok(copy_len);
    }

    [[nodiscard]] std::array<std::uint8_t, 6u> get_mac_address() const noexcept {
        return mac_address_;
    }

    // Returns true when NCR.NSR.LINK bit is set (PHY asserted link detect).
    [[nodiscard]] bool link_up() const noexcept {
        return (*nsr() & kNsrLink) != 0u;
    }

   private:
    // ---- MMIO register addresses ----------------------------------------
    static constexpr std::uint32_t kGmacBase    = 0x40050000u;
    static constexpr std::uint32_t kNcrOff      = 0x00u;
    static constexpr std::uint32_t kNsrOff      = 0x08u;
    static constexpr std::uint32_t kDcfgrOff    = 0x10u;
    static constexpr std::uint32_t kRbqbOff     = 0x18u;
    static constexpr std::uint32_t kTbqbOff     = 0x1Cu;
    static constexpr std::uint32_t kSa1bOff     = 0x88u;
    static constexpr std::uint32_t kSa1tOff     = 0x8Cu;

    // NCR bits
    static constexpr std::uint32_t kNcrRe       = 1u << 2u;   // receive enable
    static constexpr std::uint32_t kNcrTe       = 1u << 3u;   // transmit enable
    static constexpr std::uint32_t kNcrTstart   = 1u << 9u;   // transmit start

    // NSR bits
    static constexpr std::uint32_t kNsrLink     = 1u << 0u;   // link status

    // TX descriptor status bits
    static constexpr std::uint32_t kTxUsed      = 1u << 31u;
    static constexpr std::uint32_t kTxWrap      = 1u << 30u;

    // RX descriptor addr bits
    static constexpr std::uint32_t kRxOwnership = 1u << 0u;
    static constexpr std::uint32_t kRxWrap      = 1u << 1u;

    static constexpr std::size_t   kMaxFrameBytes = 1518u;

    // ---- Descriptor rings + frame buffers (inline, no heap) ---------------
    // 8-byte alignment required by GMAC DMA (ATSAME70 datasheet §37.5.3).
    alignas(8) std::array<GmacTxDescriptor, TxN> tx_desc_{};
    alignas(8) std::array<GmacRxDescriptor, RxN> rx_desc_{};
    alignas(8) std::array<std::array<std::byte, 1536u>, TxN> tx_buf_{};
    alignas(8) std::array<std::array<std::byte, 1536u>, RxN> rx_buf_{};

    std::array<std::uint8_t, 6u> mac_address_{};
    std::size_t tx_head_ = 0u;
    std::size_t rx_head_ = 0u;

    // ---- Register accessors -----------------------------------------------
    [[nodiscard]] static volatile std::uint32_t* r(std::uint32_t off) noexcept {
        return reinterpret_cast<volatile std::uint32_t*>(kGmacBase + off);
    }
    static volatile std::uint32_t* ncr()   noexcept { return r(kNcrOff);   }
    static const volatile std::uint32_t* nsr() noexcept {
        return reinterpret_cast<const volatile std::uint32_t*>(kGmacBase + kNsrOff);
    }
    static volatile std::uint32_t* dcfgr() noexcept { return r(kDcfgrOff); }
    static volatile std::uint32_t* rbqb()  noexcept { return r(kRbqbOff);  }
    static volatile std::uint32_t* tbqb()  noexcept { return r(kTbqbOff);  }
    static volatile std::uint32_t* sa1b()  noexcept { return r(kSa1bOff);  }
    static volatile std::uint32_t* sa1t()  noexcept { return r(kSa1tOff);  }
};

// Concept check: default template parameters satisfy EthernetMac.
// (Checked against a specialisation to avoid requiring full compilation.)
template <std::size_t TxN, std::size_t RxN>
inline constexpr bool kSame70GmacSatisfiesEthernetMac =
    EthernetMac<Same70Gmac<TxN, RxN>>;
static_assert(kSame70GmacSatisfiesEthernetMac<4u, 8u>);

}  // namespace alloy::hal::ethernet
