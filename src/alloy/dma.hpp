// User-facing DMA: a compile-time channel claim returning a move-aware
// token, plus typed 16-bit transfer starters (ADC samples and timer compare
// values are 16-bit — wider helpers arrive with a use case). Below the
// channel: `route` (the generated board fact) and `ring` (the circular
// stream), per docs/design/dma-streams.md.
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

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/arch/irq.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/hal/dma/dma_impl.hpp"

namespace alloy::dma {

// A DMA route is a GENERATED FACT (docs/design/dma-streams.md §1): the board
// states which channel serves which role signal, the generator validates that
// statement against the chip's routing data and emits one of these per
// assignment, attached to the role's binder. The request id is the CHIP's fact
// (`dma_requests` / a matched `dma_routes` triple) — it is never typed by a
// user and never appears in board.json. Hand-wired projects may spell one out
// explicitly; that is the documented escape hatch, not the default.
template <class Controller, unsigned Ch, std::uint8_t Request>
struct route {
    using controller = Controller;
    static constexpr unsigned channel = Ch;
    static constexpr std::uint8_t request = Request;
};

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

// Route -> claimed channel token — the design's anchor 2.3 spelling for the
// SHIPPED one-shot paths (write_dma / write_dma_begin+end, dma_waiter), on the
// board's generated assignment instead of a hand-typed <Inst, Ch>:
//
//     auto tx = alloy::dma::claim(board::dma::debug_uart_tx);
//     co_await w.run([&] { uart.write_dma_begin(tx, log_line); });
//     uart.write_dma_end(tx);
//
// This is a NAMING of channel<>::claim(), not a third ownership mechanism:
// same one-token-per-firmware semantics, same trap on a second claimant, same
// ordinal space as the rings (a ring on this route's channel and this token
// are the same resource, and the loser traps). The route's request id rides
// along in the type but is not consumed here — the one-shot starters take the
// request from the PERIPHERAL's fact (Inst::dmareq_tx) at the call site.
template <class Controller, unsigned Ch, std::uint8_t Request>
[[nodiscard]] inline channel<Controller, Ch> claim(route<Controller, Ch, Request>) {
    return channel<Controller, Ch>::claim();
}

// ── Rings: circular streams over a caller-owned buffer ────────────────────
//
// A ring is an OWNED OBJECT (design §1): construction claims the channel and
// starts the hardware; destruction stops the hardware and RELEASES the claim
// (`sub_release` — the deliberate departure from `channel::claim()`'s
// one-token-per-firmware rule, decided in the design's open question 2). The
// old `channel` API keeps its semantics; rings are the new surface.

// The natural home for a ring's buffer. `alignas(32)` makes sizeof a 32-byte
// multiple too (sizeof is always a multiple of alignof), so a half never
// shares a cache line with the other half or with neighbors — free today,
// load-bearing the day an M7 board runs this with the D-cache enabled
// (design §4). The ring APIs take this by REFERENCE so the natural spelling
// puts the buffer in static storage, which is the lifetime the stream needs.
template <class T, std::size_t N>
struct alignas(32) ring_storage {
    static_assert(N >= 2u && (N % 2u) == 0u,
                  "a ring is consumed as two halves — N must be even");
    static_assert(N <= 0xFFFFu, "transfer count registers are 16-bit on the "
                                "smallest supported engine");
    T data[N];
};

// What a controller must offer before rings exist on it: a circular mode plus
// half-transfer event delivery and a live remaining-count read. On a backend
// without these (today: SAME70 XDMAC, supports_circular = false) the ring type
// is CONSTRAINED AWAY, so a facade's `ring()` method — gated on THIS concept,
// which is public for exactly that reason — is a compile error naming the
// missing capability, never a link error or a runtime surprise.
template <class Inst, unsigned Ch>
concept ring_capable = requires {
    requires hal::dma_impl<Inst>::supports_circular;
    hal::dma_impl<Inst>::template enable_half_irq<Ch>(nullptr, nullptr);
    hal::dma_impl<Inst>::template enable_complete_irq<Ch>(nullptr, nullptr);
    hal::dma_impl<Inst>::template remaining<Ch>();
};

// Circular peripheral->memory stream with half/full events and a read cursor.
//
// Two consumption disciplines, two audiences:
//
//   take()/missed()          the CONTROL-LOOP shape (ADC): block until a half
//                            is hardware-stable, hand it over, count every
//                            half the consumer failed to collect. Checked.
//   cursor()/readable()/     the BYTE-STREAM shape (UART RX): the write
//   consume()                position is computed from the engine's live
//                            remaining-count register — no ISR per item. This
//                            path trusts the reader to keep up; a writer that
//                            laps an idle reader silently overwrites unread
//                            items. missed() still counts uncollected halves,
//                            which is the overrun evidence.
//
// NOT movable, NOT copyable: the half/full ISRs hold a pointer to this object
// for its whole lifetime. Guaranteed copy elision covers the factory-return
// spelling (`auto r = adc.ring(buf);`) — the object is constructed in place.
//
// ISR discipline (design §4): take()/readable()/consume() are thread-context
// only. take() SPINS until a boundary arrives — calling it when the hardware
// cannot produce one (peripheral not started, ring torn down) hangs honestly
// rather than returning an unstable half.
template <class T, class Route>
    requires ring_capable<typename Route::controller, Route::channel>
class ring {
    using Inst = typename Route::controller;
    static constexpr unsigned Ch = Route::channel;
    using impl = hal::dma_impl<Inst>;

