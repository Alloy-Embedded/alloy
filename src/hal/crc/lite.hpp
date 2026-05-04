/// @file hal/crc/lite.hpp
/// Hardware CRC accelerator — alloy.device.v2.1 lite driver.
///
/// Thin wrapper for the STM32 CRC peripheral: reset, feed data, read result.
/// Supports byte-, half-word-, and word-width writes for flexible input.
///
/// Address-template variant — no codegen format required:
/// @code
///   #include "hal/crc/lite.hpp"
///
///   // STM32G0 / G4 / F4 CRC base = 0x40023000
///   using Crc = alloy::hal::crc::lite::engine<0x40023000u>;
///
///   dev::peripheral_on<dev::crc>();   // enable CRC clock
///   Crc::reset();
///   Crc::feed(my_buffer);             // std::span<const uint8_t>
///   const std::uint32_t crc = Crc::result();
/// @endcode
///
/// Register layout (same on G0 / G4 / F4 / H7 / L4 / WB):
///   0x00 DR    — data register (write: feed; read: current accumulator)
///   0x04 IDR   — independent data register (8-bit scratch, user-writable)
///   0x08 CR    — RESET(0), POLYSIZE[4:3], REV_IN[6:5], REV_OUT(7)
///   0x10 INIT  — initial/seed value (default 0xFFFF_FFFF = CRC-32 seed)
///   0x14 POL   — programmable polynomial (default 0x04C1_1DB7 = CRC-32)
///
/// F1 / F2 note: only DR, IDR, CR at 0x00–0x08; INIT and POL are absent.
/// `set_init()` and `set_polynomial()` write to reserved addresses on those
/// devices — the writes are harmless but have no effect.
#pragma once

#include <cstdint>
#include <span>

namespace alloy::hal::crc::lite {

namespace detail {

inline constexpr std::uintptr_t kOfsDr   = 0x00u;
inline constexpr std::uintptr_t kOfsIdr  = 0x04u;
inline constexpr std::uintptr_t kOfsCr   = 0x08u;
inline constexpr std::uintptr_t kOfsInit = 0x10u;
inline constexpr std::uintptr_t kOfsPol  = 0x14u;

inline constexpr std::uint32_t kCrReset         = 1u << 0;
inline constexpr std::uint32_t kCrPolySizeShift = 3u;
inline constexpr std::uint32_t kCrPolySizeMask  = 0x3u << 3;
inline constexpr std::uint32_t kCrRevInShift    = 5u;
inline constexpr std::uint32_t kCrRevInMask     = 0x3u << 5;
inline constexpr std::uint32_t kCrRevOut        = 1u << 7;

}  // namespace detail

/// CRC polynomial width (CR.POLYSIZE[4:3]).
///
/// Selects the effective width of the polynomial and accumulator.
enum class PolySize : std::uint8_t {
    Bits32 = 0,  ///< 32-bit (default — CRC-32 / Ethernet)
    Bits16 = 1,  ///< 16-bit (CRC-16-CCITT, CRC-16-IBM …)
    Bits8  = 2,  ///< 8-bit
    Bits7  = 3,  ///< 7-bit (ISO 7816)
};

/// Input bit-reversal mode (CR.REV_IN[6:5]).
///
/// Reversal is applied per input word before feeding the polynomial.
enum class BitReversal : std::uint8_t {
    None       = 0,  ///< No reversal (default)
    ByByte     = 1,  ///< Reverse bits within each byte
    ByHalfWord = 2,  ///< Reverse bits within each 16-bit half-word
    ByWord     = 3,  ///< Reverse all 32 bits of the word
};

/// CRC hardware engine.
///
/// `CrcBase` is the CRC peripheral base address:
///   - STM32G0 / G4 / F0 / F3 / F4 / L4 / WB : 0x40023000
///   - STM32H7 (AHB4)                          : 0x58024C00
template <std::uintptr_t CrcBase>
class engine {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 CRC calculation unit (F0/F1/F3/G0/G4/L4/WB/H7)
   //
   // DR/IDR/CR/INIT/POL register layout and polynomial/bit-reversal control
   // are STM32-specific.  Do NOT use for SAME70 CRCCU or RP2040 DMA-CRC.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/crc/lite.hpp: CRC unit layout is STM32-only. "
        "Use a vendor-specific CRC driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(CrcBase + ofs);
    }

