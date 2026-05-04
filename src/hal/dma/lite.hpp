/// @file hal/dma/lite.hpp
/// STM32 DMA controller — alloy.device.v2.1 lite driver.
///
/// Provides two address-template classes:
///
///   `controller<DmaBase>`        — STM32 DMA v1 channel engine (all families)
///   `dmamux<DmamuxBase>`         — DMAMUX1 request router (G4 / L4 / WB / G0+)
///
/// On families with a fixed channel-to-peripheral mapping (F0 / F3 / older G0)
/// the DMAMUX is absent; simply omit it and pick the correct channel number
/// from the device reference manual.
///
/// On families with DMAMUX (G4 / L4 / WB / newer G0):
///   1. Configure the channel via `controller::configure_channel()`.
///   2. Route a peripheral request to the channel via `dmamux::set_request()`.
///   3. Set source/destination addresses and count with `controller::start()`.
///
/// Example — USART2 TX on STM32G4 (DMA1 channel 1, DMAMUX request 26):
/// @code
///   #include "hal/dma.hpp"
///
///   using Dma1    = alloy::hal::dma::lite::controller<0x40020000u>;
///   using Dmamux1 = alloy::hal::dma::lite::dmamux<0x40020800u>;
///   using Nvic    = alloy::hal::nvic::lite::controller;
///
///   // 1. Route USART2_TX (request 26) to DMA1 channel 1 (DMAMUX ch 0):
///   Dmamux1::set_request(0u, 26u);
///
///   // 2. Configure DMA channel (mem→periph, 8-bit, non-circular):
///   Dma1::configure_channel(1u, {
///       .direction = alloy::hal::dma::lite::Direction::MemToPeriph,
///       .periph_size = alloy::hal::dma::lite::DataSize::Byte,
///       .mem_size    = alloy::hal::dma::lite::DataSize::Byte,
///       .priority    = alloy::hal::dma::lite::Priority::Medium,
///       .circular    = false,
///       .periph_inc  = false,
///       .mem_inc     = true,
///   });
///
///   // 3. Enable TC interrupt (optional):
///   Dma1::enable_tc_irq(1u);
///   Nvic::enable_irq(14u);  // DMA1_CH1_IRQn on G4
///
///   // 4. Transfer:
///   Dma1::start(1u, reinterpret_cast<std::uint32_t>(&USART2->TDR),
///                   reinterpret_cast<std::uint32_t>(tx_buf), 16u);
/// @endcode
///
/// Common DMA base addresses:
///   STM32F0 / G0       DMA1  : 0x40020000
///   STM32F3 / G4 / L4 / WB
///                      DMA1  : 0x40020000
///                      DMA2  : 0x40020400
///   STM32H7            DMA1  : 0x40020000 (AXI)
///
/// DMAMUX1 base addresses (G4 / L4 / WB):  0x40020800
///
/// DMA register map (per controller, from DMA base):
///   0x00  ISR   — Interrupt status register (4 flags × 8 channels)
///   0x04  IFCR  — Interrupt flag clear register (write 1 to clear)
///   Per-channel block, channel n (1-based) starts at 0x08 + (n-1)×0x14:
///     +0x00 CCR   — Channel configuration register
///     +0x04 CNDTR — Number of data items
///     +0x08 CPAR  — Peripheral address
///     +0x0C CMAR  — Memory address
///     (+0x10 reserved)
///
/// DMAMUX1 register map (per controller, from DMAMUX base):
///   Channel n (0-based, matching DMA channel n+1) at offset n×4:
///     CxCR[6:0] = DMAREQ_ID — request line selector
#pragma once

#include <cstdint>

