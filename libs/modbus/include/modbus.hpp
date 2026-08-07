// alloy::lib::modbus — Modbus RTU protocol core, sans-IO.
//
// This umbrella pulls in the whole v0.1 surface: the error domain, the
// compile-time CRC-16, the eight-function-code PDU codecs (all four
// build/parse quadrants), the spec timing facts, and the dual-rule RTU
// framer. Everything here is pure protocol — no uart, no clock, no chip:
// bytes and microsecond timestamps go in, frames and PDUs come out. The
// client/server layers that bind this core to an alloy::ByteStream uart and
// alloy::uptime_us() arrive in the next versions; until then this header is
// the complete, host-tested foundation they build on.
//
// Informed by the modbuscore C library (the API shapes worth keeping and two
// wire-reachable framing bugs worth designing out — see rtu_framer.hpp), but
// written fresh against the Modbus Application Protocol v1.1b3 and Modbus
// over Serial Line v1.02 documents.

#pragma once

#include "modbus/crc.hpp"          // IWYU pragma: export
#include "modbus/error.hpp"        // IWYU pragma: export
#include "modbus/pdu.hpp"          // IWYU pragma: export
#include "modbus/rtu_framer.hpp"   // IWYU pragma: export
#include "modbus/rtu_timing.hpp"   // IWYU pragma: export
