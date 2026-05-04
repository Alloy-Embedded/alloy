/// @file hal/fdcan/lite.hpp
/// Lightweight, direct-MMIO FDCAN driver for STM32G4 (polling, classic CAN).
///
/// Tier 2 — PeripheralSpec-gated (P must satisfy StFdcan, kTemplate == "fdcan").
/// No descriptor-runtime dependency.  Polling only — no interrupts, no DMA.
/// Classic CAN frames up to 8 bytes data.  Accept-all receive (no filters).
///
/// Register layout targets STM32G4 (RM0440).
/// The key structural difference vs STM32H7:
///   G4: 0x090 = RXF0S (status only; no RXF0C config register)
///   H7: 0x090 = RXF0C (config), 0x094 = RXF0S, 0x098 = RXF0A
/// H7 support can be added as a second `if constexpr` branch using kIpVersion.
///
/// Message RAM layout used by this driver (with 0 filter entries):
///   Offset   Section          Size
///   ─────────────────────────────────────────────────────
///   +0       RX FIFO 0        3 elements × 16 bytes = 48 bytes
///   +48      TX buffer        1 element  × 16 bytes = 16 bytes
///
/// Each element (8-byte payload): 2 header words + 2 data words = 16 bytes.
///
/// Typical usage (STM32G4 FDCAN1, 500 kbps with 16 MHz FDCAN kernel clock):
/// @code
///   #include "hal/fdcan/lite.hpp"
///   namespace dev = alloy::device::traits;
///
///   // FDCAN1 peripheral = 0x40006400, Message RAM = 0x4000AC00
///   using Fdcan1 = alloy::hal::fdcan::lite::controller<dev::fdcan1, 0x4000AC00u>;
///
///   dev::peripheral_on<dev::fdcan1>();
///   Fdcan1::configure(2u, 12u, 3u, 1u);  // prescaler=2, tseg1=12, tseg2=3, sjw=1
///   Fdcan1::start();
///
///   std::array<std::uint8_t, 4> payload = {0x01, 0x02, 0x03, 0x04};
///   if (Fdcan1::write_tx(0x123u, false, payload.data(), 4u)) { /* queued */ }
///
///   std::array<std::uint8_t, 8> rx_buf{};
///   std::uint32_t id; bool ext; std::uint8_t dlc;
///   if (Fdcan1::read_rx(id, ext, rx_buf.data(), dlc)) { /* frame received */ }
/// @endcode
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::fdcan::lite {

// ============================================================================
// Detail — register offsets and bit constants
// ============================================================================

namespace detail {

// ----------------------------------------------------------------------------
// FDCAN peripheral register offsets (RM0440, same for G4 and H7 up to NBTP)
// ----------------------------------------------------------------------------

inline constexpr std::uintptr_t kCccrOfs  = 0x018u;  ///< CC Control register
inline constexpr std::uintptr_t kNbtpOfs  = 0x01Cu;  ///< Nominal Bit Timing
inline constexpr std::uintptr_t kRxgfcOfs = 0x080u;  ///< Global RX Filter Config (G4)
inline constexpr std::uintptr_t kRxescOfs = 0x0BCu;  ///< RX Buffer Element Size
inline constexpr std::uintptr_t kRxf0sOfs = 0x090u;  ///< RX FIFO 0 Status  (G4 offset)
inline constexpr std::uintptr_t kRxf0aOfs = 0x094u;  ///< RX FIFO 0 Acknowledge (G4)
inline constexpr std::uintptr_t kTxbcOfs  = 0x0C4u;  ///< TX Buffer Configuration
inline constexpr std::uintptr_t kTxfqsOfs = 0x0C8u;  ///< TX FIFO/Queue Status
inline constexpr std::uintptr_t kTxescOfs = 0x0CCu;  ///< TX Buffer Element Size
inline constexpr std::uintptr_t kTxbarOfs = 0x0D4u;  ///< TX Buffer Add Request

// ----------------------------------------------------------------------------
// CCCR (CC Control) bits
// ----------------------------------------------------------------------------

inline constexpr std::uint32_t kCccrInit = 1u << 0u;  ///< Initialization request
inline constexpr std::uint32_t kCccrCce  = 1u << 1u;  ///< Configuration Change Enable

// ----------------------------------------------------------------------------
// NBTP (Nominal Bit Timing) field positions — values stored as N-1
// ----------------------------------------------------------------------------

inline constexpr std::uint32_t kNbtpTseg2Pos = 0u;   ///< NTSEG2[6:0]
inline constexpr std::uint32_t kNbtpTseg1Pos = 8u;   ///< NTSEG1[7:0]
inline constexpr std::uint32_t kNbtpBrpPos   = 16u;  ///< NBRP[8:0]
inline constexpr std::uint32_t kNbtpSjwPos   = 25u;  ///< NSJW[6:0]

// ----------------------------------------------------------------------------
// RXF0S (RX FIFO 0 Status, G4 layout) field positions
// ----------------------------------------------------------------------------

inline constexpr std::uint32_t kRxf0sFlPos  = 0u;    ///< Fill Level [6:0]
inline constexpr std::uint32_t kRxf0sGiPos  = 8u;    ///< Get Index [13:8]
inline constexpr std::uint32_t kRxf0sFlMask = 0x7Fu; ///< Fill Level mask
inline constexpr std::uint32_t kRxf0sGiMask = 0x3Fu; ///< Get Index mask

// ----------------------------------------------------------------------------
// TXFQS (TX FIFO/Queue Status) field positions
// ----------------------------------------------------------------------------

inline constexpr std::uint32_t kTxfqsPiPos = 16u;        ///< TX Put Index [20:16]
inline constexpr std::uint32_t kTxfqsFull  = 1u << 21u;  ///< TX FIFO/Queue full

// ----------------------------------------------------------------------------
// Message RAM layout (with LSS=0, LSE=0 → no filter tables)
//
// RX FIFO 0: elements 0–2 starting at word offset 0  (bytes 0–47)
// TX buffer: element  0   starting at word offset 12  (bytes 48–63)
//
// Element size for DLC ≤ 8: 2 header words + 2 data words = 16 bytes = 4 words
// ----------------------------------------------------------------------------

inline constexpr std::size_t   kElemWords      = 4u;   ///< Words per element (8-byte data)
inline constexpr std::uint32_t kTxStartWord    = 12u;  ///< TX buffer start (word offset)
inline constexpr std::uint32_t kTxStartField   = kTxStartWord << 2u;  ///< TXBC.TBSA value

// ----------------------------------------------------------------------------
// TX element bit fields (words T0 and T1)
// ----------------------------------------------------------------------------

inline constexpr std::uint32_t kT0XtdBit = 1u << 30u;  ///< Extended ID flag in T0
inline constexpr std::uint32_t kT1DlcPos = 16u;          ///< DLC position in T1

// ----------------------------------------------------------------------------
// RX element bit fields (words R0 and R1)
// ----------------------------------------------------------------------------

inline constexpr std::uint32_t kR0IdMaskExt = 0x1FFFFFFFu;  ///< 29-bit ID mask
inline constexpr std::uint32_t kR0IdMaskStd = 0x1FFC0000u;  ///< 11-bit ID in bits [28:18]
inline constexpr std::uint32_t kR0IdStdPos  = 18u;          ///< Standard ID bit shift
inline constexpr std::uint32_t kR0XtdBit    = 1u << 30u;    ///< Extended ID flag
inline constexpr std::uint32_t kR1DlcMask   = 0x000F0000u;  ///< DLC mask in R1
inline constexpr std::uint32_t kR1DlcPos    = 16u;          ///< DLC bit position

// ----------------------------------------------------------------------------
// Template detector
// ----------------------------------------------------------------------------

[[nodiscard]] consteval auto is_fdcan(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "fdcan";
}

}  // namespace detail

// ============================================================================
// Concept
// ============================================================================

/// Satisfied when P is an STM32 FDCAN peripheral (kTemplate == "fdcan").
template <typename P>
concept StFdcan =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_fdcan(P::kTemplate);

// ============================================================================
// controller<P, MsgRamBase> — zero-size tag type, all methods static
// ============================================================================

/// Direct-MMIO FDCAN polling controller.
///
/// Configured for classic CAN (DLC ≤ 8 bytes), accept-all receive, no interrupts.
///
/// @tparam P           PeripheralSpec satisfying StFdcan (provides kBaseAddress)
/// @tparam MsgRamBase  Start address of this FDCAN instance's Message RAM section
///
/// STM32G4 known addresses:
///   FDCAN1: peripheral = 0x40006400, MsgRamBase = 0x4000AC00
///   FDCAN2: peripheral = 0x40006800, MsgRamBase = 0x4000AC00 + (your FDCAN1 section size)
///
/// STM32H743 known addresses:
///   FDCAN1: peripheral = 0x4000A000, MsgRamBase = 0x4000AC00
///   FDCAN2: peripheral = 0x4000A400, MsgRamBase = 0x4000AC00 + (your FDCAN1 section size)
///   NOTE: H7 requires RXF0C at 0x090 instead of G4's RXF0S; this driver targets G4.
template <typename P, std::uintptr_t MsgRamBase>
    requires StFdcan<P>
class controller {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 FDCAN (G4 register layout, RM0440)
   //
   // Do NOT use for SAME70 MCAN without adapting RX FIFO config offsets.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/fdcan/lite.hpp: FDCAN layout targets STM32 (G4). "
        "Adapt for other vendors using kIpVersion dispatch.");
#endif

   public:
    static constexpr std::uintptr_t kBase    = P::kBaseAddress;
    static constexpr std::uintptr_t kMsgRam  = MsgRamBase;

   private:
    // -----------------------------------------------------------------------
    // Register access
    // -----------------------------------------------------------------------

    /// Access a 32-bit FDCAN peripheral register at byte offset `ofs`.
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + ofs);
    }

    /// Access a 32-bit word in the Message RAM at word index `w`.
    /// Byte address = kMsgRam + w × 4.
    [[nodiscard]] static auto ram(std::uintptr_t w) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kMsgRam + w * 4u);
    }

   public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Enter initialization mode and program nominal bit timing.
    ///
    /// Configures Message RAM sections (RX FIFO 0 + TX buffer) and
    /// global filter to accept all frames.  Leaves the peripheral in
    /// init mode — call `start()` to go on-bus.
    ///
    /// Bit rate formula:
    ///   bit_rate = fdcan_clk / (prescaler × (1 + tseg1 + tseg2))
    ///
    /// Example — 500 kbps with 16 MHz FDCAN clock:
    ///   configure(2u, 12u, 3u, 1u)  →  16 MHz / (2 × 16) = 500 kbps
    ///
    /// @param prescaler  Baud Rate Prescaler, 1–512
    /// @param tseg1      Time Segment 1 (TQ before sample point), 1–256
    /// @param tseg2      Time Segment 2 (TQ after sample point), 1–128
    /// @param sjw        Resynchronization Jump Width, 1–128
    static void configure(std::uint32_t prescaler,
                          std::uint32_t tseg1,
                          std::uint32_t tseg2,
                          std::uint32_t sjw) noexcept {
        clock_on();
        // 1. Request initialization mode; wait for hardware acknowledgement.
        reg(detail::kCccrOfs) = detail::kCccrInit;
        while ((reg(detail::kCccrOfs) & detail::kCccrInit) == 0u) { /* spin */ }

        // 2. Enable Configuration Change (requires INIT=1).
        reg(detail::kCccrOfs) = detail::kCccrInit | detail::kCccrCce;

        // 3. Nominal Bit Timing — all fields stored as (N-1).
        reg(detail::kNbtpOfs) =
            ((sjw       - 1u) << detail::kNbtpSjwPos)   |
            ((prescaler - 1u) << detail::kNbtpBrpPos)   |
            ((tseg1     - 1u) << detail::kNbtpTseg1Pos) |
            ((tseg2     - 1u) << detail::kNbtpTseg2Pos);

        // 4. Global RX filter: accept all non-matching frames → RX FIFO 0.
        //    ANFE=00, ANFS=00, LSS=0, LSE=0 → all zeros.
        reg(detail::kRxgfcOfs) = 0u;

        // 5. RX element size: 8-byte data field (F0DS=0, F1DS=0, RBDS=0).
        reg(detail::kRxescOfs) = 0u;

        // 6. TX element size: 8-byte data field (TBDS=0).
        reg(detail::kTxescOfs) = 0u;

        // 7. TX buffer configuration:
        //    TBSA = word 12 (= byte 48 = after 3 × 16-byte RX FIFO 0 elements)
        //    NDTB = 0 (no dedicated TX buffers)
        //    TFQS = 1 (one TX FIFO element)
        //    TFQM = 0 (FIFO mode, not queue)
        reg(detail::kTxbcOfs) =
            detail::kTxStartField |   // TBSA[15:2] = kTxStartWord (= 12)
            (1u << 24u);              // TFQS[29:24] = 1

        // 8. Clear CCE; leave INIT set (caller must call start()).
        reg(detail::kCccrOfs) = detail::kCccrInit;
    }

    /// Leave initialization mode and go on-bus.
    ///
    /// The FDCAN core waits for 11 consecutive recessive bits (CAN bus idle)
    /// before participating.  RX starts immediately; TX completes after sync.
    static void start() noexcept {
        reg(detail::kCccrOfs) &= ~detail::kCccrInit;
    }

    /// Return to initialization mode (stop TX/RX).
    static void stop() noexcept {
        reg(detail::kCccrOfs) |= detail::kCccrInit;
        while ((reg(detail::kCccrOfs) & detail::kCccrInit) == 0u) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // Transmit
    // -----------------------------------------------------------------------

    /// Queue one classic CAN frame for transmission.
    ///
    /// @param id           Arbitration ID (11-bit standard or 29-bit extended)
    /// @param is_extended  True → 29-bit extended ID; false → 11-bit standard ID
    /// @param data         Payload bytes; only `dlc` bytes are read
    /// @param dlc          Data Length Code, 0–8
    /// @return `true` if the frame was written to the TX FIFO; `false` if full.
    [[nodiscard]] static auto write_tx(std::uint32_t       id,
                                       bool                is_extended,
                                       const std::uint8_t* data,
                                       std::uint8_t        dlc) noexcept -> bool {
        const std::uint32_t txfqs = reg(detail::kTxfqsOfs);
        if (txfqs & detail::kTxfqsFull) { return false; }

        const auto put = (txfqs >> detail::kTxfqsPiPos) & 0x1Fu;
        const auto ofs = detail::kTxStartWord + put * detail::kElemWords;

        // T0: arbitration ID + extended-ID flag
        std::uint32_t t0 = 0u;
        if (is_extended) {
            t0 = (id & 0x1FFFFFFFu) | detail::kT0XtdBit;
        } else {
            t0 = (id & 0x7FFu) << detail::kR0IdStdPos;  // 11-bit in bits [28:18]
        }
        ram(ofs + 0u) = t0;

        // T1: DLC, classic CAN (FDF=0, BRS=0, EFC=0, MM=0)
        ram(ofs + 1u) = static_cast<std::uint32_t>(dlc & 0xFu) << detail::kT1DlcPos;

        // T2–T3: data bytes packed little-endian (byte 0 at LSB of word 0)
        const auto n = static_cast<std::size_t>(dlc > 8u ? 8u : dlc);
        std::uint32_t dw0 = 0u, dw1 = 0u;
        for (std::size_t i = 0u; i < n && i < 4u; ++i) {
            dw0 |= static_cast<std::uint32_t>(data[i]) << (i * 8u);
        }
        for (std::size_t i = 4u; i < n; ++i) {
            dw1 |= static_cast<std::uint32_t>(data[i]) << ((i - 4u) * 8u);
        }
        ram(ofs + 2u) = dw0;
        ram(ofs + 3u) = dw1;

        // Add transmission request for this buffer slot
        reg(detail::kTxbarOfs) = 1u << put;
        return true;
    }

    // -----------------------------------------------------------------------
    // Receive
    // -----------------------------------------------------------------------

    /// Read one classic CAN frame from RX FIFO 0.
    ///
    /// @param out_id        Receives the arbitration ID
    /// @param out_extended  Receives true if the frame used a 29-bit extended ID
    /// @param out_data      Buffer to receive payload (must be ≥ 8 bytes)
    /// @param out_dlc       Receives the Data Length Code (0–8)
    /// @return `true` if a frame was read; `false` if RX FIFO 0 is empty.
    [[nodiscard]] static auto read_rx(std::uint32_t& out_id,
                                      bool&          out_extended,
                                      std::uint8_t*  out_data,
                                      std::uint8_t&  out_dlc) noexcept -> bool {
        const std::uint32_t rxf0s = reg(detail::kRxf0sOfs);
        const auto fill = (rxf0s >> detail::kRxf0sFlPos) & detail::kRxf0sFlMask;
        if (fill == 0u) { return false; }

        // RX FIFO 0 elements start at word offset 0 in Message RAM (with LSS=0, LSE=0)
        const auto get = (rxf0s >> detail::kRxf0sGiPos) & detail::kRxf0sGiMask;
        const auto ofs = get * detail::kElemWords;

        const std::uint32_t r0 = ram(ofs + 0u);
        const std::uint32_t r1 = ram(ofs + 1u);

        // Decode ID
        out_extended = (r0 & detail::kR0XtdBit) != 0u;
        if (out_extended) {
            out_id = r0 & detail::kR0IdMaskExt;
        } else {
            out_id = (r0 & detail::kR0IdMaskStd) >> detail::kR0IdStdPos;
        }

        // Decode DLC and payload
        out_dlc = static_cast<std::uint8_t>((r1 & detail::kR1DlcMask) >> detail::kR1DlcPos);
        const auto n = static_cast<std::size_t>(out_dlc > 8u ? 8u : out_dlc);

        const std::uint32_t dw0 = ram(ofs + 2u);
        const std::uint32_t dw1 = ram(ofs + 3u);
        for (std::size_t i = 0u; i < n && i < 4u; ++i) {
            out_data[i] = static_cast<std::uint8_t>((dw0 >> (i * 8u)) & 0xFFu);
        }
        for (std::size_t i = 4u; i < n; ++i) {
            out_data[i] = static_cast<std::uint8_t>((dw1 >> ((i - 4u) * 8u)) & 0xFFu);
        }

        // Acknowledge element — releases the FIFO slot
        reg(detail::kRxf0aOfs) = get;
        return true;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when the TX FIFO is full (write_tx will return false).
    [[nodiscard]] static auto tx_pending() noexcept -> bool {
        return (reg(detail::kTxfqsOfs) & detail::kTxfqsFull) != 0u;
    }

    /// True when RX FIFO 0 holds at least one unread frame.
    [[nodiscard]] static auto rx_available() noexcept -> bool {
        return ((reg(detail::kRxf0sOfs) >> detail::kRxf0sFlPos) &
                detail::kRxf0sFlMask) != 0u;
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    [[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
        -> std::uint32_t {
        if constexpr (requires { P::kIrqLines[0]; }) {
            return static_cast<std::uint32_t>(P::kIrqLines[idx]);
        } else {
            static_assert(sizeof(P) == 0,
                "P::kIrqLines not present; upgrade device artifact to v2.1");
            return 0u;
        }
    }

    /// Returns the number of IRQ lines for this peripheral.
    [[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
        if constexpr (requires { P::kIrqCount; }) {
            return static_cast<std::size_t>(P::kIrqCount);
        }
        return 0u;
    }

    // -----------------------------------------------------------------------
    // Clock gate — sourced from alloy.device.v2.1 flat-struct kRccEnable
    // -----------------------------------------------------------------------

    /// Enable the peripheral clock and deassert reset (if kRccReset present).
    static void clock_on() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) |= P::kRccEnable.mask;
        }
        if constexpr (requires { P::kRccReset; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) |=  P::kRccReset.mask;
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) &= ~P::kRccReset.mask;
        }
    }

    /// Disable the peripheral clock.
    static void clock_off() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) &= ~P::kRccEnable.mask;
        }
    }

    controller() = delete;
};

}  // namespace alloy::hal::fdcan::lite
