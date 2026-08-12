// User-facing DMA: a compile-time channel claim returning a move-aware
// token, plus typed 16-bit transfer starters (ADC samples and timer compare
// values are 16-bit — wider helpers arrive with a use case).
//
// Honest v1 doctrine (same as irq attach / double-open): claiming an
// already-claimed (controller, channel) TRAPS at runtime — C++ cannot make
// cross-TU resource claims a compile error. The claim itself is
// `alloy::claim::sub_exclusive`, shared with the PWM channel: one ownership
// mechanism for both of alloy's sub-resource scopes.
//
// Completion can be POLLED (wait/done) or delivered as a callback
// (on_complete). Polling is fine when the CPU has nothing else to do; a product
// that has a control loop to run does not, which is the whole reason the
// callback exists. Register it BEFORE starting a transfer: the channel's config
// register may only be written while the channel is disabled, so the interrupt
// enables are folded in when the transfer is set up.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/arch/irq.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/hal/dma/dma_impl.hpp"

namespace alloy::dma {

template <class Inst, unsigned Ch>
class channel {
    using impl = hal::dma_impl<Inst>;

public:
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    channel(channel&&) noexcept = default;

    // One token per (controller, channel) per firmware. The flag used to be a
    // private `claimed_` on this class — correctly scoped, since `channel` is
    // templated on exactly <Inst, Ch> and nothing else, but a THIRD ownership
    // mechanism with a bare `__builtin_trap()` that a fault report could not
    // tell from the transfer-size traps further down. It is the same
    // sub-resource claim the PWM channel makes now, so it carries
    // `trap_code::sub_resource_owned` and a foreign personality on one channel
    // is a different code again.
    static channel claim() {
        const arch::irq_state saved = arch::irq_save();
        alloy::claim::sub_exclusive<Inst, Ch, alloy::claim::personality::dma>();
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
                                 impl::width::b16, periph_reg,
                                 reinterpret_cast<std::uintptr_t>(dst.data()),
                                 static_cast<std::uint16_t>(dst.size()), request);
        impl::template start<Ch>();
    }

    // One-shot memory -> peripheral, byte items (UART TX and friends).
    // The source must be DMA-visible RAM: on SAME70 the XDMAC cannot read
    // embedded flash (bus error) — copy .rodata payloads to RAM first.
    void start_m2p_u8(std::span<const std::uint8_t> src, std::uintptr_t periph_reg,
                      std::uint8_t request) const {
        if (src.empty() || src.size() > 0xFFFF) {
            __builtin_trap();
        }
        impl::template setup<Ch>(impl::dir::mem_to_periph, false, impl::width::b8,
                                 impl::width::b8, periph_reg,
                                 reinterpret_cast<std::uintptr_t>(src.data()),
                                 static_cast<std::uint16_t>(src.size()), request);
        impl::template start<Ch>();
    }

    // Circular memory -> peripheral, 16-bit items; runs until stop().
    // The source span must OUTLIVE the stream (static/global buffer).
    // Gated on the controller's supports_circular capability: on a backend
    // without a circular mode (SAME70 XDMAC) this method does not exist, so a
    // port that streams to it fails at COMPILE time (NORTH_STAR goal #4) instead
    // of hard-faulting on the first transfer.
    void start_m2p_circular_u16(std::span<const std::uint16_t> src,
                                std::uintptr_t periph_reg, std::uint8_t request) const
        requires impl::supports_circular
    {
        if (src.empty() || src.size() > 0xFFFF) {
            __builtin_trap();
        }
        // psize=b32: halfword writes through the APB bridge duplicate into
        // both halves of 32-bit-capable registers (TIM2 CCR); the engine
        // zero-extends each 16-bit memory item into the word write.
        impl::template setup<Ch>(impl::dir::mem_to_periph, true, impl::width::b16,
                                 impl::width::b32, periph_reg,
                                 reinterpret_cast<std::uintptr_t>(src.data()),
                                 static_cast<std::uint16_t>(src.size()), request);
        impl::template start<Ch>();
    }

    /// Call `fn(ctx)` from the DMA interrupt when this channel finishes (or
    /// errors). Register it BEFORE the transfer that should report — see the
    /// header note. `fn` runs in interrupt context: keep it to setting a flag,
    /// waking a task, or starting the next transfer.
    ///
    /// Only exists where the backing controller implements it (ST dma_v1
    /// today; the SAM E70 XDMAC does not). The requires-gate is not cosmetic:
    /// without it these two methods DECLARE fine on every controller and fail
    /// deep inside their bodies, so a portable example could not detect the
    /// gap with `requires` and simply failed to compile for same70_xplained —
    /// which is exactly what dma_uart did before this gate existed.
    void on_complete(void (*fn)(void*), void* ctx = nullptr) const
        requires requires { impl::template enable_complete_irq<Ch>(nullptr, nullptr); }
    {
        impl::template enable_complete_irq<Ch>(fn, ctx);
    }

    /// Stop reporting completions. Safe while a transfer is in flight; the
    /// transfer itself is unaffected and `done()` still works.
    void clear_on_complete() const
        requires requires { impl::template disable_complete_irq<Ch>(); }
    {
        impl::template disable_complete_irq<Ch>();
    }

    [[nodiscard]] bool done() const { return impl::template complete<Ch>(); }
    [[nodiscard]] bool error() const { return impl::template error<Ch>(); }

    // Block until complete; false on transfer error. The FINAL error()
    // check matters: on XDMAC a bus error auto-disables the channel, which
    // satisfies done() — and an error on the last beat coincides with
    // completion on any controller.
    [[nodiscard]] bool wait() const {
        while (!done()) {
            if (error()) {
                return false;
            }
        }
        return !error();
    }

    void stop() const {
        impl::template stop<Ch>();
        impl::template clear_flags<Ch>();
    }

private:
    channel() = default;
};

}  // namespace alloy::dma
