// OTA / firmware-update core (board-free): the alloy image format + integrity, an
// A/B slot boot-state machine with trial/confirm/rollback, and a flash-streaming
// updater. Reuses FlashController (writing slots) and nvm_kv (persistent, atomic
// boot-state). Host-tested end to end (tests/test_ota.cpp); the bootloader jump and
// the per-board two-slot flash layout are HW/later.
//
// SECURITY: v1 provides INTEGRITY only — CRC-32 catches corruption from flash or
// transport, but it is unkeyed, so an attacker who rewrites an image recomputes it
// and passes. Until the signature Verifier (ota/verify.hpp) is filled in, OTA must
// ride a TRUSTED delivery channel. CRC = "did it get mangled"; a signature over a
// hash = "did an attacker author it".
#pragma once

#include "alloy/ota/boot_state.hpp"
#include "alloy/ota/boot_store.hpp"
#include "alloy/ota/crc32.hpp"
#include "alloy/ota/error.hpp"
#include "alloy/ota/image.hpp"
#include "alloy/ota/updater.hpp"
#include "alloy/ota/verify.hpp"