    [[nodiscard]] static auto dr8() noexcept -> volatile std::uint8_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint8_t*>(CrcBase + detail::kOfsDr);
    }

    [[nodiscard]] static auto dr16() noexcept -> volatile std::uint16_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint16_t*>(CrcBase + detail::kOfsDr);
    }

   public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Configure polynomial width, input bit-reversal, and output reversal.
    ///
    /// Changes take effect from the next `reset()` call.
    ///
    /// @param poly_size  Polynomial width (default: Bits32).
    /// @param rev_in     Input bit-reversal mode (default: None).
    /// @param rev_out    true = reverse output bits (used by CRC-32/ISO-HDLC).
    static void configure(PolySize    poly_size = PolySize::Bits32,
                          BitReversal rev_in    = BitReversal::None,
                          bool        rev_out   = false) noexcept {
        reg(detail::kOfsCr) =
            (static_cast<std::uint32_t>(poly_size) << detail::kCrPolySizeShift) |
            (static_cast<std::uint32_t>(rev_in)    << detail::kCrRevInShift)    |
            (rev_out ? detail::kCrRevOut : 0u);
    }

    /// Set the seed / initial value (written to the INIT register).
    ///
    /// Default after power-on: 0xFFFF_FFFF (standard CRC-32 seed).
    /// Takes effect after the next `reset()`.
    static void set_init(std::uint32_t init) noexcept {
        reg(detail::kOfsInit) = init;
    }

    /// Set a custom polynomial (written to the POL register).
    ///
    /// Only available on G0 / G4 / H7 / L4 — harmless write on F1/F2.
    /// Default: 0x04C1_1DB7 (IEEE 802.3 CRC-32 polynomial).
    static void set_polynomial(std::uint32_t poly) noexcept {
        reg(detail::kOfsPol) = poly;
    }

    // -----------------------------------------------------------------------
    // Accumulator reset
    // -----------------------------------------------------------------------

    /// Reset the CRC accumulator to the current INIT value.
    ///
    /// Must be called before starting a new CRC computation.
    /// The reset bit self-clears; no polling required.
    static void reset() noexcept {
        reg(detail::kOfsCr) |= detail::kCrReset;
    }

    // -----------------------------------------------------------------------
    // Data feed
    // -----------------------------------------------------------------------

    /// Feed one 32-bit word into the accumulator.
    static void feed(std::uint32_t word) noexcept {
        reg(detail::kOfsDr) = word;
    }

    /// Feed one 16-bit half-word.
    static void feed(std::uint16_t hw) noexcept {
        dr16() = hw;
    }

    /// Feed one byte.
    static void feed(std::uint8_t b) noexcept {
        dr8() = b;
    }

    /// Feed a span of bytes (one byte at a time).
    ///
    /// For maximum throughput on 4-byte-aligned buffers, cast to
    /// `span<const uint32_t>` and call the 32-bit overload instead.
    static void feed(std::span<const std::uint8_t> data) noexcept {
        for (const auto b : data) { dr8() = b; }
    }

    /// Feed a span of 32-bit words (most efficient path).
    static void feed(std::span<const std::uint32_t> data) noexcept {
        for (const auto w : data) { reg(detail::kOfsDr) = w; }
    }

    /// Feed a span of 16-bit half-words.
    static void feed(std::span<const std::uint16_t> data) noexcept {
        for (const auto hw : data) { dr16() = hw; }
    }

    // -----------------------------------------------------------------------
    // Result
    // -----------------------------------------------------------------------

    /// Read the current CRC accumulator value (DR).
    ///
    /// Reading DR does NOT reset the accumulator — call `reset()` explicitly
    /// before starting a new computation.
    [[nodiscard]] static auto result() noexcept -> std::uint32_t {
        return reg(detail::kOfsDr);
    }

    // -----------------------------------------------------------------------
    // Scratch register (IDR)
    // -----------------------------------------------------------------------

    /// Write a byte to the independent data register (IDR).
    ///
    /// IDR is an 8-bit scratch register not connected to the CRC pipeline.
    /// It survives `reset()` and can store a counter, context tag, or debug
    /// marker between CRC computations.
    static void set_idr(std::uint8_t val) noexcept {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<volatile std::uint8_t*>(CrcBase + detail::kOfsIdr) = val;
    }

    /// Read the independent data register.
    [[nodiscard]] static auto get_idr() noexcept -> std::uint8_t {
        return static_cast<std::uint8_t>(reg(detail::kOfsIdr) & 0xFFu);
    }

    engine() = delete;
};

}  // namespace alloy::hal::crc::lite
