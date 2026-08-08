// Per-device factory identity (serial, MAC, hw revision, batch) — the facts a
// board is born with, written once at production-flash time through the debug
// probe and readable by every firmware that runs afterwards.
//
//     #include <alloy/provision.hpp>
//     #include <alloy/slots.hpp>
//
//     if (auto id = alloy::provision::read(alloy::slots::provision_base)) {
//         uart.write(id->serial_view());
//     } else if (id.error() == alloy::provision::prov_error::blank) {
//         // never provisioned — a line fault, not a field fault
//     }
//
// Read-only from firmware BY DESIGN: identity lives outside both A/B slots, so
// it survives every update and every rollback, and nothing on the device can
// rewrite it. See alloy/provision/identity.hpp for the record format and for
// why there is no per-device private key in it.
#pragma once

#include "alloy/provision/identity.hpp"
