// The I2C one-shot DMA path — docs/design/dma-streams.md §6's phase-4 row
// ("i2c one-shot DMA read/write"), witnessed on the host in two layers.
//
// LAYER 1, the REGISTER WITNESS. detail::st_i2c_v2_engine (split out of
// st_i2c_v2.hpp for exactly this file) is instantiated over a hand-written IP
// double whose registers are plain host memory, laid out with the i2c_v2
// shape. These tests run the REAL driver sequences and read the REAL register
// words back, so the claims are about bits and not about a mock's opinion:
// delete `IP::rxdmaen.set(r())` from dma_rx_begin() and
// `i2c_dma_begin_raises_one_request_enable_per_direction` fails; drop the
// AUTOEND or the START term from dma_start_transfer()'s CR2 word and
// `i2c_dma_start_transfer_writes_the_cpu_driven_address_phase` fails.
// (Verified by doing exactly those reverts locally while writing this file.)
//
// LAYER 2, the FACADE WITNESS. The real alloy::i2c::handle runs against a
// recording fake driver and a recording fake channel, so the ARM ORDER
// (channel -> request enable -> address phase), its exact reverse at teardown,
// the request-id source rule, the NBYTES refusal, the BOUNDED wait and the
// compile-time gates are all exercised off-target — the test_uart_rx_stream.cpp
// arrangement.
//
// WHAT THIS FILE CANNOT WITNESS, and nothing else in the tree can either
// today: a real DMA request crossing real silicon. Renode 1.16.1's model of
// this IP (I2C.STM32F7_I2C) has no DMA request output at all — the commit that
// adds one landed upstream on 2026-07-10, after the pinned release — so there
// is no emulation leg for the read direction without vendoring a post-release
// model, and the WRITE direction is unwitnessable even upstream (TXDMAEN is an
// inert `.WithTag` there and no send-request GPIO exists). The register
// sequences below are the only witness this path has.

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/core/claim.hpp"
#include "alloy/core/mmio.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/i2c/st_i2c_v2_body.hpp"
#include "alloy/i2c.hpp"
#include "alloy_test.hpp"

// ══ LAYER 1 — the real driver over a memory-backed register file ═════════

namespace {

// The i2c_v2 register shape, hand-written (tests never read generated
// headers): same offsets and bit positions as registers/st/i2c_v2.yaml, so the
// sequences exercised here are the sequences the silicon sees. ICR is declared
// READABLE — it is write-only on the part, but the test is the hardware here
// and has to see what the driver acknowledged; a helper commits the
// write-1-to-clear effect explicitly, so the test picks the exact instant a
// flag disappears.
// Templated on a dummy tag purely so its unused-by-this-file field
// declarations (the polled path's TIMINGR fields, the async path's interrupt
// enables) do not trip -Wunused-const-variable: the layout is documentation as
// much as it is machinery, and trimming it to only what these tests touch
// would hide which register the driver is actually reaching into.
template <class = void>
struct fake_i2c_ip_t {
    struct regs {
        alloy::rw32 CR1;
        alloy::rw32 CR2;
        alloy::rw32 OAR1;
        alloy::rw32 OAR2;
        alloy::rw32 TIMINGR;
        alloy::rw32 TIMEOUTR;
        alloy::rw32 ISR;
        alloy::rw32 ICR;
        alloy::rw32 PECR;
        alloy::rw32 RXDR;
        alloy::rw32 TXDR;
    };

    static constexpr auto pe = alloy::field<&regs::CR1, 0u, 1>;
    static constexpr auto txie = alloy::field<&regs::CR1, 1u, 1>;
    static constexpr auto rxie = alloy::field<&regs::CR1, 2u, 1>;
    static constexpr auto stopie = alloy::field<&regs::CR1, 5u, 1>;
    static constexpr auto txdmaen = alloy::field<&regs::CR1, 14u, 1>;
    static constexpr auto rxdmaen = alloy::field<&regs::CR1, 15u, 1>;

    static constexpr auto rd_wrn = alloy::field<&regs::CR2, 10u, 1>;
    static constexpr auto start = alloy::field<&regs::CR2, 13u, 1>;
    static constexpr auto nbytes = alloy::field<&regs::CR2, 16u, 8>;
    static constexpr auto autoend = alloy::field<&regs::CR2, 25u, 1>;

    static constexpr auto scll = alloy::field<&regs::TIMINGR, 0u, 8>;
    static constexpr auto sclh = alloy::field<&regs::TIMINGR, 8u, 8>;
    static constexpr auto sdadel = alloy::field<&regs::TIMINGR, 16u, 4>;
    static constexpr auto scldel = alloy::field<&regs::TIMINGR, 20u, 4>;
    static constexpr auto presc = alloy::field<&regs::TIMINGR, 28u, 4>;

    static constexpr auto txis = alloy::field<&regs::ISR, 1u, 1>;
    static constexpr auto rxne = alloy::field<&regs::ISR, 2u, 1>;
    static constexpr auto nackf = alloy::field<&regs::ISR, 4u, 1>;
    static constexpr auto stopf = alloy::field<&regs::ISR, 5u, 1>;
    static constexpr auto tc = alloy::field<&regs::ISR, 6u, 1>;
    static constexpr auto berr = alloy::field<&regs::ISR, 8u, 1>;
    static constexpr auto arlo = alloy::field<&regs::ISR, 9u, 1>;

