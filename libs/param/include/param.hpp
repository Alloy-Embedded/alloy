// Runtime parameter registry — the piece between a wire protocol and a
// device's tunables: a constexpr descriptor table (wire id, default, range,
// props), a RAM mirror sized for control loops that read parameters dozens
// of times per tick (get_at() is an array index, nothing else), strict
// writes that refuse out-of-range values (so a Modbus FC06 can answer
// exception 0x03 with the registry's own verdict), and torn-write-safe
// persistence over two independently-erasable flash regions.
//
// Deliberately TWO classes, not one: `registry` is pure RAM semantics
// (host-testable with no flash anywhere), `bank` is the persistence engine
// binding a registry to a FlashController. Declaration of parameters is NOT
// this library's job — the table arrives as data (hand-written, or emitted
// by codegen from product definitions); this library gives it behavior.
//
// Persistence model: ping-pong. A commit always writes the OTHER region
// first and only counts on success, so power loss mid-write leaves the
// previous generation intact — the same rule alloy::ota::boot_store
// established. A record is [magic | seq | count | values | crc32]; the
// newest of the two valid regions wins on load. Deferred commit batches a
// burst of writes into one erase cycle: poll() commits after the registry
// has been quiet for `quiet_ms` (clock injected — the house rule).
//
// ── WHEN NOT TO USE THIS LIBRARY ────────────────────────────────────────────
// This registry is OPINIONATED, and three of its opinions are constructor
// traps or format commitments. A product that ports an EXISTING parameter
// system should check all four before adopting it; a standalone parameter
// service in the application is the correct pattern where any one applies —
// that is not a workaround, it is the design (this library serves products
// born on alloy):
//
//   * STORED DEFAULTS IN A DIFFERENT ENCODING THAN MIN/MAX. The registry
//     requires def within [min, max] and TRAPS in the constructor otherwise.
//     Fielded tables exist where dozens of rows store defaults in a raw
//     encoding the bounds do not describe — those tables cannot be loaded,
//     by design, and no flag disables the check.
//   * A PRE-EXISTING ON-FLASH IMAGE FORMAT. The bank's ping-pong layout
//     ([magic|seq|count|values|crc32], two regions) is not negotiable, and
//     adopting it over a fleet that already stores another layout means every
//     unit factory-resets on its first post-update boot.
//   * RUNTIME-WRITABLE DEFAULTS OR PROPERTIES. Here the descriptor table is
//     constexpr: defaults, bounds and flags are code. A product whose
//     defaults are DATA (an OEM image rewritten in the field) needs a
//     service that persists them, which this deliberately is not.
//   * PRESET VALUE SETS (per-row alternate value columns selected at
//     runtime). No column exists for them.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"
#include "alloy/ota/crc32.hpp"
#include "alloy/util/result.hpp"

namespace alloy::lib::param {

enum class set_error : std::uint8_t {
    unknown_id = 1,  // no descriptor carries this wire id
    read_only,       // the descriptor forbids writes from the wire
    out_of_range,    // outside [min, max] — the FC06-exception-0x03 case
};

struct descriptor {
    std::uint16_t id;       // wire id (e.g. the Modbus register number)
    std::int16_t def;
    std::int16_t min;
    std::int16_t max;
    bool persist = true;    // false: runtime-only (never committed)
    bool read_only = false; // false: writable from the wire
};

// RAM semantics over a caller-owned descriptor table. The table must be
// sorted by id (binary search is the wire path) and every default in range —
// both verified at construction, loudly: a bad table is a build-time data
// bug, not a runtime condition to limp through.
template <std::size_t N>
class registry {
    static_assert(N >= 1, "an empty parameter table has no reason to exist");

public:
    constexpr explicit registry(std::span<const descriptor, N> table)
        : table_(table) {
        for (std::size_t i = 0; i < N; ++i) {
            const auto& d = table_[i];
            if (i != 0u && table_[i - 1u].id >= d.id) {
                __builtin_trap();  // unsorted/duplicate id: table data bug
            }
            if (d.def < d.min || d.def > d.max) {
                __builtin_trap();  // default outside its own range
            }
            values_[i] = d.def;
        }
    }

    static constexpr std::size_t size = N;

    // The control-loop path: called tens of times per tick, so it is an
    // array read, full stop. Index comes from index_of() resolved once.
    [[nodiscard]] std::int16_t get_at(std::size_t index) const {
        return values_[index];
    }
    [[nodiscard]] const descriptor& describe_at(std::size_t index) const {
        return table_[index];
    }

