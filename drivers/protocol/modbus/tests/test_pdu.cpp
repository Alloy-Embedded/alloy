// Modbus PDU codec tests. Pure host. No hardware. Reference frames come from the
// Modbus Application Protocol Specification v1.1b3 examples (section 6).

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/pdu.hpp"

using namespace alloy::modbus;

namespace {

// std::byte literal helper to keep test data compact.
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
// Helpers
// ============================================================================

TEST_CASE("is_supported_function recognises the implemented function codes",
          "[modbus][pdu]") {
    for (std::uint8_t fc : {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10, 0x17}) {
        CHECK(is_supported_function(fc));
    }
    for (std::uint8_t fc : {0x00, 0x07, 0x08, 0x14, 0x18, 0x65}) {
        CHECK_FALSE(is_supported_function(fc));
    }
}

TEST_CASE("peek_function_code rejects empty and unknown PDUs", "[modbus][pdu]") {
    std::array<std::byte, 0> empty{};
    CHECK(peek_function_code(empty).is_err());

    std::array<std::byte, 1> bogus{b(0x42)};
    CHECK(peek_function_code(bogus).is_err());

    std::array<std::byte, 1> good{b(0x03)};
    auto ok = peek_function_code(good);
    REQUIRE(ok.is_ok());
    CHECK(ok.unwrap() == FunctionCode::ReadHoldingRegisters);
}

// ============================================================================
// FC 0x03 Read Holding Registers
// ============================================================================

TEST_CASE("encode FC 0x03 request matches reference bytes", "[modbus][pdu]") {
    // Spec example: read 3 holding registers starting at 0x006B
    //   03 00 6B 00 03
    std::array<std::byte, 5> out{};
    auto r = encode_read_request(out, FunctionCode::ReadHoldingRegisters, {0x006B, 3});
    REQUIRE(r.is_ok());
    CHECK(r.unwrap() == 5);
    auto expected = bytes<5>({0x03, 0x00, 0x6B, 0x00, 0x03});
    CHECK(out == expected);
}

TEST_CASE("decode FC 0x03 request round-trips", "[modbus][pdu]") {
    auto pdu = bytes<5>({0x03, 0x00, 0x6B, 0x00, 0x03});
    auto r = decode_read_request(pdu);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().address == 0x006B);
    CHECK(r.unwrap().quantity == 3);
}

TEST_CASE("decode FC 0x03 response with 6 bytes", "[modbus][pdu]") {
    // 03 06 02 2B 00 00 00 64
    auto pdu = bytes<8>({0x03, 0x06, 0x02, 0x2B, 0x00, 0x00, 0x00, 0x64});
    auto r = decode_read_registers_response(pdu);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().register_bytes.size() == 6);
}

TEST_CASE("encode FC 0x03 response matches reference bytes", "[modbus][pdu]") {
    auto regs = bytes<6>({0x02, 0x2B, 0x00, 0x00, 0x00, 0x64});
    std::array<std::byte, 8> out{};
    auto r = encode_read_registers_response(out, FunctionCode::ReadHoldingRegisters, regs);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap() == 8);
    auto expected = bytes<8>({0x03, 0x06, 0x02, 0x2B, 0x00, 0x00, 0x00, 0x64});
    CHECK(out == expected);
}

TEST_CASE("FC 0x03 quantity bounds enforced on encode and decode", "[modbus][pdu]") {
    std::array<std::byte, 5> out{};
    CHECK(encode_read_request(out, FunctionCode::ReadHoldingRegisters, {0, 0}).is_err());
    CHECK(encode_read_request(out, FunctionCode::ReadHoldingRegisters, {0, 126}).is_err());
    CHECK(encode_read_request(out, FunctionCode::ReadHoldingRegisters, {0, 125}).is_ok());

    auto over = bytes<5>({0x03, 0x00, 0x00, 0x00, 0x7E});  // qty=126
    CHECK(decode_read_request(over).is_err());
    auto zero = bytes<5>({0x03, 0x00, 0x00, 0x00, 0x00});
    CHECK(decode_read_request(zero).is_err());
}

