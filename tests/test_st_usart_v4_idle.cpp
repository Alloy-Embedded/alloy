// The IDLE frame-gap contract of the ST usart_v4/lpuart_v4 shared body,
// witnessed on the host — the test phase 2 owed (its changelog admitted the
// IDLE ISR "was executed by nothing": gutting enable_idle_irq/idle_isr left
// every suite green). Same shape as test_st_dma_v1_latch.cpp, deliberately:
// the REAL driver body over a hand-written IP double.
//
// WHAT RUNS HERE IS THE REAL DRIVER. detail::st_usart_v4_body was already
// host-reachable — split out for the LPUART sharing, it reaches every register
// through `typename Inst::ip` and never includes a generated header — so no
// code motion was needed for this file to exist; that reachability claim is
// PROVEN by this file compiling against the body unchanged. The double lays
// its registers out with usart_v4's shape (registers/st/usart_v4.yaml bit
// positions: ISR.IDLE=4, ICR.IDLECF=4, CR1.IDLEIE=4, ISR.ORE=3, ICR.ORECF=3,
// CR3.DMAR=6). The only thing simulated is the hardware's side: flags are
// raised by poking ISR, and ICR's write-1-to-clear is committed by an explicit
// helper, so a test controls the exact instant a flag disappears.
//
// THE WITNESS, concretely: IDLE is a level-triggered shared-vector event —
// the ISR must return untouched when the flag is down (a sibling UART's
// interrupt), and when it is up it must CLEAR it via ICR (or the line
// re-fires forever) and invoke the registered wake. enable must clear a
// STALE IDLE before arming IDLEIE — the flag sets whenever the line has been
// quiet since the last byte, so arming mid-silence without the clear reports
// a gap nobody waited through. Gut either body — enable_idle_irq or idle_isr
// to a no-op, exactly the phase-2 reviewer's experiment — and this file goes
// red (verified by doing that gut locally while writing it; the suite before
// this file stayed green under the same gut).
//
// What this file can NOT witness: a real IDLE firing one character time after
// a real frame on a real USART — that stays the modbus_rtu emulation leg's
// claim. Nor can a memory double order the stale-clear against the IDLEIE
// arm within one call: what it pins is that the clear exists at all.

#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/uart/st_usart_v4_body.hpp"
#include "alloy_test.hpp"

namespace {

// The usart_v4 register shape, hand-written (tests never read generated
// headers): the members the RX-DMA/IDLE surface touches, at the yaml's
// offsets and bit positions. ISR is declared writable — the test IS the
// hardware here, and raising a flag is a write from the hardware's side.
template <class Tag>
struct fake_usart_ip {
    struct regs {
        alloy::rw32 CR1;
        alloy::rw32 CR2;
        alloy::rw32 CR3;
        alloy::rw32 BRR;
        alloy::rw32 GTPR;
        alloy::rw32 RTOR;
        alloy::rw32 RQR;
        alloy::rw32 ISR;
        alloy::rw32 ICR;
        alloy::rw32 RDR;
        alloy::rw32 TDR;
        alloy::rw32 PRESC;
    };

    static constexpr auto idleie = alloy::field<&regs::CR1, 4u>;
    static constexpr auto rxneie = alloy::field<&regs::CR1, 5u>;
    static constexpr auto dmar = alloy::field<&regs::CR3, 6u>;
    static constexpr auto ore = alloy::field<&regs::ISR, 3u>;
    static constexpr auto idle = alloy::field<&regs::ISR, 4u>;
    static constexpr auto rxne = alloy::field<&regs::ISR, 5u>;
    static constexpr auto orecf = alloy::field<&regs::ICR, 3u>;
    static constexpr auto idlecf = alloy::field<&regs::ICR, 4u>;
};

// One fake instance per test tag: the body's idle_fn/idle_armed statics are
// keyed by Inst, so a fresh tag is a fresh registration state (the
// test_st_dma_v1_latch.cpp isolation idiom). Line 4 is inside the 8-slot
// host head table test_irq.cpp defines for the whole binary.
template <class Tag>
struct fake_usart_inst {
    using ip = fake_usart_ip<Tag>;
    static inline typename ip::regs mem{};
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem);
    static constexpr alloy::irq_line irq{4};
};