    [[nodiscard]] Result<std::size_t, set_error> index_of(std::uint16_t id) const {
        std::size_t lo = 0, hi = N;
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2u;
            if (table_[mid].id < id) {
                lo = mid + 1u;
            } else {
                hi = mid;
            }
        }
        if (lo == N || table_[lo].id != id) {
            return set_error::unknown_id;
        }
        return lo;
    }

    [[nodiscard]] Result<std::int16_t, set_error> get(std::uint16_t id) const {
        const auto i = index_of(id);
        if (!i) {
            return i.error();
        }
        return values_[*i];
    }

    // Strict write — the WIRE's entry point: refuses instead of clamping, so
    // the protocol layer answers with the true reason (0x02 vs 0x03).
    [[nodiscard]] Result<void, set_error> set(std::uint16_t id, std::int16_t v) {
        const auto i = index_of(id);
        if (!i) {
            return i.error();
        }
        const auto& d = table_[*i];
        if (d.read_only) {
            return set_error::read_only;
        }
        if (v < d.min || v > d.max) {
            return set_error::out_of_range;
        }
        if (values_[*i] != v) {
            values_[*i] = v;
            dirty_ = true;
        }
        return ok<set_error>();
    }

    // Clamping write — the PRESET/factory path: bulk applications land on
    // the nearest legal value rather than aborting a whole profile.
    [[nodiscard]] Result<void, set_error> set_clamped(std::uint16_t id, std::int16_t v) {
        const auto i = index_of(id);
        if (!i) {
            return i.error();
        }
        const auto& d = table_[*i];
        const std::int16_t c = v < d.min ? d.min : (v > d.max ? d.max : v);
        if (values_[*i] != c) {
            values_[*i] = c;
            dirty_ = true;
        }
        return ok<set_error>();
    }

    void reset_to_defaults() {
        for (std::size_t i = 0; i < N; ++i) {
            values_[i] = table_[i].def;
        }
        dirty_ = true;
    }

    [[nodiscard]] bool dirty() const { return dirty_; }
    void clear_dirty() { dirty_ = false; }

    // Raw mirror for the persistence engine (and bulk protocol reads).
    [[nodiscard]] std::span<const std::int16_t, N> values() const {
        return std::span<const std::int16_t, N>{values_};
    }
    // Restore path (bank::load): bypasses range checks on purpose — flash
    // holds what an OLDER schema found legal; out-of-range survivors are
    // re-clamped so a tightened range heals the stored value instead of
    // resetting the device.
    void restore_at(std::size_t index, std::int16_t v) {
        const auto& d = table_[index];
        values_[index] = v < d.min ? d.min : (v > d.max ? d.max : v);
    }

private:
    std::span<const descriptor, N> table_;
    std::int16_t values_[N]{};
    bool dirty_ = false;
};

// Persistence engine: two regions, ping-pong, newest-valid-wins.
template <alloy::FlashController Flash>
class bank {
public:
    struct config {
        std::uintptr_t region_a;
        std::uintptr_t region_b;
        std::uint32_t region_bytes;  // per region, a multiple of page_size
        std::uint32_t page_size;
        std::uint32_t quiet_ms = 500;  // deferred-commit window
    };

    constexpr bank(const Flash& flash, config c) : flash_(flash), c_(c) {}

    // Newest valid region wins; both invalid = factory state (defaults stay).
    template <std::size_t N>
    [[nodiscard]] bool load(registry<N>& reg) {
        const auto a = inspect(c_.region_a, N);
        const auto b = inspect(c_.region_b, N);
        if (!a.valid && !b.valid) {
            return false;
        }
        const std::uintptr_t src = (!b.valid || (a.valid && newer(a.seq, b.seq)))
                                       ? c_.region_a
                                       : c_.region_b;
        seq_ = (src == c_.region_a ? a.seq : b.seq);
        const auto* v = reinterpret_cast<const volatile std::int16_t*>(src + 16u);
        for (std::size_t i = 0; i < N; ++i) {
            reg.restore_at(i, static_cast<std::int16_t>(v[i]));
        }
        reg.clear_dirty();
        return true;
    }

