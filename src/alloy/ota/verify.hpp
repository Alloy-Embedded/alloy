// The ONE trust gate for a flash slot, run by BOTH the updater (to check what it
// just wrote) and the bootloader (to check what it's about to boot) — so "what we
// wrote" and "what we boot" go through identical logic. v1 enforces INTEGRITY
// (CRC); authenticity is a typed Verifier seam, defaulting to a no-op, so a
// signed check drops in later with no change to the header, updater, or boot core.
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/ota/image.hpp"

namespace alloy::ota {

// A flash slot: an absolute, memory-mapped region (base + size). Board config;
// the core stays board-free.
struct slot {
    std::uintptr_t base;
    std::uint32_t size;
};

// Authenticity check, run AFTER integrity, over the covered bytes [base, end).
template <class V>
concept Verifier = requires(V v, const image_header& h, std::span<const std::uint8_t> covered) {
    { v.authenticate(h, covered) } -> std::same_as<Result<void, ota_error>>;
};

// v1 default: integrity only, authenticity not yet enforced. A v2 signed verifier
// implements the SAME authenticate() signature (hash `covered`, verify a signature
// from an image trailer against a baked-in public key) and is swapped in at the
// verify_slot call sites — nothing else changes.
struct no_auth {
    [[nodiscard]] Result<void, ota_error> authenticate(const image_header&,
                                                       std::span<const std::uint8_t>) const {
        return alloy::ok<ota_error>();
    }
};

// Parse + fully verify the [header|payload] living at slot `s` (flash is
// memory-mapped, so a plain pointer reads it). Returns the header on success.
template <Verifier V = no_auth>
[[nodiscard]] Result<image_header, ota_error> verify_slot(const slot& s, V v = {}) {
    const auto* base = reinterpret_cast<const std::uint8_t*>(s.base);
    auto ph = image_header::parse({base, image_header::byte_size});
    if (!ph) return ph.error();
    const image_header& h = *ph;
    const std::uint64_t end = static_cast<std::uint64_t>(h.header_size) + h.payload_length;
    if (end > s.size) return ota_error::slot_overflow;  // bound BEFORE trusting the length
    if (auto r = h.verify_payload({base + h.header_size, h.payload_length}); !r) return r.error();
    if (auto r = v.authenticate(h, {base, static_cast<std::size_t>(end)}); !r) return r.error();
    return h;
}

}  // namespace alloy::ota
