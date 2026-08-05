// Host tests for the OTA/firmware-update core (alloy/ota): CRC32, the image
// format, the A/B trial/confirm/rollback boot-state machine, and the flash-
// streaming updater — all exercised WITHOUT hardware. Boot-state persistence uses
// the REAL alloy::nvm::store over a static "flash" region, so a fresh manager
// re-opened over the same bytes models a power cycle.
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/ota.hpp"
#include "alloy_test.hpp"

namespace {

using namespace alloy::ota;

// ── Fakes ──────────────────────────────────────────────────────────────────

// Two OTA slots in one NOR-flash region: erase fills 0xFF, program is AND-only
// (bits 1->0, so a double-write to the same dword is visible under ASan).
struct fake_flash {
    static constexpr std::uint32_t page_size = 512;
    alignas(std::uint64_t) static inline std::uint8_t region[4096];
    [[nodiscard]] bool erase_page(std::uintptr_t addr) const {
        std::memset(reinterpret_cast<void*>(addr), 0xFF, page_size);
        return true;
    }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* data, std::size_t n) const {
        auto* w = reinterpret_cast<std::uint64_t*>(addr);
        for (std::size_t i = 0; i < n; ++i) w[i] &= data[i];
        return true;
    }
};
std::uintptr_t slot_base(std::uint8_t idx) {
    return reinterpret_cast<std::uintptr_t>(fake_flash::region) + std::uintptr_t{idx} * 2048u;
}
slot slot_of(std::uint8_t idx) { return {slot_base(idx), 2048u}; }
void wipe_slots() { std::memset(fake_flash::region, 0xFF, sizeof(fake_flash::region)); }

// A FlashController whose erase/program fail on the Nth call (to drive flash_io).
struct flaky_flash {
    static constexpr std::uint32_t page_size = 512;
    static inline int fail_at = -1;   // call index that returns false (-1 = never)
    static inline int calls = 0;
    [[nodiscard]] bool erase_page(std::uintptr_t addr) const { return step(addr, 0xFFu, page_size); }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* d, std::size_t n) const {
        if (calls++ == fail_at) return false;
        auto* w = reinterpret_cast<std::uint64_t*>(addr);
        for (std::size_t i = 0; i < n; ++i) w[i] &= d[i];
        return true;
    }
    static bool step(std::uintptr_t addr, std::uint8_t v, std::uint32_t len) {
        if (calls++ == fail_at) return false;
        std::memset(reinterpret_cast<void*>(addr), v, len);
        return true;
    }
};

// A two-page NOR flash for the power-atomic boot_store. program() can be told to
// fail on the Nth call to model a torn write (erase done, program interrupted).
struct bs_flash {
    static constexpr std::uint32_t page_size = 512;
    alignas(std::uint64_t) static inline std::uint8_t region[1024];
    static inline int fail_prog_at = -1;
    static inline int prog_calls = 0;
    [[nodiscard]] bool erase_page(std::uintptr_t addr) const {
        std::memset(reinterpret_cast<void*>(addr), 0xFF, page_size);
        return true;
    }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* data, std::size_t n) const {
        if (prog_calls++ == fail_prog_at) return false;
        auto* w = reinterpret_cast<std::uint64_t*>(addr);
        for (std::size_t i = 0; i < n; ++i) w[i] &= data[i];
        return true;
    }
    static std::uintptr_t page(int i) {
        return reinterpret_cast<std::uintptr_t>(region) + std::uintptr_t(i) * 512u;
    }
};
using bstore = alloy::ota::boot_store<bs_flash>;
bstore fresh_bootstore() {  // a factory device: both pages erased
    std::memset(bs_flash::region, 0xFF, sizeof(bs_flash::region));
    bs_flash::prog_calls = 0;
    bs_flash::fail_prog_at = -1;
    return bstore{bs_flash{}, bs_flash::page(0), bs_flash::page(1)};
}
bstore reopen_bootstore() {  // same bytes, NOT wiped — the "reboot"
    return bstore{bs_flash{}, bs_flash::page(0), bs_flash::page(1)};
}

void fill_pattern(std::span<std::uint8_t> buf, std::uint8_t seed) {
    for (std::size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<std::uint8_t>(i * 7u + seed);
}

// Build a valid image [header|payload] into `out`; returns total bytes.
std::uint32_t make_image(std::uint8_t* out, std::uint32_t version,
                         const std::uint8_t* payload, std::uint32_t plen) {
    image_header h;
    h.image_version = version;
    h.payload_length = plen;
    h.payload_crc32 = crc::crc32_of({payload, plen});
    h.serialize(out);
    std::memcpy(out + image_header::byte_size, payload, plen);
    return image_header::byte_size + plen;
}

std::span<const std::uint8_t> str(const char* s) {
    return {reinterpret_cast<const std::uint8_t*>(s), std::strlen(s)};
}

}  // namespace

