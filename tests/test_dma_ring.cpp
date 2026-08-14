// Unit tests for alloy::dma::ring — the family-neutral half of the DMA stream
// design (docs/design/dma-streams.md §1/§4), driven through a recording fake
// backend in the tests/doubles.hpp spirit: the REAL ring runs against a fake
// dma_impl, so the cursor arithmetic, the two-slot half/full latch, the
// stop-on-destruction ordering and the claim/release story are all exercised
// off-target. What these tests can NOT witness: real HTIF/TCIF ordering out of
// silicon or a model — that is the Renode leg's claim, not this file's.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <span>

#include "alloy/dma.hpp"
#include "alloy_test.hpp"

namespace {

// One fake controller per test (the claim registry is per (Inst, Ch) per
// PROCESS, same reason test_claim.cpp burns one instance tag per test).
template <class Tag>
struct fake_ctrl {};

}  // namespace

namespace alloy::hal {

// The recording fake: same static-template shape as the real backends, so the
// real ring<> compiles against it unchanged. Knobs: `rem` (the live count the
// cursor reads) and fire_half()/fire_full() (the "ISR" edge, host-called).
template <class Tag>
struct dma_impl<fake_ctrl<Tag>> {
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };
    static constexpr bool supports_circular = true;

    // recorded setup
    static inline bool controller_enabled = false;
    static inline bool running = false;
    static inline bool was_circular = false;
    static inline dir last_dir = dir::mem_to_periph;
    static inline width last_msize = width::b8;
    static inline width last_psize = width::b8;
    static inline std::uintptr_t last_periph = 0;
    static inline std::uintptr_t last_mem = 0;
    static inline std::uint16_t last_items = 0;
    static inline std::uint8_t last_request = 0;
    static inline std::uint32_t stop_calls = 0;
    // teardown-order evidence: what was still true at the moment stop() ran
    static inline bool half_attached_at_stop = false;
    // event delivery
    static inline void (*half_fn)(void*) = nullptr;
    static inline void* half_ctx = nullptr;
    static inline void (*full_fn)(void*) = nullptr;
    static inline void* full_ctx = nullptr;
    // the live remaining-count register
    static inline std::uint16_t rem = 0;

    static void enable_controller() { controller_enabled = true; }

    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph, std::uintptr_t mem,
                      std::uint16_t items, std::uint8_t request) {
        last_dir = d;
        was_circular = circular;
        last_msize = msize;
        last_psize = psize;
        last_periph = periph;
        last_mem = mem;
        last_items = items;
        last_request = request;
        rem = items;  // idle engine: nothing moved yet
    }
    template <unsigned Ch>
    static void start() { running = true; }
    template <unsigned Ch>
    static void stop() {
        running = false;
        stop_calls = stop_calls + 1;
        half_attached_at_stop = (half_fn != nullptr);
    }
    template <unsigned Ch>
    static void clear_flags() {}
    template <unsigned Ch>
    static std::uint16_t remaining() { return rem; }
    template <unsigned Ch>
    static void enable_half_irq(void (*fn)(void*), void* ctx) {
        half_fn = fn;
        half_ctx = ctx;
    }
    template <unsigned Ch>
    static void disable_half_irq() { half_fn = nullptr; }
    template <unsigned Ch>
    static void enable_complete_irq(void (*fn)(void*), void* ctx) {
        full_fn = fn;
        full_ctx = ctx;
    }
    template <unsigned Ch>
    static void disable_complete_irq() { full_fn = nullptr; }

    // the "ISR": the hardware crossed a half/full boundary
    static void fire_half() {
        if (half_fn != nullptr) {
            half_fn(half_ctx);
        }
    }
    static void fire_full() {
        if (full_fn != nullptr) {
            full_fn(full_ctx);
        }
    }
};

}  // namespace alloy::hal

