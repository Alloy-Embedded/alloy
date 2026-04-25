// Modbus RTU framing tests.
// Reference frames from the Modbus Application Protocol Specification v1.1b3
// and the Modbus over Serial Line Specification v1.02.
// All CRC values verified against the standard algorithm.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/rtu_frame.hpp"
#include "alloy/modbus/transport/loopback_stream.hpp"

using namespace alloy::modbus;

namespace {

constexpr std::byte b(std::uint8_t v) { return std::byte{v}; }

template <std::size_t N>
constexpr std::array<std::byte, N> bytes(std::initializer_list<std::uint8_t> il) {
    std::array<std::byte, N> out{};
    std::size_t i = 0;
    for (auto v : il) out[i++] = std::byte{v};
    return out;
}

}  // namespace

// ============================================================================
// CRC-16 known-good values
// ============================================================================

TEST_CASE("crc16 empty span returns 0xFFFF") {
    CHECK(crc16({}) == 0xFFFFu);
}

TEST_CASE("crc16 single byte 0x01") {
    // 0x01 → table[0xFF ^ 0x01] XOR (0xFF >> 8) = table[0xFE] XOR 0x00
    // Verified externally: 0x807E
    const auto data = bytes<1>({0x01});
    CHECK(crc16(data) == 0x807Eu);
}

TEST_CASE("crc16 known Modbus frame: FC03 request slave 0x11 addr 0x006B qty 3") {
    // Modbus Application Protocol Specification v1.1b3 section 6.3 example.
    // Slave 0x11: frame = 11 03 00 6B 00 03 → CRC value 0x8776 (Lo=0x76, Hi=0x87)
    const auto frame = bytes<6>({0x11, 0x03, 0x00, 0x6B, 0x00, 0x03});
    CHECK(crc16(frame) == 0x8776u);
}

TEST_CASE("crc16 known Modbus frame: FC06 write single register slave 0x11") {
    // Slave 0x11: 11 06 00 01 00 03 → CRC 0x9B9A (Lo=0x9A, Hi=0x9B)
    const auto frame = bytes<6>({0x11, 0x06, 0x00, 0x01, 0x00, 0x03});
    CHECK(crc16(frame) == 0x9B9Au);
}

// ============================================================================
// silence_us
// ============================================================================

TEST_CASE("silence_us at 9600 baud") {
    // 3.5 × 11 / 9600 ≈ 4010 µs; ceiling division → 4011
    const auto us = silence_us(9600u);
    CHECK(us >= 4010u);
    CHECK(us <= 4013u);
}

TEST_CASE("silence_us at 19200 baud uses fixed 1750 µs") {
    CHECK(silence_us(19200u) == 1750u);
}

TEST_CASE("silence_us at 115200 baud uses fixed 1750 µs") {
    CHECK(silence_us(115200u) == 1750u);
}

// ============================================================================
// check_silence
// ============================================================================

TEST_CASE("check_silence passes when elapsed >= budget") {
    std::uint64_t fake_now = 2000u;
    auto now_fn = [&]() { return fake_now; };

    const auto res = check_silence(now_fn, /*last_byte_us=*/0u, /*budget_us=*/1750u);
    CHECK(res.is_ok());
}

TEST_CASE("check_silence fails when elapsed < budget") {
    std::uint64_t fake_now = 500u;
    auto now_fn = [&]() { return fake_now; };

    const auto res = check_silence(now_fn, /*last_byte_us=*/0u, /*budget_us=*/1750u);
    REQUIRE(res.is_err());
    CHECK(res.err() == RtuError::SilenceViolation);
}

TEST_CASE("check_silence exactly at budget passes") {
    std::uint64_t fake_now = 1750u;
    auto now_fn = [&]() { return fake_now; };

    const auto res = check_silence(now_fn, 0u, 1750u);
    CHECK(res.is_ok());
}

// ============================================================================
// encode_rtu_frame
// ============================================================================