    static constexpr auto nackcf = alloy::field<&regs::ICR, 4u, 1>;
    static constexpr auto stopcf = alloy::field<&regs::ICR, 5u, 1>;
    static constexpr auto berrcf = alloy::field<&regs::ICR, 8u, 1>;
    static constexpr auto arlocf = alloy::field<&regs::ICR, 9u, 1>;

    enum class icr : std::uint32_t {
        nackcf = std::uint32_t{1} << 4u,
        stopcf = std::uint32_t{1} << 5u,
        berrcf = std::uint32_t{1} << 8u,
        arlocf = std::uint32_t{1} << 9u,
    };
};

using fake_i2c_ip = fake_i2c_ip_t<>;

// One instance per test tag, so the engine's statics (busy_, ok_) and its
// register file are fresh.
template <class Tag>
struct raw_inst {
    using ip = fake_i2c_ip;
    static inline std::uint32_t mem[16]{};
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
    static constexpr alloy::irq_line irq{0};
};

template <class Tag>
using engine = alloy::hal::detail::st_i2c_v2_engine<raw_inst<Tag>>;

}  // namespace

namespace alloy {
template <>
inline constexpr bool reg_flags_enabled<fake_i2c_ip::icr> = true;
}  // namespace alloy

namespace {

// The hardware's half of ICR: write-1-to-clear, committed on demand so a test
// asserts what the driver acknowledged BEFORE the flag disappears.
template <class Tag>
void commit_w1c() {
    auto& r = engine<Tag>::r();
    r.ISR = r.ISR & ~r.ICR;
    r.ICR = 0u;
}

}  // namespace

// ── the two request enables ───────────────────────────────────────────────

ALLOY_TEST(i2c_dma_begin_raises_one_request_enable_per_direction) {
    struct tag {};
    using eng = engine<tag>;
    eng::r().CR1 = 0u;

    // RX arms bit 15 and NOTHING else — an i2c_v2 has two separate enables
    // (unlike i2c_v1's single CR2.DMAEN), and arming the wrong one is a
    // transfer that never moves a byte.
    eng::dma_rx_begin();
    ALLOY_CHECK_EQ(eng::r().CR1, std::uint32_t{1} << 15u);
    eng::dma_rx_end();
    ALLOY_CHECK_EQ(eng::r().CR1, 0u);

    // TX arms bit 14, same discipline.
    eng::dma_tx_begin();
    ALLOY_CHECK_EQ(eng::r().CR1, std::uint32_t{1} << 14u);
    eng::dma_tx_end();
    ALLOY_CHECK_EQ(eng::r().CR1, 0u);

    // Neither touches the other's bit, nor PE, nor the interrupt enables the
    // async path owns: both directions armed at once is still exactly two bits.
    eng::r().CR1 = 1u;  // PE, as an open() bus has it
    eng::dma_rx_begin();
    eng::dma_tx_begin();
    ALLOY_CHECK_EQ(eng::r().CR1,
                   1u | (std::uint32_t{1} << 14u) | (std::uint32_t{1} << 15u));
    eng::dma_rx_end();
    eng::dma_tx_end();
    ALLOY_CHECK_EQ(eng::r().CR1, 1u);
}

ALLOY_TEST(i2c_dma_names_a_data_register_per_direction) {
    struct tag {};
    using eng = engine<tag>;
    // RXDR at 0x24 read-only and TXDR at 0x28 write-only: two registers,
    // unlike SPI's single bidirectional DR, so a direction that took the
    // other's address would read the register it just wrote.
    ALLOY_CHECK_EQ(eng::rxdr_addr(), raw_inst<tag>::base + 0x24u);
    ALLOY_CHECK_EQ(eng::txdr_addr(), raw_inst<tag>::base + 0x28u);
}

// ── the CPU-driven address phase ──────────────────────────────────────────

ALLOY_TEST(i2c_dma_start_transfer_writes_the_cpu_driven_address_phase) {
    struct tag {};
    using eng = engine<tag>;
    eng::r().CR2 = 0u;
    eng::r().ISR = 0u;
    eng::r().ICR = 0u;

    // A 3-byte READ: SADD<<1, RD_WRN, NBYTES=3, AUTOEND, START. The whole
    // point of the phase-4 boundary is that this word is IDENTICAL to the one
    // the polled read() writes — DMA moves the payload, not the transaction.
    ALLOY_CHECK(eng::dma_start_transfer(0x08, 3u, /*is_read=*/true));
    const std::uint32_t expect_read = (std::uint32_t{0x08} << 1) |
                                      (std::uint32_t{3} << 16) |
                                      (std::uint32_t{1} << 10) |   // RD_WRN
                                      (std::uint32_t{1} << 25) |   // AUTOEND
                                      (std::uint32_t{1} << 13);    // START
    ALLOY_CHECK_EQ(eng::r().CR2, expect_read);
    // Stale flags acknowledged before START, or a previous transaction's STOPF
    // would satisfy this one's completion wait instantly.
    ALLOY_CHECK_EQ(eng::r().ICR, (std::uint32_t{1} << 4) | (std::uint32_t{1} << 5) |
                                     (std::uint32_t{1} << 8) | (std::uint32_t{1} << 9));

    // A 3-byte WRITE is the same word without RD_WRN.
    eng::r().CR2 = 0u;
    ALLOY_CHECK(eng::dma_start_transfer(0x08, 3u, /*is_read=*/false));
    ALLOY_CHECK_EQ(eng::r().CR2, expect_read & ~(std::uint32_t{1} << 10));
}

