// PDU codecs: byte-exact wire layouts against the spec's worked example,
// build→parse round-trips across the client/server boundary for every
// function code, bounds rejection on every parser ("limit before trusting"),
// and the length-prediction table the framer's primary rule runs on —
// including FC04, the table-stakes function the reference C library could
// not carry over RTU at all.

#include "modbus/pdu.hpp"

#include <cstdint>

#include "alloy_test.hpp"
#include "modbus/crc.hpp"

namespace {
using namespace alloy::lib::modbus;
}

ALLOY_TEST(modbus_pdu_read_request_matches_the_spec_example_byte_for_byte) {
    // PDU for "read 3 holding registers at 0x006B"; with unit 0x11 + CRC it
    // must reproduce the spec's canonical ADU 11 03 00 6B 00 03 76 87.
    std::uint8_t adu[8] = {0x11};
    const auto n = build_read_request(function::read_holding_registers, 0x006B, 3,
                                      {adu + 1, 7});
    ALLOY_CHECK(n.has_value());
    ALLOY_CHECK_EQ(*n, 5u);
    const std::size_t total = append_crc(adu, 1 + *n);
    ALLOY_CHECK_EQ(total, 8u);
    const std::uint8_t expect[8] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03, 0x76, 0x87};
    bool same = true;
    for (std::size_t i = 0; i < 8; ++i) {
        same = same && adu[i] == expect[i];
    }
    ALLOY_CHECK(same);
}

ALLOY_TEST(modbus_pdu_read_request_refuses_zero_overask_and_short_buffers) {
    std::uint8_t out[5];
    ALLOY_CHECK(!build_read_request(function::read_holding_registers, 0, 0, out));
    ALLOY_CHECK(!build_read_request(function::read_holding_registers, 0,
                                    max_read_registers + 1, out));
    ALLOY_CHECK(build_read_request(function::read_coils, 0, max_read_bits, out)
                    .has_value());  // bits ceiling is the bit one, not 125

    std::uint8_t tiny[4];
    const auto r = build_read_request(function::read_holding_registers, 0, 1, tiny);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::too_large);
}

ALLOY_TEST(modbus_pdu_write_single_coil_accepts_only_the_two_spec_values) {
    std::uint8_t out[5];
    ALLOY_CHECK(build_write_single(function::write_single_coil, 1, 0xFF00, out)
                    .has_value());
    ALLOY_CHECK(build_write_single(function::write_single_coil, 1, 0x0000, out)
                    .has_value());
    ALLOY_CHECK(!build_write_single(function::write_single_coil, 1, 0x0001, out));
    // FC06 carries any value.
    ALLOY_CHECK(build_write_single(function::write_single_register, 1, 0x1234, out)
                    .has_value());
}

ALLOY_TEST(modbus_pdu_write_registers_round_trips_client_to_server) {
    const std::uint16_t values[3] = {0x0102, 0xA0B0, 0xFFFF};
    std::uint8_t pdu[16];
    const auto n = build_write_registers_request(0x0010, values, pdu);
    ALLOY_CHECK(n.has_value());
    ALLOY_CHECK_EQ(*n, 6u + 6u);

    const auto req = parse_request({pdu, *n});
    ALLOY_CHECK(req.has_value());
    ALLOY_CHECK(req->fc == function::write_multiple_registers);
    ALLOY_CHECK_EQ(req->address, 0x0010u);
    ALLOY_CHECK_EQ(req->count, 3u);
    ALLOY_CHECK_EQ(req->payload.size(), 6u);
    ALLOY_CHECK_EQ(req->payload[0], 0x01u);  // big-endian on the wire
    ALLOY_CHECK_EQ(req->payload[1], 0x02u);
}

ALLOY_TEST(modbus_pdu_write_coils_round_trips_with_packed_bits) {
    const std::uint8_t packed[2] = {0b1100'1101, 0b0000'0001};  // 9 coils
    std::uint8_t pdu[16];
    const auto n = build_write_coils_request(0x0013, 9, packed, pdu);
    ALLOY_CHECK(n.has_value());

    const auto req = parse_request({pdu, *n});
    ALLOY_CHECK(req.has_value());
    ALLOY_CHECK_EQ(req->count, 9u);
    ALLOY_CHECK_EQ(req->payload.size(), 2u);

    // A packed span that disagrees with the count is the caller's bug.
    ALLOY_CHECK(!build_write_coils_request(0, 20, packed, pdu));
}

ALLOY_TEST(modbus_pdu_parse_request_answers_unknown_fc_with_the_wire_exception) {
    const std::uint8_t vendor[5] = {0x41, 0x00, 0x00, 0x00, 0x01};
    const auto r = parse_request(vendor);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::exception_illegal_function);

    // byte_count that disagrees with count: malformed, not an exception —
    // the frame itself is broken, a compliant server stays silent.
    const std::uint8_t lying[9] = {0x10, 0x00, 0x00, 0x00, 0x02, 0x02, 0xAA, 0xBB, 0xCC};
    const auto bad = parse_request(lying);
    ALLOY_CHECK(!bad);
    ALLOY_CHECK(bad.error() == modbus_error::malformed);
}