namespace {

using alloy::dma::ring;
using alloy::dma::ring_storage;
using alloy::dma::route;

// The refusal idiom from test_claim.cpp: the guard under test must kill the
// child, a clean exit means it failed to fire.
template <class Fn>
bool refuses(Fn body) {
    const pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

}  // namespace

// ── construction: claim + program + start ─────────────────────────────────

ALLOY_TEST(dma_ring_construction_claims_programs_circular_p2m_and_starts) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint16_t, 8> buf;

    ring r{route<fake_ctrl<tag>, 3, 42>{}, 0x1234u, buf};

    ALLOY_CHECK(impl::controller_enabled);
    ALLOY_CHECK(impl::running);
    ALLOY_CHECK(impl::was_circular);
    ALLOY_CHECK(impl::last_dir == impl::dir::periph_to_mem);
    ALLOY_CHECK(impl::last_msize == impl::width::b16);
    ALLOY_CHECK(impl::last_psize == impl::width::b16);
    ALLOY_CHECK_EQ(impl::last_periph, 0x1234u);
    ALLOY_CHECK_EQ(impl::last_mem, reinterpret_cast<std::uintptr_t>(buf.data));
    ALLOY_CHECK_EQ(impl::last_items, 8u);
    ALLOY_CHECK_EQ(impl::last_request, 42u);  // the route's fact, not a caller's
    // the channel is claimed while the ring lives
    ALLOY_CHECK((alloy::claim::sub_held<fake_ctrl<tag>, 3,
                                        alloy::claim::personality::dma>()));
}

ALLOY_TEST(dma_ring_item_width_follows_t) {
    struct tag8 {};
    struct tag32 {};
    using impl8 = alloy::hal::dma_impl<fake_ctrl<tag8>>;
    using impl32 = alloy::hal::dma_impl<fake_ctrl<tag32>>;
    static ring_storage<std::uint8_t, 4> b8;
    static ring_storage<std::uint32_t, 4> b32;
    ring r8{route<fake_ctrl<tag8>, 1, 1>{}, 0u, b8};
    ring r32{route<fake_ctrl<tag32>, 1, 1>{}, 0u, b32};
    ALLOY_CHECK(impl8::last_msize == impl8::width::b8);
    ALLOY_CHECK(impl32::last_msize == impl32::width::b32);
}

// ── the checked discipline: take()/missed() ───────────────────────────────

ALLOY_TEST(dma_ring_take_hands_over_halves_in_event_order) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint16_t, 8> buf;
    ring r{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};

    ALLOY_CHECK(!r.pending());
    impl::fire_half();  // first half stable
    ALLOY_CHECK(r.pending());
    std::span<const std::uint16_t> h0 = r.take();
    ALLOY_CHECK_EQ(h0.data(), &buf.data[0]);
    ALLOY_CHECK_EQ(h0.size(), 4u);
    ALLOY_CHECK(!r.pending());  // consumed

    impl::fire_full();  // second half stable
    std::span<const std::uint16_t> h1 = r.take();
    ALLOY_CHECK_EQ(h1.data(), &buf.data[4]);
    ALLOY_CHECK_EQ(h1.size(), 4u);

    impl::fire_half();  // wrapped: first half again
    ALLOY_CHECK_EQ(r.take().data(), &buf.data[0]);

    ALLOY_CHECK_EQ(r.missed(), 0u);  // lockstep consumer misses nothing
}

ALLOY_TEST(dma_ring_slow_consumer_resyncs_to_most_recent_and_counts_the_miss) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint16_t, 8> buf;
    ring r{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};

    // Consumer sleeps through a full half-period: both halves go stable.
    impl::fire_half();
    impl::fire_full();
    std::span<const std::uint16_t> s = r.take();
    // Resynchronized to the MOST RECENT stable half (the second one)...
    ALLOY_CHECK_EQ(s.data(), &buf.data[4]);
    // ...and the skipped first half is counted, never silent (§4).
    ALLOY_CHECK_EQ(r.missed(), 1u);
    ALLOY_CHECK(!r.pending());
}