ALLOY_TEST(i2c_dma_inherits_the_nbytes_refusal_without_touching_cr2) {
    struct tag {};
    using eng = engine<tag>;

    // NBYTES is an 8-bit CR2 field; 256 would overflow into RELOAD/AUTOEND.
    // The DMA path refuses the SAME lengths read()/write() refuse, from the
    // same constant — and refuses BEFORE writing CR2, so a rejected call
    // cannot leave a half-programmed transaction on the bus.
    eng::r().CR2 = 0xDEADu;
    ALLOY_CHECK(!eng::dma_start_transfer(0x08, 256u, true));
    ALLOY_CHECK_EQ(eng::r().CR2, 0xDEADu);
    ALLOY_CHECK(!eng::dma_start_transfer(0x08, 0u, true));
    ALLOY_CHECK_EQ(eng::r().CR2, 0xDEADu);
    // 255 is legal, and is the last legal one.
    ALLOY_CHECK(eng::dma_start_transfer(0x08, 255u, true));
    ALLOY_CHECK_EQ((eng::r().CR2 >> 16) & 0xFFu, 255u);
    ALLOY_CHECK_EQ(eng::kMaxNbytes, std::size_t{255});
}

ALLOY_TEST(i2c_dma_start_transfer_refuses_while_the_irq_path_owns_the_bus) {
    struct tag {};
    using eng = engine<tag>;
    eng::r().CR2 = 0u;
    // The three transfer paths consume the same flags; a DMA transfer started
    // under an in-flight interrupt-driven one would have both of them reading
    // each other's STOPF. Refused, with nothing programmed.
    eng::busy_ = true;
    ALLOY_CHECK(!eng::dma_start_transfer(0x08, 1u, true));
    ALLOY_CHECK_EQ(eng::r().CR2, 0u);
    eng::busy_ = false;
    ALLOY_CHECK(eng::dma_start_transfer(0x08, 1u, true));
}

// ── completion: the STOP the engine cannot see ────────────────────────────

ALLOY_TEST(i2c_dma_wait_stop_reports_the_stop_and_acknowledges_it) {
    struct tag {};
    using eng = engine<tag>;
    eng::r().ISR = 0u;
    eng::r().ICR = 0u;

    // The DMA finishing means the last byte crossed the data register; the
    // transaction ends at AUTOEND's STOP, which only the CPU sees.
    eng::r().ISR = std::uint32_t{1} << 5;  // STOPF
    ALLOY_CHECK(eng::dma_wait_stop());
    ALLOY_CHECK_EQ(eng::r().ICR, std::uint32_t{1} << 5);  // STOPCF written
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(eng::r().ISR, 0u);  // and the bus is idle for the next call
}

ALLOY_TEST(i2c_dma_wait_stop_reports_a_nack_the_way_the_polled_path_does) {
    struct tag {};
    using eng = engine<tag>;
    // A device that does not answer: AUTOEND still makes a STOP, and NACKF is
    // the only evidence the transfer moved nothing. false, and both flags
    // acknowledged so the next call starts on an idle peripheral.
    eng::r().ISR = (std::uint32_t{1} << 4) | (std::uint32_t{1} << 5);
    eng::r().ICR = 0u;
    ALLOY_CHECK(!eng::dma_wait_stop());
    ALLOY_CHECK_EQ(eng::r().ICR, (std::uint32_t{1} << 4) | (std::uint32_t{1} << 5));
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(eng::r().ISR, 0u);
}

ALLOY_TEST(i2c_dma_wait_stop_is_bounded_on_a_wedged_bus) {
    struct tag {};
    using eng = engine<tag>;
    // No STOPF, no NACK, no bus error — SDA held low by a stuck device is what
    // this looks like. The wait is bounded by the driver's kPollBudget, so it
    // becomes an honest false. An unbounded spin here would turn a stuck
    // device into a hang, and a hang is not a test failure, it is a timeout
    // that looks like anything at all.
    eng::r().ISR = 0u;
    ALLOY_CHECK(!eng::dma_wait_stop());
}

// ══ LAYER 2 — the real facade over recording fakes ═══════════════════════

namespace {

// One shared sequence counter, so cross-object ordering is a comparison of two
// recorded integers. 0 = never called.
inline int g_seq = 0;
inline int next_seq() { return ++g_seq; }

// A free-router instance (G0 shape): the request id is a chip-wide fact.
template <class Tag>
struct free_router_inst {
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
    [[maybe_unused]] static constexpr std::uint8_t dmareq_rx = 10u;
    [[maybe_unused]] static constexpr std::uint8_t dmareq_tx = 11u;
};

// A stream-engine instance (F4/F7 shape): no chip-wide id exists, because
// CHSEL is a per-stream fact of the matched dma_routes triple.
template <class Tag>
struct stream_inst {
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
};

// An instance whose driver has no DMA hooks at all (every non-ST-v2 I2C
// driver today).
struct hookless_inst {
    [[maybe_unused]] static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
    // The request ids ARE here: the point of this instance is that the gate
    // still refuses it, because the DRIVER has no hooks — the board's half of
    // the contract is not the whole contract.
    [[maybe_unused]] static constexpr std::uint8_t dmareq_rx = 10u;
    [[maybe_unused]] static constexpr std::uint8_t dmareq_tx = 11u;
};

// The recording driver: the hook set the facade's DMA methods name, with the
// same static shape as st_i2c_v2_engine so the real facade compiles against it
// unchanged.
template <class Inst>
struct recording_i2c {
    static constexpr std::size_t kMaxNbytes = 255;

