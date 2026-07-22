// User-facing DMA: a compile-time channel claim returning a move-aware
// token, plus typed 16-bit transfer starters (ADC samples and timer compare
// values are 16-bit — wider helpers arrive with a use case).
//
// Honest v1 doctrine (same as irq attach / double-open): claiming an
// already-claimed (controller, channel) TRAPS at runtime — C++ cannot make
// cross-TU resource claims a compile error. Completion is polled
// (wait/done); interrupt completion callbacks arrive with the DMA IRQ
// grouping story.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/arch/irq.hpp"
#include "alloy/hal/dma/dma_impl.hpp"

namespace alloy::dma {

template <class Inst, unsigned Ch>
class channel {
    using impl = hal::dma_impl<Inst>;

public:
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    channel(channel&&) noexcept = default;

    static channel claim() {
        const arch::irq_state saved = arch::irq_save();
        if (claimed_) {
            __builtin_trap();  // one token per (controller, channel) per firmware
        }
        claimed_ = true;
        arch::irq_restore(saved);
        impl::enable_controller();
        return channel{};
    }

    // One-shot peripheral -> memory, 16-bit items.
    void start_p2m_u16(std::uintptr_t periph_reg, std::span<std::uint16_t> dst,
                       std::uint8_t request) const {
        if (dst.empty() || dst.size() > 0xFFFF) {
            __builtin_trap();  // CNDTR is 16-bit
        }
        impl::template setup<Ch>(impl::dir::periph_to_mem, false, impl::width::b16,
                                 periph_reg,
                                 reinterpret_cast<std::uintptr_t>(dst.data()),
                                 static_cast<std::uint16_t>(dst.size()), request);
        impl::template start<Ch>();
    }

    // Circular memory -> peripheral, 16-bit items; runs until stop().
    // The source span must OUTLIVE the stream (static/global buffer).
    void start_m2p_circular_u16(std::span<const std::uint16_t> src,
                                std::uintptr_t periph_reg, std::uint8_t request) const {
        if (src.empty() || src.size() > 0xFFFF) {
            __builtin_trap();
        }
        impl::template setup<Ch>(impl::dir::mem_to_periph, true, impl::width::b16,
                                 periph_reg,
                                 reinterpret_cast<std::uintptr_t>(src.data()),
                                 static_cast<std::uint16_t>(src.size()), request);
        impl::template start<Ch>();
    }

    [[nodiscard]] bool done() const { return impl::template complete<Ch>(); }
    [[nodiscard]] bool error() const { return impl::template error<Ch>(); }

    // Block until complete; false on transfer error.
    [[nodiscard]] bool wait() const {
        while (!done()) {
            if (error()) {
                return false;
            }
        }
        return true;
    }

    void stop() const {
        impl::template stop<Ch>();
        impl::template clear_flags<Ch>();
    }

private:
    inline static bool claimed_ = false;
    channel() = default;
};

}  // namespace alloy::dma