ALLOY_TEST(dma_ring_overwritten_stale_half_counts_once_per_lost_half) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint16_t, 8> buf;
    ring r{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};

    // Two full laps with no take(): 4 stable halves, consumer collects 1.
    impl::fire_half();
    impl::fire_full();
    impl::fire_half();  // first half pending AGAIN -> the stale one is lost
    impl::fire_full();  // second half pending again -> lost too
    ALLOY_CHECK_EQ(r.missed(), 2u);  // the two overwritten-in-place halves
    std::span<const std::uint16_t> s = r.take();
    ALLOY_CHECK_EQ(s.data(), &buf.data[4]);  // most recent stable
    // + the skipped still-pending first half from the resync = 3 total: every
    // stable half the consumer never saw is counted exactly once.
    ALLOY_CHECK_EQ(r.missed(), 3u);
}

// ── the byte-stream discipline: cursor()/readable()/consume() ─────────────

ALLOY_TEST(dma_ring_cursor_counts_items_across_wraps) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint8_t, 16> buf;
    ring r{route<fake_ctrl<tag>, 1, 7>{}, 0u, buf};

    ALLOY_CHECK_EQ(r.cursor(), 0u);  // idle: remaining == items
    impl::rem = 11;                  // engine wrote 5
    ALLOY_CHECK_EQ(r.cursor(), 5u);
    impl::rem = 1;  // wrote 15
    ALLOY_CHECK_EQ(r.cursor(), 15u);
    // wrap: full event increments the lap counter, count reloads
    impl::fire_full();
    impl::rem = 16;
    ALLOY_CHECK_EQ(r.cursor(), 16u);
    impl::rem = 13;  // 3 into the second lap
    ALLOY_CHECK_EQ(r.cursor(), 19u);
}

ALLOY_TEST(dma_ring_readable_returns_contiguous_runs_tail_first_at_wrap) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint8_t, 16> buf;
    ring r{route<fake_ctrl<tag>, 1, 7>{}, 0u, buf};

    ALLOY_CHECK(r.readable().empty());  // nothing written yet

    impl::rem = 10;  // wrote items [0, 6)
    std::span<const std::uint8_t> a = r.readable();
    ALLOY_CHECK_EQ(a.data(), &buf.data[0]);
    ALLOY_CHECK_EQ(a.size(), 6u);
    r.consume(6);
    ALLOY_CHECK(r.readable().empty());  // caught up

    // Writer wraps past the end: read cursor 6, write position 2 (lap 2).
    impl::fire_full();
    impl::rem = 14;
    // Contiguous chunk first: the tail [6, 16)...
    std::span<const std::uint8_t> tail = r.readable();
    ALLOY_CHECK_EQ(tail.data(), &buf.data[6]);
    ALLOY_CHECK_EQ(tail.size(), 10u);
    r.consume(10);
    // ...then the wrapped head [0, 2).
    std::span<const std::uint8_t> head = r.readable();
    ALLOY_CHECK_EQ(head.data(), &buf.data[0]);
    ALLOY_CHECK_EQ(head.size(), 2u);
    r.consume(2);
    ALLOY_CHECK(r.readable().empty());
}

ALLOY_TEST(dma_ring_consume_past_the_writer_traps) {
    ALLOY_CHECK(refuses([] {
        struct tag {};
        using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
        static ring_storage<std::uint8_t, 16> buf;
        ring r{route<fake_ctrl<tag>, 1, 7>{}, 0u, buf};
        impl::rem = 12;  // 4 items written
        r.consume(5);    // a cursor pushed past the writer "reads" nothing real
    }));
}

// ── lifetime: stop-on-destruction, release, double-claim ──────────────────

