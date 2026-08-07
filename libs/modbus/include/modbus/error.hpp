// Modbus error domain — one enum for BOTH failure families a caller must tell
// apart: local failures (timeout, CRC, framing) and exceptions the peer put on
// the wire. The wire marks an exception response by setting bit 7 of the
// function code, so this enum reuses exactly that fact: values >= 0x80 ARE
// wire exception codes (0x80 | code), and is_exception() is a bit test — the
// enum cannot drift from the protocol because it is the protocol's encoding.
//
// Flat-bool would lose the reason; two enums would force every call site to
// carry a variant. One enum, one bit, both families distinguishable — the
// same "honest error reporting" trade the concepts header documents for I2C.

#pragma once

#include <cstdint>

namespace alloy::lib::modbus {

enum class modbus_error : std::uint8_t {
    // ── local: this side detected the problem ──────────────────────────────
    timeout = 1,          // no (complete) response within the deadline
    crc,                  // frame arrived, CRC16 disagreed
    malformed,            // frame arrived, contents violate the PDU layout
    too_large,            // request/response cannot fit the caller's buffer
    unexpected_unit,      // response carries another server's unit id
    unexpected_function,  // response function does not match the request
    busy,                 // a transaction is already in flight

    // ── wire: the SERVER answered with an exception PDU ────────────────────
    // Value = 0x80 | exception code, mirroring the fc|0x80 wire convention.
    exception_illegal_function = 0x81,      // code 0x01
    exception_illegal_data_address = 0x82,  // code 0x02
    exception_illegal_data_value = 0x83,    // code 0x03
    exception_server_failure = 0x84,        // code 0x04
    exception_acknowledge = 0x85,           // code 0x05
    exception_server_busy = 0x86,           // code 0x06
};

// True when the error is the peer's exception response, not a local failure —
// callers branch on this to separate "the wire is broken" from "the server
// said no" (the latter carries the exact refusal in exception_code()).
[[nodiscard]] constexpr bool is_exception(modbus_error e) {
    return (static_cast<std::uint8_t>(e) & 0x80u) != 0u;
}

// The raw exception code as the server sent it (only meaningful when
// is_exception(); 0x01 = illegal function, ... per the Modbus spec).
[[nodiscard]] constexpr std::uint8_t exception_code(modbus_error e) {
    return static_cast<std::uint8_t>(e) & 0x7Fu;
}

// Wrap a code received on the wire. Codes beyond the named ones (vendor
// extensions, gateway codes 0x0A/0x0B) round-trip unchanged — the enum names
// the common six, the encoding carries all of them.
[[nodiscard]] constexpr modbus_error to_exception(std::uint8_t code) {
    return static_cast<modbus_error>(0x80u | (code & 0x7Fu));
}

}  // namespace alloy::lib::modbus