namespace alloy::hal::dma::lite {

// ============================================================================
// Configuration enumerations
// ============================================================================

/// Data transfer direction (CCR.DIR).
enum class Direction : std::uint8_t {
    PeriphToMem = 0,  ///< Peripheral → memory (default for RX)
    MemToPeriph = 1,  ///< Memory → peripheral (default for TX)
    MemToMem    = 2,  ///< Memory → memory  (requires MEM2MEM + PINC; no circular)
};

/// Peripheral / memory data item size (PSIZE / MSIZE in CCR).
enum class DataSize : std::uint8_t {
    Byte     = 0,  ///< 8-bit
    HalfWord = 1,  ///< 16-bit
    Word     = 2,  ///< 32-bit
};

/// Channel priority level (CCR.PL).
enum class Priority : std::uint8_t {
    Low      = 0,
    Medium   = 1,
    High     = 2,
    VeryHigh = 3,
};

// ============================================================================
// Channel configuration struct
// ============================================================================

/// Channel configuration passed to `controller::configure_channel()`.
struct ChannelConfig {
    Direction direction  = Direction::PeriphToMem;
    DataSize  periph_size = DataSize::Byte;
    DataSize  mem_size    = DataSize::Byte;
    Priority  priority    = Priority::Medium;
    bool      circular    = false;  ///< Circular mode (CIRC=1)
    bool      periph_inc  = false;  ///< Auto-increment peripheral address
    bool      mem_inc     = true;   ///< Auto-increment memory address
};

// ============================================================================
// Detail — register layout
// ============================================================================

namespace detail {

inline constexpr std::uintptr_t kOfsIsr  = 0x00u;  ///< Interrupt status (ro)
inline constexpr std::uintptr_t kOfsIfcr = 0x04u;  ///< Interrupt flag clear (w)
inline constexpr std::uintptr_t kChBase  = 0x08u;  ///< Channel 1 start offset
inline constexpr std::uintptr_t kChStep  = 0x14u;  ///< Bytes per channel block

// CCR bit positions
inline constexpr std::uint32_t kCcrEn      = 1u << 0;   ///< Channel enable
inline constexpr std::uint32_t kCcrTcie    = 1u << 1;   ///< TC interrupt enable
inline constexpr std::uint32_t kCcrHtie    = 1u << 2;   ///< HT interrupt enable
inline constexpr std::uint32_t kCcrTeie    = 1u << 3;   ///< TE interrupt enable
inline constexpr std::uint32_t kCcrDir     = 1u << 4;   ///< DIR (0=per→mem, 1=mem→per)
inline constexpr std::uint32_t kCcrCirc    = 1u << 5;   ///< Circular mode
inline constexpr std::uint32_t kCcrPinc    = 1u << 6;   ///< Peripheral increment
inline constexpr std::uint32_t kCcrMinc    = 1u << 7;   ///< Memory increment
inline constexpr std::uint32_t kCcrPsizeShift = 8u;     ///< PSIZE[9:8]
inline constexpr std::uint32_t kCcrMsizeShift = 10u;    ///< MSIZE[11:10]
inline constexpr std::uint32_t kCcrPlShift    = 12u;    ///< PL[13:12]
inline constexpr std::uint32_t kCcrMem2mem = 1u << 14;  ///< Memory-to-memory

// ISR/IFCR: 4 flags per channel (GIF, TCIF, HTIF, TEIF), starting at bit (ch-1)*4
inline constexpr auto flag_shift(std::uint8_t ch) noexcept -> std::uint32_t {
    return static_cast<std::uint32_t>(ch - 1u) * 4u;
}
inline constexpr std::uint32_t kGif  = 0u;  ///< Bit offset within 4-bit group: Global
inline constexpr std::uint32_t kTcif = 1u;  ///< Transfer complete
inline constexpr std::uint32_t kHtif = 2u;  ///< Half transfer
inline constexpr std::uint32_t kTeif = 3u;  ///< Transfer error

// DMAMUX CxCR
inline constexpr std::uint32_t kMuxReqMask  = 0x7Fu;  ///< DMAREQ_ID[6:0]

}  // namespace detail

// ============================================================================
// controller<DmaBase>
// ============================================================================

/// STM32 DMA v1 controller — address-template variant.
///
/// Channel numbers are 1-based (1–7 for most STM32 DMA1 instances).
/// All methods are static; no state is stored.
template <std::uintptr_t DmaBase>
class controller {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 DMA v1 (F0/F1/F3/G0/G4/L4/WB)
   //
   // Register offsets are identical across all listed STM32 families.
   // Do NOT use this driver for SAME70 XDMAC, nRF52 DMA, or RP2040 DMA —
   // they have incompatible register maps.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/dma/lite.hpp: DMA v1 layout is STM32-only. "
        "Use a vendor-specific DMA driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto dma_reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(DmaBase + ofs);
    }

    /// Base offset of channel `ch` (1-based).
    static constexpr auto ch_ofs(std::uint8_t ch) noexcept -> std::uintptr_t {
        return detail::kChBase + static_cast<std::uintptr_t>(ch - 1u) * detail::kChStep;
    }

    [[nodiscard]] static auto ccr(std::uint8_t ch) noexcept
        -> volatile std::uint32_t& {
        return dma_reg(ch_ofs(ch) + 0x00u);
    }
    [[nodiscard]] static auto cndtr(std::uint8_t ch) noexcept
        -> volatile std::uint32_t& {
        return dma_reg(ch_ofs(ch) + 0x04u);
    }
    [[nodiscard]] static auto cpar(std::uint8_t ch) noexcept
        -> volatile std::uint32_t& {
        return dma_reg(ch_ofs(ch) + 0x08u);
    }
    [[nodiscard]] static auto cmar(std::uint8_t ch) noexcept
        -> volatile std::uint32_t& {
        return dma_reg(ch_ofs(ch) + 0x0Cu);
    }