// ── CRC32 ───────────────────────────────────────────────────────────────────

ALLOY_TEST(ota_crc32_known_vectors) {
    ALLOY_CHECK_EQ(crc::crc32_of({}), 0u);
    ALLOY_CHECK_EQ(crc::crc32_of(str("123456789")), 0xCBF43926u);  // canonical CRC-32 check
    ALLOY_CHECK_EQ(crc::crc32_of(str("a")), 0xE8B7BE43u);
}

ALLOY_TEST(ota_crc32_incremental_equals_oneshot) {
    const auto whole = str("the quick brown fox");
    for (std::size_t cut : {std::size_t{0}, std::size_t{1}, std::size_t{7}, whole.size()}) {
        crc::crc32 c;
        c.update(whole.first(cut));
        c.update(whole.subspan(cut));
        ALLOY_CHECK_EQ(c.value(), crc::crc32_of(whole));
    }
}

// ── image_header ─────────────────────────────────────────────────────────────

ALLOY_TEST(ota_header_roundtrip) {
    std::uint8_t payload[40];
    fill_pattern(payload, 3);
    std::uint8_t buf[image_header::byte_size];
    image_header h;
    h.image_version = 42u;
    h.payload_length = sizeof(payload);
    h.payload_crc32 = crc::crc32_of(payload);
    h.serialize(buf);

    auto p = image_header::parse(buf);
    ALLOY_CHECK(static_cast<bool>(p));
    ALLOY_CHECK_EQ(p->image_version, 42u);
    ALLOY_CHECK_EQ(p->payload_length, static_cast<std::uint32_t>(sizeof(payload)));
    ALLOY_CHECK_EQ(p->payload_crc32, crc::crc32_of(payload));
    ALLOY_CHECK_EQ(p->header_size, image_header::byte_size);
}

ALLOY_TEST(ota_header_rejects_erased) {
    std::uint8_t buf[image_header::byte_size];
    std::memset(buf, 0xFF, sizeof(buf));  // erased flash
    auto p = image_header::parse(buf);
    ALLOY_CHECK(!p);
    ALLOY_CHECK(p.error() == ota_error::bad_magic);
}

ALLOY_TEST(ota_header_rejects_bad_header_crc) {
    std::uint8_t buf[image_header::byte_size];
    image_header{}.serialize(buf);
    buf[10] ^= 0x20u;  // corrupt a metadata byte inside [0,28)
    auto p = image_header::parse(buf);
    ALLOY_CHECK(!p);
    ALLOY_CHECK(p.error() == ota_error::bad_header_crc);
}

ALLOY_TEST(ota_header_rejects_unsupported_version) {
    std::uint8_t buf[image_header::byte_size];
    image_header h;
    h.header_version = 99u;
    h.serialize(buf);  // header_crc computed over the bumped version, so CRC passes
    auto p = image_header::parse(buf);
    ALLOY_CHECK(!p);
    ALLOY_CHECK(p.error() == ota_error::unsupported_version);
}

ALLOY_TEST(ota_header_rejects_truncated) {
    std::uint8_t buf[image_header::byte_size - 1] = {};
    auto p = image_header::parse(buf);
    ALLOY_CHECK(!p);
    ALLOY_CHECK(p.error() == ota_error::truncated);
}

ALLOY_TEST(ota_verify_payload_good_and_bad) {
    std::uint8_t payload[64];
    fill_pattern(payload, 9);
    image_header h;
    h.payload_length = sizeof(payload);
    h.payload_crc32 = crc::crc32_of(payload);

    ALLOY_CHECK(static_cast<bool>(h.verify_payload(payload)));
    payload[20] ^= 1u;
    auto bad = h.verify_payload(payload);
    ALLOY_CHECK(!bad);
    ALLOY_CHECK(bad.error() == ota_error::bad_payload_crc);
    // Too few bytes -> truncated.
    ALLOY_CHECK(h.verify_payload(std::span<const std::uint8_t>{payload, 10}).error() == ota_error::truncated);
}

// ── boot-state machine (pure decide + manager over real nvm::store) ──────────

