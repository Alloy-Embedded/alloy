// Unit tests for alloy::spi::handle::transfer_dma — the facade half of
// anchor 2.4 (docs/design/dma-streams.md §2.4), driven the
// test_uart_rx_stream.cpp way: the REAL facade and the REAL dma::pair run
// against a recording fake SPI driver and a recording fake DMA controller, so
// the arm order (RX channel -> RXDMAEN -> TX channel -> TXDMAEN), the §4
// teardown order (requests down, THEN channels stop), the endpoint plumbing,
// the two-source request rule and the meaning of the returned bool are all
// exercised off-target.
//
// ONE FAKE PORT PER TEST, because a port is claimed per INSTANCE for the life
// of the process (alloy/core/claim.hpp) — a second open() of one instance is a
// trap, and rightly so. The tag parameter is what makes each test's port,
// controller and recording its own.
//
// What this file can NOT witness: that a byte actually crossed a wire, or that
// arming TX first really loses the first byte back. Those are the emulation
// leg's claims. Nor can it witness the REQUEST ids: they are read out of chip
// data into a register write here exactly as they are on target, and only
// silicon knows whether 16 really means SPI1_RX.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/dma.hpp"
#include "alloy/spi.hpp"
#include "alloy_test.hpp"

namespace {

// A port whose SPI carries chip-wide DMA request ids — the DMAMUX/free-router
// shape (G0), where any channel can serve any request.
template <class Tag>
struct muxed_spi {
    static constexpr std::uint8_t dmareq_rx = 16u;
    static constexpr std::uint8_t dmareq_tx = 17u;
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
};

// A port whose SPI carries NONE — the stream-engine shape (F4/F7), where the
// request is CHSEL, a per-stream field, so the matched route is the only
// honest source.
template <class Tag>
struct streamed_spi {
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
};

template <class Tag>
struct fake_ctrl {};

template <class Inst, int N>
struct fake_pin {};

struct fake_clock {
    static constexpr std::uint32_t sysclk_hz = 16'000'000u;
    static constexpr std::uint32_t ahb_hz = 16'000'000u;
    static constexpr std::uint32_t apb_hz = 16'000'000u;
    static constexpr std::uint32_t apb2_hz = 16'000'000u;
};

// One shared sequence counter, so "the channel was armed before the port's
// request was raised" is a comparison of two recorded integers across two
// different peripherals (the test_uart_rx_stream.cpp idiom).
inline int g_seq = 0;
inline int next_seq() { return ++g_seq; }

}  // namespace

// The routes that make those pins legal on those ports — codegen emits exactly
// this shape per (pin, peripheral, signal) triple.
namespace alloy::routes {
template <class Inst, int N, alloy::signal S>
struct route<fake_pin<Inst, N>, Inst, S> {
    static constexpr kind k = kind::af_fixed;
    static constexpr std::uint8_t af = 0;
};
}  // namespace alloy::routes

namespace alloy::hal {

template <class Inst, int N>
struct pin_impl<fake_pin<Inst, N>> {
    static void make_af(std::uint8_t) {}
};

// Recording fake SPI driver: the DMA endpoint hooks the real facade needs,
// same static shape as st_spi_v2 so the shipped facade compiles against it
// unchanged. enable() exists so bind::open() is the way to a handle, same as
// a board.
template <class Port>
struct fake_spi_body {
    static inline int rx_begin_at = 0;
    static inline int tx_begin_at = 0;
    static inline int end_at = 0;
    static inline bool drained = true;
    static inline std::uint32_t dr = 0;

    static void enable(std::uint32_t, std::uint32_t, std::uint8_t) {}

    [[nodiscard]] static std::uintptr_t dr_addr() {
        return reinterpret_cast<std::uintptr_t>(&dr);
    }
    static void dma_rx_begin() { rx_begin_at = next_seq(); }
    static void dma_tx_begin() { tx_begin_at = next_seq(); }
    [[nodiscard]] static bool dma_end() {
        end_at = next_seq();
        return drained;
    }
};

template <class Tag>
struct spi_impl<muxed_spi<Tag>> : fake_spi_body<muxed_spi<Tag>> {};
template <class Tag>
struct spi_impl<streamed_spi<Tag>> : fake_spi_body<streamed_spi<Tag>> {};

// Recording fake DMA controller, PER CHANNEL — a pair is two channels and
// every ordering question here is about telling them apart.
template <class Tag>
struct dma_impl<fake_ctrl<Tag>> {
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };
    static constexpr bool supports_circular = true;

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
    // The fake "hardware": a channel completes the moment it starts, unless a
    // test pre-set a failure on it.
    static inline bool autocomplete = true;

