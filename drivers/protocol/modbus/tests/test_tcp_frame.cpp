#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/pdu.hpp"
#include "alloy/modbus/tcp_frame.hpp"

using namespace alloy::modbus;

// Published MBAP reference frame (Modbus Application Protocol spec, section 4.1):
//   Transaction ID = 0x0001
//   Protocol ID   = 0x0000
//   Length        = 0x0006  (unit_id + 5 PDU bytes)
//   Unit ID       = 0x11
//   PDU           = FC03 read 2 registers at 0x006B
//     [0x03][0x00][0x6B][0x00][0x02]
static constexpr std::array<std::byte, 12u> kRefAdu{
    std::byte{0x00u}, std::byte{0x01u},  // tx_id
    std::byte{0x00u}, std::byte{0x00u},  // proto
    std::byte{0x00u}, std::byte{0x06u},  // length
    std::byte{0x11u},                    // unit_id
    std::byte{0x03u}, std::byte{0x00u}, std::byte{0x6Bu},
    std::byte{0x00u}, std::byte{0x02u},  // FC03 PDU
};

// ============================================================================
// Decode reference frame
// ============================================================================

TEST_CASE("tcp_frame: decode published MBAP reference frame") {
    const auto res = decode_tcp_frame(kRefAdu);
    REQUIRE(res.is_ok());
    const auto& f = res.unwrap();

    CHECK(f.transaction_id == 0x0001u);
    CHECK(f.unit_id        == 0x11u);
    REQUIRE(f.pdu.size()   == 5u);
    CHECK(f.pdu[0] == std::byte{0x03u});
    CHECK(f.pdu[1] == std::byte{0x00u});
    CHECK(f.pdu[2] == std::byte{0x6Bu});
    CHECK(f.pdu[3] == std::byte{0x00u});
    CHECK(f.pdu[4] == std::byte{0x02u});
}

// ============================================================================
// Encode reference frame
// ============================================================================

TEST_CASE("tcp_frame: encode matches reference ADU") {
    static constexpr std::array<std::byte, 5u> pdu{
        std::byte{0x03u}, std::byte{0x00u}, std::byte{0x6Bu},
        std::byte{0x00u}, std::byte{0x02u},
    };

    std::array<std::byte, kTcpMaxAduBytes> out{};
    const auto res = encode_tcp_frame(
        out, 0x0001u, 0x11u,
        std::span<const std::byte>{pdu.data(), pdu.size()});
    REQUIRE(res.is_ok());
    REQUIRE(res.unwrap() == 12u);

    for (std::size_t i = 0u; i < 12u; ++i) {
        CHECK(out[i] == kRefAdu[i]);
    }
}

// ============================================================================
// Round-trip: encode → decode
// ============================================================================

TEST_CASE("tcp_frame: round-trip transaction_id preserved") {
    static constexpr std::array<std::byte, 3u> pdu{
        std::byte{0x06u}, std::byte{0x00u}, std::byte{0x01u},
    };
    std::array<std::byte, kTcpMaxAduBytes> buf{};

    const auto enc = encode_tcp_frame(
        buf, 0xABCDu, 0x02u,
        std::span<const std::byte>{pdu.data(), pdu.size()});
    REQUIRE(enc.is_ok());

    const auto dec = decode_tcp_frame(
        std::span<const std::byte>{buf.data(), enc.unwrap()});
    REQUIRE(dec.is_ok());

    CHECK(dec.unwrap().transaction_id == 0xABCDu);
    CHECK(dec.unwrap().unit_id        == 0x02u);
    REQUIRE(dec.unwrap().pdu.size()   == 3u);
    CHECK(dec.unwrap().pdu[0] == std::byte{0x06u});
}

TEST_CASE("tcp_frame: round-trip max-size PDU (253 bytes)") {
    std::array<std::byte, kTcpMaxPduBytes> pdu{};
    for (std::size_t i = 0u; i < pdu.size(); ++i) {
        pdu[i] = std::byte{static_cast<std::uint8_t>(i & 0xFFu)};
    }

    std::array<std::byte, kTcpMaxAduBytes> buf{};
    const auto enc = encode_tcp_frame(
        buf, 0x0042u, 0x01u,
        std::span<const std::byte>{pdu.data(), pdu.size()});
    REQUIRE(enc.is_ok());
    REQUIRE(enc.unwrap() == kTcpMaxAduBytes);

    const auto dec = decode_tcp_frame(
        std::span<const std::byte>{buf.data(), enc.unwrap()});
    REQUIRE(dec.is_ok());
    REQUIRE(dec.unwrap().pdu.size() == kTcpMaxPduBytes);
    CHECK(dec.unwrap().pdu[0]   == std::byte{0x00u});
    CHECK(dec.unwrap().pdu[252] == std::byte{static_cast<std::uint8_t>(252u & 0xFFu)});
}

