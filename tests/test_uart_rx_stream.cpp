// Unit tests for alloy::uart::rx_stream / uart.rx_ring() — the facade half of
// the phase-2 read side (docs/design/dma-streams.md §2.2/§4), driven the
// test_adc_stream.cpp way: the REAL rx_stream + REAL ring run against a
// recording fake UART driver and a recording fake DMA controller, so the arm
// order (channel -> DMAR -> IDLE), the §4 teardown order (IDLE off, DMAR off,
// THEN channel stop), the request/address plumbing, the IDLE hookup and the
// byte-stream forwarding are all exercised off-target. What this file can NOT
// witness: a real frame crossing a real USART with no per-byte ISR — that is
// the modbus_rtu emulation leg's claim, not this file's.

#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/dma.hpp"
#include "alloy/uart.hpp"
#include "alloy_test.hpp"

namespace {

template <class Tag>
struct fake_uart {};

template <class Tag>
struct fake_ctrl {};

// One shared sequence counter so cross-peripheral ordering is a comparison of
// two recorded integers. 0 = never called.
inline int g_seq = 0;
inline int next_seq() { return ++g_seq; }

}  // namespace

namespace alloy::hal {

// Recording fake UART driver: the RX-DMA + IDLE hooks the real rx_stream
// needs, same static shape as st_usart_v4_body so the real facade compiles
// against it unchanged. enable() exists so rom_bind::open() is the way to a
// handle, same as a board.
template <class Tag>
struct uart_impl<fake_uart<Tag>> {
    static inline int rx_begin_at = 0;
    static inline int rx_end_at = 0;
    static inline int idle_arm_at = 0;
    static inline int idle_off_at = 0;
    static inline bool idle_armed = false;
    static inline void (*idle_fn)(void*) = nullptr;
    static inline void* idle_ctx = nullptr;
    static inline std::uint16_t rdr = 0;

    template <uart_opts<fake_uart<Tag>> O = {}>
    static void enable(std::uint32_t, uart_config) {}

    static void dma_rx_begin() { rx_begin_at = next_seq(); }
    static void dma_rx_end() { rx_end_at = next_seq(); }
    [[nodiscard]] static std::uintptr_t rdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&rdr);
    }
    static void enable_idle_irq(void (*fn)(void*), void* ctx) {
        if (!idle_armed) {
            idle_arm_at = next_seq();
        }
        idle_armed = true;
        idle_fn = fn;
        idle_ctx = ctx;
    }
    static void disable_idle_irq() {
        idle_off_at = next_seq();
        idle_armed = false;
        idle_fn = nullptr;
        idle_ctx = nullptr;
    }

    // the "hardware": one frame gap
    static void fire_idle() {
        if (idle_fn != nullptr) {
            idle_fn(idle_ctx);
        }
    }
};

// Recording fake DMA controller — the dma_impl contract the real ring<> needs
// (test_adc_stream.cpp's fake, byte-width edition).
template <class Tag>
struct dma_impl<fake_ctrl<Tag>> {
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };
    static constexpr bool supports_circular = true;
    // Rings are gated on supports_ring, not on the circular bit
    // (alloy::dma::ring_capable); on an ST-shaped double they agree.
    static constexpr bool supports_ring = true;

    static inline int setup_at = 0;
    static inline int start_at = 0;
    static inline int stop_at = 0;
    static inline bool was_circular = false;
    static inline dir last_dir = dir::mem_to_periph;
    static inline width last_msize = width::b32;
    static inline width last_psize = width::b32;
    static inline std::uintptr_t last_periph = 0;
    static inline std::uintptr_t last_mem = 0;
    static inline std::uint16_t last_items = 0;
    static inline std::uint8_t last_request = 0;
    static inline std::uint16_t rem = 0;
    static inline void (*half_fn)(void*) = nullptr;
    static inline void* half_ctx = nullptr;
    static inline void (*full_fn)(void*) = nullptr;
    static inline void* full_ctx = nullptr;

    static void enable_controller() {}
    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph, std::uintptr_t mem,
                      std::uint16_t items, std::uint8_t request) {
        setup_at = next_seq();
        last_dir = d;
        was_circular = circular;
        last_msize = msize;
        last_psize = psize;
        last_periph = periph;
        last_mem = mem;
        last_items = items;
        last_request = request;
        rem = items;
    }
    template <unsigned Ch>
    static void start() { start_at = next_seq(); }
    template <unsigned Ch>
    static void stop() { stop_at = next_seq(); }
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

// ── arm order, plumbing, teardown order ───────────────────────────────────