    static_assert(std::is_trivially_copyable_v<T>,
                  "DMA moves bytes; T must be trivially copyable");
    static_assert(sizeof(T) == 1u || sizeof(T) == 2u || sizeof(T) == 4u,
                  "item width must be one the engine has (8/16/32-bit)");

public:
    // The default spelling: buffer lifetime is the storage object's, which the
    // reference parameter pushes toward static storage (design §4).
    template <std::size_t N>
    ring(Route r, std::uintptr_t periph_reg, ring_storage<T, N>& storage)
        : ring(r, periph_reg, std::span<T>{storage.data, N}) {}

    // The sharp edge, offered deliberately: a raw span the caller guarantees
    // outlives the stream. A ring stored in a static holding a span to a
    // shorter-lived buffer is the corruption scope cannot close — this is a
    // doctrine rule, not a compiler guarantee, and this comment is the doc.
    ring(Route, std::uintptr_t periph_reg, std::span<T> buf)
        : data_(buf.data()), items_(static_cast<std::uint16_t>(buf.size())) {
        if (buf.empty() || buf.size() > 0xFFFFu || (buf.size() % 2u) != 0u) {
            __builtin_trap();  // two halves and a 16-bit count register
        }
        {
            const arch::irq_state saved = arch::irq_save();
            // Same ordinal space as channel::claim(): a ring on DMA1 ch1 and a
            // channel token on DMA1 ch1 are the SAME resource, and the second
            // claimant traps (trap_code::sub_resource_owned).
            alloy::claim::sub_exclusive<Inst, Ch, alloy::claim::personality::dma>();
            arch::irq_restore(saved);
        }
        impl::enable_controller();
        // Events BEFORE setup: the channel's config register may only be
        // written while the channel is disabled, so the driver folds the
        // interrupt enables in at setup based on what is registered by then.
        //
        // The ORDER of these two lines is deliberate. alloy::irq::attach
        // PREPENDS to the shared-line chain, so dispatch runs the COMPLETION
        // handler before the half handler. On silicon the order should not
        // matter — the RM describes HTIF and TCIF as events half a buffer
        // apart — but no silicon run has witnessed that yet. On
        // Renode's STM32G0DMA (the phase-1 witness) it is load-bearing:
        // the model raises both flags together at the wrap and its per-flag
        // clears also drop the sibling flag (CHTIF clears TransferComplete —
        // read in the 1.16.1 source), so a half handler running first would
        // eat the wrap's completion event. Swap these lines and the
        // adc_stream emulation leg loses every FULL boundary.
        impl::template enable_half_irq<Ch>(&half_thunk, this);
        impl::template enable_complete_irq<Ch>(&full_thunk, this);
        impl::template setup<Ch>(impl::dir::periph_to_mem, /*circular=*/true,
                                 item_width(), item_width(), periph_reg,
                                 reinterpret_cast<std::uintptr_t>(data_), items_,
                                 Route::request);
        impl::template start<Ch>();
    }

