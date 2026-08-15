// Unit tests for alloy::adc::stream / adc.ring() — the facade half of the DMA
// stream design (docs/design/dma-streams.md §2.1/§4), driven through recording
// fakes in the test_dma_ring.cpp spirit: the REAL stream + REAL ring run
// against a fake adc_impl and a fake dma_impl that share one sequence counter,
// so the §4 ARM order (ADC configured -> channel armed -> ADSTART last) and
// TEARDOWN order (ADC request stream off BEFORE the channel stops) are
// asserted as recorded ordinals, not argued from destructor prose. What these
// tests can NOT witness: a real converter feeding a real engine — that is the
// adc_stream emulation leg's claim, not this file's.

#include <cstdint>
#include <span>

#include "alloy/adc.hpp"
#include "alloy/dma.hpp"
#include "alloy_test.hpp"

namespace {

template <class Tag>
struct fake_adc {
    // bind::kernel_hz() switches on this — any node will do for a fake.
    static constexpr alloy::clock_node kernel = alloy::clock_node::ahb;
};

template <class Tag>
struct fake_ctrl {};

// One shared sequence counter so cross-peripheral ordering is a comparison of
// two recorded integers. 0 = never called.
inline int g_seq = 0;
inline int next_seq() { return ++g_seq; }

}  // namespace

namespace alloy::hal {

// Recording fake converter: the four DMA-burst hooks + enable(), same static
// shape as st_adc_v2 so the real facade compiles against it unchanged.
template <class Tag>
struct adc_impl<fake_adc<Tag>> {
    static inline int begin_at = 0;
    static inline int kick_at = 0;
    static inline int end_at = 0;
    static inline std::uint8_t begun_channel = 0xFF;
    static inline std::uint16_t dr = 0;

    static void enable(std::uint32_t) {}
    static void dma_burst_begin(std::uint8_t channel) {
        begin_at = next_seq();
        begun_channel = channel;
    }
    static void dma_burst_kick() { kick_at = next_seq(); }
    static void dma_burst_end() { end_at = next_seq(); }
    [[nodiscard]] static std::uintptr_t dr_addr() {
        return reinterpret_cast<std::uintptr_t>(&dr);
    }
};

// Recording fake DMA controller — the dma_impl contract the real ring<> needs
// (test_dma_ring.cpp's fake, reduced to what these tests assert).
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
    static inline width last_msize = width::b8;
    static inline std::uintptr_t last_periph = 0;
    static inline std::uint16_t last_items = 0;
    static inline std::uint8_t last_request = 0;
    static inline std::uint16_t rem = 0;
    static inline void (*half_fn)(void*) = nullptr;
    static inline void* half_ctx = nullptr;
    static inline void (*full_fn)(void*) = nullptr;
    static inline void* full_ctx = nullptr;

    static void enable_controller() {}
    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width,
                      std::uintptr_t periph, std::uintptr_t, std::uint16_t items,
                      std::uint8_t request) {
        setup_at = next_seq();
        last_dir = d;
        was_circular = circular;
        last_msize = msize;
        last_periph = periph;
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

namespace {

struct fake_clock {
    static constexpr std::uint32_t ahb_hz = 1;
    static constexpr std::uint32_t apb_hz = 1;
    static constexpr std::uint32_t apb2_hz = 1;
    static constexpr std::uint32_t sysclk_hz = 1;
};

}  // namespace

// ── arm order, request plumbing, teardown order ───────────────────────────

ALLOY_TEST(adc_stream_arms_in_design_order_and_tears_down_in_reverse) {
    struct tag {};
    using Inst = fake_adc<tag>;
    using Ctrl = fake_ctrl<tag>;
    using adc_f = alloy::hal::adc_impl<Inst>;
    using dma_f = alloy::hal::dma_impl<Ctrl>;
    using Route = alloy::dma::route<Ctrl, 2, 5>;

    static alloy::dma::ring_storage<std::uint16_t, 8> buf;
    {
        // bind::open() is the only public way to a handle, same as a board.
        auto adc = alloy::adc::bind<Inst, fake_clock, Route>::open();
        auto s = adc.ring(buf, /*channel=*/3u);

        // §4 arm order: configure the ADC (DMAEN/CONT, ADSTART still 0),
        // arm the channel (setup then start), kick conversions LAST.
        ALLOY_CHECK(adc_f::begin_at != 0);
        ALLOY_CHECK(adc_f::begin_at < dma_f::setup_at);
        ALLOY_CHECK(dma_f::setup_at < dma_f::start_at);
        ALLOY_CHECK(dma_f::start_at < adc_f::kick_at);
        ALLOY_CHECK_EQ(adc_f::begun_channel, 3u);

        // The stream is a circular p2m of 16-bit items from the converter's
        // DR, and the request id is the ROUTE's fact, never caller-typed.
        ALLOY_CHECK(dma_f::was_circular);
        ALLOY_CHECK(dma_f::last_dir == dma_f::dir::periph_to_mem);
        ALLOY_CHECK(dma_f::last_msize == dma_f::width::b16);
        ALLOY_CHECK_EQ(dma_f::last_periph, adc_f::dr_addr());
        ALLOY_CHECK_EQ(dma_f::last_items, 8u);
        ALLOY_CHECK_EQ(dma_f::last_request, 5u);

        // take() forwards the ring's checked discipline.
        dma_f::fire_half();
        std::span<const std::uint16_t> h = s.take();
        ALLOY_CHECK_EQ(h.data(), &buf.data[0]);
        ALLOY_CHECK_EQ(h.size(), 4u);
        ALLOY_CHECK_EQ(s.missed(), 0u);
    }
    // §4 teardown order: the ADC's request stream stopped BEFORE the channel
    // (facade destructor body, then the ring member) — a request landing on a
    // disabled channel mid-teardown is how overrun flags get stuck.
    ALLOY_CHECK(adc_f::end_at != 0);
    ALLOY_CHECK(dma_f::stop_at != 0);
    ALLOY_CHECK(adc_f::end_at < dma_f::stop_at);
}

// ── the compile-time gates, probed the way a portable example probes ──────
//
// The probes are variable templates so the failing call is DEPENDENT: a
// requires-expression over concrete types is ill-formed on the failure path
// (clang enforces this), which is also why the portable example probes from
// inside a templated lambda.

namespace {

template <class H>
constexpr bool offers_ring =
    requires(H h, alloy::dma::ring_storage<std::uint16_t, 8>& s) { h.ring(s); };

template <class H, class Route>
constexpr bool offers_explicit_ring =
    requires(H h, alloy::dma::ring_storage<std::uint16_t, 8>& s) {
        h.ring(Route{}, s);
    };

}  // namespace

ALLOY_TEST(adc_stream_gates_on_route_and_capability) {
    struct tag2 {};
    using Inst = fake_adc<tag2>;
    using Ctrl = fake_ctrl<tag2>;
    using Route = alloy::dma::route<Ctrl, 1, 9>;

    // A handle whose binder carries a route offers ring(); one with void
    // (board assigned nothing) does not — the missing fact is a compile
    // error at the requires-gate, never a silent misroute.
    ALLOY_CHECK((offers_ring<alloy::adc::handle<Inst, Route>>));
    ALLOY_CHECK((!offers_ring<alloy::adc::handle<Inst, void>>));

    // The explicit-route escape hatch exists even on a route-less handle.
    ALLOY_CHECK((offers_explicit_ring<alloy::adc::handle<Inst, void>, Route>));
}
