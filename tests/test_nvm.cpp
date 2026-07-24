// Unit tests for the portable NVM key/value store (alloy/util/nvm_kv.hpp),
// driven by a RAM-backed fake flash. The STM32 flash HAL is exercised on
// silicon (the nvm example); here we pin the store logic that hardware can't
// cheaply reach: latest-wins reads, append, and the compaction/erase path when
// the region fills.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "alloy/util/nvm_kv.hpp"
#include "alloy_test.hpp"

namespace {
// One 64-byte "page" in RAM = 8 record slots. erase sets it to 0xFF (like real
// flash); program writes double-words. Shared static so the store's own flash_
// instance and the test see the same bytes.
struct fake_flash {
    alignas(std::uint64_t) static inline std::uint8_t region[64];

    [[nodiscard]] bool erase_page(std::uintptr_t) const {
        std::memset(region, 0xFF, sizeof(region));
        return true;
    }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* data,
                               std::size_t n) const {
        for (std::size_t i = 0; i < n; ++i) {
            *reinterpret_cast<std::uint64_t*>(addr + i * 8u) = data[i];
        }
        return true;
    }
};

std::uintptr_t base() { return reinterpret_cast<std::uintptr_t>(fake_flash::region); }
void fresh() { std::memset(fake_flash::region, 0xFF, sizeof(fake_flash::region)); }
}  // namespace

using kv = alloy::nvm::store<fake_flash>;

ALLOY_TEST(nvm_empty_returns_fallback) {
    fresh();
    kv s{base(), 64u};
    ALLOY_CHECK_EQ(s.get(1u, 7u), 7u);
    ALLOY_CHECK_EQ(s.get(42u), 0u);  // default fallback
}

ALLOY_TEST(nvm_set_then_get) {
    fresh();
    kv s{base(), 64u};
    ALLOY_CHECK(s.set(1u, 100u));
    ALLOY_CHECK_EQ(s.get(1u), 100u);
    ALLOY_CHECK_EQ(s.get(2u, 9u), 9u);  // untouched key
}

ALLOY_TEST(nvm_latest_write_wins) {
    fresh();
    kv s{base(), 64u};
    ALLOY_CHECK(s.set(1u, 10u));
    ALLOY_CHECK(s.set(1u, 20u));
    ALLOY_CHECK(s.set(1u, 30u));
    ALLOY_CHECK_EQ(s.get(1u), 30u);
}

ALLOY_TEST(nvm_multiple_keys_independent) {
    fresh();
    kv s{base(), 64u};
    ALLOY_CHECK(s.set(1u, 111u));
    ALLOY_CHECK(s.set(2u, 222u));
    ALLOY_CHECK(s.set(1u, 333u));
    ALLOY_CHECK_EQ(s.get(1u), 333u);
    ALLOY_CHECK_EQ(s.get(2u), 222u);
}

ALLOY_TEST(nvm_compacts_when_full_preserving_values) {
    fresh();
    kv s{base(), 64u};  // 8 slots
    // 10 writes to one key > 8 slots: forces a compaction mid-way.
    for (std::uint32_t v = 1; v <= 10u; ++v) {
        ALLOY_CHECK(s.set(1u, v));
    }
    ALLOY_CHECK_EQ(s.get(1u), 10u);
    // still usable after compaction
    ALLOY_CHECK(s.set(1u, 99u));
    ALLOY_CHECK_EQ(s.get(1u), 99u);
}

ALLOY_TEST(nvm_compaction_keeps_all_live_keys) {
    fresh();
    kv s{base(), 64u};
    // interleave two keys past the 8-slot capacity so compaction must keep both
    for (std::uint32_t v = 1; v <= 6u; ++v) {
        ALLOY_CHECK(s.set(1u, v));
        ALLOY_CHECK(s.set(2u, v * 10u));
    }  // 12 writes total -> compaction happened
    ALLOY_CHECK_EQ(s.get(1u), 6u);
    ALLOY_CHECK_EQ(s.get(2u), 60u);
}
