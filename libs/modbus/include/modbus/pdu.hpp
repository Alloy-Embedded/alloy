// Modbus PDU codecs — the eight core function codes, all four quadrants
// (build/parse × request/response), so ONE header serves both roles: a client
// builds requests + parses responses, a server parses requests + builds
// responses. The reference C library shipped only the client half of three
// FCs and its server side had to hand-pack bytes; keeping all four quadrants
// beside each other is what lets the tests round-trip every layout.
//
// Every field on the wire is BIG-endian (hi byte first) — only the RTU CRC
// trailer (crc.hpp) is little-endian. Every parser bounds-checks before it
// trusts a length ("limit before trusting", the house rule), and returns
// modbus_error::malformed instead of reading past the span.
//
// A "PDU" here is [fc | data...] — no unit id, no CRC. The RTU ADU wrapper
// (unit prefix + CRC trailer) belongs to the framer and client/server layers.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/util/result.hpp"
#include "modbus/error.hpp"

namespace alloy::lib::modbus {

inline constexpr std::size_t max_pdu = 253;  // spec: 256 ADU - unit - CRC
inline constexpr std::size_t max_adu = 256;  // RTU: unit + PDU + CRC

enum class function : std::uint8_t {
    read_coils = 0x01,
    read_discrete_inputs = 0x02,
    read_holding_registers = 0x03,
    read_input_registers = 0x04,
    write_single_coil = 0x05,
    write_single_register = 0x06,
    write_multiple_coils = 0x0F,
    write_multiple_registers = 0x10,
};

// Spec quantity ceilings (Modbus Application Protocol v1.1b3, §6).
inline constexpr std::uint16_t max_read_bits = 2000;
inline constexpr std::uint16_t max_read_registers = 125;
inline constexpr std::uint16_t max_write_bits = 1968;
inline constexpr std::uint16_t max_write_registers = 123;

[[nodiscard]] constexpr bool is_supported(std::uint8_t fc) {
    return (fc >= 0x01 && fc <= 0x06) || fc == 0x0F || fc == 0x10;
}
[[nodiscard]] constexpr bool reads_bits(function fc) {
    return fc == function::read_coils || fc == function::read_discrete_inputs;
}
[[nodiscard]] constexpr bool reads_registers(function fc) {
    return fc == function::read_holding_registers ||
           fc == function::read_input_registers;
}

namespace detail {
constexpr void put_u16(std::span<std::uint8_t> out, std::size_t at, std::uint16_t v) {
    out[at] = static_cast<std::uint8_t>(v >> 8);
    out[at + 1] = static_cast<std::uint8_t>(v & 0xFFu);
}
[[nodiscard]] constexpr std::uint16_t get_u16(std::span<const std::uint8_t> in,
                                              std::size_t at) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(in[at]) << 8 |
                                      in[at + 1]);
}
// Packed-bit byte count: ceil(bits / 8).
[[nodiscard]] constexpr std::size_t bit_bytes(std::uint16_t bits) {
    return (static_cast<std::size_t>(bits) + 7u) / 8u;
}
}  // namespace detail

// ── requests: client builds, server parses ──────────────────────────────────

// FC01/02/03/04 — [fc | addr u16 | count u16]. The count ceiling is the
// function's own (bits vs registers), checked here so an over-ask never
// reaches the wire.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_read_request(
    function fc, std::uint16_t address, std::uint16_t count,
    std::span<std::uint8_t> out) {
    const bool bits = reads_bits(fc);
    if (!bits && !reads_registers(fc)) {
        return modbus_error::malformed;
    }
    const std::uint16_t ceiling = bits ? max_read_bits : max_read_registers;
    if (count == 0u || count > ceiling) {
        return modbus_error::malformed;
    }
    if (out.size() < 5u) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(fc);
    detail::put_u16(out, 1, address);
    detail::put_u16(out, 3, count);
    return std::size_t{5};
}

// FC05/06 — [fc | addr u16 | value u16]. FC05's value is the spec's coil
// encoding: 0xFF00 = on, 0x0000 = off, anything else is illegal.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_write_single(
    function fc, std::uint16_t address, std::uint16_t value,
    std::span<std::uint8_t> out) {
    if (fc != function::write_single_coil && fc != function::write_single_register) {
        return modbus_error::malformed;
    }
    if (fc == function::write_single_coil && value != 0x0000u && value != 0xFF00u) {
        return modbus_error::malformed;
    }
    if (out.size() < 5u) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(fc);
    detail::put_u16(out, 1, address);
    detail::put_u16(out, 3, value);
    return std::size_t{5};
}

