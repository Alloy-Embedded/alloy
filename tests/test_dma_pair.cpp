// Unit tests for alloy::dma::pair — the two-channels-as-one-unit half of the
// DMA stream design (docs/design/dma-streams.md §1's `pair` row, anchor 2.4),
// driven the test_dma_ring.cpp way: the REAL pair runs against a recording
// fake dma_impl, so the claim order, the arm order, the joint wait, the
// stop order and the release story are exercised off-target.
//
// THE ORDER TESTS ARE THE POINT OF THIS FILE, and they had to be built to
// survive a trap: a pair whose second claim loses kills the process, so the
// evidence of what it had ALREADY taken has to be written from the trap's
// signal handler into a page the parent shares. A test that only asserted
// "the child died" could not tell RX-first from TX-first, and RX-first is the
// rule §1 exists to state.
//
// What this file can NOT witness: that arming TX first really loses the first
// byte on a wire. That is the emulation leg's claim (on Renode's STM32G0DMA a
// memory-to-peripheral block runs entirely at the enable write, so every
// receive request lands on a channel that is not yet listening and the RX
// buffer stays zero) and, on silicon, nobody's yet.

#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
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
// real pair<> compiles against it unchanged. Per-CHANNEL state, because a pair
// is two channels and every ordering question this file asks is about telling
// them apart.
template <class Tag>
struct dma_impl<fake_ctrl<Tag>> {
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };
    static constexpr bool supports_circular = true;
    // Rings are gated on supports_ring, not on the circular bit
    // (alloy::dma::ring_capable); on an ST-shaped double they agree.
    static constexpr bool supports_ring = true;

    static constexpr unsigned kChannels = 8;

    struct ch_state {
        int setup_at = 0;
        int start_at = 0;
        int stop_at = 0;
        dir last_dir = dir::mem_to_periph;
        width last_msize = width::b32;
        width last_psize = width::b32;
        std::uintptr_t last_periph = 0;
        std::uintptr_t last_mem = 0;
        std::uint16_t last_items = 0;
        std::uint8_t last_request = 0;
        bool complete = false;
        bool error = false;
    };
    static inline ch_state chan[kChannels];
    static inline int seq = 0;
    static inline bool controller_enabled = false;

    static void reset() {
        for (auto& c : chan) {
            c = ch_state{};
        }
        seq = 0;
        controller_enabled = false;
    }

    static void enable_controller() { controller_enabled = true; }

    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph, std::uintptr_t mem,
                      std::uint16_t items, std::uint8_t request) {
        (void)circular;
        ch_state& c = chan[Ch];
        c.setup_at = ++seq;
        c.last_dir = d;
        c.last_msize = msize;
        c.last_psize = psize;
        c.last_periph = periph;
        c.last_mem = mem;
        c.last_items = items;
        c.last_request = request;
        c.complete = false;
        c.error = false;
    }
    template <unsigned Ch>
    static void start() { chan[Ch].start_at = ++seq; }
    template <unsigned Ch>
    static void stop() { chan[Ch].stop_at = ++seq; }
    template <unsigned Ch>
    static void clear_flags() {}
    template <unsigned Ch>
    static bool complete() { return chan[Ch].complete; }
    template <unsigned Ch>
    static bool error() { return chan[Ch].error; }
};

}  // namespace alloy::hal

