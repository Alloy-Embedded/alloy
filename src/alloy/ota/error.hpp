// OTA subsystem error codes — passed as the E of alloy::Result (util/result.hpp
// lets a subsystem carry its own enum).
#pragma once

#include <cstdint>

namespace alloy::ota {

enum class ota_error : std::uint8_t {
    bad_magic = 1,        // not an alloy image (or erased flash)
    bad_header_crc,       // the 32-byte header failed its own CRC
    unsupported_version,  // header_version this core can't parse
    bad_payload_crc,      // payload integrity check failed
    truncated,            // buffer/slot smaller than the declared image
    slot_overflow,        // image doesn't fit the target slot
    invalid_slot,         // bad slot index (must be 0/1, and != active)
    flash_io,             // erase/program returned false
    not_open,             // write()/finish() before begin()
    not_authentic,        // signature missing/invalid for the key the image names
    rollback,             // image_version below this device's anti-rollback floor
    unknown_key,          // header key_id names no LIVE key in the trusted ring
                          // (past the end of the ring, or a retired entry) —
                          // indistinguishable on-device, and deliberately so
};

}  // namespace alloy::ota