    static void enable_controller() {}
    template <unsigned Ch>
    static void setup(dir d, bool, width msize, width psize,
                      std::uintptr_t periph, std::uintptr_t mem,
                      std::uint16_t items, std::uint8_t request) {
        ch_state& c = chan[Ch];
        c.setup_at = next_seq();
        c.last_dir = d;
        c.last_msize = msize;
        c.last_psize = psize;
        c.last_periph = periph;
        c.last_mem = mem;
        c.last_items = items;
        c.last_request = request;
    }
    template <unsigned Ch>
    static void start() {
        chan[Ch].start_at = next_seq();
        if (autocomplete) {
            chan[Ch].complete = true;
        }
    }
    template <unsigned Ch>
    static void stop() { chan[Ch].stop_at = next_seq(); }
    template <unsigned Ch>
    static void clear_flags() {}
    template <unsigned Ch>
    static bool complete() { return chan[Ch].complete; }
    template <unsigned Ch>
    static bool error() { return chan[Ch].error; }
};

}  // namespace alloy::hal

namespace {

using alloy::dma::route;

// The untagged binder — a board that assigned nothing.
template <class Inst>
using pins = alloy::spi::bind<Inst, alloy::spi::sck<fake_pin<Inst, 0>>,
                              alloy::spi::miso<fake_pin<Inst, 1>>,
                              alloy::spi::mosi<fake_pin<Inst, 2>>, fake_clock>;

// The board-assigned spelling: the pair rides the binder as tags, exactly as
// emit/board.py attaches them.
template <class Inst, class Rx, class Tx>
using assigned = alloy::spi::bind<Inst, alloy::spi::sck<fake_pin<Inst, 0>>,
                                  alloy::spi::miso<fake_pin<Inst, 1>>,
                                  alloy::spi::mosi<fake_pin<Inst, 2>>,
                                  fake_clock, alloy::spi::rx_dma<Rx>,
                                  alloy::spi::tx_dma<Tx>>;

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

// ── the arm order and the endpoint plumbing ───────────────────────────────

ALLOY_TEST(spi_transfer_dma_arms_rx_then_raises_rxdmaen_then_tx_then_txdmaen) {
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;
    using spi_f = alloy::hal::spi_impl<Port>;

    static const std::uint8_t tx[4] = {0x9F, 0x00, 0x00, 0x00};
    static std::uint8_t rx[4] = {};

    auto s = assigned<Port, route<Ctrl, 4, 16>, route<Ctrl, 5, 17>>::open();
    ALLOY_CHECK(s.transfer_dma(std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));

    const auto& rxc = dma_f::chan[4];
    const auto& txc = dma_f::chan[5];
    // THE RM's FULL-DUPLEX ORDER, which ST's own HAL follows and which the
    // emulation leg can turn red: arm the receive channel, raise RXDMAEN, arm
    // the transmit channel, and raise TXDMAEN last — that write is what starts
    // the traffic.
    ALLOY_CHECK(rxc.start_at != 0);
    ALLOY_CHECK(rxc.start_at < spi_f::rx_begin_at);
    ALLOY_CHECK(spi_f::rx_begin_at < txc.setup_at);
    ALLOY_CHECK(txc.start_at < spi_f::tx_begin_at);

    // One data register serves both directions...
    ALLOY_CHECK_EQ(rxc.last_periph, spi_f::dr_addr());
    ALLOY_CHECK_EQ(txc.last_periph, spi_f::dr_addr());
    // ...BYTE-wide on both sides of both channels, because a wider access
    // data-packs two frames into one transfer on this IP.
    ALLOY_CHECK(rxc.last_msize == dma_f::width::b8);
    ALLOY_CHECK(rxc.last_psize == dma_f::width::b8);
    ALLOY_CHECK(txc.last_msize == dma_f::width::b8);
    ALLOY_CHECK(txc.last_psize == dma_f::width::b8);
    ALLOY_CHECK(rxc.last_dir == dma_f::dir::periph_to_mem);
    ALLOY_CHECK(txc.last_dir == dma_f::dir::mem_to_periph);
    ALLOY_CHECK_EQ(rxc.last_mem, reinterpret_cast<std::uintptr_t>(rx));
    ALLOY_CHECK_EQ(txc.last_mem, reinterpret_cast<std::uintptr_t>(tx));
    ALLOY_CHECK_EQ(rxc.last_items, 4u);
    ALLOY_CHECK_EQ(txc.last_items, 4u);
}

ALLOY_TEST(spi_transfer_dma_drops_the_requests_before_the_channels_stop) {
    // Design §4's teardown order, and the trap it closes: the DMA's completion
    // says the last byte reached the FIFO, not the wire, and a request that
    // lands mid-teardown on a disabled channel is how an overrun flag gets
    // stuck. dma_end() (the drain plus both request enables down) runs while
    // the channels are still armed; the pair's destructor stops them after.
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;
    using spi_f = alloy::hal::spi_impl<Port>;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 4, 16>, route<Ctrl, 5, 17>>::open();
    ALLOY_CHECK(s.transfer_dma(std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));

    ALLOY_CHECK(spi_f::end_at != 0);
    ALLOY_CHECK(dma_f::chan[4].stop_at != 0);
    ALLOY_CHECK(dma_f::chan[5].stop_at != 0);
    ALLOY_CHECK(spi_f::end_at < dma_f::chan[5].stop_at);
    ALLOY_CHECK(spi_f::end_at < dma_f::chan[4].stop_at);
    // ...transmit stops first, the reverse of the arm order.
    ALLOY_CHECK(dma_f::chan[5].stop_at < dma_f::chan[4].stop_at);
}

ALLOY_TEST(spi_transfer_dma_releases_both_channels_so_the_next_call_can_run) {
    // The pair is scoped to the exchange: two transfers in a row on one board
    // are the normal case, and the second would trap on its claim if the first
    // pair held its channels forever (channel::claim's rule, which the ring's
    // release precedent departs from deliberately).
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 4, 16>, route<Ctrl, 5, 17>>::open();
    ALLOY_CHECK(s.transfer_dma(std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));
    ALLOY_CHECK(!(alloy::claim::sub_held<Ctrl, 4,
                                         alloy::claim::personality::dma>()));
    ALLOY_CHECK(!(alloy::claim::sub_held<Ctrl, 5,
                                         alloy::claim::personality::dma>()));
    ALLOY_CHECK(s.transfer_dma(std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));
}

// ── the bool means something ──────────────────────────────────────────────

ALLOY_TEST(spi_transfer_dma_reports_false_when_the_receive_half_failed) {
    // A half-failed duplex did not succeed, even though every byte was clocked
    // out. Job A's error latch is what keeps the failure readable after the
    // completion interrupt consumed its flag — without it wait() answers true
    // for a transfer that raised TEIF, and this bool would be a lie with a
    // straight face.
    struct tag {};
    using Port = streamed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    alloy::hal::dma_impl<Ctrl>::chan[0].error = true;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 0, 3>, route<Ctrl, 3, 3>>::open();
    ALLOY_CHECK(!s.transfer_dma(std::span<const std::uint8_t>{tx},
                                std::span<std::uint8_t>{rx}));
}

ALLOY_TEST(spi_transfer_dma_reports_false_when_the_transmit_half_failed) {
    // ...and the mirror image: every byte came back and the exchange still
    // failed, because the transmit channel reported an error.
    struct tag {};
    using Port = streamed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    alloy::hal::dma_impl<Ctrl>::chan[3].error = true;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 0, 3>, route<Ctrl, 3, 3>>::open();
    ALLOY_CHECK(!s.transfer_dma(std::span<const std::uint8_t>{tx},
                                std::span<std::uint8_t>{rx}));
}

ALLOY_TEST(spi_transfer_dma_reports_false_when_a_half_never_moved) {
    // The stall a pair has and a single channel does not: one side armed, the
    // other waiting for traffic that never comes — what a board whose two
    // assignments are not the two halves of one endpoint looks like. The
    // bounded wait reports it instead of hanging.
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    alloy::hal::dma_impl<Ctrl>::autocomplete = false;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 4, 16>, route<Ctrl, 5, 17>>::open();
    ALLOY_CHECK(!s.transfer_dma(std::span<const std::uint8_t>{tx},
                                std::span<std::uint8_t>{rx}));
}

ALLOY_TEST(spi_transfer_dma_reports_false_when_the_port_never_drained) {
    // Both channels moved every byte and the port still has a frame in flight
    // (BSY never fell). Dropping chip-select there truncates the last frame on
    // the wire, so the exchange did not succeed even though the DMA did.
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    using spi_f = alloy::hal::spi_impl<Port>;
    spi_f::drained = false;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 4, 16>, route<Ctrl, 5, 17>>::open();
    ALLOY_CHECK(!s.transfer_dma(std::span<const std::uint8_t>{tx},
                                std::span<std::uint8_t>{rx}));
    // ...and the requests still came down: a port that failed to drain is
    // exactly the one that must not be left with its DMA enables raised.
    ALLOY_CHECK(spi_f::end_at != 0);
}

ALLOY_TEST(spi_transfer_dma_refuses_a_length_a_duplex_cannot_have) {
    // Every byte clocked out brings a byte back, so a duplex has ONE length
    // and it is not zero.
    struct tag_a {};
    struct tag_b {};
    ALLOY_CHECK(refuses([] {
        using Ctrl = fake_ctrl<tag_a>;
        static const std::uint8_t tx[4] = {};
        static std::uint8_t rx[2] = {};
        auto s = assigned<muxed_spi<tag_a>, route<Ctrl, 4, 16>,
                          route<Ctrl, 5, 17>>::open();
        (void)s.transfer_dma(std::span<const std::uint8_t>{tx},
                             std::span<std::uint8_t>{rx});
    }));
    ALLOY_CHECK(refuses([] {
        using Ctrl = fake_ctrl<tag_b>;
        auto s = assigned<muxed_spi<tag_b>, route<Ctrl, 4, 16>,
                          route<Ctrl, 5, 17>>::open();
        (void)s.transfer_dma(std::span<const std::uint8_t>{},
                             std::span<std::uint8_t>{});
    }));
}

// ── the two-source request rule ───────────────────────────────────────────

ALLOY_TEST(spi_transfer_dma_takes_the_request_from_the_chip_when_there_is_one) {
    // Free-router shape (G0 + DMAMUX): the id is a chip-wide fact, and the
    // route's copy of it — which the generator derived from that same chip
    // data — is not the source. The routes here deliberately carry NONSENSE
    // ids, so the assertion cannot pass by the two agreeing.
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 4, 99>, route<Ctrl, 5, 98>>::open();
    ALLOY_CHECK(s.transfer_dma(std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));
    ALLOY_CHECK_EQ(dma_f::chan[4].last_request, Port::dmareq_rx);
    ALLOY_CHECK_EQ(dma_f::chan[5].last_request, Port::dmareq_tx);
}

ALLOY_TEST(spi_transfer_dma_takes_the_request_from_the_route_on_a_stream_engine) {
    // Stream-engine shape (F4/F7): there is no chip-wide id at all. CHSEL is a
    // per-stream field, so SPI1_RX is request 3 on dma2 stream 0 and also on
    // stream 2 — only the triple the BOARD matched is honest, and that is the
    // route's request. Same two-source rule the UART's write_dma follows, and
    // the reason dma::pair takes its ids as parameters.
    struct tag {};
    using Port = streamed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = assigned<Port, route<Ctrl, 0, 3>, route<Ctrl, 3, 5>>::open();
    ALLOY_CHECK(s.transfer_dma(std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));
    ALLOY_CHECK_EQ(dma_f::chan[0].last_request, 3u);
    ALLOY_CHECK_EQ(dma_f::chan[3].last_request, 5u);
}

// ── the escape hatch: routes the caller spelled out ───────────────────────

ALLOY_TEST(spi_transfer_dma_runs_on_explicitly_spelled_routes) {
    // Design §1's documented escape hatch, for hand-wired projects: the same
    // exchange on a handle whose board assigned nothing.
    struct tag {};
    using Port = muxed_spi<tag>;
    using Ctrl = fake_ctrl<tag>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;

    static const std::uint8_t tx[2] = {};
    static std::uint8_t rx[2] = {};
    auto s = pins<Port>::open();
    ALLOY_CHECK(s.transfer_dma(route<Ctrl, 2, 16>{}, route<Ctrl, 6, 17>{},
                               std::span<const std::uint8_t>{tx},
                               std::span<std::uint8_t>{rx}));
    ALLOY_CHECK(dma_f::chan[2].last_dir == dma_f::dir::periph_to_mem);
    ALLOY_CHECK(dma_f::chan[6].last_dir == dma_f::dir::mem_to_periph);
    ALLOY_CHECK(dma_f::chan[2].start_at < dma_f::chan[6].setup_at);
}

// ── the requires-gate: a board with no pair has no transfer_dma ───────────

namespace {

template <class H>
concept offers_transfer_dma =
    requires(H h, std::span<const std::uint8_t> t, std::span<std::uint8_t> r) {
        h.transfer_dma(t, r);
    };

template <class H, class Rx, class Tx>
concept offers_explicit_transfer_dma =
    requires(H h, Rx rx_route, Tx tx_route, std::span<const std::uint8_t> t,
             std::span<std::uint8_t> r) {
        h.transfer_dma(rx_route, tx_route, t, r);
    };

struct probe_tag {};
using probe_port = muxed_spi<probe_tag>;
using probe_ctrl = fake_ctrl<probe_tag>;
using probe_rx = route<probe_ctrl, 4, 16>;
using probe_tx = route<probe_ctrl, 5, 17>;

// A driver with no DMA hooks at all — the microchip_spi_v1 / SAME70 shape,
// where the CAPABILITY is missing rather than the assignment.
struct hookless_spi {};

}  // namespace

namespace alloy::hal {
template <>
struct spi_impl<hookless_spi> {
    static void enable(std::uint32_t, std::uint32_t, std::uint8_t) {}
};
}  // namespace alloy::hal

// Both routes assigned and the driver has the hooks: the anchor's spelling
// exists.
static_assert(offers_transfer_dma<
              alloy::spi::handle<probe_port, probe_rx, probe_tx>>);
// A board that assigned NOTHING: constrained away, with the missing fact
// named, instead of a link error or a runtime surprise.
static_assert(!offers_transfer_dma<alloy::spi::handle<probe_port>>);
// HALF a pair is not a pair. The generator will not invent the missing channel
// (design §1), so this is the diagnostic a board that states one direction
// gets.
static_assert(!offers_transfer_dma<
              alloy::spi::handle<probe_port, probe_rx, void>>);
static_assert(!offers_transfer_dma<
              alloy::spi::handle<probe_port, void, probe_tx>>);
// A port whose DRIVER has no DMA hooks: refused even with both assignments.
// This is the half of the gate that keeps a portable example COMPILING on a
// board like same70_xplained rather than failing deep inside a body — the
// lesson dma_uart paid for before dma::channel's gates existed.
static_assert(!offers_transfer_dma<
              alloy::spi::handle<hookless_spi, probe_rx, probe_tx>>);
static_assert(!alloy::spi::pair_capable<hookless_spi, probe_rx, probe_tx>);
static_assert(alloy::spi::pair_capable<probe_port, probe_rx, probe_tx>);
// The explicit-route escape hatch exists on a route-less handle — that is what
// makes it an escape hatch and not a second board mechanism — and is refused
// on a hookless driver just the same.
static_assert(offers_explicit_transfer_dma<alloy::spi::handle<probe_port>,
                                           probe_rx, probe_tx>);
static_assert(!offers_explicit_transfer_dma<alloy::spi::handle<hookless_spi>,
                                            probe_rx, probe_tx>);

// The binder carries the pair as tags and exposes it under the dependent names
// portable code gates on (never `board::dma::spi_rx`: a namespace-scope
// constant does not fold in a requires-clause — the phase-2 lesson).
static_assert(std::is_same_v<
              typename assigned<probe_port, probe_rx, probe_tx>::rx_route,
              probe_rx>);
static_assert(std::is_same_v<
              typename assigned<probe_port, probe_rx, probe_tx>::tx_route,
              probe_tx>);
static_assert(std::is_void_v<typename pins<probe_port>::rx_route>);
static_assert(std::is_void_v<typename pins<probe_port>::tx_route>);
// ...and an untagged binder still opens the plain handle it always did, so
// every board without an assignment is unaffected.
static_assert(std::is_same_v<decltype(pins<probe_port>::open()),
                             alloy::spi::handle<probe_port, void, void>>);
