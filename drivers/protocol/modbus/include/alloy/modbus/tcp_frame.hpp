#pragma once

// tcp_frame.hpp: Modbus TCP MBAP header encode/decode.
//
// MBAP header layout (7 bytes):
//   [tx_id_hi][tx_id_lo]          — transaction identifier (echoed by slave)
//   [proto_hi][proto_lo]          — protocol id = 0x0000
//   [length_hi][length_lo]        — byte count of unit_id + PDU
//   [unit_id]                     — slave address (unit identifier)
// Followed by the PDU (up to 253 bytes per Modbus spec).
//
// Max ADU = 7 (MBAP) + 253 (PDU) = 260 bytes.
//
// Encoding (client/master side): encode_tcp_frame() → writes full ADU.
// Decoding (server/slave side):  decode_tcp_frame() → returns TcpFrame view
//   with .pdu pointing into the caller's buffer (zero-copy).
//
// No transport dependency; compiles unconditionally on any host.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/modbus/pdu.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

// ============================================================================
// Constants
// ============================================================================

constexpr std::uint16_t kTcpProtocolId     = 0x0000u;
constexpr std::size_t   kMbapHeaderBytes   = 7u;
constexpr std::size_t   kTcpMaxPduBytes    = 253u;
constexpr std::size_t   kTcpMaxAduBytes    = kMbapHeaderBytes + kTcpMaxPduBytes;

// ============================================================================
// TcpFrame: decoded view (zero-copy — pdu points into caller's buffer)
// ============================================================================

struct TcpFrame {
    std::uint16_t           transaction_id;
    std::uint8_t            unit_id;
    std::span<const std::byte> pdu;
};

// ============================================================================
// TcpError
// ============================================================================

enum class TcpError : std::uint8_t {
    BufferTooSmall,   // output buffer insufficient
    Truncated,        // input ADU shorter than MBAP header or declared length
    BadProtocol,      // protocol id != 0x0000
    PduTooLarge,      // PDU exceeds kTcpMaxPduBytes
};

// ============================================================================
// Encoding (client side)
// ============================================================================

// Encode a Modbus TCP ADU. Writes MBAP header + PDU into out.
// Returns the number of bytes written on success.
[[nodiscard]] inline core::Result<std::size_t, TcpError> encode_tcp_frame(
    std::span<std::byte>       out,
    std::uint16_t              transaction_id,
    std::uint8_t               unit_id,
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() > kTcpMaxPduBytes) {
        return core::Err(TcpError::PduTooLarge);
    }
    const std::size_t total = kMbapHeaderBytes + pdu.size();
    if (out.size() < total) {
        return core::Err(TcpError::BufferTooSmall);
    }

    // Length field = unit_id (1) + PDU bytes
    const auto length = static_cast<std::uint16_t>(1u + pdu.size());

    out[0] = std::byte{static_cast<std::uint8_t>(transaction_id >> 8u)};
    out[1] = std::byte{static_cast<std::uint8_t>(transaction_id & 0xFFu)};
    out[2] = std::byte{static_cast<std::uint8_t>(kTcpProtocolId >> 8u)};
    out[3] = std::byte{static_cast<std::uint8_t>(kTcpProtocolId & 0xFFu)};
    out[4] = std::byte{static_cast<std::uint8_t>(length >> 8u)};
    out[5] = std::byte{static_cast<std::uint8_t>(length & 0xFFu)};
    out[6] = std::byte{unit_id};

    for (std::size_t i = 0u; i < pdu.size(); ++i) {
        out[7u + i] = pdu[i];
    }

    return core::Ok(std::size_t{total});
}

// ============================================================================
// Decoding (server side)
// ============================================================================

// Decode a Modbus TCP ADU from adu. Returns a TcpFrame whose .pdu is a
// subspan of adu (zero-copy). adu must remain valid while TcpFrame is in use.
[[nodiscard]] inline core::Result<TcpFrame, TcpError> decode_tcp_frame(
    std::span<const std::byte> adu) noexcept {
    if (adu.size() < kMbapHeaderBytes) {
        return core::Err(TcpError::Truncated);
    }

    const std::uint16_t tx_id =
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(adu[0])) << 8u) |
         static_cast<std::uint8_t>(adu[1]);
    const std::uint16_t proto =
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(adu[2])) << 8u) |
         static_cast<std::uint8_t>(adu[3]);
    const std::uint16_t length =
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(adu[4])) << 8u) |
         static_cast<std::uint8_t>(adu[5]);
    const std::uint8_t unit_id = static_cast<std::uint8_t>(adu[6]);

    if (proto != kTcpProtocolId) {
        return core::Err(TcpError::BadProtocol);
    }
    if (length < 1u) {
        return core::Err(TcpError::Truncated);
    }
    const std::size_t pdu_len = static_cast<std::size_t>(length) - 1u;
    if (pdu_len > kTcpMaxPduBytes) {
        return core::Err(TcpError::PduTooLarge);
    }
    if (adu.size() < kMbapHeaderBytes + pdu_len) {
        return core::Err(TcpError::Truncated);
    }

    return core::Ok(TcpFrame{
        .transaction_id = tx_id,
        .unit_id        = unit_id,
        .pdu            = adu.subspan(kMbapHeaderBytes, pdu_len),
    });
}

}  // namespace alloy::modbus
