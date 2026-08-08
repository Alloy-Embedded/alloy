// Ed25519 image authenticity — the v2 the Verifier seam was designed for.
//
// v1 proved a slot's INTEGRITY (CRC): the bytes are what the sender sent. That
// says nothing about WHO sent them, so v1 had to ride a trusted channel. This
// adds AUTHENTICITY: the device boots only images signed by the private key
// whose public half is baked into the bootloader, so a stranger on the UART —
// or a tampered image in flash — is rejected.
//
// Image layout (see alloy/ota/image.hpp + the host's `alloy image --sign`):
//
//     [ header(32) | payload(payload_length) | signature(64) ]
//     `------------ signed message ---------'
//
// The signature covers the header AND the payload, so image_version can't be
// bumped by an attacker either (no rollback-by-relabel). It lives in a TRAILER
// after the covered bytes, which is why nothing about payload_length/CRC
// changes: verify_slot's `covered` span IS the signed message, and the trailer
// starts exactly where it ends.
//
// Verification is RFC 8032 Ed25519 via vendored Monocypher (third_party/
// monocypher — audited, public domain/BSD; alloy hand-rolls no crypto). Cost:
// ~14 KB of flash on Cortex-M0+, ~11 KB on M7, which is why the bootloader
// region is sized for it whether or not a given build signs (see emit/slots.py:
// the layout must not shift when a product turns signing on, or fielded devices
// couldn't take the update that introduces it).
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/ota/error.hpp"
#include "alloy/ota/verify.hpp"
#include "alloy/util/result.hpp"

extern "C" {
#include "monocypher-ed25519.h"
}

namespace alloy::ota {

inline constexpr std::size_t signature_bytes = 64;
inline constexpr std::size_t public_key_bytes = 32;

// KEY ROTATION (v3). A device trusts a fixed RING of public keys instead of one,
// and the image header's `key_id` says which ring entry signed it. That is what
// turns a leaked signing key from a fleet-ending event into a maintenance task:
// ship the next release signed by a ring key that never left the HSM, then
// retire the leaked one in the next bootloader.
//
// Retiring is by ZEROING an entry, never by removing it: key_id is a positional
// index, so deleting entry 0 would silently re-point every image that names key
// 0 at what used to be key 1. An all-zero entry is refused before Ed25519 is
// ever called (it is not a valid public key anyway).
//
// What this does NOT buy — say it plainly: revocation still requires shipping a
// new BOOTLOADER (the ring is .rodata baked in at build time). There is no field
// revocation message, because a revocation floor would need persistent state
// that ota::boot_store cannot hold (it stores exactly one u32). Until that
// bootloader lands, a device keeps trusting the leaked key.
//
// Construct with the slot being verified (so the trailer read is bounds-checked
// against the slot, never past it) and the ring — generated code puts one in
// alloy/ota_key.hpp from `[ota] public_key` / `public_keys`.
class signed_verifier {
public:
    // ONE key — the v2 shape, still exact: a ring of one, and every image whose
    // key_id is 0 (which is every image ever produced before this field existed)
    // selects it.
    constexpr signed_verifier(const slot& s, const std::uint8_t* public_key)
        : slot_(s), keys_(public_key), key_count_(1u) {}

    // A RING: `keys` is `count` contiguous 32-byte public keys, indexed by the
    // header's key_id.
    constexpr signed_verifier(const slot& s, const std::uint8_t* keys, std::uint16_t count)
        : slot_(s), keys_(keys), key_count_(count) {}

    [[nodiscard]] Result<void, ota_error>
    authenticate(const image_header& h, std::span<const std::uint8_t> covered) const {
        // Which key does this image claim? An id past the ring, or a retired
        // (zeroed) entry, is refused BEFORE any signature work — an attacker
        // must not be able to make the device spend Ed25519 on a dead key, and
        // must not be able to use key_id as a bypass.
        if (h.key_id >= key_count_) return ota_error::unknown_key;
        const std::uint8_t* key = keys_ + static_cast<std::size_t>(h.key_id) * public_key_bytes;
        bool live = false;
        for (std::size_t i = 0; i < public_key_bytes; ++i) {
            live = live || key[i] != 0u;
        }
        if (!live) return ota_error::unknown_key;
        // The trailer must fit INSIDE the slot: a crafted payload_length that
        // pushes it past the end must be rejected, not read across the boundary.
        if (static_cast<std::uint64_t>(covered.size()) + signature_bytes > slot_.size) {
            return ota_error::not_authentic;
        }
        const std::uint8_t* signature = covered.data() + covered.size();
        // Monocypher returns 0 only on a valid signature (constant-time compare).
        // Exactly ONE check runs, whatever the ring size: key_id selects, it
        // never causes a try-all-keys sweep.
        if (crypto_ed25519_check(signature, key, covered.data(), covered.size()) != 0) {
            return ota_error::not_authentic;
        }
        return alloy::ok<ota_error>();
    }

private:
    slot slot_;
    const std::uint8_t* keys_;
    std::uint16_t key_count_;
};

static_assert(Verifier<signed_verifier>);

}  // namespace alloy::ota