ALLOY_TEST(uart_rx_stream_arms_channel_then_dmar_then_idle_and_reverses) {
    struct tag {};
    using Inst = fake_uart<tag>;
    using Ctrl = fake_ctrl<tag>;
    using uart_f = alloy::hal::uart_impl<Inst>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;
    using Route = alloy::dma::route<Ctrl, 2, 52>;

    static alloy::dma::ring_storage<std::uint8_t, 16> buf;
    {
        // rom_bind::open() is the pinless way to a handle, same claim story
        // as a board; the board-route path is compile-probed further down.
        auto u = alloy::uart::rom_bind<Inst>::open();
        auto s = u.rx_ring(Route{}, buf);

        // The ring programmed a circular byte-wide p2m transfer from RDR with
        // the ROUTE's request id (the generated board fact) into the caller's
        // storage.
        ALLOY_CHECK(dma_f::was_circular);
        ALLOY_CHECK(dma_f::last_dir == dma_f::dir::periph_to_mem);
        ALLOY_CHECK(dma_f::last_msize == dma_f::width::b8);
        ALLOY_CHECK_EQ(dma_f::last_periph, uart_f::rdr_addr());
        ALLOY_CHECK_EQ(dma_f::last_mem, reinterpret_cast<std::uintptr_t>(buf.data));
        ALLOY_CHECK_EQ(dma_f::last_items, 16u);
        ALLOY_CHECK_EQ(dma_f::last_request, 52u);
        // The channel is claimed while the stream lives...
        ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 2,
                                            alloy::claim::personality::dma>()));
        // ...and the ARM ORDER is channel first, request stream second, wake
        // third: a request routed to an unarmed channel is a lost byte.
        ALLOY_CHECK(dma_f::start_at != 0);
        ALLOY_CHECK(dma_f::start_at < uart_f::rx_begin_at);
        ALLOY_CHECK(uart_f::rx_begin_at < uart_f::idle_arm_at);
        // IDLE armed by rx_ring itself (§2.2: the sleep_until_event wake) —
        // registered, with no callback chosen yet.
        ALLOY_CHECK(uart_f::idle_armed);
        ALLOY_CHECK(uart_f::idle_fn == nullptr);
    }
    // §4 TEARDOWN ORDER, reverse of arming: wake off, request stream off,
    // THEN the channel stops (facade destructor body, then the ring member).
    ALLOY_CHECK(uart_f::idle_off_at != 0);
    ALLOY_CHECK(uart_f::idle_off_at < uart_f::rx_end_at);
    ALLOY_CHECK(uart_f::rx_end_at < dma_f::stop_at);
    ALLOY_CHECK(!uart_f::idle_armed);
    // ...and the claim is released (rings release; tokens hold).
    ALLOY_CHECK(!(alloy::claim::sub_held<Ctrl, 2,
                                         alloy::claim::personality::dma>()));
}

// ── the anchor's consumption loop, miniaturized ───────────────────────────

ALLOY_TEST(uart_rx_stream_forwards_the_byte_stream_discipline) {
    struct tag {};
    using Inst = fake_uart<tag>;
    using Ctrl = fake_ctrl<tag>;
    using uart_f = alloy::hal::uart_impl<Inst>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;
    using Route = alloy::dma::route<Ctrl, 1, 52>;

    static alloy::dma::ring_storage<std::uint8_t, 16> buf;
    auto u = alloy::uart::rom_bind<Inst>::open();
    auto s = u.rx_ring(Route{}, buf);

    // Idle line, nothing received.
    ALLOY_CHECK_EQ(s.cursor(), 0u);
    ALLOY_CHECK(s.readable().empty());

    // A 5-byte frame lands by DMA (the engine's count register moves; no ISR
    // ran), then the line goes quiet — IDLE fires and wakes the consumer,
    // which drains readable() and consumes what it parsed.
    buf.data[0] = 0x11;
    buf.data[4] = 0x55;
    dma_f::rem = 11;
    uart_f::fire_idle();  // the wake; delivery is the WFI return, not a hook
    std::span<const std::uint8_t> frame = s.readable();
    ALLOY_CHECK_EQ(frame.size(), 5u);
    ALLOY_CHECK_EQ(frame.data(), &buf.data[0]);
    ALLOY_CHECK_EQ(frame[0], 0x11u);
    ALLOY_CHECK_EQ(frame[4], 0x55u);
    s.consume(frame.size());
    ALLOY_CHECK(s.readable().empty());
    ALLOY_CHECK_EQ(s.cursor(), 5u);
    ALLOY_CHECK_EQ(s.missed(), 0u);

    // The half-granular discipline composes on the same stream: a stable half
    // arrives, pending() flips, take() hands it over.
    ALLOY_CHECK(!s.pending());
    dma_f::fire_half();
    ALLOY_CHECK(s.pending());
    ALLOY_CHECK_EQ(s.take().data(), &buf.data[0]);
}