    static inline int rx_begin_at = 0;
    static inline int rx_end_at = 0;
    static inline int tx_begin_at = 0;
    static inline int tx_end_at = 0;
    static inline int start_at = 0;
    static inline int stop_wait_at = 0;
    static inline std::uint8_t last_addr = 0;
    static inline std::size_t last_n = 0;
    static inline bool last_is_read = false;
    static inline bool start_result = true;
    static inline bool stop_result = true;
    static inline std::uint32_t rxdr = 0;
    static inline std::uint32_t txdr = 0;

    static void reset() {
        rx_begin_at = rx_end_at = tx_begin_at = tx_end_at = 0;
        start_at = stop_wait_at = 0;
        last_addr = 0;
        last_n = 0;
        last_is_read = false;
        start_result = true;
        stop_result = true;
    }

    static void enable(std::uint32_t, std::uint32_t) {}

    static void dma_rx_begin() { rx_begin_at = next_seq(); }
    static void dma_rx_end() { rx_end_at = next_seq(); }
    static void dma_tx_begin() { tx_begin_at = next_seq(); }
    static void dma_tx_end() { tx_end_at = next_seq(); }
    [[nodiscard]] static std::uintptr_t rxdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&rxdr);
    }
    [[nodiscard]] static std::uintptr_t txdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&txdr);
    }
    [[nodiscard]] static bool dma_start_transfer(std::uint8_t addr, std::size_t n,
                                                 bool is_read) {
        start_at = next_seq();
        last_addr = addr;
        last_n = n;
        last_is_read = is_read;
        return start_result;
    }
    [[nodiscard]] static bool dma_wait_stop() {
        stop_wait_at = next_seq();
        return stop_result;
    }
};

}  // namespace

namespace alloy::hal {
template <class Tag>
struct i2c_impl<free_router_inst<Tag>> : recording_i2c<free_router_inst<Tag>> {};
template <class Tag>
struct i2c_impl<stream_inst<Tag>> : recording_i2c<stream_inst<Tag>> {};
template <>
struct i2c_impl<hookless_inst> {
    static void enable(std::uint32_t, std::uint32_t) {}
};
}  // namespace alloy::hal

namespace {

// A claimed-channel double: the four operations alloy::i2c::rx_channel /
// tx_channel name, recording what was programmed. `completes` is the
// hardware's side — a channel that never finishes is a wedged bus.
struct fake_chan {
    static inline int p2m_at = 0;
    static inline int m2p_at = 0;
    static inline int stop_at = 0;
    static inline std::uintptr_t last_periph = 0;
    static inline const void* last_mem = nullptr;
    static inline std::size_t last_items = 0;
    static inline std::uint8_t last_request = 0;
    static inline bool completes = true;
    static inline bool failed = false;

    static void reset() {
        p2m_at = m2p_at = stop_at = 0;
        last_periph = 0;
        last_mem = nullptr;
        last_items = 0;
        last_request = 0;
        completes = true;
        failed = false;
    }

    void start_p2m_u8(std::uintptr_t periph, std::span<std::uint8_t> dst,
                      std::uint8_t request) const {
        p2m_at = next_seq();
        last_periph = periph;
        last_mem = dst.data();
        last_items = dst.size();
        last_request = request;
    }
    void start_m2p_u8(std::span<const std::uint8_t> src, std::uintptr_t periph,
                      std::uint8_t request) const {
        m2p_at = next_seq();
        last_periph = periph;
        last_mem = src.data();
        last_items = src.size();
        last_request = request;
    }
    [[nodiscard]] bool done() const { return completes; }
    [[nodiscard]] bool error() const { return failed; }
    void stop() const { stop_at = next_seq(); }
};

// A token with everything BUT the byte-wide p2m starter — the shape a channel
// on a controller with no such starter would have.
struct no_p2m_chan {
    void start_m2p_u8(std::span<const std::uint8_t>, std::uintptr_t,
                      std::uint8_t) const {}
    [[nodiscard]] bool done() const { return true; }
    [[nodiscard]] bool error() const { return false; }
    void stop() const {}
};

struct fake_pin {};

struct fake_clock {
    static constexpr std::uint32_t ahb_hz = 16'000'000;
    static constexpr std::uint32_t apb_hz = 16'000'000;
    static constexpr std::uint32_t apb2_hz = 16'000'000;
    static constexpr std::uint32_t sysclk_hz = 16'000'000;
};

}  // namespace

namespace alloy::hal {
template <>
struct pin_impl<fake_pin> {
    static void make_af(std::uint8_t) {}
    static void make_af_od(std::uint8_t) {}
};
}  // namespace alloy::hal