// The CRTP hook owner. Empty on purpose: none of the vendor hooks
// (admit_rate/baud_div/program_vendor/isr_vendor) is reached by the IDLE or
// RX-DMA surface, and an unused template member never instantiates.
struct no_vendor {};

template <class Tag>
using engine =
    alloy::hal::detail::st_usart_v4_body<fake_usart_inst<Tag>, no_vendor>;

// The hardware's half of ICR: write-1-to-clear, committed on demand so a test
// chooses the exact instant a flag disappears.
template <class Tag>
void commit_w1c() {
    auto& r = engine<Tag>::r();
    r.ISR = r.ISR & ~r.ICR;
    r.ICR = 0u;
}

int g_gaps = 0;
void* g_gap_ctx = nullptr;
void count_gap(void* ctx) {
    ++g_gaps;
    g_gap_ctx = ctx;
}

}  // namespace

// ── the arming half: stale clear, then IDLEIE ─────────────────────────────

ALLOY_TEST(st_usart_v4_idle_enable_clears_the_stale_flag_it_would_report) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_usart_ip<tag>;
    auto& r = eng::r();

    // The line has been quiet for a while: IDLE is already up when rx_ring()
    // arms the wake. Without the ICR clear, IDLEIE would fire immediately —
    // a frame-gap report with no frame.
    r.ISR = IP::idle.mask;
    eng::enable_idle_irq(nullptr, nullptr);
    ALLOY_CHECK((r.ICR & IP::idlecf.mask) != 0u);  // the stale clear was issued
    ALLOY_CHECK_EQ(IP::idleie.read(r), 1u);        // and the event is armed
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(IP::idle.read(r), 0u);

    eng::disable_idle_irq();
    ALLOY_CHECK_EQ(IP::idleie.read(r), 0u);
}

// ── THE OWED WITNESS: the ISR consumes the flag and delivers the wake ─────

ALLOY_TEST(st_usart_v4_idle_isr_clears_the_flag_and_wakes_the_listener) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_usart_ip<tag>;
    auto& r = eng::r();
    g_gaps = 0;
    g_gap_ctx = nullptr;
    static int ctx_obj = 0;

    eng::enable_idle_irq(&count_gap, &ctx_obj);
    commit_w1c<tag>();  // absorb the arm's own stale clear

    // Shared-vector discipline first: a sibling's interrupt runs this ISR
    // with OUR flag down — it must touch nothing. No ICR write (a clear here
    // could eat a flag the sibling owns), no callback.
    eng::idle_isr(nullptr);
    ALLOY_CHECK_EQ(g_gaps, 0);
    ALLOY_CHECK_EQ(r.ICR, 0u);

    // A frame ends: the line goes quiet for one character time, IDLE rises,
    // the ISR runs. It must clear the level-triggered flag via ICR — or the
    // vector re-enters forever — and hand the gap to the listener.
    r.ISR = r.ISR | IP::idle.mask;
    eng::idle_isr(nullptr);
    ALLOY_CHECK_EQ(g_gaps, 1);
    ALLOY_CHECK(g_gap_ctx == &ctx_obj);
    ALLOY_CHECK((r.ICR & IP::idlecf.mask) != 0u);
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(IP::idle.read(r), 0u);

    // The next gap is a fresh event, not a wedged repeat of the first.
    r.ISR = r.ISR | IP::idle.mask;
    eng::idle_isr(nullptr);
    ALLOY_CHECK_EQ(g_gaps, 2);
    commit_w1c<tag>();

    eng::disable_idle_irq();
}