    // Immediate commit: erase + program the OTHER region, then adopt it.
    // Power loss anywhere in here leaves the previous generation readable.
    template <std::size_t N>
    [[nodiscard]] bool commit(registry<N>& reg) {
        const auto cur = inspect(c_.region_a, N);
        const std::uintptr_t target =
            (cur.valid && seq_ == cur.seq) ? c_.region_b : c_.region_a;
        for (std::uint32_t off = 0; off < c_.region_bytes; off += c_.page_size) {
            if (!flash_.erase_page(target + off)) {
                return false;
            }
        }
        const std::uint32_t next_seq = seq_ + 1u;
        // Header dwords: [magic|seq] and [count|reserved]. Magic from chars —
        // a 32-bit literal here would be an 8-hex-digit fact-shaped constant.
        const std::uint32_t magic = pack_magic();
        alloy::ota::crc::crc32 crc;
        std::uint64_t hdr0 = magic | (static_cast<std::uint64_t>(next_seq) << 32);
        std::uint64_t hdr1 = static_cast<std::uint32_t>(N);
        crc_update_dword(crc, hdr0);
        crc_update_dword(crc, hdr1);
        if (!flash_.program(target, &hdr0, 1u) ||
            !flash_.program(target + 8u, &hdr1, 1u)) {
            return false;
        }
        const auto vals = reg.values();
        std::uint64_t dw = 0;
        std::size_t in_dw = 0;
        std::uintptr_t at = target + 16u;
        for (std::size_t i = 0; i < N; ++i) {
            dw |= static_cast<std::uint64_t>(static_cast<std::uint16_t>(vals[i]))
                  << (16u * in_dw);
            if (++in_dw == 4u || i + 1u == N) {
                crc_update_dword(crc, dw);
                if (!flash_.program(at, &dw, 1u)) {
                    return false;
                }
                at += 8u;
                dw = 0;
                in_dw = 0;
            }
        }
        std::uint64_t trailer = crc.value();
        if (!flash_.program(at, &trailer, 1u)) {
            return false;
        }
        seq_ = next_seq;
        reg.clear_dirty();
        pending_ = false;
        return true;
    }

    // Deferred commit: a burst of writes becomes ONE erase cycle. The first
    // poll that sees the registry dirty starts the quiet window; a commit
    // fires once no new write has arrived for quiet_ms.
    template <std::size_t N>
    [[nodiscard]] bool poll(registry<N>& reg, std::uint32_t now_ms) {
        if (reg.dirty()) {
            reg.clear_dirty();
            pending_ = true;
            last_change_ms_ = now_ms;
            return false;
        }
        if (pending_ && now_ms - last_change_ms_ >= c_.quiet_ms) {
            return commit(reg);
        }
        return false;
    }

private:
    struct probe {
        bool valid;
        std::uint32_t seq;
    };

    static constexpr std::uint32_t pack_magic() {
        return static_cast<std::uint32_t>('A') |
               static_cast<std::uint32_t>('P') << 8 |
               static_cast<std::uint32_t>('R') << 16 |
               static_cast<std::uint32_t>('1') << 24;
    }
    static void crc_update_dword(alloy::ota::crc::crc32& c, std::uint64_t dw) {
        std::uint8_t b[8];
        for (unsigned i = 0; i < 8u; ++i) {
            b[i] = static_cast<std::uint8_t>(dw >> (8u * i));
        }
        c.update({b, 8u});
    }
    // Wrap-safe "a is newer than b" (same idiom as the async timer deadline).
    static constexpr bool newer(std::uint32_t a, std::uint32_t b) {
        return static_cast<std::int32_t>(a - b) > 0;
    }

    [[nodiscard]] probe inspect(std::uintptr_t base, std::size_t n) const {
        const auto* p = reinterpret_cast<const volatile std::uint32_t*>(base);
        if (p[0] != pack_magic() || p[2] != n) {
            return {false, 0};
        }
        const std::size_t value_dwords = (n + 3u) / 4u;
        alloy::ota::crc::crc32 crc;
        const auto* dws = reinterpret_cast<const volatile std::uint64_t*>(base);
        for (std::size_t i = 0; i < 2u + value_dwords; ++i) {
            crc_update_dword(crc, dws[i]);
        }
        const std::uint32_t stored =
            *reinterpret_cast<const volatile std::uint32_t*>(base + 16u +
                                                             value_dwords * 8u);
        if (stored != crc.value()) {
            return {false, 0};
        }
        return {true, p[1]};
    }

    const Flash& flash_;
    config c_;
    std::uint32_t seq_ = 0;
    std::uint32_t last_change_ms_ = 0;
    bool pending_ = false;
};

}  // namespace alloy::lib::param