ALLOY_TEST(ota_boot_decide_invariants_exhaustive) {
    // For every reachable state, the boot decision must (1) boot only `active` or
    // `pending`, and (2) NEVER mutate active. This is the anti-brick invariant.
    for (std::uint8_t active = 0; active <= 1; ++active) {
        for (int pi = 0; pi < 3; ++pi) {
            const std::uint8_t pending =
                (pi == 0) ? boot_state::no_slot : static_cast<std::uint8_t>(pi - 1);
            for (std::uint8_t att = 0; att <= 4; ++att) {
                const boot_state s{active, pending, att};
                const plan_result r = decide(s);
                ALLOY_CHECK(r.decision.slot == s.active || r.decision.slot == s.pending);
                ALLOY_CHECK_EQ(r.next.active, s.active);  // active is never touched by boot
            }
        }
    }
}

ALLOY_TEST(ota_boot_factory_defaults) {
    boot_manager boot{fresh_bootstore()};
    const boot_decision d = boot.plan_boot();
    ALLOY_CHECK_EQ(d.slot, 0u);
    ALLOY_CHECK(d.kind == boot_kind::normal);
    ALLOY_CHECK(!boot.update_in_progress());
    ALLOY_CHECK_EQ(boot.update_target(), 1u);
}

ALLOY_TEST(ota_boot_mark_and_confirm) {
    boot_manager boot{fresh_bootstore()};
    ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
    ALLOY_CHECK(boot.update_in_progress());

    const boot_decision d = boot.plan_boot();
    ALLOY_CHECK_EQ(d.slot, 1u);
    ALLOY_CHECK(d.kind == boot_kind::trial);

    ALLOY_CHECK(static_cast<bool>(boot.confirm()));
    ALLOY_CHECK_EQ(boot.active(), 1u);
    ALLOY_CHECK(!boot.update_in_progress());
    ALLOY_CHECK(boot.plan_boot().kind == boot_kind::normal);
    ALLOY_CHECK_EQ(boot.plan_boot().slot, 1u);
}

ALLOY_TEST(ota_boot_mark_rejects_bad_slot) {
    boot_manager boot{fresh_bootstore()};  // active = 0
    ALLOY_CHECK(boot.mark_updated(0).error() == ota_error::invalid_slot);  // == active
    ALLOY_CHECK(boot.mark_updated(2).error() == ota_error::invalid_slot);  // out of range
}

ALLOY_TEST(ota_boot_rollback_after_N) {
    boot_manager boot{fresh_bootstore(), 3u};  // N=3
    ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
    for (int i = 0; i < 3; ++i) {
        const boot_decision d = boot.plan_boot();  // three trial boots
        ALLOY_CHECK(d.kind == boot_kind::trial);
        ALLOY_CHECK_EQ(d.slot, 1u);
    }
    const boot_decision d = boot.plan_boot();  // 4th -> revert
    ALLOY_CHECK(d.kind == boot_kind::reverted);
    ALLOY_CHECK_EQ(d.slot, 0u);              // back to the confirmed slot
    ALLOY_CHECK(!boot.update_in_progress());  // pending cleared
    ALLOY_CHECK_EQ(boot.active(), 0u);
}

ALLOY_TEST(ota_boot_single_trial_N1) {
    boot_manager boot{fresh_bootstore(), 1u};  // N=1 (MCUboot/ESP single-trial)
    ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
    ALLOY_CHECK(boot.plan_boot().kind == boot_kind::trial);
    ALLOY_CHECK(boot.plan_boot().kind == boot_kind::reverted);
}

ALLOY_TEST(ota_boot_reject_and_confirm_idempotent) {
    boot_manager boot{fresh_bootstore()};
    ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
    ALLOY_CHECK(static_cast<bool>(boot.reject_pending()));
    ALLOY_CHECK(!boot.update_in_progress());
    ALLOY_CHECK(boot.plan_boot().kind == boot_kind::normal);
    // confirm/reject with nothing pending are no-op successes.
    ALLOY_CHECK(static_cast<bool>(boot.confirm()));
    ALLOY_CHECK(static_cast<bool>(boot.reject_pending()));
    ALLOY_CHECK_EQ(boot.active(), 0u);
}

// ── updater + verify_slot (over the NOR fake) ────────────────────────────────