namespace alloy::routes {
// One pin that routes to anything: the pin half is not what this file is
// about, and bind<> refuses to compile without it.
template <class Periph, alloy::signal S>
struct route<fake_pin, Periph, S> {
    static constexpr kind k = kind::af_fixed;
    static constexpr std::uint8_t af = 4;
};
}  // namespace alloy::routes

namespace {

template <class Inst, class... Extra>
using binder = alloy::i2c::bind<Inst, alloy::i2c::scl<fake_pin>,
                                alloy::i2c::sda<fake_pin>, fake_clock, Extra...>;

}  // namespace

// ── arm order, plumbing, teardown order ───────────────────────────────────

ALLOY_TEST(i2c_read_dma_arms_the_channel_then_dmaen_then_the_address_phase) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    drv::reset();
    fake_chan::reset();

    auto bus = binder<Inst>::open({.speed_hz = 100'000});
    std::uint8_t buf[3] = {};
    const fake_chan ch{};
    ALLOY_CHECK(bus.read_dma(ch, 0x48, buf));

    // The channel was programmed from RXDR into the caller's buffer, with the
    // chip's own request id — user code never names a DMAMUX number.
    ALLOY_CHECK_EQ(fake_chan::last_periph, drv::rxdr_addr());
    ALLOY_CHECK_EQ(fake_chan::last_mem, static_cast<const void*>(&buf[0]));
    ALLOY_CHECK_EQ(fake_chan::last_items, std::size_t{3});
    ALLOY_CHECK_EQ(fake_chan::last_request, 10u);

    // The address phase stayed CPU-driven and carried the whole transaction:
    // the device address, the byte count, and the direction.
    ALLOY_CHECK_EQ(drv::last_addr, 0x48u);
    ALLOY_CHECK_EQ(drv::last_n, std::size_t{3});
    ALLOY_CHECK(drv::last_is_read);

    // ARM ORDER: channel, then the request enable, then the START that puts
    // traffic on the wire. A request raised at a channel that is not armed is
    // a byte on the floor.
    ALLOY_CHECK(fake_chan::p2m_at != 0);
    ALLOY_CHECK(fake_chan::p2m_at < drv::rx_begin_at);
    ALLOY_CHECK(drv::rx_begin_at < drv::start_at);
    // ...and the STOP wait comes after the payload wait, which is the whole
    // reason it exists: the DMA finishing is not the transaction ending.
    ALLOY_CHECK(drv::start_at < drv::stop_wait_at);
    // TEARDOWN is the exact reverse: request enable off, THEN the channel.
    ALLOY_CHECK(drv::stop_wait_at < drv::rx_end_at);
    ALLOY_CHECK(drv::rx_end_at < fake_chan::stop_at);
    // The other direction was never touched.
    ALLOY_CHECK_EQ(drv::tx_begin_at, 0);
    ALLOY_CHECK_EQ(fake_chan::m2p_at, 0);
}

ALLOY_TEST(i2c_write_dma_arms_the_same_order_in_the_other_direction) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    drv::reset();
    fake_chan::reset();

    auto bus = binder<Inst>::open();
    static const std::uint8_t out[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    const fake_chan ch{};
    ALLOY_CHECK(bus.write_dma(ch, 0x48, out));

    ALLOY_CHECK_EQ(fake_chan::last_periph, drv::txdr_addr());
    ALLOY_CHECK_EQ(fake_chan::last_mem, static_cast<const void*>(&out[0]));
    ALLOY_CHECK_EQ(fake_chan::last_items, std::size_t{4});
    ALLOY_CHECK_EQ(fake_chan::last_request, 11u);  // dmareq_tx, not rx
    ALLOY_CHECK_EQ(drv::last_n, std::size_t{4});
    ALLOY_CHECK(!drv::last_is_read);

    ALLOY_CHECK(fake_chan::m2p_at != 0);
    ALLOY_CHECK(fake_chan::m2p_at < drv::tx_begin_at);
    ALLOY_CHECK(drv::tx_begin_at < drv::start_at);
    ALLOY_CHECK(drv::start_at < drv::stop_wait_at);
    ALLOY_CHECK(drv::stop_wait_at < drv::tx_end_at);
    ALLOY_CHECK(drv::tx_end_at < fake_chan::stop_at);
    ALLOY_CHECK_EQ(drv::rx_begin_at, 0);
    ALLOY_CHECK_EQ(fake_chan::p2m_at, 0);
}

// ── the refusals, and what they leave behind ──────────────────────────────

ALLOY_TEST(i2c_dma_refuses_the_nbytes_boundary_before_arming_anything) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    auto bus = binder<Inst>::open();
    static std::uint8_t big[300] = {};

    // > NBYTES: false, and NOTHING was armed — no channel programmed, no
    // request enable raised, no START. A refusal that had already set RXDMAEN
    // would leave the bus armed for a transfer that will never come.
    drv::reset();
    fake_chan::reset();
    const fake_chan ch{};
    ALLOY_CHECK(!bus.read_dma(ch, 0x48, std::span<std::uint8_t>{big, 256}));
    ALLOY_CHECK_EQ(fake_chan::p2m_at, 0);
    ALLOY_CHECK_EQ(drv::rx_begin_at, 0);
    ALLOY_CHECK_EQ(drv::start_at, 0);

    // Empty: same answer. A zero-length DMA is a trap in the channel starter,
    // so the facade must refuse it before it gets there — probe() is the
    // address-only transaction, and it is not a DMA operation.
    drv::reset();
    fake_chan::reset();
    ALLOY_CHECK(!bus.read_dma(ch, 0x48, std::span<std::uint8_t>{}));
    ALLOY_CHECK_EQ(fake_chan::p2m_at, 0);
    ALLOY_CHECK_EQ(drv::rx_begin_at, 0);

    // 255 is accepted — the boundary is the driver's, and the facade reads it
    // from the driver rather than restating it.
    drv::reset();
    fake_chan::reset();
    ALLOY_CHECK(bus.read_dma(ch, 0x48, std::span<std::uint8_t>{big, 255}));
    ALLOY_CHECK_EQ(drv::last_n, std::size_t{255});

    // Same boundary on the write side.
    drv::reset();
    fake_chan::reset();
    ALLOY_CHECK(!bus.write_dma(ch, 0x48, std::span<const std::uint8_t>{big, 256}));
    ALLOY_CHECK_EQ(fake_chan::m2p_at, 0);
    ALLOY_CHECK_EQ(drv::tx_begin_at, 0);
}

