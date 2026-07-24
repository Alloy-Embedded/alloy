// Tiny persistent key/value store over a flash region — the "remember a few
// settings across resets" layer end users actually reach for (boot counters,
// calibration, a device id). Portable: written against the FlashController
// concept, so the SAME store runs on any chip whose flash HAL satisfies it, and
// unit-tests on the host against a fake flash.
//
// Format: append-only log of fixed 8-byte records = {key:u32 high, value:u32
// low}, which is exactly the STM32 flash program unit (one double-word). The
// latest record for a key wins; an all-ones slot (0xFFFF'FFFF key) is free.
// set() appends; when the region fills it compacts — collect each key's latest
// value, erase the page, rewrite them contiguously. Keys must not be
// 0xFFFF'FFFF (that value marks a free slot).
//
// The region is ONE flash page (erase granularity); Size == the page size.

#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/concepts.hpp"

namespace alloy::nvm {

template <alloy::FlashController Flash, std::uint32_t MaxKeys = 16>
class store {
    static constexpr std::uint32_t empty_key = 0xFFFF'FFFFu;  // erased flash

    std::uintptr_t base_;
    std::uint32_t slot_count_;
    Flash flash_{};

    const volatile std::uint64_t* slots() const {
        return reinterpret_cast<const volatile std::uint64_t*>(base_);
    }
    static std::uint32_t key_of(std::uint64_t rec) { return static_cast<std::uint32_t>(rec >> 32); }
    static std::uint32_t val_of(std::uint64_t rec) { return static_cast<std::uint32_t>(rec); }

public:
    // `base` is the (page-aligned) start of the reserved flash region, `size`
    // its length in bytes (one erase page). Both are compile-time constants on
    // silicon (board data) but plain values so the store unit-tests on the host.
    constexpr store(std::uintptr_t base, std::uint32_t size)
        : base_(base), slot_count_(size / 8u) {}
    // Latest value stored for `key`, or `fallback` if it was never written.
    [[nodiscard]] std::uint32_t get(std::uint32_t key, std::uint32_t fallback = 0u) const {
        std::uint32_t out = fallback;
        for (std::uint32_t i = 0; i < slot_count_; ++i) {
            const std::uint32_t k = key_of(slots()[i]);
            if (k == empty_key) {
                break;  // append-only: the first free slot ends the live data
            }
            if (k == key) {
                out = val_of(slots()[i]);
            }
        }
        return out;
    }

    // Persist key=value. Appends a record; compacts first if the region is full.
    // Returns false on a flash error or if more than MaxKeys distinct keys exist.
    [[nodiscard]] bool set(std::uint32_t key, std::uint32_t value) {
        std::uint32_t i = free_slot();
        if (i >= slot_count_) {
            if (!compact()) {
                return false;
            }
            i = free_slot();
            if (i >= slot_count_) {
                return false;  // region too small for the live set
            }
        }
        const std::uint64_t rec = (static_cast<std::uint64_t>(key) << 32) | value;
        return flash_.program(base_ + static_cast<std::uintptr_t>(i) * 8u, &rec, 1u);
    }

private:
    [[nodiscard]] std::uint32_t free_slot() const {
        for (std::uint32_t i = 0; i < slot_count_; ++i) {
            if (key_of(slots()[i]) == empty_key) {
                return i;
            }
        }
        return slot_count_;
    }

    // Garbage-collect: keep each key's latest value, erase the page, rewrite.
    [[nodiscard]] bool compact() {
        std::uint32_t keys[MaxKeys];
        std::uint32_t vals[MaxKeys];
        std::uint32_t n = 0;
        for (std::uint32_t i = 0; i < slot_count_; ++i) {
            const std::uint64_t rec = slots()[i];
            const std::uint32_t k = key_of(rec);
            if (k == empty_key) {
                break;
            }
            std::uint32_t j = 0;
            for (; j < n; ++j) {
                if (keys[j] == k) {
                    break;
                }
            }
            if (j == n) {
                if (n >= MaxKeys) {
                    return false;  // more distinct keys than the store can gc
                }
                keys[n] = k;
                vals[n] = val_of(rec);
                ++n;
            } else {
                vals[j] = val_of(rec);
            }
        }
        if (!flash_.erase_page(base_)) {
            return false;
        }
        for (std::uint32_t i = 0; i < n; ++i) {
            const std::uint64_t rec = (static_cast<std::uint64_t>(keys[i]) << 32) | vals[i];
            if (!flash_.program(base_ + static_cast<std::uintptr_t>(i) * 8u, &rec, 1u)) {
                return false;
            }
        }
        return true;
    }
};

// No-op stand-in for boards without an nvm role — get() returns the fallback,
// set() reports failure, so caps-guarded code compiles everywhere.
struct null_store {
    [[nodiscard]] std::uint32_t get(std::uint32_t, std::uint32_t fallback = 0u) const {
        return fallback;
    }
    [[nodiscard]] bool set(std::uint32_t, std::uint32_t) const { return false; }
};

}  // namespace alloy::nvm