TEST_CASE("encode_rtu_frame: FC03 request using spec slave 0x11") {
    // Spec example: slave 0x11, PDU = 03 00 6B 00 03 → frame CRC Lo=0x76, Hi=0x87
    const auto pdu = bytes<5>({0x03, 0x00, 0x6B, 0x00, 0x03});
    std::array<std::byte, 256> buf{};

    const auto res = encode_rtu_frame(buf, /*slave_id=*/0x11, pdu);
    REQUIRE(res.is_ok());
    CHECK(res.unwrap() == 8u);  // 1 id + 5 pdu + 2 crc

    CHECK(buf[0] == b(0x11));
    CHECK(buf[1] == b(0x03));
    CHECK(buf[5] == b(0x03));
    CHECK(buf[6] == b(0x76));  // CRC Lo
    CHECK(buf[7] == b(0x87));  // CRC Hi
}

TEST_CASE("encode_rtu_frame: buffer too small returns error") {
    const auto pdu = bytes<5>({0x03, 0x00, 0x6B, 0x00, 0x03});
    std::array<std::byte, 4> small{};

    const auto res = encode_rtu_frame(small, 0x01, pdu);
    REQUIRE(res.is_err());
    CHECK(res.unwrap_err() == RtuError::BufferTooSmall);
}

// ============================================================================
// decode_rtu_frame
// ============================================================================

TEST_CASE("decode_rtu_frame: valid FC03 request (spec example slave 0x11)") {
    // Spec frame: 11 03 00 6B 00 03 76 87
    const auto frame = bytes<8>({0x11, 0x03, 0x00, 0x6B, 0x00, 0x03, 0x76, 0x87});

    const auto res = decode_rtu_frame(frame);
    REQUIRE(res.is_ok());
    const auto& rf = res.unwrap();
    CHECK(rf.slave_id == 0x11u);
    REQUIRE(rf.pdu.size() == 5u);
    CHECK(rf.pdu[0] == b(0x03));
    CHECK(rf.pdu[1] == b(0x00));
    CHECK(rf.pdu[2] == b(0x6B));
}

TEST_CASE("decode_rtu_frame: valid FC06 write single register (spec example slave 0x11)") {
    // Spec frame: 11 06 00 01 00 03 9A 9B
    const auto frame = bytes<8>({0x11, 0x06, 0x00, 0x01, 0x00, 0x03, 0x9A, 0x9B});

    const auto res = decode_rtu_frame(frame);
    REQUIRE(res.is_ok());
    CHECK(res.unwrap().slave_id == 0x11u);
    CHECK(res.unwrap().pdu[0] == b(0x06));
}

TEST_CASE("decode_rtu_frame: frame too short returns error") {
    const auto frame = bytes<3>({0x01, 0x03, 0x00});
    const auto res = decode_rtu_frame(frame);
    REQUIRE(res.is_err());
    CHECK(res.unwrap_err() == RtuError::FrameTooShort);
}

TEST_CASE("decode_rtu_frame: CRC mismatch returns error") {
    // Corrupt the CRC bytes
    const auto frame = bytes<8>({0x01, 0x03, 0x00, 0x6B, 0x00, 0x03, 0xFF, 0xFF});
    const auto res = decode_rtu_frame(frame);
    REQUIRE(res.is_err());
    CHECK(res.unwrap_err() == RtuError::CrcMismatch);
}

TEST_CASE("decode_rtu_frame: single-byte PDU minimum frame") {
    // Minimum valid frame: id(1) + fc(1) + crc(2) = 4 bytes
    // Frame: 01 01 + CRC of [01 01]
    const auto hdr = bytes<2>({0x01, 0x01});
    const std::uint16_t crc = crc16(hdr);
    std::array<std::byte, 4> frame{};
    frame[0] = b(0x01);
    frame[1] = b(0x01);
    frame[2] = std::byte{static_cast<std::uint8_t>(crc & 0xFFu)};
    frame[3] = std::byte{static_cast<std::uint8_t>(crc >> 8u)};

    const auto res = decode_rtu_frame(frame);
    REQUIRE(res.is_ok());
    CHECK(res.unwrap().pdu.size() == 1u);
}

// ============================================================================
// encode → decode round-trip
// ============================================================================

TEST_CASE("RTU encode/decode round-trip preserves PDU and slave_id") {
    const auto pdu = bytes<6>({0x03, 0x00, 0x00, 0x00, 0x0A, 0x00});
    std::array<std::byte, 256> buf{};

    const auto enc = encode_rtu_frame(buf, /*slave_id=*/0x05, pdu);
    REQUIRE(enc.is_ok());

    const auto dec = decode_rtu_frame(std::span<const std::byte>{buf.data(), enc.unwrap()});
    REQUIRE(dec.is_ok());
    CHECK(dec.unwrap().slave_id == 0x05u);
    REQUIRE(dec.unwrap().pdu.size() == pdu.size());
    for (std::size_t i = 0; i < pdu.size(); ++i) {
        CHECK(dec.unwrap().pdu[i] == pdu[i]);
    }
}