namespace {

using alloy::dma::pair;
using alloy::dma::route;

// ── evidence that survives a trap ─────────────────────────────────────────
//
// __builtin_trap() takes the whole process down, so a claim guard's victim
// cannot report what it had taken by returning. This page is MAP_SHARED, the
// child records into it from the signal handler the trap raises, and _exit()s
// with a code the parent can tell from "ran to completion".

struct observation {
    int rx_held;
    int tx_held;
    int trapped;
};

constexpr int kTrappedExit = 42;

observation* g_obs = nullptr;
void (*g_record)() = nullptr;

[[noreturn]] void on_trap(int) {
    if (g_obs != nullptr) {
        g_obs->trapped = 1;
        if (g_record != nullptr) {
            g_record();
        }
    }
    _exit(kTrappedExit);
}

observation* shared_observation() {
    void* p = mmap(nullptr, sizeof(observation), PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    auto* o = static_cast<observation*>(p);
    o->rx_held = -1;
    o->tx_held = -1;
    o->trapped = 0;
    return o;
}

// Run `body` in a child that reports a trap through `record`. Returns true
// when the child trapped (rather than running to completion).
template <class Fn>
bool trapped_with(observation* o, void (*record)(), Fn body) {
    const pid_t pid = fork();
    if (pid == 0) {
        g_obs = o;
        g_record = record;
        for (int sig : {SIGTRAP, SIGILL, SIGABRT, SIGBUS, SIGSEGV}) {
            (void)std::signal(sig, &on_trap);
        }
        body();
        _exit(0);  // reached only if the guard failed to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == kTrappedExit;
}

// The plain refusal idiom, for the cases where "it trapped" is the whole
// claim (test_claim.cpp / test_dma_ring.cpp's `refuses`).
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

// ── the claim: both channels, RX first, one masked section ────────────────

ALLOY_TEST(dma_pair_claims_both_channels_and_enables_both_controllers) {
    struct tag {};
    using Ctrl = fake_ctrl<tag>;
    using impl = alloy::hal::dma_impl<Ctrl>;
    impl::reset();

    pair p{route<Ctrl, 4, 16>{}, route<Ctrl, 5, 17>{}};

    ALLOY_CHECK(impl::controller_enabled);
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 4, alloy::claim::personality::dma>()));
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 5, alloy::claim::personality::dma>()));
}

ALLOY_TEST(dma_pair_claims_rx_before_tx) {
    // THE ORDER ASSERTION §1 exists for. A FOREIGN owner sits on the TX
    // channel, so the pair's SECOND claim is the one that loses — and what it
    // had already taken when it lost says which claim ran first. Swap the two
    // sub_exclusive calls in dma::pair's constructor and this goes red: the
    // pair would trap on the foreign TX channel immediately, having taken
    // nothing, and rx_held would read 0.
    struct tag {};
    using Ctrl = fake_ctrl<tag>;
    alloy::hal::dma_impl<Ctrl>::reset();

    observation* o = shared_observation();
    const bool died = trapped_with(o, +[] {
        g_obs->rx_held =
            alloy::claim::sub_held<fake_ctrl<tag>, 4,
                                   alloy::claim::personality::dma>() ? 1 : 0;
        g_obs->tx_held =
            alloy::claim::sub_held<fake_ctrl<tag>, 5,
                                   alloy::claim::personality::dma>() ? 1 : 0;
    }, [] {
        // Somebody else — a PWM channel, a different personality entirely —
        // already owns the TX ordinal.
        alloy::claim::sub_exclusive<fake_ctrl<tag>, 5,
                                    alloy::claim::personality::pwm>();
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        (void)p;
    });

    ALLOY_CHECK(died);
    ALLOY_CHECK_EQ(o->trapped, 1);
    ALLOY_CHECK_EQ(o->rx_held, 1);  // RX was taken FIRST...
    ALLOY_CHECK_EQ(o->tx_held, 0);  // ...and TX never became ours
    (void)munmap(o, sizeof(observation));
}