TEST_CASE("tcp_frame: round-trip empty PDU") {
    std::array<std::byte, kTcpMaxAduBytes> buf{};
    const auto enc = encode_tcp_frame(
        buf, 0x0000u, 0x00u,
        std::span<const std::byte>{});
    REQUIRE(enc.is_ok());
    REQUIRE(enc.unwrap() == kMbapHeaderBytes);

    const auto dec = decode_tcp_frame(
        std::span<const std::byte>{buf.data(), enc.unwrap()});
    REQUIRE(dec.is_ok());
    CHECK(dec.unwrap().pdu.empty());
}

// ============================================================================
// Protocol field validation
// ============================================================================

TEST_CASE("tcp_frame: decode rejects non-zero protocol id") {
    std::array<std::byte, 12u> bad = kRefAdu;
    bad[2] = std::byte{0x00u};
    bad[3] = std::byte{0x01u};  // proto = 0x0001
    const auto res = decode_tcp_frame(bad);
    CHECK(res.is_err());
}

TEST_CASE("tcp_frame: decode rejects ADU shorter than 7 bytes") {
    std::array<std::byte, 6u> short_adu{};
    const auto res = decode_tcp_frame(short_adu);
    CHECK(res.is_err());
}

TEST_CASE("tcp_frame: decode rejects when declared length exceeds actual data") {
    std::array<std::byte, 8u> trunc{
        std::byte{0x00u}, std::byte{0x01u},
        std::byte{0x00u}, std::byte{0x00u},
        std::byte{0x00u}, std::byte{0x0Au},  // length=10 → needs 9 PDU bytes
        std::byte{0x01u},
        std::byte{0x03u},                    // only 1 PDU byte present
    };
    const auto res = decode_tcp_frame(trunc);
    CHECK(res.is_err());
}

// ============================================================================
// Encode error paths
// ============================================================================

TEST_CASE("tcp_frame: encode returns BufferTooSmall when out too small") {
    static constexpr std::array<std::byte, 3u> pdu{
        std::byte{0x03u}, std::byte{0x00u}, std::byte{0x01u}};
    std::array<std::byte, 5u> tiny{};  // 5 < 7 + 3 = 10
    const auto res = encode_tcp_frame(
        tiny, 0x0001u, 0x01u,
        std::span<const std::byte>{pdu.data(), pdu.size()});
    CHECK(res.is_err());
}

TEST_CASE("tcp_frame: encode returns PduTooLarge for PDU > 253 bytes") {
    std::array<std::byte, 254u> oversized_pdu{};
    std::array<std::byte, kTcpMaxAduBytes + 10u> out{};
    const auto res = encode_tcp_frame(
        out, 0x0001u, 0x01u,
        std::span<const std::byte>{oversized_pdu.data(), oversized_pdu.size()});
    CHECK(res.is_err());
}

// ============================================================================
// PDU subspan is zero-copy (points into input buffer)
// ============================================================================

TEST_CASE("tcp_frame: decoded pdu subspan points into original buffer") {
    std::array<std::byte, kTcpMaxAduBytes> buf{};
    static constexpr std::array<std::byte, 2u> pdu{
        std::byte{0x01u}, std::byte{0x02u}};
    const auto enc = encode_tcp_frame(
        buf, 0x0001u, 0x01u,
        std::span<const std::byte>{pdu.data(), pdu.size()});
    REQUIRE(enc.is_ok());

    const auto dec = decode_tcp_frame(
        std::span<const std::byte>{buf.data(), enc.unwrap()});
    REQUIRE(dec.is_ok());
    // pdu must point into buf, not a copy
    CHECK(dec.unwrap().pdu.data() == buf.data() + kMbapHeaderBytes);
}

// ============================================================================
// Integration: encode FC03 request + decode → use as Modbus PDU
// ============================================================================

TEST_CASE("tcp_frame: FC03 request ADU encodes and decodes correctly") {
    // Build FC03 PDU
    std::array<std::byte, kMaxPduBytes> pdu_buf{};
    const auto pdu_enc = encode_read_request(
        pdu_buf, FunctionCode::ReadHoldingRegisters,
        ReadRequest{.address = 0x006Bu, .quantity = 2u});
    REQUIRE(pdu_enc.is_ok());
    const std::size_t pdu_len = pdu_enc.unwrap();

    // Wrap in TCP ADU
    std::array<std::byte, kTcpMaxAduBytes> adu_buf{};
    const auto adu_enc = encode_tcp_frame(
        adu_buf, 0x0001u, 0x11u,
        std::span<const std::byte>{pdu_buf.data(), pdu_len});
    REQUIRE(adu_enc.is_ok());

    // Decode
    const auto adu_dec = decode_tcp_frame(
        std::span<const std::byte>{adu_buf.data(), adu_enc.unwrap()});
    REQUIRE(adu_dec.is_ok());

    const auto& frame = adu_dec.unwrap();
    CHECK(frame.transaction_id == 0x0001u);
    CHECK(frame.unit_id        == 0x11u);

    // Decode the inner PDU
    const auto req_dec = decode_read_request(frame.pdu);
    REQUIRE(req_dec.is_ok());
    CHECK(req_dec.unwrap().address  == 0x006Bu);
    CHECK(req_dec.unwrap().quantity == 2u);
}
