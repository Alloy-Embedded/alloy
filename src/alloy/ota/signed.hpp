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

// A Verifier that demands an Ed25519 signature over [header|payload]. Construct
// it with the slot being verified (so the trailer read is bounds-checked against
// the slot, never past it) and the 32-byte public key — generated code puts one
// in alloy/ota_key.hpp when the project configures `[ota] public_key`.
class signed_verifier {
public:
    constexpr signed_verifier(const slot& s, const std::uint8_t* public_key)
        : slot_(s), key_(public_key) {}

    [[nodiscard]] Result<void, ota_error>
    authenticate(const image_header&, std::span<const std::uint8_t> covered) const {
        // The trailer must fit INSIDE the slot: a crafted payload_length that
        // pushes it past the end must be rejected, not read across the boundary.
        if (static_cast<std::uint64_t>(covered.size()) + signature_bytes > slot_.size) {
            return ota_error::not_authentic;
        }
        const std::uint8_t* signature = covered.data() + covered.size();
        // Monocypher returns 0 only on a valid signature (constant-time compare).
        if (crypto_ed25519_check(signature, key_, covered.data(), covered.size()) != 0) {
            return ota_error::not_authentic;
        }
        return alloy::ok<ota_error>();
    }

private:
    slot slot_;
    const std::uint8_t* key_;
};

static_assert(Verifier<signed_verifier>);

}  // namespace alloy::ota