// FC10 — [fc | addr u16 | count u16 | byte_count u8 | data...], data is
// count registers, big-endian each, taken from host-order u16s.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_write_registers_request(
    std::uint16_t address, std::span<const std::uint16_t> values,
    std::span<std::uint8_t> out) {
    if (values.empty() || values.size() > max_write_registers) {
        return modbus_error::malformed;
    }
    const std::size_t total = 6u + values.size() * 2u;
    if (out.size() < total) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(function::write_multiple_registers);
    detail::put_u16(out, 1, address);
    detail::put_u16(out, 3, static_cast<std::uint16_t>(values.size()));
    out[5] = static_cast<std::uint8_t>(values.size() * 2u);
    for (std::size_t i = 0; i < values.size(); ++i) {
        detail::put_u16(out, 6u + i * 2u, values[i]);
    }
    return total;
}

// FC0F — [fc | addr u16 | count u16 | byte_count u8 | packed bits...].
// `packed` is LSB-first per the spec (bit 0 of byte 0 = first coil) and must
// be exactly ceil(count/8) bytes — a mismatch is the caller's bug, refused.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_write_coils_request(
    std::uint16_t address, std::uint16_t count,
    std::span<const std::uint8_t> packed, std::span<std::uint8_t> out) {
    if (count == 0u || count > max_write_bits ||
        packed.size() != detail::bit_bytes(count)) {
        return modbus_error::malformed;
    }
    const std::size_t total = 6u + packed.size();
    if (out.size() < total) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(function::write_multiple_coils);
    detail::put_u16(out, 1, address);
    detail::put_u16(out, 3, count);
    out[5] = static_cast<std::uint8_t>(packed.size());
    for (std::size_t i = 0; i < packed.size(); ++i) {
        out[6u + i] = packed[i];
    }
    return total;
}

// One parsed request, every field already bounds-checked against the PDU it
// came from. `payload` borrows FROM THE INPUT SPAN — valid only as long as
// the framer's buffer holds the frame (i.e. until consume()/next feed).
struct request_view {
    function fc{};
    std::uint16_t address{};
    std::uint16_t count{};   // FC01..04/0F/10; 1 for FC05/06
    std::uint16_t value{};   // FC05/06 only
    std::span<const std::uint8_t> payload{};  // FC0F packed bits / FC10 register bytes
};

// Server-side request parse. Unsupported function codes come back as the
// exact wire exception a compliant server must answer with (illegal
// function), so the dispatch layer can forward the error to build_exception
// without a translation table.
[[nodiscard]] constexpr Result<request_view, modbus_error> parse_request(
    std::span<const std::uint8_t> pdu) {
    if (pdu.empty()) {
        return modbus_error::malformed;
    }
    if (!is_supported(pdu[0])) {
        return modbus_error::exception_illegal_function;
    }
    const auto fc = static_cast<function>(pdu[0]);
    request_view v{.fc = fc};
    switch (fc) {
    case function::read_coils:
    case function::read_discrete_inputs:
    case function::read_holding_registers:
    case function::read_input_registers: {
        if (pdu.size() != 5u) {
            return modbus_error::malformed;
        }
        v.address = detail::get_u16(pdu, 1);
        v.count = detail::get_u16(pdu, 3);
        const std::uint16_t ceiling = reads_bits(fc) ? max_read_bits : max_read_registers;
        if (v.count == 0u || v.count > ceiling) {
            return modbus_error::exception_illegal_data_value;
        }
        return v;
    }
    case function::write_single_coil:
    case function::write_single_register: {
        if (pdu.size() != 5u) {
            return modbus_error::malformed;
        }
        v.address = detail::get_u16(pdu, 1);
        v.value = detail::get_u16(pdu, 3);
        v.count = 1u;
        if (fc == function::write_single_coil && v.value != 0x0000u &&
            v.value != 0xFF00u) {
            return modbus_error::exception_illegal_data_value;
        }
        return v;
    }
    case function::write_multiple_coils:
    case function::write_multiple_registers: {
        if (pdu.size() < 6u) {
            return modbus_error::malformed;
        }
        v.address = detail::get_u16(pdu, 1);
        v.count = detail::get_u16(pdu, 3);
        const std::uint8_t byte_count = pdu[5];
        const bool coils = fc == function::write_multiple_coils;
        const std::uint16_t ceiling = coils ? max_write_bits : max_write_registers;
        const std::size_t expect =
            coils ? detail::bit_bytes(v.count) : std::size_t{v.count} * 2u;
        if (v.count == 0u || v.count > ceiling) {
            return modbus_error::exception_illegal_data_value;
        }
        // byte_count must agree with count AND with the actual bytes present.
        if (byte_count != expect || pdu.size() != 6u + expect) {
            return modbus_error::malformed;
        }
        v.payload = pdu.subspan(6);
        return v;
    }
    }
    return modbus_error::malformed;  // unreachable; keeps -Werror switch-happy
}