ALLOY_TEST(ota_updater_roundtrip) {
    wipe_slots();
    std::uint8_t payload[300];
    fill_pattern(payload, 5);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 7u, payload, sizeof(payload));

    updater<fake_flash> up{fake_flash{}, slot_of(1), fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    // Feed in awkward chunk sizes to exercise the 8-byte staging + partial tail.
    for (std::uint32_t off = 0; off < n; off += 13u) {
        const std::uint32_t take = (n - off < 13u) ? n - off : 13u;
        ALLOY_CHECK(static_cast<bool>(up.write({img + off, take})));
    }
    ALLOY_CHECK_EQ(up.written(), n);
    auto h = up.finish();
    ALLOY_CHECK(static_cast<bool>(h));
    ALLOY_CHECK_EQ(h->image_version, 7u);
    // What landed in slot 1 is byte-for-byte the image.
    ALLOY_CHECK(std::memcmp(reinterpret_cast<const void*>(slot_base(1)), img, n) == 0);
}

ALLOY_TEST(ota_updater_tail_padding) {
    wipe_slots();
    std::uint8_t payload[13];  // header(32) + 13 = 45 bytes, not a multiple of 8
    fill_pattern(payload, 1);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 1u, payload, sizeof(payload));

    updater<fake_flash> up{fake_flash{}, slot_of(0), fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    ALLOY_CHECK(static_cast<bool>(up.write({img, n})));
    ALLOY_CHECK(static_cast<bool>(up.finish()));  // 0xFF tail padding excluded from CRC
}

ALLOY_TEST(ota_updater_rejects_corrupt_payload) {
    wipe_slots();
    std::uint8_t payload[64];
    fill_pattern(payload, 2);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 1u, payload, sizeof(payload));
    img[image_header::byte_size + 30] ^= 0xFFu;  // corrupt a payload byte

    updater<fake_flash> up{fake_flash{}, slot_of(1), fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    ALLOY_CHECK(static_cast<bool>(up.write({img, n})));
    auto h = up.finish();
    ALLOY_CHECK(!h);
    ALLOY_CHECK(h.error() == ota_error::bad_payload_crc);
}

ALLOY_TEST(ota_updater_slot_overflow) {
    wipe_slots();
    std::uint8_t big[2048 + 64];  // bigger than the 2048-byte slot
    fill_pattern(big, 4);
    updater<fake_flash> up{fake_flash{}, slot_of(1), fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    // Some write, or finish, must report the overflow (never program past the slot).
    bool overflowed = false;
    for (std::uint32_t off = 0; off < sizeof(big) && !overflowed; off += 64u) {
        if (auto r = up.write({big + off, 64u}); !r) {
            overflowed = (r.error() == ota_error::slot_overflow);
            break;
        }
    }
    ALLOY_CHECK(overflowed);
}

ALLOY_TEST(ota_updater_flash_io) {
    flaky_flash::calls = 0;
    flaky_flash::fail_at = 0;  // first flash op (an erase) fails
    updater<flaky_flash> up{flaky_flash{}, slot_of(1), flaky_flash::page_size};
    ALLOY_CHECK(up.begin().error() == ota_error::flash_io);
    flaky_flash::fail_at = -1;
}

ALLOY_TEST(ota_updater_leaves_other_slot_untouched) {
    wipe_slots();
    // Put a marker in slot 0, then write an image into slot 1; slot 0 stays intact.
    std::uint8_t marker[2048];
    fill_pattern(marker, 0x11);
    std::memcpy(reinterpret_cast<void*>(slot_base(0)), marker, sizeof(marker));

    std::uint8_t payload[100];
    fill_pattern(payload, 6);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 1u, payload, sizeof(payload));
    updater<fake_flash> up{fake_flash{}, slot_of(1), fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    ALLOY_CHECK(static_cast<bool>(up.write({img, n})));
    ALLOY_CHECK(static_cast<bool>(up.finish()));
    ALLOY_CHECK(std::memcmp(reinterpret_cast<const void*>(slot_base(0)), marker, sizeof(marker)) == 0);
}

ALLOY_TEST(ota_verify_slot_good_and_corrupt) {
    wipe_slots();
    std::uint8_t payload[80];
    fill_pattern(payload, 8);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 3u, payload, sizeof(payload));
    std::memcpy(reinterpret_cast<void*>(slot_base(1)), img, n);

    ALLOY_CHECK(static_cast<bool>(verify_slot(slot_of(1))));
    // Corrupt a payload byte in flash.
    reinterpret_cast<std::uint8_t*>(slot_base(1))[image_header::byte_size + 40] ^= 0xFFu;
    ALLOY_CHECK(verify_slot(slot_of(1)).error() == ota_error::bad_payload_crc);
    // A header claiming more than the slot holds -> slot_overflow (use a tiny slot).
    ALLOY_CHECK(verify_slot(slot{slot_base(1), image_header::byte_size + 8u}).error()
                == ota_error::slot_overflow);
}