// ============================================================================
// FC 0x01 Read Coils + 0x02 Read Discrete Inputs
// ============================================================================

TEST_CASE("FC 0x01 read coils request and response", "[modbus][pdu]") {
    // request: read 19 coils starting at 0x0013
    auto req_pdu = bytes<5>({0x01, 0x00, 0x13, 0x00, 0x13});
    auto req = decode_read_request(req_pdu);
    REQUIRE(req.is_ok());
    CHECK(req.unwrap().quantity == 19);

    // response: byte_count = ceil(19/8) = 3, packed bits
    auto resp_pdu = bytes<5>({0x01, 0x03, 0xCD, 0x6B, 0x05});
    auto resp = decode_read_coils_response(resp_pdu);
    REQUIRE(resp.is_ok());
    CHECK(resp.unwrap().packed_bits.size() == 3);
}

TEST_CASE("FC 0x01 honours coil-quantity cap of 2000", "[modbus][pdu]") {
    std::array<std::byte, 5> out{};
    CHECK(encode_read_request(out, FunctionCode::ReadCoils, {0, 2000}).is_ok());
    CHECK(encode_read_request(out, FunctionCode::ReadCoils, {0, 2001}).is_err());
}

// ============================================================================
// FC 0x05 Write Single Coil
// ============================================================================

TEST_CASE("FC 0x05 write single coil ON", "[modbus][pdu]") {
    auto pdu = bytes<5>({0x05, 0x00, 0xAC, 0xFF, 0x00});
    auto r = decode_write_single_coil_request(pdu);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().address == 0x00AC);
    CHECK(r.unwrap().value == true);

    std::array<std::byte, 5> out{};
    auto e = encode_write_single_coil_request(out, {0x00AC, true});
    REQUIRE(e.is_ok());
    CHECK(out == pdu);
}

TEST_CASE("FC 0x05 rejects non-0xFF00/0x0000 values", "[modbus][pdu]") {
    auto pdu = bytes<5>({0x05, 0x00, 0xAC, 0x12, 0x34});
    CHECK(decode_write_single_coil_request(pdu).is_err());
}

// ============================================================================
// FC 0x06 Write Single Register
// ============================================================================

TEST_CASE("FC 0x06 write single register round-trip", "[modbus][pdu]") {
    auto pdu = bytes<5>({0x06, 0x00, 0x01, 0x00, 0x03});
    auto r = decode_write_single_register_request(pdu);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().address == 0x0001);
    CHECK(r.unwrap().value == 0x0003);

    std::array<std::byte, 5> out{};
    REQUIRE(encode_write_single_register_request(out, {0x0001, 0x0003}).is_ok());
    CHECK(out == pdu);
}

// ============================================================================
// FC 0x10 Write Multiple Registers
// ============================================================================

TEST_CASE("FC 0x10 write multiple registers", "[modbus][pdu]") {
    // Spec example: write 2 registers starting at 0x0001 with values 0x000A, 0x0102
    //   10 00 01 00 02 04 00 0A 01 02
    auto pdu = bytes<10>({0x10, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x0A, 0x01, 0x02});
    auto r = decode_write_multiple_registers_request(pdu);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().address == 0x0001);
    CHECK(r.unwrap().quantity == 2);
    CHECK(r.unwrap().register_bytes.size() == 4);

    auto regs = bytes<4>({0x00, 0x0A, 0x01, 0x02});
    std::array<std::byte, 10> out{};
    REQUIRE(encode_write_multiple_registers_request(out, {0x0001, 2, regs}).is_ok());
    CHECK(out == pdu);
}

TEST_CASE("FC 0x10 byte-count and quantity must agree", "[modbus][pdu]") {
    // declared qty=2 but byte_count=2 (should be 4)
    auto bad = bytes<10>({0x10, 0x00, 0x01, 0x00, 0x02, 0x02, 0x00, 0x0A, 0x01, 0x02});
    CHECK(decode_write_multiple_registers_request(bad).is_err());
}