ALLOY_TEST(i2c_dma_tears_down_on_every_failure_path) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    auto bus = binder<Inst>::open();
    std::uint8_t buf[2] = {};
    const fake_chan ch{};

    // (1) the driver refused the address phase (the async path owns the bus).
    drv::reset();
    fake_chan::reset();
    drv::start_result = false;
    ALLOY_CHECK(!bus.read_dma(ch, 0x48, buf));
    ALLOY_CHECK(drv::rx_end_at != 0);
    ALLOY_CHECK(fake_chan::stop_at != 0);
    ALLOY_CHECK(drv::rx_end_at < fake_chan::stop_at);
    ALLOY_CHECK_EQ(drv::stop_wait_at, 0);  // never waited for a STOP that
                                           // was never asked for

    // (2) the transaction NACKed: the STOP wait says so.
    drv::reset();
    fake_chan::reset();
    drv::stop_result = false;
    ALLOY_CHECK(!bus.read_dma(ch, 0x48, buf));
    ALLOY_CHECK(drv::rx_end_at != 0);
    ALLOY_CHECK(fake_chan::stop_at != 0);

    // (3) the ENGINE failed (a bus error on the DMA side, latched by the
    // driver so a poller after the ISR still sees it). No STOP wait: the
    // transfer that would have produced the STOP did not happen.
    drv::reset();
    fake_chan::reset();
    fake_chan::failed = true;
    ALLOY_CHECK(!bus.read_dma(ch, 0x48, buf));
    ALLOY_CHECK_EQ(drv::stop_wait_at, 0);
    ALLOY_CHECK(drv::rx_end_at != 0);
    ALLOY_CHECK(fake_chan::stop_at != 0);
}

ALLOY_TEST(i2c_dma_wait_is_bounded_when_the_channel_never_completes) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    auto bus = binder<Inst>::open();
    std::uint8_t buf[2] = {};

    // The request stream stalls (a wedged bus, a channel the board pointed
    // somewhere else): done() never becomes true and error() never becomes
    // true either. dma::channel::wait() would spin here forever — right for a
    // transfer the CPU alone finishes, wrong for one a bus has to feed. The
    // facade's own bounded wait turns it into false, and the teardown still
    // runs so the bus is not left with DMAEN set.
    drv::reset();
    fake_chan::reset();
    fake_chan::completes = false;
    const fake_chan ch{};
    ALLOY_CHECK(!bus.read_dma(ch, 0x48, buf));
    ALLOY_CHECK_EQ(drv::stop_wait_at, 0);
    ALLOY_CHECK(drv::rx_end_at != 0);
    ALLOY_CHECK(fake_chan::stop_at != 0);
    fake_chan::completes = true;
}

// ── the request-id source rule ────────────────────────────────────────────

ALLOY_TEST(i2c_dma_takes_the_request_from_the_route_when_the_chip_has_none) {
    struct tag {};
    using Inst = stream_inst<tag>;  // no dmareq_rx / dmareq_tx
    using drv = alloy::hal::i2c_impl<Inst>;
    struct ctrl {};
    using RxRoute = alloy::dma::route<ctrl, 0, 3>;
    using TxRoute = alloy::dma::route<ctrl, 3, 3>;

    drv::reset();
    fake_chan::reset();
    auto bus = binder<Inst, alloy::i2c::rx_dma<RxRoute>,
                      alloy::i2c::tx_dma<TxRoute>>::open();
    std::uint8_t buf[1] = {};
    const fake_chan ch{};
    ALLOY_CHECK(bus.read_dma(ch, 0x48, buf));
    // On a stream engine there is no chip-wide request id — CHSEL is a fact of
    // the matched triple — so the board's route is the only honest source.
    ALLOY_CHECK_EQ(fake_chan::last_request, 3u);

    // The binder carries both assignments, and the handle republishes them, so
    // portable code has a dependent name to claim.
    static_assert(std::is_same_v<binder<Inst, alloy::i2c::rx_dma<RxRoute>,
                                        alloy::i2c::tx_dma<TxRoute>>::rx_route,
                                 RxRoute>);
    static_assert(std::is_same_v<binder<Inst, alloy::i2c::rx_dma<RxRoute>,
                                        alloy::i2c::tx_dma<TxRoute>>::tx_route,
                                 TxRoute>);
    static_assert(std::is_same_v<decltype(bus)::rx_route, RxRoute>);
    static_assert(std::is_same_v<decltype(bus)::tx_route, TxRoute>);
    // A board that assigns nothing has void there, not a missing name.
    static_assert(std::is_same_v<binder<Inst>::rx_route, void>);
    static_assert(std::is_same_v<binder<Inst>::tx_route, void>);
    // Order in the tag pack does not matter (the emitter's append order is
    // not a contract).
    static_assert(std::is_same_v<binder<Inst, alloy::i2c::tx_dma<TxRoute>,
                                        alloy::i2c::rx_dma<RxRoute>>::rx_route,
                                 RxRoute>);
}