// ── full A/B lifecycle + power-loss (slots + real nvm boot-state) ────────────

ALLOY_TEST(ota_lifecycle_update_confirm) {
    wipe_slots();
    auto store = fresh_bootstore();

    std::uint8_t payload[256];
    fill_pattern(payload, 7);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 2u, payload, sizeof(payload));

    {  // running from slot 0, install a new image into slot 1
        boot_manager boot{store};
        const std::uint8_t tgt = boot.update_target();
        ALLOY_CHECK_EQ(tgt, 1u);
        updater<fake_flash> up{fake_flash{}, slot_of(tgt), fake_flash::page_size};
        ALLOY_CHECK(static_cast<bool>(up.begin()));
        ALLOY_CHECK(static_cast<bool>(up.write({img, n})));
        ALLOY_CHECK(static_cast<bool>(install(up, boot, tgt)));
    }
    {  // "reboot": bootloader plans a trial boot of slot 1, which verifies
        boot_manager boot{reopen_bootstore()};
        const boot_decision d = boot.plan_boot();
        ALLOY_CHECK(d.kind == boot_kind::trial);
        ALLOY_CHECK_EQ(d.slot, 1u);
        ALLOY_CHECK(static_cast<bool>(verify_slot(slot_of(d.slot))));
        ALLOY_CHECK(static_cast<bool>(boot.confirm()));  // app health check passed
    }
    {  // next reboot: steady state on slot 1
        boot_manager boot{reopen_bootstore()};
        const boot_decision d = boot.plan_boot();
        ALLOY_CHECK(d.kind == boot_kind::normal);
        ALLOY_CHECK_EQ(d.slot, 1u);
    }
}

ALLOY_TEST(ota_lifecycle_update_then_rollback) {
    auto store = fresh_bootstore();
    {  // start already confirmed on slot 1, arm a trial in slot 0
        boot_manager boot{store};
        ALLOY_CHECK(static_cast<bool>(boot.confirm()));  // no-op, active stays 0
        ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
        ALLOY_CHECK(static_cast<bool>(boot.confirm()));  // now active = 1
        ALLOY_CHECK_EQ(boot.active(), 1u);
        ALLOY_CHECK(static_cast<bool>(boot.mark_updated(0)));  // trial slot 0
    }
    // Never confirm; exhaust the trials across reboots -> revert to slot 1.
    boot_kind last = boot_kind::normal;
    for (int i = 0; i < 5; ++i) {
        boot_manager boot{reopen_bootstore(), 3u};
        last = boot.plan_boot().kind;
        if (last == boot_kind::reverted) {
            ALLOY_CHECK_EQ(boot.active(), 1u);  // reverted to the confirmed slot
            break;
        }
        ALLOY_CHECK(last == boot_kind::trial);
    }
    ALLOY_CHECK(last == boot_kind::reverted);
}