ALLOY_TEST(modbus_pdu_read_registers_response_round_trips_and_fc04_works) {
    // FC04 — unreachable over RTU in the reference C library; core here.
    const std::uint16_t regs[2] = {0xDEAD, 0xBEEF};
    std::uint8_t pdu[8];
    const auto n = build_read_registers_response(function::read_input_registers,
                                                 regs, pdu);
    ALLOY_CHECK(n.has_value());
    ALLOY_CHECK_EQ(*n, 6u);

    std::uint16_t out[2] = {};
    const auto got = parse_read_registers_response({pdu, *n},
                                                   function::read_input_registers, out);
    ALLOY_CHECK(got.has_value());
    ALLOY_CHECK_EQ(*got, 2u);
    ALLOY_CHECK_EQ(out[0], 0xDEADu);
    ALLOY_CHECK_EQ(out[1], 0xBEEFu);
}

ALLOY_TEST(modbus_pdu_response_parsers_enforce_correlation_and_bounds) {
    const std::uint16_t regs[2] = {1, 2};
    std::uint8_t pdu[8];
    const auto n = build_read_registers_response(function::read_holding_registers,
                                                 regs, pdu);
    ALLOY_CHECK(n.has_value());

    // Wrong function answering: the correlation failure the reference C
    // library never checked for.
    std::uint16_t out[2] = {};
    const auto wrong = parse_read_registers_response(
        {pdu, *n}, function::read_input_registers, out);
    ALLOY_CHECK(!wrong);
    ALLOY_CHECK(wrong.error() == modbus_error::unexpected_function);

    // A server sending more than the caller's array holds cannot overrun it.
    std::uint16_t small[1] = {};
    const auto over = parse_read_registers_response(
        {pdu, *n}, function::read_holding_registers, small);
    ALLOY_CHECK(!over);
    ALLOY_CHECK(over.error() == modbus_error::too_large);

    // Odd byte_count is nonsense for registers.
    std::uint8_t odd[4] = {0x03, 0x01, 0xAA, 0x00};
    const auto bad = parse_read_registers_response(
        {odd, 3}, function::read_holding_registers, out);
    ALLOY_CHECK(!bad);
    ALLOY_CHECK(bad.error() == modbus_error::malformed);
}

ALLOY_TEST(modbus_pdu_exception_round_trips_with_its_code_intact) {
    std::uint8_t pdu[2];
    const auto n = build_exception(function::read_holding_registers,
                                   modbus_error::exception_illegal_data_address, pdu);
    ALLOY_CHECK(n.has_value());
    ALLOY_CHECK_EQ(pdu[0], 0x83u);  // fc | 0x80
    ALLOY_CHECK_EQ(pdu[1], 0x02u);

    std::uint16_t out[1] = {};
    const auto r = parse_read_registers_response({pdu, *n},
                                                 function::read_holding_registers, out);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(is_exception(r.error()));
    ALLOY_CHECK_EQ(exception_code(r.error()), 0x02u);

    // Building an exception from a LOCAL error is a dispatch-layer bug.
    ALLOY_CHECK(!build_exception(function::read_coils, modbus_error::timeout, pdu));
}

ALLOY_TEST(modbus_pdu_write_response_echo_must_match_the_request) {
    std::uint8_t pdu[5];
    const auto n = build_write_response(function::write_single_register, 0x0001,
                                        0x0003, pdu);
    ALLOY_CHECK(n.has_value());
    ALLOY_CHECK(parse_write_response({pdu, *n}, function::write_single_register,
                                     0x0001, 0x0003)
                    .has_value());
    const auto bad = parse_write_response({pdu, *n}, function::write_single_register,
                                          0x0001, 0x0004);
    ALLOY_CHECK(!bad);
    ALLOY_CHECK(bad.error() == modbus_error::malformed);
}

ALLOY_TEST(modbus_pdu_length_prediction_covers_both_directions) {
    // Requests (what a server's framer sees).
    const std::uint8_t rd[3] = {0x11, 0x03, 0x00};
    ALLOY_CHECK_EQ(expected_adu_length(direction::request, {rd, 2}), 8u);
    const std::uint8_t wr[7] = {0x11, 0x10, 0x00, 0x00, 0x00, 0x02, 0x04};
    ALLOY_CHECK_EQ(expected_adu_length(direction::request, {wr, 6}), 0u);  // need bc
    ALLOY_CHECK_EQ(expected_adu_length(direction::request, wr), 9u + 4u);

    // Responses (what a client's framer sees).
    const std::uint8_t resp[3] = {0x11, 0x04, 0x06};
    ALLOY_CHECK_EQ(expected_adu_length(direction::response, {resp, 2}), 0u);
    ALLOY_CHECK_EQ(expected_adu_length(direction::response, resp), 5u + 6u);
    const std::uint8_t exc[2] = {0x11, 0x83};
    ALLOY_CHECK_EQ(expected_adu_length(direction::response, exc), 5u);
    const std::uint8_t echo[2] = {0x11, 0x06};
    ALLOY_CHECK_EQ(expected_adu_length(direction::response, echo), 8u);

    // Unknown FC → silence framing; too-short → feed more.
    const std::uint8_t vendor[2] = {0x11, 0x41};
    ALLOY_CHECK_EQ(expected_adu_length(direction::request, vendor), length_unknown);
    ALLOY_CHECK_EQ(expected_adu_length(direction::request, {vendor, 1}), 0u);

    // The wedge input: FC16 claiming 0xFF data bytes predicts 264 — the
    // framer must compare that against its capacity and resync, never jam.
    const std::uint8_t wedge[7] = {0x11, 0x10, 0x00, 0x00, 0x00, 0x7F, 0xFF};
    ALLOY_CHECK_EQ(expected_adu_length(direction::request, wedge), 9u + 255u);
    ALLOY_CHECK(9u + 255u > max_adu);
}
