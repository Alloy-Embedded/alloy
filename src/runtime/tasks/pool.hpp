#pragma once

// Fixed-capacity coroutine frame pool.
//
// The C++20 coroutine spec lets the promise type override `operator new` /
// `operator delete`; this pool is what those overrides allocate from. Every
// scheduler instance owns one. The pool has `Slots` fixed-size slots, each big
// enough for `BytesPerSlot` of frame. A request larger than the slot size is
// rejected (returns nullptr) rather than spilling onto another slot or the
// heap. The bitmap-indexed free list keeps allocate/deallocate at O(slots/64).
//
// Thread-safety: none. Cooperative single-threaded scheduler => the pool is
// only ever touched from one execution context. ISR posters do not allocate.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace alloy::tasks {

template <std::size_t Slots, std::size_t BytesPerSlot>
class FramePool {
    static_assert(Slots > 0, "FramePool needs at least one slot");
    static_assert(BytesPerSlot >= 32, "BytesPerSlot must be at least 32; smaller is rejected as misuse");
    static_assert(BytesPerSlot % alignof(std::max_align_t) == 0,
                  "BytesPerSlot must be aligned to max_align_t");

   public:
    static constexpr std::size_t kSlots = Slots;
    static constexpr std::size_t kBytesPerSlot = BytesPerSlot;

    [[nodiscard]] constexpr auto slot_size() const noexcept -> std::size_t { return BytesPerSlot; }
    [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t { return Slots; }
    [[nodiscard]] auto in_use() const noexcept -> std::size_t {
        std::size_t n = 0;
        for (auto word : free_bitmap_) n += static_cast<std::size_t>(std::popcount(~word));
        // The trailing bits past `Slots` are always set to 1 in `free_bitmap_`
        // (they represent "not free, never available"); subtract their count.
        const std::size_t trailing = words_ * 64u - Slots;
        return n - trailing;
    }

    [[nodiscard]] auto allocate(std::size_t bytes) noexcept -> void* {
        if (bytes > BytesPerSlot) {
            last_oversize_request_ = bytes;
            return nullptr;
        }
        for (std::size_t w = 0; w < words_; ++w) {
            std::uint64_t free_word = free_bitmap_[w];
            if (free_word == 0u) continue;
            const auto bit = static_cast<std::size_t>(std::countr_zero(free_word));
            const std::size_t idx = w * 64u + bit;
            if (idx >= Slots) continue;  // padding bit
            free_bitmap_[w] &= ~(std::uint64_t{1} << bit);
            return &storage_[idx * BytesPerSlot];
        }
        return nullptr;
    }

    void deallocate(void* ptr) noexcept {
        if (ptr == nullptr) return;
        const auto* p = static_cast<std::byte*>(ptr);
        const std::size_t offset = static_cast<std::size_t>(p - storage_.data());
        const std::size_t idx = offset / BytesPerSlot;
        if (idx >= Slots) return;
        const std::size_t w = idx / 64u;
        const std::size_t b = idx % 64u;
        free_bitmap_[w] |= (std::uint64_t{1} << b);
    }

    /// Bytes the most recent oversize allocation requested. Used by the
    /// scheduler to surface a useful error message when spawn fails.
    [[nodiscard]] auto last_oversize_request() const noexcept -> std::size_t {
        return last_oversize_request_;
    }

    FramePool() noexcept {
        // Mark every real slot free; the trailing padding bits stay 0
        // ("permanently in use") so `allocate` never picks them.
        for (auto& word : free_bitmap_) word = 0;
        for (std::size_t i = 0; i < Slots; ++i) {
            free_bitmap_[i / 64u] |= (std::uint64_t{1} << (i % 64u));
        }
    }

   private:
    static constexpr std::size_t words_ = (Slots + 63u) / 64u;

    alignas(std::max_align_t) std::array<std::byte, Slots * BytesPerSlot> storage_{};
    std::array<std::uint64_t, words_> free_bitmap_{};
    std::size_t last_oversize_request_ = 0;
};

}  // namespace alloy::tasks