// ============================================================================
// FC 0x0F Write Multiple Coils
// ============================================================================

TEST_CASE("FC 0x0F write multiple coils round-trip", "[modbus][pdu]") {
    // Spec example: 10 coils starting at 0x0013, packed value 0xCD 0x01
    auto pdu = bytes<9>({0x0F, 0x00, 0x13, 0x00, 0x0A, 0x02, 0xCD, 0x01});
    auto r = decode_write_multiple_coils_request({pdu.data(), 8});
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().address == 0x0013);
    CHECK(r.unwrap().quantity == 10);
    CHECK(r.unwrap().packed_bits.size() == 2);
}

// ============================================================================
// FC 0x17 Read/Write Multiple Registers
// ============================================================================

TEST_CASE("FC 0x17 read/write multiple registers round-trip", "[modbus][pdu]") {
    // Spec example: read 6 starting 0x0003, write 3 starting 0x000E with values 0x00FF
    //   17 00 03 00 06 00 0E 00 03 06 00 FF 00 FF 00 FF
    auto pdu = bytes<16>(
        {0x17, 0x00, 0x03, 0x00, 0x06, 0x00, 0x0E, 0x00, 0x03, 0x06, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF});
    auto r = decode_read_write_multiple_registers_request(pdu);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().read_address == 0x0003);
    CHECK(r.unwrap().read_quantity == 6);
    CHECK(r.unwrap().write_address == 0x000E);
    CHECK(r.unwrap().write_quantity == 3);
    CHECK(r.unwrap().write_register_bytes.size() == 6);

    auto write_regs = bytes<6>({0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF});
    std::array<std::byte, 16> out{};
    REQUIRE(encode_read_write_multiple_registers_request(out,
            {0x0003, 6, 0x000E, 3, write_regs}).is_ok());
    CHECK(out == pdu);
}

// ============================================================================
// Exceptions
// ============================================================================

TEST_CASE("exception encode and decode round-trip", "[modbus][pdu]") {
    std::array<std::byte, 2> out{};
    auto r = encode_exception(out, FunctionCode::ReadHoldingRegisters,
                              ExceptionCode::IllegalDataAddress);
    REQUIRE(r.is_ok());
    CHECK(r.unwrap() == 2);
    CHECK(out[0] == b(0x83));
    CHECK(out[1] == b(0x02));

    auto d = decode_exception(out);
    REQUIRE(d.is_ok());
    CHECK(d.unwrap().function == FunctionCode::ReadHoldingRegisters);
    CHECK(d.unwrap().code == ExceptionCode::IllegalDataAddress);
}

TEST_CASE("is_exception detects high-bit FCs", "[modbus][pdu]") {
    std::array<std::byte, 2> exc{b(0x83), b(0x02)};
    std::array<std::byte, 5> normal{b(0x03), b(0x00), b(0x00), b(0x00), b(0x01)};
    CHECK(is_exception(exc));
    CHECK_FALSE(is_exception(normal));
}

TEST_CASE("decode_exception rejects non-exception PDUs", "[modbus][pdu]") {
    std::array<std::byte, 2> normal{b(0x03), b(0x02)};
    auto r = decode_exception(normal);
    REQUIRE(r.is_err());
}

// ============================================================================
// Truncation and buffer-too-small paths
// ============================================================================

TEST_CASE("decoders reject truncated PDUs", "[modbus][pdu]") {
    std::array<std::byte, 4> short_pdu{b(0x03), b(0x00), b(0x6B), b(0x00)};
    CHECK(decode_read_request(short_pdu).is_err());

    std::array<std::byte, 1> too_short_response{b(0x03)};
    CHECK(decode_read_registers_response(too_short_response).is_err());
}

TEST_CASE("encoders reject undersized output buffers", "[modbus][pdu]") {
    std::array<std::byte, 3> small{};
    CHECK(encode_read_request(small, FunctionCode::ReadHoldingRegisters, {0, 1}).is_err());
}