ALLOY_TEST(ota_powerloss_crash_before_mark_updated) {
    wipe_slots();
    auto store = fresh_bootstore();
    std::uint8_t payload[128];
    fill_pattern(payload, 3);
    std::uint8_t img[image_header::byte_size + sizeof(payload)];
    const std::uint32_t n = make_image(img, 9u, payload, sizeof(payload));

    // Fully write + verify slot 1, but "crash" before install()/mark_updated.
    updater<fake_flash> up{fake_flash{}, slot_of(1), fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    ALLOY_CHECK(static_cast<bool>(up.write({img, n})));
    ALLOY_CHECK(static_cast<bool>(up.finish()));  // verified, but NOT armed
    (void)store;

    // Fresh boot after the crash: the trial was never armed -> boot the old slot 0.
    boot_manager boot{reopen_bootstore()};
    const boot_decision d = boot.plan_boot();
    ALLOY_CHECK(d.kind == boot_kind::normal);
    ALLOY_CHECK_EQ(d.slot, 0u);
}

ALLOY_TEST(ota_persistence_survives_reset) {
    auto store = fresh_bootstore();
    {
        boot_manager boot{store};
        ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
        ALLOY_CHECK(static_cast<bool>(boot.confirm()));  // active -> 1
    }
    // Re-open over the SAME flash bytes (a power cycle): state reads back identical.
    boot_manager boot{reopen_bootstore()};
    ALLOY_CHECK_EQ(boot.active(), 1u);
    ALLOY_CHECK(!boot.update_in_progress());
}

// ── power-atomic boot_store: torn writes + churn (the anti-brick foundation) ──

// A write interrupted at ANY program step leaves the PRIOR committed value intact
// (the other ping-pong page is untouched) — the property nvm::store's compaction
// cannot provide, and the whole reason boot_store exists.
ALLOY_TEST(ota_bootstore_torn_write_keeps_prior) {
    for (int fail_step = 0; fail_step <= 1; ++fail_step) {  // interrupt dw0, then dw1
        auto s = fresh_bootstore();
        ALLOY_CHECK(s.set(0, 111u));  // committed
        bs_flash::prog_calls = 0;
        bs_flash::fail_prog_at = fail_step;
        ALLOY_CHECK(!s.set(0, 222u));  // torn write: erase stale page, program fails
        bs_flash::fail_prog_at = -1;
        // A fresh handle (reboot) reads back the last COMMITTED value, not garbage.
        ALLOY_CHECK_EQ(reopen_bootstore().get(0, 999u), 111u);
    }
}

// Far more writes than one page holds: the ping-pong keeps the latest with no
// compaction (nvm::store would erase-rewrite here — the non-atomic window).
ALLOY_TEST(ota_bootstore_survives_churn) {
    auto s = fresh_bootstore();
    for (std::uint32_t i = 1; i <= 50u; ++i) {
        ALLOY_CHECK(s.set(0, i));
        ALLOY_CHECK_EQ(reopen_bootstore().get(0, 0u), i);
    }
}

// ── save-failure handling (fail safe, never a torn/booting-wrong state) ──────

// If plan_boot() cannot persist the consumed trial attempt, it boots the known-good
// active slot rather than a trial it can't bound (which would loop forever).
ALLOY_TEST(ota_plan_boot_failsafe_on_save_error) {
    auto s = fresh_bootstore();
    boot_manager boot{s};
    ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));  // arm the trial
    bs_flash::prog_calls = 0;
    bs_flash::fail_prog_at = 0;  // fail the decrement save
    const boot_decision d = boot.plan_boot();
    bs_flash::fail_prog_at = -1;
    ALLOY_CHECK(d.kind == boot_kind::normal);  // fell back...
    ALLOY_CHECK_EQ(d.slot, 0u);                // ...to the confirmed slot, not the trial
}

// A dropped save is reported (flash_io) and leaves the PRIOR state intact, so a
// retry succeeds — never a half-applied transition.
ALLOY_TEST(ota_transition_save_failure_reports_flash_io) {
    {  // mark_updated: nothing gets armed on a failed save
        auto s = fresh_bootstore();
        boot_manager boot{s};
        bs_flash::prog_calls = 0;
        bs_flash::fail_prog_at = 0;
        ALLOY_CHECK(boot.mark_updated(1).error() == ota_error::flash_io);
        bs_flash::fail_prog_at = -1;
        ALLOY_CHECK(!boot.update_in_progress());
    }
    {  // confirm: failed save keeps the trial, a retry confirms
        auto s = fresh_bootstore();
        boot_manager boot{s};
        ALLOY_CHECK(static_cast<bool>(boot.mark_updated(1)));
        bs_flash::prog_calls = 0;
        bs_flash::fail_prog_at = 0;
        ALLOY_CHECK(boot.confirm().error() == ota_error::flash_io);
        bs_flash::fail_prog_at = -1;
        ALLOY_CHECK(boot.update_in_progress());  // still pending — not half-applied
        ALLOY_CHECK(static_cast<bool>(boot.confirm()));
        ALLOY_CHECK_EQ(boot.active(), 1u);
    }
}

// After a write error the updater is fenced: a subsequent write() returns not_open
// instead of touching the staging buffer out of bounds.
ALLOY_TEST(ota_updater_fenced_after_error) {
    wipe_slots();
    updater<fake_flash> up{fake_flash{}, slot{slot_base(1), 16u}, fake_flash::page_size};
    ALLOY_CHECK(static_cast<bool>(up.begin()));
    std::uint8_t data[64];
    fill_pattern(data, 1);
    auto r1 = up.write({data, sizeof(data)});
    ALLOY_CHECK(!r1);
    ALLOY_CHECK(r1.error() == ota_error::slot_overflow);
    ALLOY_CHECK(up.write({data, sizeof(data)}).error() == ota_error::not_open);  // fenced
}