   public:
    // -----------------------------------------------------------------------
    // Channel configure (does NOT enable)
    // -----------------------------------------------------------------------

    /// Configure a DMA channel without starting a transfer.
    ///
    /// The channel must be disabled before reconfiguring (call `stop()` first
    /// if it was previously enabled).  CPAR and CMAR are set by `start()`.
    ///
    /// @param ch  Channel number, 1-based (1–7 for most STM32 DMA1/DMA2).
    /// @param cfg Transfer configuration.
    static void configure_channel(std::uint8_t ch,
                                  const ChannelConfig& cfg) noexcept {
        std::uint32_t ccr_val = 0u;

        // Direction: MEM2MEM takes priority; otherwise set DIR bit.
        if (cfg.direction == Direction::MemToMem) {
            ccr_val |= detail::kCcrMem2mem;
            ccr_val |= detail::kCcrPinc;    // source (periph ptr) must increment
        } else if (cfg.direction == Direction::MemToPeriph) {
            ccr_val |= detail::kCcrDir;
        }

        if (cfg.circular)   { ccr_val |= detail::kCcrCirc; }
        if (cfg.periph_inc) { ccr_val |= detail::kCcrPinc; }
        if (cfg.mem_inc)    { ccr_val |= detail::kCcrMinc; }

        ccr_val |= (static_cast<std::uint32_t>(cfg.periph_size) << detail::kCcrPsizeShift);
        ccr_val |= (static_cast<std::uint32_t>(cfg.mem_size)    << detail::kCcrMsizeShift);
        ccr_val |= (static_cast<std::uint32_t>(cfg.priority)    << detail::kCcrPlShift);

        ccr(ch) = ccr_val;  // EN=0, all IRQ enables=0
    }

    // -----------------------------------------------------------------------
    // Transfer start / stop
    // -----------------------------------------------------------------------

    /// Set addresses + count then enable the channel (start transfer).
    ///
    /// Preserves the CCR configuration written by `configure_channel()`.
    /// The channel is enabled after all three setup registers are written.
    ///
    /// @param ch           Channel number (1-based).
    /// @param periph_addr  Peripheral data register address (e.g. &USART->RDR).
    /// @param mem_addr     Memory buffer address.
    /// @param count        Number of data items (not bytes — depends on DataSize).
    static void start(std::uint8_t  ch,
                      std::uint32_t periph_addr,
                      std::uint32_t mem_addr,
                      std::uint16_t count) noexcept {
        cndtr(ch) = static_cast<std::uint32_t>(count);
        cpar(ch)  = periph_addr;
        cmar(ch)  = mem_addr;
        ccr(ch)  |= detail::kCcrEn;
    }

    /// Disable (stop) a channel immediately.
    ///
    /// For non-circular transfers, the channel auto-disables when CNDTR
    /// reaches zero.  Call `stop()` to abort a transfer early or to
    /// disable after polling `transfer_complete()`.
    static void stop(std::uint8_t ch) noexcept {
        ccr(ch) &= ~detail::kCcrEn;
    }

    /// True while the channel is enabled (CCR.EN=1).
    ///
    /// For a one-shot transfer, EN is cleared by hardware when CNDTR hits
    /// zero — so this also serves as a busy/done test.
    [[nodiscard]] static auto is_busy(std::uint8_t ch) noexcept -> bool {
        return (ccr(ch) & detail::kCcrEn) != 0u;
    }

    // -----------------------------------------------------------------------
    // Interrupt flags (ISR / IFCR)
    // -----------------------------------------------------------------------

    /// True if the global interrupt flag (GIF) is set for `ch`.
    [[nodiscard]] static auto global_flag(std::uint8_t ch) noexcept -> bool {
        const auto shift = detail::flag_shift(ch) + detail::kGif;
        return (dma_reg(detail::kOfsIsr) & (1u << shift)) != 0u;
    }

    /// True when a transfer-complete interrupt is pending for `ch`.
    [[nodiscard]] static auto transfer_complete(std::uint8_t ch) noexcept -> bool {
        const auto shift = detail::flag_shift(ch) + detail::kTcif;
        return (dma_reg(detail::kOfsIsr) & (1u << shift)) != 0u;
    }

    /// True when a half-transfer interrupt is pending for `ch`.
    [[nodiscard]] static auto half_transfer(std::uint8_t ch) noexcept -> bool {
        const auto shift = detail::flag_shift(ch) + detail::kHtif;
        return (dma_reg(detail::kOfsIsr) & (1u << shift)) != 0u;
    }

