// Host tests for the SIGNED image path and the KEY ROTATION ring
// (alloy/ota/signed.hpp), against the same vendored Monocypher the firmware
// links. Real Ed25519 keypairs, real signatures, real verification — the only
// fake here is the "flash" (a static buffer standing in for a memory-mapped
// slot).
//
// The rejections are the point. Anyone can write a test where the right key
// works; what makes rotation real is that key_id cannot be used as a BYPASS
// (an image naming key 1 but signed by key 0 must fail) and that a RETIRED
// entry can never authenticate again (that is what "revocation" has to mean).
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/ota/image.hpp"
#include "alloy/ota/signed.hpp"
#include "alloy/ota/verify.hpp"
#include "alloy_test.hpp"

extern "C" {
#include "monocypher-ed25519.h"
}

namespace {

using namespace alloy::ota;

// One 2 KB "slot" of memory-mapped flash.
alignas(std::uint64_t) std::uint8_t g_slot[2048];
constexpr std::uint32_t kSlotSize = sizeof(g_slot);

slot the_slot() {
    return {reinterpret_cast<std::uintptr_t>(g_slot), kSlotSize};
}

struct keypair {
    std::uint8_t secret[64];
    std::uint8_t pub[32];
};

// Deterministic keys: the seed is the test's, so a failure is reproducible and
// no RNG is involved. (Never do this for a real signing key.)
keypair key_from_seed(std::uint8_t s) {
    keypair k{};
    std::uint8_t seed[32];
    std::memset(seed, s, sizeof(seed));
    crypto_ed25519_key_pair(k.secret, k.pub, seed);  // NB: wipes `seed`
    return k;
}

// [header|payload|signature] laid into g_slot; returns total bytes written.
std::uint32_t lay_signed_image(const keypair& signer, std::uint32_t version,
                               std::uint16_t key_id, std::uint32_t plen = 200u) {
    std::memset(g_slot, 0xFF, kSlotSize);
    std::uint8_t payload[512];
    for (std::uint32_t i = 0; i < plen; ++i) {
        payload[i] = static_cast<std::uint8_t>(i * 11u + version);
    }
    image_header h;
    h.image_version = version;
    h.key_id = key_id;
    h.payload_length = plen;
    h.payload_crc32 = crc::crc32_of({payload, plen});
    h.serialize(g_slot);
    std::memcpy(g_slot + image_header::byte_size, payload, plen);
    const std::uint32_t covered = image_header::byte_size + plen;
    crypto_ed25519_sign(g_slot + covered, signer.secret, g_slot, covered);
    return covered + static_cast<std::uint32_t>(signature_bytes);
}

}  // namespace

// ── one key: the v2 shape must keep working exactly ─────────────────────────

ALLOY_TEST(ota_signed_single_key_accepts_its_own_signature) {
    const keypair k = key_from_seed(1);
    (void)lay_signed_image(k, 5u, 0u);
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), k.pub});
    ALLOY_CHECK(static_cast<bool>(r));
    ALLOY_CHECK_EQ(r->image_version, 5u);
    ALLOY_CHECK_EQ(r->key_id, 0u);
}

ALLOY_TEST(ota_signed_single_key_refuses_another_key) {
    const keypair mine = key_from_seed(1);
    const keypair attacker = key_from_seed(2);
    (void)lay_signed_image(attacker, 5u, 0u);
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), mine.pub});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::not_authentic);
}

ALLOY_TEST(ota_signed_tampered_payload_with_crcs_repaired_is_refused) {
    const keypair k = key_from_seed(1);
    (void)lay_signed_image(k, 5u, 0u);
    // Flip an app byte and REPAIR both CRCs — integrity now passes and only the
    // signature can catch it. This is the case that would fail if authenticity
    // were fake.
    g_slot[64] ^= 0x01u;
    auto ph = image_header::parse({g_slot, image_header::byte_size});
    ALLOY_CHECK(static_cast<bool>(ph));
    image_header h = *ph;
    h.payload_crc32 =
        crc::crc32_of({g_slot + image_header::byte_size, h.payload_length});
    h.serialize(g_slot);  // recomputes header_crc32 itself
    ALLOY_CHECK(static_cast<bool>(verify_slot(the_slot())));  // integrity-only: passes
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), k.pub});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::not_authentic);
}