// ── the compile-time gates ────────────────────────────────────────────────

namespace {

template <class H, class Chan>
constexpr bool offers_read_dma = requires(H h, Chan c, std::span<std::uint8_t> d) {
    h.read_dma(c, std::uint8_t{}, d);
};

template <class H, class Chan>
constexpr bool offers_write_dma =
    requires(H h, Chan c, std::span<const std::uint8_t> d) {
        h.write_dma(c, std::uint8_t{}, d);
    };

template <class H, class Chan>
constexpr bool offers_write_read_dma =
    requires(H h, Chan c, std::span<const std::uint8_t> w, std::span<std::uint8_t> r) {
        h.write_read_dma(c, std::uint8_t{}, w, r);
    };

}  // namespace

ALLOY_TEST(i2c_dma_methods_gate_on_the_route_the_driver_and_the_channel) {
    struct tag {};
    using Free = free_router_inst<tag>;
    using Stream = stream_inst<tag>;
    struct ctrl {};
    using Route = alloy::dma::route<ctrl, 0, 3>;

    // A free-router chip carries the request id itself, so a board that
    // assigned no route still gets the methods — the escape hatch (claim a
    // channel by hand) keeps working.
    ALLOY_CHECK((offers_read_dma<alloy::i2c::handle<Free>, fake_chan>));
    ALLOY_CHECK((offers_write_dma<alloy::i2c::handle<Free>, fake_chan>));

    // A stream engine has no chip-wide id, so with NO route assigned the
    // methods are constrained away, naming the missing fact instead of failing
    // deep in a body. Assign the route and they appear.
    ALLOY_CHECK((!offers_read_dma<alloy::i2c::handle<Stream>, fake_chan>));
    ALLOY_CHECK((!offers_write_dma<alloy::i2c::handle<Stream>, fake_chan>));
    ALLOY_CHECK((offers_read_dma<alloy::i2c::handle<Stream, Route, Route>, fake_chan>));
    ALLOY_CHECK((offers_write_dma<alloy::i2c::handle<Stream, Route, Route>, fake_chan>));
    // Per DIRECTION, not per handle: a board may assign i2c.rx and not i2c.tx
    // (nucleo_g071rb does), and half a one-shot is a real thing.
    ALLOY_CHECK((offers_read_dma<alloy::i2c::handle<Stream, Route, void>, fake_chan>));
    ALLOY_CHECK((!offers_write_dma<alloy::i2c::handle<Stream, Route, void>, fake_chan>));

    // A driver with no DMA hooks is refused even with the route and the id —
    // the whole contract is the gate, not just the board's half.
    ALLOY_CHECK((!offers_read_dma<alloy::i2c::handle<hookless_inst>, fake_chan>));
    ALLOY_CHECK((!offers_write_dma<alloy::i2c::handle<hookless_inst>, fake_chan>));

    // And a channel token that cannot do a byte-wide peripheral->memory
    // transfer is refused for the READ while still serving the WRITE.
    ALLOY_CHECK((!offers_read_dma<alloy::i2c::handle<Free>, no_p2m_chan>));
    ALLOY_CHECK((offers_write_dma<alloy::i2c::handle<Free>, no_p2m_chan>));
    ALLOY_CHECK((!alloy::i2c::rx_channel<no_p2m_chan>));
    ALLOY_CHECK((alloy::i2c::rx_channel<fake_chan>));
    ALLOY_CHECK((alloy::i2c::tx_channel<fake_chan>));
}

ALLOY_TEST(i2c_repeated_start_under_dma_is_a_named_absence) {
    struct tag {};
    using Free = free_router_inst<tag>;
    // write_read() runs AUTOEND=0 + a repeated START and hands off on TC. Under
    // DMA that handoff has no reachable event (CR1.TCIE is uncurated), so the
    // DMA'd repeated start is DEFERRED — and the deferral is a deleted
    // declaration rather than a method nobody wrote, so the probe below is
    // false and `bus.write_read_dma(...)` is a compile error naming it.
    // docs/design/dma-streams.md says nothing about I2C repeated start; this is
    // a call made in i2c.hpp, not doctrine quoted from there.
    ALLOY_CHECK((!offers_write_read_dma<alloy::i2c::handle<Free>, fake_chan>));
    // The polled repeated-start path is untouched and still exists.
    ALLOY_CHECK((requires(alloy::i2c::handle<Free> h, std::span<const std::uint8_t> w,
                          std::span<std::uint8_t> r) { h.write_read(0u, w, r); }));
}