    ring(const ring&) = delete;
    ring& operator=(const ring&) = delete;
    ring(ring&&) = delete;
    ring& operator=(ring&&) = delete;

    // Destruction stops the hardware, detaches the events, and releases the
    // claim — in that order, so no ISR can fire into a dead object and no new
    // claimant can grab a channel that is still moving data. The PERIPHERAL
    // side of teardown (e.g. ADC DMAEN off) is the facade's job and happens
    // before this destructor runs (facade wrapper's destructor body, then
    // members) — the §4 teardown order.
    ~ring() {
        impl::template stop<Ch>();
        impl::template disable_half_irq<Ch>();
        impl::template disable_complete_irq<Ch>();
        impl::template clear_flags<Ch>();
        const arch::irq_state saved = arch::irq_save();
        alloy::claim::sub_release<Inst, Ch, alloy::claim::personality::dma>(0u);
        arch::irq_restore(saved);
    }

    // ── the control-loop discipline ──────────────────────────────────────
    // Block until a half is hardware-stable, then hand it over. The half being
    // filled is never visible. If BOTH halves went stable since the last call
    // the consumer fell behind: take() resynchronizes to the MOST RECENT
    // stable half and the skipped one is counted in missed() — never silent
    // (§4's two-slot latch, consumer side).
    [[nodiscard]] std::span<const T> take() {
        while (pending_ == 0u) {
            // spin: the engine is filling the other half
        }
        unsigned h;
        {
            const arch::irq_state saved = arch::irq_save();
            const std::uint8_t p = pending_;
            h = last_stable_;
            pending_ = 0u;
            if (p == 3u) {
                missed_ = missed_ + 1u;  // the older stable half was skipped
            }
            arch::irq_restore(saved);
        }
        const std::size_t half_items = static_cast<std::size_t>(items_) / 2u;
        return std::span<const T>{data_ + (h == 0u ? 0u : half_items), half_items};
    }

    // Cumulative count of stable halves the consumer never collected: halves
    // overwritten while still pending, plus halves skipped by a resynchronizing
    // take(). Each uncollected half counts exactly once.
    [[nodiscard]] std::uint32_t missed() const { return missed_; }

    // Advisory: is a stable half waiting? (The async waiter's readiness
    // predicate; racy as a standalone check, exact under park()'s mask.)
    [[nodiscard]] bool pending() const { return pending_ != 0u; }

    // ── the byte-stream discipline ───────────────────────────────────────
    // Items written since start, from the engine's LIVE remaining-count
    // register plus the wrap count the full ISR maintains. HONEST LIMIT: read
    // between a wrap and its ISR this lags by up to one buffer — conservative
    // (never overstates), and the retry loop pins the wrap count across the
    // register read so the two are never from different laps.
    [[nodiscard]] std::uint32_t cursor() const {
        for (;;) {
            const std::uint32_t w = wraps_;
            const std::uint16_t rem = impl::template remaining<Ch>();
            if (wraps_ == w) {
                return w * static_cast<std::uint32_t>(items_) +
                       (static_cast<std::uint32_t>(items_) - rem);
            }
        }
    }

    // Everything written since the last consume(), as ONE contiguous span: at
    // a buffer wrap this returns the tail run first and the next call returns
    // the rest. Empty span = nothing new.
    [[nodiscard]] std::span<const T> readable() const {
        const std::uint32_t wr = write_pos();
        if (wr >= read_pos_) {
            return std::span<const T>{data_ + read_pos_, wr - read_pos_};
        }
        return std::span<const T>{data_ + read_pos_, items_ - read_pos_};
    }