ALLOY_TEST(dma_pair_overlapping_claimants_trap_the_same_way_in_either_order) {
    // Two claimants whose pairs OVERLAP crosswise: A wants rx=4/tx=5, B wants
    // rx=5/tx=4. §1's fixed order is what makes this deterministic — the
    // claimant that runs SECOND always loses, and it loses on its FIRST claim
    // holding nothing, whichever of the two it is. (Two channel::claim() calls
    // would give two masked sections and a window between them, in which an
    // interrupt-context claimant could take the other half and leave both
    // parties holding half a pair.)
    struct tag_a {};
    struct tag_b {};
    using A = fake_ctrl<tag_a>;
    using B = fake_ctrl<tag_b>;
    alloy::hal::dma_impl<A>::reset();
    alloy::hal::dma_impl<B>::reset();

    observation* first = shared_observation();
    const bool a_then_b = trapped_with(first, +[] {
        g_obs->rx_held = alloy::claim::sub_held<
            fake_ctrl<tag_a>, 4, alloy::claim::personality::dma>() ? 1 : 0;
        g_obs->tx_held = alloy::claim::sub_held<
            fake_ctrl<tag_a>, 5, alloy::claim::personality::dma>() ? 1 : 0;
    }, [] {
        pair<route<fake_ctrl<tag_a>, 4, 16>, route<fake_ctrl<tag_a>, 5, 17>> a{};
        pair<route<fake_ctrl<tag_a>, 5, 16>, route<fake_ctrl<tag_a>, 4, 17>> b{};
        (void)a;
        (void)b;
    });

    observation* second = shared_observation();
    const bool b_then_a = trapped_with(second, +[] {
        g_obs->rx_held = alloy::claim::sub_held<
            fake_ctrl<tag_b>, 4, alloy::claim::personality::dma>() ? 1 : 0;
        g_obs->tx_held = alloy::claim::sub_held<
            fake_ctrl<tag_b>, 5, alloy::claim::personality::dma>() ? 1 : 0;
    }, [] {
        pair<route<fake_ctrl<tag_b>, 5, 16>, route<fake_ctrl<tag_b>, 4, 17>> b{};
        pair<route<fake_ctrl<tag_b>, 4, 16>, route<fake_ctrl<tag_b>, 5, 17>> a{};
        (void)a;
        (void)b;
    });

    ALLOY_CHECK(a_then_b);
    ALLOY_CHECK(b_then_a);
    // Same outcome from either side: the winner holds BOTH channels at the
    // moment the loser traps — never one each.
    ALLOY_CHECK_EQ(first->rx_held, 1);
    ALLOY_CHECK_EQ(first->tx_held, 1);
    ALLOY_CHECK_EQ(second->rx_held, 1);
    ALLOY_CHECK_EQ(second->tx_held, 1);
    (void)munmap(first, sizeof(observation));
    (void)munmap(second, sizeof(observation));

    // The negative control: pairs that do NOT overlap coexist.
    ALLOY_CHECK(!refuses([] {
        struct tag {};
        pair<route<fake_ctrl<tag>, 1, 1>, route<fake_ctrl<tag>, 2, 2>> a{};
        pair<route<fake_ctrl<tag>, 3, 3>, route<fake_ctrl<tag>, 4, 4>> b{};
        (void)a;
        (void)b;
    }));
}

ALLOY_TEST(dma_pair_shares_one_ordinal_space_with_rings_and_tokens) {
    // A pair, a ring and a channel token on one (controller, channel) are the
    // SAME resource — same claim, same trap code — from either direction.
    ALLOY_CHECK(refuses([] {
        struct tag {};
        auto tok = alloy::dma::claim(route<fake_ctrl<tag>, 4, 16>{});
        (void)tok;
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        (void)p;
    }));
    ALLOY_CHECK(refuses([] {
        struct tag {};
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        auto tok = alloy::dma::claim(route<fake_ctrl<tag>, 5, 17>{});
        (void)tok;
    }));
}

// ── arming: RX endpoint, TX endpoint, and the refusal in between ──────────

ALLOY_TEST(dma_pair_arms_byte_wide_endpoints_with_the_facades_request_ids) {
    struct tag {};
    using Ctrl = fake_ctrl<tag>;
    using impl = alloy::hal::dma_impl<Ctrl>;
    impl::reset();

    static std::uint8_t rx[4] = {};
    static const std::uint8_t tx[4] = {0x9F, 0, 0, 0};

    pair p{route<Ctrl, 4, 16>{}, route<Ctrl, 5, 17>{}};
    p.start_rx_p2m_u8(0x1234u, std::span<std::uint8_t>{rx}, 16u);
    p.start_tx_m2p_u8(std::span<const std::uint8_t>{tx}, 0x1234u, 17u);

    const auto& rxc = impl::chan[4];
    const auto& txc = impl::chan[5];
    ALLOY_CHECK(rxc.last_dir == impl::dir::periph_to_mem);
    ALLOY_CHECK(txc.last_dir == impl::dir::mem_to_periph);
    // BYTE items on BOTH sides of BOTH channels: a wider peripheral access
    // data-packs two frames into one transfer on ST's FIFO SPI.
    ALLOY_CHECK(rxc.last_msize == impl::width::b8);
    ALLOY_CHECK(rxc.last_psize == impl::width::b8);
    ALLOY_CHECK(txc.last_msize == impl::width::b8);
    ALLOY_CHECK(txc.last_psize == impl::width::b8);
    // One data register, both directions.
    ALLOY_CHECK_EQ(rxc.last_periph, 0x1234u);
    ALLOY_CHECK_EQ(txc.last_periph, 0x1234u);
    ALLOY_CHECK_EQ(rxc.last_mem, reinterpret_cast<std::uintptr_t>(rx));
    ALLOY_CHECK_EQ(txc.last_mem, reinterpret_cast<std::uintptr_t>(tx));
    ALLOY_CHECK_EQ(rxc.last_items, 4u);
    ALLOY_CHECK_EQ(txc.last_items, 4u);
    // The request ids are the FACADE's, passed in — not read off the routes,
    // because which fact is honest is a per-family question (design §1).
    ALLOY_CHECK_EQ(rxc.last_request, 16u);
    ALLOY_CHECK_EQ(txc.last_request, 17u);
    // ...and the RX channel was started before the TX channel was even
    // programmed.
    ALLOY_CHECK(rxc.start_at != 0 && txc.setup_at != 0);
    ALLOY_CHECK(rxc.start_at < txc.setup_at);
}