// rx_ring() arms with fn == nullptr — the interrupt firing IS the wake
// (sleep_until_event returns). The ISR must still consume the flag: a
// wake-only registration that left IDLE set would pin the CPU in the vector.
ALLOY_TEST(st_usart_v4_idle_wake_only_registration_still_consumes_the_flag) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_usart_ip<tag>;
    auto& r = eng::r();

    eng::enable_idle_irq(nullptr, nullptr);
    commit_w1c<tag>();
    r.ISR = r.ISR | IP::idle.mask;
    eng::idle_isr(nullptr);  // nobody to call — and no crash calling them
    ALLOY_CHECK((r.ICR & IP::idlecf.mask) != 0u);
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(IP::idle.read(r), 0u);

    eng::disable_idle_irq();
}

// ── re-registration is a listener swap, not a re-arm ──────────────────────

ALLOY_TEST(st_usart_v4_idle_reregistration_swaps_the_listener_without_rearming) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_usart_ip<tag>;
    auto& r = eng::r();
    g_gaps = 0;

    // rx_ring()'s arm (wake-only)...
    eng::enable_idle_irq(nullptr, nullptr);
    ALLOY_CHECK_EQ(IP::idleie.read(r), 1u);
    commit_w1c<tag>();

    // ...then on_idle() chooses who is told. The facade re-calls
    // enable_idle_irq for this ("registering here only chooses who is told"),
    // so a second call while armed must be a listener swap: no second
    // irq::attach of the same ISR (the alloy::irq duplicate trap — this very
    // line died by SIGILL against the pre-guard body), and no ICR clear (the
    // armed ISR owns the flag now; a swap must not eat a pending gap).
    eng::enable_idle_irq(&count_gap, nullptr);
    ALLOY_CHECK_EQ(IP::idleie.read(r), 1u);
    ALLOY_CHECK_EQ(r.ICR, 0u);

    // The swapped-in listener is the one delivered to:
    r.ISR = r.ISR | IP::idle.mask;
    eng::idle_isr(nullptr);
    ALLOY_CHECK_EQ(g_gaps, 1);
    commit_w1c<tag>();

    // disable resets the registration state: a fresh arm re-attaches and
    // re-arms rather than believing it is still armed.
    eng::disable_idle_irq();
    ALLOY_CHECK_EQ(IP::idleie.read(r), 0u);
    eng::enable_idle_irq(nullptr, nullptr);
    ALLOY_CHECK_EQ(IP::idleie.read(r), 1u);
    eng::disable_idle_irq();
}

// ── the RX-DMA request stream: arm clears the wedge, §4 teardown order ────

ALLOY_TEST(st_usart_v4_rx_dma_begin_unwedges_ore_and_end_stops_the_requests) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_usart_ip<tag>;
    auto& r = eng::r();

    // A set ORE freezes reception on this IP: entering DMA mode with it set
    // would stall the request stream before the first byte moved.
    r.ISR = IP::ore.mask;
    eng::dma_rx_begin();
    ALLOY_CHECK((r.ICR & IP::orecf.mask) != 0u);  // the wedge clear was issued
    ALLOY_CHECK_EQ(IP::dmar.read(r), 1u);         // request stream on
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(IP::ore.read(r), 0u);

    // The wake arms last (rx_stream's constructor order: channel — outside
    // this driver — then DMAR, then IDLE).
    eng::enable_idle_irq(nullptr, nullptr);
    ALLOY_CHECK_EQ(IP::idleie.read(r), 1u);
    commit_w1c<tag>();

    // §4 teardown, the facade's destructor order against the REAL registers:
    // wake off first, request stream second — and DMAR must still be up
    // between the two, because the ring's channel (which stops third) may
    // still be draining a byte the peripheral already requested.
    eng::disable_idle_irq();
    ALLOY_CHECK_EQ(IP::idleie.read(r), 0u);
    ALLOY_CHECK_EQ(IP::dmar.read(r), 1u);
    eng::dma_rx_end();
    ALLOY_CHECK_EQ(IP::dmar.read(r), 0u);
}