// ============================================================================
// LoopbackPair / byte_stream
// ============================================================================

TEST_CASE("LoopbackPair: write on a reads on b") {
    LoopbackPair<> pair;
    auto ep_a = pair.a();
    auto ep_b = pair.b();

    const auto data = bytes<4>({0xDE, 0xAD, 0xBE, 0xEF});
    REQUIRE(ep_a.write(data).is_ok());

    std::array<std::byte, 4> recv{};
    const auto n = ep_b.read(recv, 0u);
    REQUIRE(n.is_ok());
    CHECK(n.unwrap() == 4u);
    CHECK(recv[0] == b(0xDE));
    CHECK(recv[3] == b(0xEF));
}

TEST_CASE("LoopbackPair: write on b reads on a") {
    LoopbackPair<> pair;
    auto ep_a = pair.a();
    auto ep_b = pair.b();

    const auto data = bytes<3>({0x01, 0x02, 0x03});
    REQUIRE(ep_b.write(data).is_ok());

    std::array<std::byte, 8> recv{};
    const auto n = ep_a.read(recv, 0u);
    REQUIRE(n.is_ok());
    CHECK(n.unwrap() == 3u);
    CHECK(recv[0] == b(0x01));
    CHECK(recv[2] == b(0x03));
}

TEST_CASE("LoopbackPair: bidirectional in same test") {
    LoopbackPair<> pair;
    auto a = pair.a();
    auto b = pair.b();

    const auto req = bytes<4>({0x01, 0x03, 0x00, 0x01});
    const auto rsp = bytes<3>({0x01, 0x03, 0x02});

    REQUIRE(a.write(req).is_ok());
    REQUIRE(b.write(rsp).is_ok());

    std::array<std::byte, 8> buf{};
    CHECK(b.read(buf, 0u).unwrap() == 4u);
    CHECK(a.read(buf, 0u).unwrap() == 3u);
}

TEST_CASE("LoopbackPair: overrun returns error when buffer is full") {
    LoopbackPair<4> pair;  // tiny 4-byte buffer
    auto ep_a = pair.a();

    const auto big = bytes<8>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
    const auto res = ep_a.write(big);
    REQUIRE(res.is_err());
    CHECK(res.err() == StreamError::Overrun);
}

TEST_CASE("LoopbackPair: read returns 0 when empty") {
    LoopbackPair<> pair;
    auto ep_b = pair.b();

    std::array<std::byte, 8> buf{};
    const auto n = ep_b.read(buf, 0u);
    REQUIRE(n.is_ok());
    CHECK(n.unwrap() == 0u);
}

TEST_CASE("LoopbackPair: full RTU frame end-to-end over loopback") {
    LoopbackPair<> pair;
    auto tx = pair.a();
    auto rx = pair.b();

    // Encode an RTU frame on the TX side
    const auto pdu = bytes<5>({0x03, 0x00, 0x6B, 0x00, 0x03});
    std::array<std::byte, 256> enc_buf{};
    const auto n_enc = encode_rtu_frame(enc_buf, 0x01, pdu);
    REQUIRE(n_enc.is_ok());

    REQUIRE(tx.write(std::span<const std::byte>{enc_buf.data(), n_enc.unwrap()}).is_ok());

    // Receive and decode on the RX side
    std::array<std::byte, 256> dec_buf{};
    const auto n_recv = rx.read(dec_buf, 0u);
    REQUIRE(n_recv.is_ok());
    CHECK(n_recv.unwrap() == n_enc.unwrap());

    const auto frame = decode_rtu_frame(std::span<const std::byte>{dec_buf.data(), n_recv.unwrap()});
    REQUIRE(frame.is_ok());
    CHECK(frame.unwrap().slave_id == 0x01u);
    CHECK(frame.unwrap().pdu.size() == 5u);
    CHECK(frame.unwrap().pdu[0] == b(0x03));
}