ALLOY_TEST(dma_ring_destruction_stops_hardware_then_releases_claim) {
    struct tag {};
    using impl = alloy::hal::dma_impl<fake_ctrl<tag>>;
    static ring_storage<std::uint16_t, 8> buf;
    {
        ring r{route<fake_ctrl<tag>, 2, 5>{}, 0u, buf};
        ALLOY_CHECK(impl::running);
    }
    // hardware stopped BEFORE the stack frame died (§4's memory corruptor,
    // closed by scope)...
    ALLOY_CHECK(!impl::running);
    // ...with the events still attached at the moment of the stop (no window
    // where a still-running channel has nowhere to deliver)...
    ALLOY_CHECK(impl::half_attached_at_stop);
    ALLOY_CHECK(impl::half_fn == nullptr);  // then detached
    // ...and the claim RELEASED (the design's open-question-2 departure from
    // channel::claim's hold-forever), so the channel is reusable:
    ALLOY_CHECK(!(alloy::claim::sub_held<fake_ctrl<tag>, 2,
                                         alloy::claim::personality::dma>()));
    ring r2{route<fake_ctrl<tag>, 2, 5>{}, 0u, buf};  // would trap if held
    ALLOY_CHECK(impl::running);
}

ALLOY_TEST(dma_ring_second_claimant_of_a_live_ring_channel_traps) {
    // ring vs ring on one (controller, channel)
    ALLOY_CHECK(refuses([] {
        struct tag {};
        static ring_storage<std::uint16_t, 8> buf;
        ring a{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};
        ring b{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};
    }));
    // ring vs the old channel token API: SAME ordinal space, same trap
    ALLOY_CHECK(refuses([] {
        struct tag {};
        static ring_storage<std::uint16_t, 8> buf;
        ring a{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};
        auto c = alloy::dma::channel<fake_ctrl<tag>, 1>::claim();
        (void)c;
    }));
    // different channels coexist — the negative control
    ALLOY_CHECK(!refuses([] {
        struct tag {};
        static ring_storage<std::uint16_t, 8> buf;
        static ring_storage<std::uint16_t, 8> buf2;
        ring a{route<fake_ctrl<tag>, 1, 5>{}, 0u, buf};
        ring b{route<fake_ctrl<tag>, 2, 6>{}, 0u, buf2};
    }));
}

ALLOY_TEST(dma_ring_odd_or_empty_buffer_traps) {
    ALLOY_CHECK(refuses([] {
        struct tag {};
        static std::uint16_t raw[7];
        ring r{route<fake_ctrl<tag>, 1, 5>{}, 0u,
               std::span<std::uint16_t>{raw, 7}};  // no even halves
    }));
    ALLOY_CHECK(refuses([] {
        struct tag {};
        ring r{route<fake_ctrl<tag>, 1, 5>{}, 0u, std::span<std::uint16_t>{}};
    }));
}

// ── the capability gate ───────────────────────────────────────────────────

namespace {
// A backend WITHOUT a circular mode (the SAME70 XDMAC shape): the full event
// API exists, the capability bit says no.
struct no_circ_tag {};
struct no_circ_ctrl {};
// And one with circular but NO half events (a hypothetical poll-only engine).
struct no_half_ctrl {};
}  // namespace

namespace alloy::hal {
template <>
struct dma_impl<no_circ_ctrl> : dma_impl<fake_ctrl<no_circ_tag>> {
    static constexpr bool supports_circular = false;
};
template <>
struct dma_impl<no_half_ctrl> {
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };
    static constexpr bool supports_circular = true;
    // deliberately: no enable_half_irq / remaining
};
}  // namespace alloy::hal

// The design's compile-error promise (§1): a ring on a backend that cannot do
// circular-with-half-events fails the PUBLIC ring_capable concept — the same
// predicate a facade's requires-gated ring() states — so the missing
// capability is reported at compile time. This is the host-side witness of
// the "stub, not error" story on SAME70: code that never asks for a ring
// builds clean (this whole TU compiles with these backends in scope); code
// that asks is refused with the concept's name in the diagnostic. (The
// concept, not `requires { typename ring<...>; }`: Apple clang hard-errors
// on naming a constraint-failing specialization inside a requires-expression
// instead of folding it to false, so the probe spells the predicate itself.)
static_assert(!alloy::dma::ring_capable<no_circ_ctrl, 1>);
static_assert(!alloy::dma::ring_capable<no_half_ctrl, 1>);
static_assert(alloy::dma::ring_capable<fake_ctrl<no_circ_tag>, 1>);