    // Retire `n` items (≤ the total unread count — overshooting traps, because
    // a cursor pushed past the writer would "read" items that never existed).
    void consume(std::size_t n) {
        const std::uint32_t wr = write_pos();
        const std::uint32_t unread =
            (wr + static_cast<std::uint32_t>(items_) - read_pos_) %
            static_cast<std::uint32_t>(items_);
        if (n > unread) {
            __builtin_trap();
        }
        read_pos_ = (read_pos_ + static_cast<std::uint32_t>(n)) %
                    static_cast<std::uint32_t>(items_);
    }

    // ── event hook (ISR context) ─────────────────────────────────────────
    // Called from the half/full ISR after the latch is updated: set a flag or
    // wake a task (the alloy::irq contract). This is what the async waiter
    // registers; at most one hook.
    void on_boundary(void (*fn)(void*), void* ctx = nullptr) {
        const arch::irq_state saved = arch::irq_save();
        evt_fn_ = fn;
        evt_ctx_ = ctx;
        arch::irq_restore(saved);
    }

private:
    // ISR side of the two-slot latch (§4): one slot per half. A boundary whose
    // slot is ALREADY pending means that half went stale unconsumed — counted,
    // never silent.
    void boundary(unsigned h) {
        const std::uint8_t bit = (h == 0u) ? std::uint8_t{1} : std::uint8_t{2};
        if ((pending_ & bit) != 0u) {
            missed_ = missed_ + 1u;
        }
        pending_ = static_cast<std::uint8_t>(pending_ | bit);
        last_stable_ = static_cast<std::uint8_t>(h);
        if (evt_fn_ != nullptr) {
            evt_fn_(evt_ctx_);
        }
    }

    // Half event: the FIRST half (items [0, N/2)) just became stable.
    static void half_thunk(void* self) { static_cast<ring*>(self)->boundary(0u); }
    // Full event: the SECOND half became stable and the engine wrapped.
    static void full_thunk(void* self) {
        auto* r = static_cast<ring*>(self);
        r->wraps_ = r->wraps_ + 1u;
        r->boundary(1u);
    }

    // Write position in items, [0, N). remaining() reads N..0; the transient
    // 0-before-reload reading maps onto position 0, which the modulo folds.
    [[nodiscard]] std::uint32_t write_pos() const {
        const std::uint32_t rem = impl::template remaining<Ch>();
        return (static_cast<std::uint32_t>(items_) - rem) %
               static_cast<std::uint32_t>(items_);
    }

    static constexpr typename impl::width item_width() {
        if constexpr (sizeof(T) == 1u) {
            return impl::width::b8;
        } else if constexpr (sizeof(T) == 2u) {
            return impl::width::b16;
        } else {
            return impl::width::b32;
        }
    }

    T* data_;
    std::uint16_t items_;
    // ISR-shared state. volatile, and compound ops are spelled out — the ISR
    // and thread sides only ever touch these with interrupts masked or from
    // the ISR itself, so plain load/store is the whole protocol.
    volatile std::uint8_t pending_ = 0u;      // bit0: first half, bit1: second
    volatile std::uint8_t last_stable_ = 0u;  // which half went stable last
    volatile std::uint32_t missed_ = 0u;
    volatile std::uint32_t wraps_ = 0u;
    void (*evt_fn_)(void*) = nullptr;
    void* evt_ctx_ = nullptr;
    // thread-only
    std::uint32_t read_pos_ = 0u;
};

template <class Route, class T, std::size_t N>
ring(Route, std::uintptr_t, ring_storage<T, N>&) -> ring<T, Route>;
template <class Route, class T>
ring(Route, std::uintptr_t, std::span<T>) -> ring<T, Route>;

}  // namespace alloy::dma
