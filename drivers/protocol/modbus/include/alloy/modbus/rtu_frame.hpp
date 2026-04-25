#pragma once

// Modbus RTU framing: CRC-16, inter-frame silence, encode/decode.
//
// RTU frame layout: [slave_id(1)] [PDU(1..253)] [CRC_Lo(1)] [CRC_Hi(1)]
//
// CRC-16/IBM (Modbus): polynomial 0xA001 (reflected 0x8005), init 0xFFFF,
// no final XOR. Transmitted as Lo byte then Hi byte at the end of the frame.
//
// Inter-frame silence: Modbus requires 3.5 character times between frames.
// At baud >= 19200 the spec fixes this at 1750 µs; below 19200 it is computed
// as ceil(3.5 × 11 / baud) seconds.
//
// The `check_silence` template takes a microsecond `now_us()` callable so that
// tests can inject a deterministic clock without touching hardware.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace alloy::modbus {

// Minimum RTU frame: slave_id + 1 PDU byte + 2 CRC bytes.
inline constexpr std::size_t kRtuMinFrameBytes = 4u;
// Maximum RTU frame: slave_id + 253-byte PDU + 2 CRC bytes.
inline constexpr std::size_t kRtuMaxFrameBytes = 256u;

enum class RtuError : std::uint8_t {
    BufferTooSmall,    // output buffer cannot hold the frame
    FrameTooShort,     // received bytes < kRtuMinFrameBytes
    CrcMismatch,       // CRC in frame does not match computed CRC
    SilenceViolation,  // inter-frame gap too short
};

// Compute inter-frame silence budget in microseconds for a given baud rate.
// Modbus spec: >= 19200 baud → fixed 1750 µs; < 19200 → 3.5 × 11 bit-times.
[[nodiscard]] constexpr std::uint32_t silence_us(std::uint32_t baud) noexcept {
    if (baud >= 19200u) {
        return 1750u;
    }
    // 3.5 char-times × 11 bits/char ÷ baud = 38.5 / baud seconds → µs
    return (38'500'000u + baud - 1u) / baud;  // ceiling division
}

// CRC-16/IBM (Modbus). Uses a precomputed 256-entry table.
[[nodiscard]] std::uint16_t crc16(std::span<const std::byte> data) noexcept;

// Parsed RTU frame — views into the original buffer; no allocation.
struct RtuFrame {
    std::uint8_t slave_id;
    std::span<const std::byte> pdu;  // does not include slave_id or CRC bytes
};

// Encode an RTU frame into `out`: [slave_id][pdu][crc_lo][crc_hi].
// Returns the number of bytes written, or BufferTooSmall if out is too small.
[[nodiscard]] core::Result<std::size_t, RtuError> encode_rtu_frame(
    std::span<std::byte> out, std::uint8_t slave_id,
    std::span<const std::byte> pdu) noexcept;

// Decode and validate an RTU frame from a received byte buffer.
// Returns Err(FrameTooShort) for < 4 bytes, Err(CrcMismatch) on bad CRC.
// On success, RtuFrame::pdu is a subspan of `frame` (no copy).
[[nodiscard]] core::Result<RtuFrame, RtuError> decode_rtu_frame(
    std::span<const std::byte> frame) noexcept;

// Inter-frame silence check. Call before accepting a new frame start.
// `now_us`        — callable returning current µs timestamp (uint64_t or uint32_t).
// `last_byte_us`  — timestamp of the last received byte.
// `budget_us`     — required silence in µs (use silence_us(baud)).
// Returns Err(SilenceViolation) if elapsed time < budget.
template <typename NowFn>
[[nodiscard]] core::Result<void, RtuError> check_silence(
    NowFn now_us, std::uint64_t last_byte_us, std::uint32_t budget_us) noexcept {
    const auto now = static_cast<std::uint64_t>(now_us());
    const auto elapsed = now >= last_byte_us ? (now - last_byte_us) : 0u;
    if (elapsed < static_cast<std::uint64_t>(budget_us)) {
        return core::Err(RtuError::SilenceViolation);
    }
    return core::Ok();
}

}  // namespace alloy::modbus