ALLOY_TEST(dma_pair_refuses_to_arm_tx_before_rx) {
    // The arm-order rule as a MECHANISM: a caller that starts the transmit
    // half first is refused, rather than discovering it as a lost first byte.
    ALLOY_CHECK(refuses([] {
        struct tag {};
        static const std::uint8_t tx[4] = {};
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        p.start_tx_m2p_u8(std::span<const std::uint8_t>{tx}, 0x1234u, 17u);
    }));
    // ...and the same call is fine once RX is armed (the negative control, so
    // the refusal cannot be "start_tx always traps").
    ALLOY_CHECK(!refuses([] {
        struct tag {};
        static std::uint8_t rx[4] = {};
        static const std::uint8_t tx[4] = {};
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        p.start_rx_p2m_u8(0x1234u, std::span<std::uint8_t>{rx}, 16u);
        p.start_tx_m2p_u8(std::span<const std::uint8_t>{tx}, 0x1234u, 17u);
    }));
}

ALLOY_TEST(dma_pair_refuses_an_empty_buffer_on_either_half) {
    ALLOY_CHECK(refuses([] {
        struct tag {};
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        p.start_rx_p2m_u8(0u, std::span<std::uint8_t>{}, 16u);
    }));
    ALLOY_CHECK(refuses([] {
        struct tag {};
        static std::uint8_t rx[4] = {};
        static const std::uint8_t tx[4] = {};
        pair<route<fake_ctrl<tag>, 4, 16>, route<fake_ctrl<tag>, 5, 17>> p{};
        p.start_rx_p2m_u8(0u, std::span<std::uint8_t>{rx}, 16u);
        p.start_tx_m2p_u8(std::span<const std::uint8_t>{tx, 0}, 0u, 17u);
    }));
}

// ── the joint wait: a half-failed duplex did not succeed ──────────────────

ALLOY_TEST(dma_pair_wait_answers_for_both_halves) {
    struct tag {};
    using Ctrl = fake_ctrl<tag>;
    using impl = alloy::hal::dma_impl<Ctrl>;
    impl::reset();

    pair p{route<Ctrl, 4, 16>{}, route<Ctrl, 5, 17>{}};
    // Both halves finished cleanly.
    impl::chan[4].complete = true;
    impl::chan[5].complete = true;
    ALLOY_CHECK(p.wait());
    ALLOY_CHECK(p.done());
    ALLOY_CHECK(!p.error());
}

ALLOY_TEST(dma_pair_wait_reports_false_when_either_half_failed) {
    // The bool anchor 2.4 returns has to MEAN something, and this is what it
    // means: a duplex whose receive side errored is a failed exchange even
    // though every byte was clocked out, and the reverse is a failed exchange
    // even though every byte came back. Job A's error latch is what makes the
    // failed side still readable after its own ISR consumed the flag.
    struct rx_tag {};
    struct tx_tag {};
    using RxBad = fake_ctrl<rx_tag>;
    using TxBad = fake_ctrl<tx_tag>;
    alloy::hal::dma_impl<RxBad>::reset();
    alloy::hal::dma_impl<TxBad>::reset();

    {
        pair p{route<RxBad, 4, 16>{}, route<RxBad, 5, 17>{}};
        // RX failed (and, per the driver contract, a failed transfer is also
        // "over": done() is true on the error path too).
        alloy::hal::dma_impl<RxBad>::chan[4].complete = true;
        alloy::hal::dma_impl<RxBad>::chan[4].error = true;
        alloy::hal::dma_impl<RxBad>::chan[5].complete = true;
        ALLOY_CHECK(!p.wait());
        ALLOY_CHECK(p.error());
    }
    {
        pair p{route<TxBad, 4, 16>{}, route<TxBad, 5, 17>{}};
        alloy::hal::dma_impl<TxBad>::chan[4].complete = true;
        alloy::hal::dma_impl<TxBad>::chan[5].complete = true;
        alloy::hal::dma_impl<TxBad>::chan[5].error = true;
        ALLOY_CHECK(!p.wait());
        ALLOY_CHECK(p.error());
    }
}