// ── the SHIPPED channel token, not only the double ────────────────────────

namespace {

// A recording DMA controller, so a REAL alloy::dma::channel can be claimed and
// handed to the facade — the type user code actually passes.
template <class Tag>
struct fake_ctrl {};

}  // namespace

namespace alloy::hal {
template <class Tag>
struct dma_impl<fake_ctrl<Tag>> {
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };
    static constexpr bool supports_circular = true;
    // Rings are gated on supports_ring, not on the circular bit
    // (alloy::dma::ring_capable); on an ST-shaped double they agree.
    static constexpr bool supports_ring = true;

    static inline dir last_dir = dir::periph_to_mem;
    static inline width last_msize = width::b32;
    static inline width last_psize = width::b32;
    static inline std::uintptr_t last_periph = 0;
    static inline std::uintptr_t last_mem = 0;
    static inline std::uint16_t last_items = 0;
    static inline std::uint8_t last_request = 0;
    static inline bool stopped = false;

    static void enable_controller() {}
    template <unsigned Ch>
    static void setup(dir d, bool, width msize, width psize, std::uintptr_t periph,
                      std::uintptr_t mem, std::uint16_t items, std::uint8_t request) {
        last_dir = d;
        last_msize = msize;
        last_psize = psize;
        last_periph = periph;
        last_mem = mem;
        last_items = items;
        last_request = request;
    }
    template <unsigned Ch>
    static void start() {}
    template <unsigned Ch>
    static void stop() { stopped = true; }
    template <unsigned Ch>
    static void clear_flags() {}
    template <unsigned Ch>
    static bool complete() { return true; }
    template <unsigned Ch>
    static bool error() { return false; }
};
}  // namespace alloy::hal

ALLOY_TEST(i2c_write_dma_runs_on_a_real_claimed_channel_token) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using Ctrl = fake_ctrl<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    using ctrl_f = alloy::hal::dma_impl<Ctrl>;
    drv::reset();

    auto bus = binder<Inst>::open();
    // The shipped spelling: claim ONCE from the board's route, reuse the token.
    // No claim/release churn per transfer, and no second ownership mechanism —
    // release-on-destruction belongs to streams (design §1), which own hardware
    // for a lifetime a one-shot does not have.
    auto tx = alloy::dma::claim(alloy::dma::route<Ctrl, 3, 99>{});
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 3, alloy::claim::personality::dma>()));

    static const std::uint8_t out[2] = {0x11, 0x22};
    ALLOY_CHECK(bus.write_dma(tx, 0x48, out));

    // Byte-wide, memory->peripheral, into TXDR, with the chip's request id.
    ALLOY_CHECK(ctrl_f::last_dir == ctrl_f::dir::mem_to_periph);
    ALLOY_CHECK(ctrl_f::last_msize == ctrl_f::width::b8);
    ALLOY_CHECK(ctrl_f::last_psize == ctrl_f::width::b8);
    ALLOY_CHECK_EQ(ctrl_f::last_periph, drv::txdr_addr());
    ALLOY_CHECK_EQ(ctrl_f::last_items, 2u);
    ALLOY_CHECK_EQ(ctrl_f::last_request, 11u);
    ALLOY_CHECK(ctrl_f::stopped);
    // The token still HOLDS the claim afterwards — a one-shot borrows the
    // channel, it does not take ownership of the claim (only rings release).
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 3, alloy::claim::personality::dma>()));
}

ALLOY_TEST(i2c_read_dma_runs_on_a_real_claimed_channel_token) {
    struct tag {};
    using Inst = free_router_inst<tag>;
    using Ctrl = fake_ctrl<tag>;
    using drv = alloy::hal::i2c_impl<Inst>;
    using ctrl_f = alloy::hal::dma_impl<Ctrl>;
    drv::reset();

    auto bus = binder<Inst>::open();
    auto rx = alloy::dma::claim(alloy::dma::route<Ctrl, 5, 99>{});
    std::uint8_t in[3] = {};
    ALLOY_CHECK(bus.read_dma(rx, 0x48, in));

    // BYTE-wide both sides. A 16-bit access to RXDR would read one byte and
    // fabricate the other; psize is not an engine default here, it is a
    // property of the register.
    ALLOY_CHECK(ctrl_f::last_dir == ctrl_f::dir::periph_to_mem);
    ALLOY_CHECK(ctrl_f::last_msize == ctrl_f::width::b8);
    ALLOY_CHECK(ctrl_f::last_psize == ctrl_f::width::b8);
    ALLOY_CHECK_EQ(ctrl_f::last_periph, drv::rxdr_addr());
    ALLOY_CHECK_EQ(ctrl_f::last_mem, reinterpret_cast<std::uintptr_t>(&in[0]));
    ALLOY_CHECK_EQ(ctrl_f::last_items, 3u);
    ALLOY_CHECK_EQ(ctrl_f::last_request, 10u);
    ALLOY_CHECK((alloy::claim::sub_held<Ctrl, 5, alloy::claim::personality::dma>()));
}