// ── responses: server builds, client parses ─────────────────────────────────

// FC01/02 — [fc | byte_count u8 | packed bits...].
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_read_bits_response(
    function fc, std::span<const std::uint8_t> packed, std::span<std::uint8_t> out) {
    if (!reads_bits(fc) || packed.empty() || packed.size() > max_pdu - 2u) {
        return modbus_error::malformed;
    }
    if (out.size() < 2u + packed.size()) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(fc);
    out[1] = static_cast<std::uint8_t>(packed.size());
    for (std::size_t i = 0; i < packed.size(); ++i) {
        out[2u + i] = packed[i];
    }
    return 2u + packed.size();
}

// FC03/04 — [fc | byte_count u8 | registers...], big-endian each.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_read_registers_response(
    function fc, std::span<const std::uint16_t> regs, std::span<std::uint8_t> out) {
    if (!reads_registers(fc) || regs.empty() || regs.size() > max_read_registers) {
        return modbus_error::malformed;
    }
    const std::size_t total = 2u + regs.size() * 2u;
    if (out.size() < total) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(fc);
    out[1] = static_cast<std::uint8_t>(regs.size() * 2u);
    for (std::size_t i = 0; i < regs.size(); ++i) {
        detail::put_u16(out, 2u + i * 2u, regs[i]);
    }
    return total;
}

// FC05/06 — the response IS the request echoed; FC0F/10 — [fc | addr | count].
// One builder serves all four: what varies is only which u16 pair goes out.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_write_response(
    function fc, std::uint16_t address, std::uint16_t value_or_count,
    std::span<std::uint8_t> out) {
    if (fc != function::write_single_coil && fc != function::write_single_register &&
        fc != function::write_multiple_coils &&
        fc != function::write_multiple_registers) {
        return modbus_error::malformed;
    }
    if (out.size() < 5u) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(fc);
    detail::put_u16(out, 1, address);
    detail::put_u16(out, 3, value_or_count);
    return std::size_t{5};
}

// Exception response — [fc|0x80 | code]. `e` must be a wire exception
// (is_exception); a local error here is a dispatch-layer bug, refused.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> build_exception(
    function fc, modbus_error e, std::span<std::uint8_t> out) {
    if (!is_exception(e)) {
        return modbus_error::malformed;
    }
    if (out.size() < 2u) {
        return modbus_error::too_large;
    }
    out[0] = static_cast<std::uint8_t>(static_cast<std::uint8_t>(fc) | 0x80u);
    out[1] = exception_code(e);
    return std::size_t{2};
}

namespace detail {
// Shared response prologue: empty check, exception check (fc|0x80 carries the
// server's refusal), then the fc-matches-request check. Returns 0 on success.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> check_response_fc(
    std::span<const std::uint8_t> pdu, function expected) {
    if (pdu.empty()) {
        return modbus_error::malformed;
    }
    if (pdu[0] == (static_cast<std::uint8_t>(expected) | 0x80u)) {
        if (pdu.size() != 2u) {
            return modbus_error::malformed;
        }
        return to_exception(pdu[1]);
    }
    if (pdu[0] != static_cast<std::uint8_t>(expected)) {
        return modbus_error::unexpected_function;
    }
    return std::size_t{0};
}
}  // namespace detail

// Client-side FC03/04 response: registers land in `out` host-order; returns
// how many. The wire byte_count must agree with the PDU length AND fit the
// caller's array — a server sending more than asked cannot overrun us.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> parse_read_registers_response(
    std::span<const std::uint8_t> pdu, function expected,
    std::span<std::uint16_t> out) {
    if (auto pre = detail::check_response_fc(pdu, expected); !pre) {
        return pre.error();
    }
    if (!reads_registers(expected) || pdu.size() < 2u) {
        return modbus_error::malformed;
    }
    const std::uint8_t byte_count = pdu[1];
    if (byte_count == 0u || (byte_count & 1u) != 0u || pdu.size() != 2u + byte_count) {
        return modbus_error::malformed;
    }
    const std::size_t n = byte_count / 2u;
    if (n > out.size()) {
        return modbus_error::too_large;
    }
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = detail::get_u16(pdu, 2u + i * 2u);
    }
    return n;
}