ALLOY_TEST(dma_pair_wait_is_bounded_when_a_half_never_moves) {
    // The stall a pair has and a single channel does not: one side armed, the
    // other waiting for traffic that never comes. wait() returns false; it
    // does not hang, because a bool whose false case is only reachable by
    // hanging is not a bool.
    struct tag {};
    using Ctrl = fake_ctrl<tag>;
    using impl = alloy::hal::dma_impl<Ctrl>;
    impl::reset();

    pair p{route<Ctrl, 4, 16>{}, route<Ctrl, 5, 17>{}};
    impl::chan[5].complete = true;  // TX drained; RX never completes
    ALLOY_CHECK(!p.wait());
}

// ── teardown: stop TX then RX, then release both ──────────────────────────

ALLOY_TEST(dma_pair_destruction_stops_both_then_releases_both) {
    struct tag {};
    using Ctrl = fake_ctrl<tag>;
    using impl = alloy::hal::dma_impl<Ctrl>;
    impl::reset();

    static std::uint8_t rx[4] = {};
    static const std::uint8_t tx[4] = {};
    {
        pair p{route<Ctrl, 4, 16>{}, route<Ctrl, 5, 17>{}};
        p.start_rx_p2m_u8(0x1234u, std::span<std::uint8_t>{rx}, 16u);
        p.start_tx_m2p_u8(std::span<const std::uint8_t>{tx}, 0x1234u, 17u);
    }
    // Both stopped...
    ALLOY_CHECK(impl::chan[4].stop_at != 0);
    ALLOY_CHECK(impl::chan[5].stop_at != 0);
    // ...TRANSMIT first, the reverse of the arm order: stopping the receive
    // half of a still-transmitting duplex is how bytes arrive with nowhere to
    // go and an overrun flag gets stuck.
    ALLOY_CHECK(impl::chan[5].stop_at < impl::chan[4].stop_at);
    // ...and both claims given back (the ring's release precedent), so the
    // channels are reusable — this second pair would trap if they were not.
    ALLOY_CHECK(!(alloy::claim::sub_held<Ctrl, 4, alloy::claim::personality::dma>()));
    ALLOY_CHECK(!(alloy::claim::sub_held<Ctrl, 5, alloy::claim::personality::dma>()));
    pair again{route<Ctrl, 4, 16>{}, route<Ctrl, 5, 17>{}};
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 4, alloy::claim::personality::dma>()));
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 5, alloy::claim::personality::dma>()));
}

ALLOY_TEST(dma_pair_may_span_two_controllers) {
    // Nothing in the pair requires one controller: the F4/F7 shape assigns
    // both halves on dma2, but a board is free to state one half on each, and
    // the claim/stop/release story is per (controller, channel) either way.
    struct tag_r {};
    struct tag_t {};
    using RxC = fake_ctrl<tag_r>;
    using TxC = fake_ctrl<tag_t>;
    alloy::hal::dma_impl<RxC>::reset();
    alloy::hal::dma_impl<TxC>::reset();
    {
        pair p{route<RxC, 0, 3>{}, route<TxC, 3, 3>{}};
        ALLOY_CHECK(alloy::hal::dma_impl<RxC>::controller_enabled);
        ALLOY_CHECK(alloy::hal::dma_impl<TxC>::controller_enabled);
        ALLOY_CHECK((alloy::claim::sub_held<RxC, 0, alloy::claim::personality::dma>()));
        ALLOY_CHECK((alloy::claim::sub_held<TxC, 3, alloy::claim::personality::dma>()));
    }
    ALLOY_CHECK(!(alloy::claim::sub_held<RxC, 0, alloy::claim::personality::dma>()));
    ALLOY_CHECK(!(alloy::claim::sub_held<TxC, 3, alloy::claim::personality::dma>()));
}
