// alloy::lib::modbus — Modbus RTU protocol core, sans-IO.
//
// This umbrella pulls in the whole surface: the error domain, the
// compile-time CRC-16, the eight-function-code PDU codecs (all four
// build/parse quadrants), the spec timing facts, the dual-rule RTU framer,
// the blocking rtu_client (master), and the poll-driven rtu_server (slave)
// whose register bank is any user type satisfying the DataModel concept —
// both bound to any alloy::ByteStream uart with an injected µs clock.
//
// Informed by the modbuscore C library (the API shapes worth keeping and two
// wire-reachable framing bugs worth designing out — see rtu_framer.hpp), but
// written fresh against the Modbus Application Protocol v1.1b3 and Modbus
// over Serial Line v1.02 documents.

#pragma once

#include "modbus/client.hpp"       // IWYU pragma: export
#include "modbus/clock.hpp"        // IWYU pragma: export
#include "modbus/crc.hpp"          // IWYU pragma: export
#include "modbus/data_model.hpp"   // IWYU pragma: export
#include "modbus/error.hpp"        // IWYU pragma: export
#include "modbus/pdu.hpp"          // IWYU pragma: export
#include "modbus/rtu_framer.hpp"   // IWYU pragma: export
#include "modbus/rtu_timing.hpp"   // IWYU pragma: export
#include "modbus/server.hpp"       // IWYU pragma: export