ALLOY_TEST(ota_signed_single_key_is_a_ring_of_one) {
    // An image naming key 1 on a device that trusts exactly one key must be
    // refused as unknown, not silently checked against key 0.
    const keypair k = key_from_seed(1);
    (void)lay_signed_image(k, 5u, 1u);
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), k.pub});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::unknown_key);
}

// ── the ring: rotation, revocation, and key_id as a non-bypass ──────────────

ALLOY_TEST(ota_signed_ring_accepts_the_rotated_in_key) {
    const keypair a = key_from_seed(1);
    const keypair b = key_from_seed(2);
    std::uint8_t ring[2][32];
    std::memcpy(ring[0], a.pub, 32);
    std::memcpy(ring[1], b.pub, 32);

    (void)lay_signed_image(b, 7u, 1u);  // signed by the NEW key, naming slot 1
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), &ring[0][0], 2u});
    ALLOY_CHECK(static_cast<bool>(r));
    ALLOY_CHECK_EQ(r->key_id, 1u);

    (void)lay_signed_image(a, 7u, 0u);  // the OLD key still works while it is live
    ALLOY_CHECK(static_cast<bool>(
        verify_slot(the_slot(), signed_verifier{the_slot(), &ring[0][0], 2u})));
}

ALLOY_TEST(ota_signed_ring_key_id_is_not_a_bypass) {
    // Signed by key 0, but claiming key 1. If key_id merely SELECTED without
    // being covered by the signature check, this would boot.
    const keypair a = key_from_seed(1);
    const keypair b = key_from_seed(2);
    std::uint8_t ring[2][32];
    std::memcpy(ring[0], a.pub, 32);
    std::memcpy(ring[1], b.pub, 32);
    (void)lay_signed_image(a, 7u, 1u);
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), &ring[0][0], 2u});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::not_authentic);
}

ALLOY_TEST(ota_signed_ring_refuses_a_key_id_past_the_end) {
    const keypair a = key_from_seed(1);
    std::uint8_t ring[2][32];
    std::memcpy(ring[0], a.pub, 32);
    std::memcpy(ring[1], key_from_seed(2).pub, 32);
    (void)lay_signed_image(a, 7u, 2u);  // ids 0..1 exist
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), &ring[0][0], 2u});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::unknown_key);
}

ALLOY_TEST(ota_signed_retired_key_can_never_authenticate_again) {
    // THE REVOCATION CLAIM. The same image, byte for byte, that a two-key ring
    // accepts is refused once entry 0 is zeroed — and the replacement key in
    // slot 1 still works, so the fleet is not bricked by the retirement.
    const keypair leaked = key_from_seed(1);
    const keypair fresh = key_from_seed(2);
    std::uint8_t live_ring[2][32];
    std::memcpy(live_ring[0], leaked.pub, 32);
    std::memcpy(live_ring[1], fresh.pub, 32);
    std::uint8_t retired_ring[2][32];
    std::memset(retired_ring[0], 0, 32);  // what `public_keys = ["retired", ...]` emits
    std::memcpy(retired_ring[1], fresh.pub, 32);

    (void)lay_signed_image(leaked, 7u, 0u);
    ALLOY_CHECK(static_cast<bool>(
        verify_slot(the_slot(), signed_verifier{the_slot(), &live_ring[0][0], 2u})));
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), &retired_ring[0][0], 2u});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::unknown_key);

    (void)lay_signed_image(fresh, 8u, 1u);
    ALLOY_CHECK(static_cast<bool>(
        verify_slot(the_slot(), signed_verifier{the_slot(), &retired_ring[0][0], 2u})));
}

ALLOY_TEST(ota_signed_trailer_must_fit_inside_the_slot) {
    // A crafted payload_length that pushes the 64-byte trailer past the slot
    // end must be refused, never read across the boundary (ASan would catch a
    // regression here).
    const keypair k = key_from_seed(1);
    (void)lay_signed_image(k, 1u, 0u);
    auto ph = image_header::parse({g_slot, image_header::byte_size});
    ALLOY_CHECK(static_cast<bool>(ph));
    image_header h = *ph;
    h.payload_length = kSlotSize - image_header::byte_size - 8u;  // fits, trailer does not
    h.payload_crc32 =
        crc::crc32_of({g_slot + image_header::byte_size, h.payload_length});
    h.serialize(g_slot);
    auto r = verify_slot(the_slot(), signed_verifier{the_slot(), k.pub});
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == ota_error::not_authentic);
}
