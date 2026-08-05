// The A/B boot-state machine: trial / confirm / rollback, as PURE logic over an
// injected persistence seam (100% host-testable). Direct-boot-from-slot (no image
// swap): the active slot is the known-good fallback; an update lands in the other
// slot and is armed as a one-shot trial; the app must confirm() after a real
// health check or the bootloader reverts within N boots.
//
// The whole state (active, pending, attempts) packs into ONE u32, so a single
// store write commits any transition and there is no multi-field intermediate to
// tear. The anti-brick guarantee then rests on the STORE being power-atomic: pair
// this with ota::boot_store (a two-page ping-pong whose set() leaves the prior
// value authoritative on a torn write). alloy::nvm::store also satisfies BootStore
// but its compaction is NOT power-atomic, so it must not hold the boot-state.
// plan_boot() additionally fails safe: if it cannot persist a consumed trial
// attempt, it boots the known-good `active` slot rather than a trial it can't bound.
#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/ota/error.hpp"
#include "alloy/util/result.hpp"

namespace alloy::ota {

struct boot_state {
    static constexpr std::uint8_t no_slot = 0xFFu;
    std::uint8_t active{0};         // committed known-good slot (0|1) — the fallback
    std::uint8_t pending{no_slot};  // trial candidate (0|1), or no_slot
    std::uint8_t attempts{0};       // trial boots remaining

    [[nodiscard]] constexpr std::uint32_t pack() const {
        return std::uint32_t{active} | (std::uint32_t{pending} << 8) | (std::uint32_t{attempts} << 16);
    }
    [[nodiscard]] static constexpr boot_state unpack(std::uint32_t w) {
        return {static_cast<std::uint8_t>(w), static_cast<std::uint8_t>(w >> 8),
                static_cast<std::uint8_t>(w >> 16)};
    }
};

enum class boot_kind : std::uint8_t { normal, trial, reverted };
struct boot_decision {
    std::uint8_t slot;
    boot_kind kind;
};

// The boot decision as a PURE function of the state: which slot to boot, the next
// state to persist, and whether it changed. Separated from any I/O so the safety
// invariants (never boot a slot other than active/pending; never mutate active)
// are exhaustively testable. plan_boot() = load -> decide -> save-if-changed.
struct plan_result {
    boot_decision decision;
    boot_state next;
    bool changed;
};
[[nodiscard]] constexpr plan_result decide(boot_state s) {
    if (s.pending == boot_state::no_slot) return {{s.active, boot_kind::normal}, s, false};
    if (s.attempts == 0u) {  // trials exhausted -> revert to the confirmed slot
        s.pending = boot_state::no_slot;
        return {{s.active, boot_kind::reverted}, s, true};
    }
    s.attempts = static_cast<std::uint8_t>(s.attempts - 1u);  // consume one trial
    return {{s.pending, boot_kind::trial}, s, true};
}

// The persistence seam — exactly nvm::store's get/set surface (u32 key -> u32
// value, survives reset). alloy::nvm::store<Flash> and null_store satisfy it; host
// tests inject a RAM map.
template <class T>
concept BootStore = requires(T s, std::uint32_t k, std::uint32_t v) {
    { s.get(k, v) } -> std::convertible_to<std::uint32_t>;
    { s.set(k, v) } -> std::same_as<bool>;
};

template <BootStore Store>
class boot_manager {
    Store store_;
    std::uint8_t max_attempts_;
    static constexpr std::uint32_t k_state = 0x0B07u;               // nvm key (!= empty_key)
    static constexpr std::uint32_t k_factory = boot_state{}.pack();  // {active 0, no pending, 0}

    [[nodiscard]] boot_state load() const { return boot_state::unpack(store_.get(k_state, k_factory)); }
    [[nodiscard]] bool save(const boot_state& s) { return store_.set(k_state, s.pack()); }
    using rvoid = Result<void, ota_error>;

public:
    // N = trial boots before automatic revert (default 3; N=1 == MCUboot/ESP single-trial).
    explicit boot_manager(Store s, std::uint8_t n = 3u) : store_(s), max_attempts_(n) {}

    [[nodiscard]] boot_state state() const { return load(); }
    [[nodiscard]] std::uint8_t active() const { return load().active; }
    [[nodiscard]] bool update_in_progress() const { return load().pending != boot_state::no_slot; }
    [[nodiscard]] std::uint8_t update_target() const {
        return static_cast<std::uint8_t>(1u - load().active);
    }

    // Trusted bootloader, once per reset. Picks the slot and CONSUMES one trial
    // (persisted BEFORE the jump) so a hang can't loop forever. Steady state writes
    // nothing. The caller still verify_slot()s the returned slot and, on failure,
    // reject_pending()s + boots the other slot (anti-brick).
    [[nodiscard]] boot_decision plan_boot() {
        const plan_result r = decide(load());
        if (r.changed && !save(r.next)) {
            // Couldn't persist the consumed attempt — fail safe to the known-good
            // slot rather than boot a trial whose count we can't decrement (which
            // could loop forever). active is never mutated by decide().
            return {r.next.active, boot_kind::normal};
        }
        return r.decision;
    }

    // Arm a freshly-written+verified candidate as the trial (the updater's last
    // step). One atomic record.
    [[nodiscard]] rvoid mark_updated(std::uint8_t cand) {
        boot_state s = load();
        if (cand > 1u || cand == s.active) return ota_error::invalid_slot;
        s.pending = cand;
        s.attempts = max_attempts_;
        return save(s) ? alloy::ok<ota_error>() : rvoid{ota_error::flash_io};
    }

    // Promote the trial to confirmed — call ONLY after a real health check (net up,
    // self-test passed), never merely on reaching main(). Idempotent.
    [[nodiscard]] rvoid confirm() {
        boot_state s = load();
        if (s.pending == boot_state::no_slot) return alloy::ok<ota_error>();
        s.active = s.pending;
        s.pending = boot_state::no_slot;
        s.attempts = 0u;
        return save(s) ? alloy::ok<ota_error>() : rvoid{ota_error::flash_io};
    }

    // Drop the pending trial (bootloader, when verify_slot(pending) fails). Idempotent.
    [[nodiscard]] rvoid reject_pending() {
        boot_state s = load();
        if (s.pending == boot_state::no_slot) return alloy::ok<ota_error>();
        s.pending = boot_state::no_slot;
        return save(s) ? alloy::ok<ota_error>() : rvoid{ota_error::flash_io};
    }
};

}  // namespace alloy::ota