ALLOY_TEST(uart_rx_stream_on_idle_chooses_the_listener) {
    struct tag {};
    using Inst = fake_uart<tag>;
    using Ctrl = fake_ctrl<tag>;
    using uart_f = alloy::hal::uart_impl<Inst>;
    using Route = alloy::dma::route<Ctrl, 1, 14>;

    static alloy::dma::ring_storage<std::uint8_t, 8> buf;
    auto u = alloy::uart::rom_bind<Inst>::open();
    auto s = u.rx_ring(Route{}, buf);

    static int gaps = 0;
    gaps = 0;
    uart_f::fire_idle();  // armed, nobody listening: wake-only, no call
    ALLOY_CHECK_EQ(gaps, 0);
    s.on_idle([](void*) { ++gaps; });
    uart_f::fire_idle();
    uart_f::fire_idle();
    ALLOY_CHECK_EQ(gaps, 2);
}

// ── the compile-time gates ────────────────────────────────────────────────

namespace {

template <class H>
constexpr bool offers_rx_ring =
    requires(H h, alloy::dma::ring_storage<std::uint8_t, 8>& s) {
        h.rx_ring(s);
    };

template <class H, class Route>
constexpr bool offers_explicit_rx_ring =
    requires(H h, alloy::dma::ring_storage<std::uint8_t, 8>& s) {
        h.rx_ring(Route{}, s);
    };

// A UART driver with NO RX-DMA/IDLE hooks (every non-ST driver today).
struct hookless_uart {};

}  // namespace

namespace alloy::hal {
template <>
struct uart_impl<hookless_uart> {};
}  // namespace alloy::hal

ALLOY_TEST(uart_rx_ring_gates_on_route_and_capability) {
    struct tag {};
    using Inst = fake_uart<tag>;
    using Ctrl = fake_ctrl<tag>;
    using Route = alloy::dma::route<Ctrl, 1, 52>;

    // A handle whose binder carries a route offers rx_ring(); one with void
    // (no board assignment) does not — the design's §1 gate.
    ALLOY_CHECK((offers_rx_ring<alloy::uart::handle<Inst, Route>>));
    ALLOY_CHECK((!offers_rx_ring<alloy::uart::handle<Inst, void>>));
    // The escape hatch stays: an explicit route works on a route-less handle.
    ALLOY_CHECK((offers_explicit_rx_ring<alloy::uart::handle<Inst, void>, Route>));
    // A driver without the RX-DMA/IDLE hooks is refused even WITH a route:
    // rx_stream_capable is the whole contract, not just the route's half.
    ALLOY_CHECK((!offers_rx_ring<alloy::uart::handle<hookless_uart, Route>>));
    ALLOY_CHECK((!alloy::uart::rx_stream_capable<hookless_uart, Route>));
    ALLOY_CHECK((alloy::uart::rx_stream_capable<Inst, Route>));

    // The binder-side attachment point: rx_dma<> is found anywhere in the
    // Extra... list (the emitter appends it after de<>), void when absent.
    ALLOY_CHECK(
        (std::is_same_v<
            typename alloy::uart::detail::rx_route_of<
                alloy::uart::de<int>, alloy::uart::rx_dma<Route>>::type,
            Route>));
    ALLOY_CHECK(
        (std::is_same_v<
            typename alloy::uart::detail::rx_route_of<alloy::uart::de<int>>::type,
            void>));

    // The TX side's attachment point (anchor 2.3): tx_dma<> is found anywhere
    // in Extra..., void when absent, and each extractor sees only ITS tag —
    // an rx assignment must never leak into tx_route or vice versa.
    ALLOY_CHECK(
        (std::is_same_v<
            typename alloy::uart::detail::tx_route_of<
                alloy::uart::de<int>, alloy::uart::rx_dma<Route>,
                alloy::uart::tx_dma<alloy::dma::route<Ctrl, 3, 53>>>::type,
            alloy::dma::route<Ctrl, 3, 53>>));
    ALLOY_CHECK(
        (std::is_same_v<
            typename alloy::uart::detail::tx_route_of<
                alloy::uart::de<int>, alloy::uart::rx_dma<Route>>::type,
            void>));
    ALLOY_CHECK(
        (std::is_same_v<
            typename alloy::uart::detail::rx_route_of<
                alloy::uart::tx_dma<alloy::dma::route<Ctrl, 3, 53>>>::type,
            void>));
}