    /// True when a transfer-error flag is set for `ch`.
    [[nodiscard]] static auto error(std::uint8_t ch) noexcept -> bool {
        const auto shift = detail::flag_shift(ch) + detail::kTeif;
        return (dma_reg(detail::kOfsIsr) & (1u << shift)) != 0u;
    }

    /// Clear all interrupt flags (GIF, TCIF, HTIF, TEIF) for `ch`.
    ///
    /// Call inside the DMA ISR before re-enabling the channel.
    static void clear_flags(std::uint8_t ch) noexcept {
        // Write 1 to the 4-bit group for this channel in IFCR.
        const auto shift = detail::flag_shift(ch);
        dma_reg(detail::kOfsIfcr) = 0xFu << shift;
    }

    // -----------------------------------------------------------------------
    // Interrupt enables (CCR bits 1–3)
    // -----------------------------------------------------------------------

    /// Enable the transfer-complete interrupt for `ch` (CCR.TCIE=1).
    static void enable_tc_irq(std::uint8_t ch) noexcept {
        ccr(ch) |= detail::kCcrTcie;
    }

    /// Enable the half-transfer interrupt for `ch` (CCR.HTIE=1).
    static void enable_ht_irq(std::uint8_t ch) noexcept {
        ccr(ch) |= detail::kCcrHtie;
    }

    /// Enable the transfer-error interrupt for `ch` (CCR.TEIE=1).
    static void enable_te_irq(std::uint8_t ch) noexcept {
        ccr(ch) |= detail::kCcrTeie;
    }

    /// Disable all DMA interrupts for `ch` (clears TCIE, HTIE, TEIE).
    static void disable_irqs(std::uint8_t ch) noexcept {
        ccr(ch) &= ~(detail::kCcrTcie | detail::kCcrHtie | detail::kCcrTeie);
    }

    /// Remaining data item count for `ch` (CNDTR register).
    ///
    /// Returns 0 when the transfer is complete (non-circular) or the
    /// channel has not yet been started.
    [[nodiscard]] static auto remaining(std::uint8_t ch) noexcept -> std::uint16_t {
        return static_cast<std::uint16_t>(cndtr(ch) & 0xFFFFu);
    }

    controller() = delete;
};

// ============================================================================
// dmamux<DmamuxBase>
// ============================================================================

/// STM32 DMAMUX1 request router — address-template variant.
///
/// DMAMUX channels are 0-based and map 1:1 to DMA channels:
///   DMAMUX channel 0  →  DMA1 channel 1
///   DMAMUX channel 1  →  DMA1 channel 2
///   … (DMA2 channels follow after all DMA1 channels)
///
/// Request IDs are device-specific — refer to the reference manual DMA
/// request mapping table (e.g. G4 RM0440 table 91).
///
/// Common DMAMUX1 base addresses:
///   STM32G4 / L4 / WB : 0x40020800
///   STM32G0 (with DMAMUX): 0x40020800
template <std::uintptr_t DmamuxBase>
class dmamux {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 DMAMUX1 (G0/G4/L4/WB/H7)
   //
   // Channel-register layout (4 bytes per channel, DMAREQ_ID[6:0]) and
   // request-ID encoding are STM32-specific.
   // Do NOT use for SAME70 XDMAC PERID or nRF52 PPI/DPPIC.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/dma/lite.hpp: DMAMUX layout is STM32-only. "
        "Use a vendor-specific DMA mux driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto ch_reg(std::uint8_t mux_ch) noexcept
        -> volatile std::uint32_t& {
        // Each channel occupies one 32-bit register; channel 0 is at base+0.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(
            DmamuxBase + static_cast<std::uintptr_t>(mux_ch) * 4u);
    }

   public:
    /// Map a DMAMUX channel to a peripheral request line.
    ///
    /// @param mux_ch     DMAMUX channel index (0-based; 0 = DMA1 channel 1).
    /// @param request_id Peripheral request identifier (7-bit, device-specific).
    static void set_request(std::uint8_t mux_ch, std::uint8_t request_id) noexcept {
        ch_reg(mux_ch) = (ch_reg(mux_ch) & ~detail::kMuxReqMask) |
                         (static_cast<std::uint32_t>(request_id) & detail::kMuxReqMask);
    }

    /// Read back the currently mapped request ID for a DMAMUX channel.
    [[nodiscard]] static auto get_request(std::uint8_t mux_ch) noexcept -> std::uint8_t {
        return static_cast<std::uint8_t>(ch_reg(mux_ch) & detail::kMuxReqMask);
    }

    dmamux() = delete;
};

}  // namespace alloy::hal::dma::lite