// Client-side FC01/02 response: packed bits land in `out` as they came
// (LSB-first); returns the byte count.
[[nodiscard]] constexpr Result<std::size_t, modbus_error> parse_read_bits_response(
    std::span<const std::uint8_t> pdu, function expected,
    std::span<std::uint8_t> out) {
    if (auto pre = detail::check_response_fc(pdu, expected); !pre) {
        return pre.error();
    }
    if (!reads_bits(expected) || pdu.size() < 2u) {
        return modbus_error::malformed;
    }
    const std::uint8_t byte_count = pdu[1];
    if (byte_count == 0u || pdu.size() != 2u + byte_count) {
        return modbus_error::malformed;
    }
    if (byte_count > out.size()) {
        return modbus_error::too_large;
    }
    for (std::size_t i = 0; i < byte_count; ++i) {
        out[i] = pdu[2u + i];
    }
    return std::size_t{byte_count};
}

// Client-side FC05/06/0F/10 response: verifies the echo (address, and for the
// singles the value) matches what was requested — the correlation check the
// reference C library never performed.
[[nodiscard]] constexpr Result<void, modbus_error> parse_write_response(
    std::span<const std::uint8_t> pdu, function expected, std::uint16_t address,
    std::uint16_t value_or_count) {
    if (auto pre = detail::check_response_fc(pdu, expected); !pre) {
        return pre.error();
    }
    if (pdu.size() != 5u) {
        return modbus_error::malformed;
    }
    if (detail::get_u16(pdu, 1) != address ||
        detail::get_u16(pdu, 3) != value_or_count) {
        return modbus_error::malformed;
    }
    return ok<modbus_error>();
}

// ── RTU ADU length prediction (the framer's primary rule) ───────────────────

enum class direction : std::uint8_t {
    request,   // a server parses requests
    response,  // a client parses responses
};

// Marker: this function code's length cannot be predicted — the framer falls
// back to t3.5 silence framing (which handles ANY function code, including
// vendor extensions, at the cost of needing the clock).
inline constexpr std::size_t length_unknown = static_cast<std::size_t>(-1);

// Expected TOTAL RTU ADU length (unit + PDU + CRC) given the bytes so far.
// 0 = cannot tell yet, feed more. The caller compares the result against its
// buffer capacity BEFORE accumulating further — an absurd claimed length must
// switch the framer to resync, never wedge it (the reference C library kept
// the bytes and jammed at "expected 264 > buffer 260" forever).
[[nodiscard]] constexpr std::size_t expected_adu_length(
    direction dir, std::span<const std::uint8_t> sofar) {
    if (sofar.size() < 2u) {
        return 0;
    }
    const std::uint8_t fc = sofar[1];
    if (dir == direction::response && (fc & 0x80u) != 0u) {
        return 5;  // unit + (fc|0x80) + code + crc2
    }
    if (!is_supported(fc)) {
        return length_unknown;
    }
    if (dir == direction::request) {
        switch (static_cast<function>(fc)) {
        case function::read_coils:
        case function::read_discrete_inputs:
        case function::read_holding_registers:
        case function::read_input_registers:
        case function::write_single_coil:
        case function::write_single_register:
            return 8;  // unit + fc + u16 + u16 + crc2
        case function::write_multiple_coils:
        case function::write_multiple_registers:
            // unit + fc + addr2 + count2 + bc + data[bc] + crc2
            return sofar.size() < 7u ? 0 : 9u + std::size_t{sofar[6]};
        }
        return length_unknown;
    }
    switch (static_cast<function>(fc)) {
    case function::read_coils:
    case function::read_discrete_inputs:
    case function::read_holding_registers:
    case function::read_input_registers:
        // unit + fc + bc + data[bc] + crc2
        return sofar.size() < 3u ? 0 : 5u + std::size_t{sofar[2]};
    case function::write_single_coil:
    case function::write_single_register:
    case function::write_multiple_coils:
    case function::write_multiple_registers:
        return 8;  // echo / addr+count shape
    }
    return length_unknown;
}

}  // namespace alloy::lib::modbus
