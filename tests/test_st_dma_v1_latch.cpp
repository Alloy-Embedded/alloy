// The ISR->poller latch contract of the ST dma_v1 engine, witnessed on the
// host — the test phase 1 owed (docs/design/dma-streams.md §4: "the latch rule
// is now a driver contract", and st_dma_v1's own header note that the
// half-side latch was "the rule applied, not the rule proven").
//
// WHAT RUNS HERE IS THE REAL DRIVER. detail::st_dma_v1_engine (split out of
// st_dma_v1.hpp for exactly this file) is instantiated over a hand-written IP
// double whose registers are plain host memory, laid out with the dma_v1
// shape. The tests drive the engine's OWN ISRs and poll its OWN accessors; the
// only thing simulated is the hardware's side of the protocol — flags are
// raised by poking ISR, and the write-1-to-clear behavior of IFCR is committed
// by an explicit helper, so the test controls the exact instant a flag
// disappears.
//
// THE WITNESS, concretely: the half ISR must clear HTIF (level-triggered line)
// and hand the fact to callback<Ch>::half_latched, because a poller calling
// half<Ch>() AFTER the ISR consumed the flag has no other source of truth.
// Revert either half-latch line in st_dma_v1_body.hpp — the `half_latched =
// true` in half_isr, or the `half_latched ||` term in half() — and
// `st_dma_v1_half_survives_its_own_isr_for_a_late_poller` fails. (Verified by
// doing exactly that revert locally while writing this file.)
//
// What this file can NOT witness: real HTIF/TCIF timing out of silicon or a
// model — that stays the emulation legs' claim (adc_stream.robot and the
// phase-2 UART leg).

#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/dma/st_dma_v1_body.hpp"
#include "alloy_test.hpp"

namespace {

// The dma_v1 register shape, hand-written (tests never read generated
// headers): same offsets, strides and bit positions as registers/st/dma_v1.yaml
// so the sequences exercised here are the sequences the silicon sees. ISR is
// declared writable — the test IS the hardware here, and raising a flag is a
// write from the hardware's side.
template <class Tag>
struct fake_dma_ip {
    struct regs {
        alloy::rw32 ISR;
        alloy::rw32 IFCR;
    };

    static constexpr std::uintptr_t CCR_offset = 0x08;
    static constexpr unsigned CCR_stride = 20u;
    static constexpr std::uintptr_t CNDTR_offset = 0x0C;
    static constexpr unsigned CNDTR_stride = 20u;
    static constexpr std::uintptr_t CPAR_offset = 0x10;
    static constexpr unsigned CPAR_stride = 20u;
    static constexpr std::uintptr_t CMAR_offset = 0x14;
    static constexpr unsigned CMAR_stride = 20u;

    template <unsigned I>
    static constexpr auto gif = alloy::field<&regs::ISR, 0u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto tcif = alloy::field<&regs::ISR, 1u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto htif = alloy::field<&regs::ISR, 2u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto teif = alloy::field<&regs::ISR, 3u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto cgif = alloy::field<&regs::IFCR, 0u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto ctcif = alloy::field<&regs::IFCR, 1u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto chtif = alloy::field<&regs::IFCR, 2u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto cteif = alloy::field<&regs::IFCR, 3u + I * 4u, 1>;

    static constexpr alloy::raw_field en{0u, 1u};
    static constexpr alloy::raw_field tcie{1u, 1u};
    static constexpr alloy::raw_field htie{2u, 1u};
    static constexpr alloy::raw_field teie{3u, 1u};
    static constexpr alloy::raw_field dir{4u, 1u};
    static constexpr alloy::raw_field circ{5u, 1u};
    static constexpr alloy::raw_field pinc{6u, 1u};
    static constexpr alloy::raw_field minc{7u, 1u};
    static constexpr alloy::raw_field psize{8u, 2u};
    static constexpr alloy::raw_field msize{10u, 2u};
    static constexpr alloy::raw_field pl{12u, 2u};
};

template <class Tag>
struct fake_mux {
    struct ip {
        static constexpr std::uintptr_t CCR_offset = 0x00;
        static constexpr unsigned CCR_stride = 4u;
        static constexpr alloy::raw_field dmareq_id{0u, 7u};
    };
    static inline std::uint32_t mem[16]{};
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
};

// One fake instance per test tag: the engine's callback<Ch> statics are keyed
// by Inst, so a fresh tag is a fresh latch (same isolation idiom as
// test_dma_ring.cpp's fake_ctrl).
template <class Tag>
struct fake_dma_inst {
    using ip = fake_dma_ip<Tag>;
    using mux_t = fake_mux<Tag>;
    // 64 words cover ISR/IFCR + 7 channels of CCR/CNDTR/CPAR/CMAR at stride 20.
    static inline std::uint32_t mem[64]{};
    static inline std::uint32_t rcc_reg = 0;
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
    static inline const alloy::clock_gate gate{
        reinterpret_cast<std::uintptr_t>(&rcc_reg), 0x1u};
    static constexpr unsigned ch_count = 7;
    static constexpr unsigned ch_mux_offset = 0;
    static constexpr std::uint16_t ch_irqline1 = 5;
    static constexpr std::uint16_t ch_irqline2_3 = 6;
    static constexpr std::uint16_t ch_irqline4_7 = 7;
};

template <class Tag>
using engine = alloy::hal::detail::st_dma_v1_engine<fake_dma_inst<Tag>>;

// The hardware's half of IFCR: write-1-to-clear, committed on demand so a test
// chooses the exact instant the flag disappears (on silicon it is the same
// store; here the ISR's IFCR write is first ASSERTED, then committed).
template <class Tag>
void commit_w1c() {
    auto& r = engine<Tag>::r();
    r.ISR = r.ISR & ~r.IFCR;
    r.IFCR = 0u;
}

int g_half_calls = 0;
void count_half(void*) { ++g_half_calls; }
int g_full_calls = 0;
void count_full(void*) { ++g_full_calls; }

}  // namespace

// ── THE OWED WITNESS: ISR-then-poll on the HALF flag ──────────────────────

ALLOY_TEST(st_dma_v1_half_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_ip<tag>;
    g_half_calls = 0;

    // The polled consumer's real sequence: events registered BEFORE setup (the
    // CCR fold rule), circular p2m, start.
    eng::enable_half_irq<1>(&count_half, nullptr);
    eng::setup<1>(eng::dir::periph_to_mem, /*circular=*/true, eng::width::b8,
                  eng::width::b8, 0x40u, 0x80u, 8u, 3u);
    eng::start<1>();
    // Register-level witness of the fold: HTIE went in with the whole-CCR
    // write, EN with start().
    ALLOY_CHECK((eng::ccr<1>() & IP::htie.mask()) != 0u);
    ALLOY_CHECK((eng::ccr<1>() & IP::en.mask()) != 0u);
    ALLOY_CHECK(!eng::half<1>());  // negative control: nothing latched yet

    // Hardware: the first half fills — HTIF rises, the ISR runs.
    eng::r().ISR = eng::r().ISR | IP::htif<0>.mask;
    eng::half_isr<1>(nullptr);
    ALLOY_CHECK_EQ(g_half_calls, 1);
    // The ISR consumed the flag: it wrote CHTIF (it must — level-triggered
    // line)...
    ALLOY_CHECK((eng::r().IFCR & IP::chtif<0>.mask) != 0u);
    commit_w1c<tag>();
    // ...and the hardware flag is really gone:
    ALLOY_CHECK((eng::r().ISR & IP::htif<0>.mask) == 0u);

    // THE WITNESS. A poller arriving after the ISR has no hardware flag left
    // to read — half<1>() must report from the latch. Revert either half-latch
    // line in st_dma_v1_body.hpp and this check fails.
    ALLOY_CHECK(eng::half<1>());

    eng::disable_half_irq<1>();  // unhook from the shared host irq chain
}

// The completion twin — the latch that closed the original bug class, now
// pinned by the same host witness instead of only the emulation leg.
ALLOY_TEST(st_dma_v1_completion_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_ip<tag>;
    g_full_calls = 0;

    eng::enable_complete_irq<1>(&count_full, nullptr);
    eng::setup<1>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    eng::start<1>();
    ALLOY_CHECK((eng::ccr<1>() & IP::tcie.mask()) != 0u);
    ALLOY_CHECK((eng::ccr<1>() & IP::teie.mask()) != 0u);
    ALLOY_CHECK(!eng::complete<1>());

    eng::r().ISR = eng::r().ISR | IP::tcif<0>.mask;
    eng::complete_isr<1>(nullptr);
    ALLOY_CHECK_EQ(g_full_calls, 1);
    ALLOY_CHECK((eng::r().IFCR & IP::ctcif<0>.mask) != 0u);
    commit_w1c<tag>();
    ALLOY_CHECK((eng::r().ISR & IP::tcif<0>.mask) == 0u);

    ALLOY_CHECK(eng::complete<1>());  // the latch, or nothing

    eng::disable_complete_irq<1>();
}

// ── the latch's lifecycle and the ISRs' precision ─────────────────────────

ALLOY_TEST(st_dma_v1_setup_resets_stale_latches) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_ip<tag>;

    eng::enable_half_irq<2>(&count_half, nullptr);
    eng::enable_complete_irq<2>(&count_full, nullptr);
    eng::setup<2>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();
    // A full lap of the previous transfer latched both events... (each ISR's
    // IFCR store commits immediately — on hardware a W1C write takes effect at
    // the store, and the driver rightly OVERWRITES IFCR rather than RMW-ing a
    // write-only register, so a deferred commit would lose the first clear)
    eng::r().ISR = eng::r().ISR | IP::htif<1>.mask | IP::tcif<1>.mask;
    eng::half_isr<2>(nullptr);
    commit_w1c<tag>();
    eng::complete_isr<2>(nullptr);
    commit_w1c<tag>();
    ALLOY_CHECK(eng::half<2>());
    ALLOY_CHECK(eng::complete<2>());

    // ...and programming a NEW transfer must forget them: a fresh transfer is
    // not already half-done, and a poll must not report the old one's events.
    eng::setup<2>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();
    ALLOY_CHECK(!eng::half<2>());
    ALLOY_CHECK(!eng::complete<2>());

    eng::disable_half_irq<2>();
    eng::disable_complete_irq<2>();
}

ALLOY_TEST(st_dma_v1_isrs_are_no_ops_for_foreign_interrupts) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_ip<tag>;
    g_half_calls = 0;
    g_full_calls = 0;

    eng::enable_half_irq<1>(&count_half, nullptr);
    eng::enable_complete_irq<1>(&count_full, nullptr);
    eng::setup<1>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();  // absorb setup's own clear_flags store

    // A SIBLING channel's flag is up; channel 1's are all clear. Both handlers
    // run (shared NVIC line) and must touch NOTHING: no IFCR write — a clear
    // here would eat the sibling's event — no callback, no latch.
    eng::r().ISR = eng::r().ISR | IP::htif<2>.mask | IP::tcif<2>.mask;
    eng::half_isr<1>(nullptr);
    eng::complete_isr<1>(nullptr);
    ALLOY_CHECK_EQ(eng::r().IFCR, 0u);
    ALLOY_CHECK_EQ(g_half_calls, 0);
    ALLOY_CHECK_EQ(g_full_calls, 0);
    ALLOY_CHECK(!eng::half<1>());
    ALLOY_CHECK(!eng::complete<1>());
    eng::r().ISR = 0u;

    eng::disable_half_irq<1>();
    eng::disable_complete_irq<1>();
}

ALLOY_TEST(st_dma_v1_complete_isr_clears_only_what_it_consumed) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_ip<tag>;
    g_full_calls = 0;

    eng::enable_complete_irq<1>(&count_full, nullptr);
    eng::setup<1>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);

    // The wrap instant on a ring: HTIF and TCIF up together (Renode raises
    // them together; silicon can interleave them arbitrarily close). The
    // completion handler runs first (attach order) and must clear ONLY the
    // flags it consumed — a blanket per-channel clear here would eat the half
    // event before its own handler saw it.
    eng::r().ISR = eng::r().ISR | IP::htif<0>.mask | IP::tcif<0>.mask;
    eng::complete_isr<1>(nullptr);
    ALLOY_CHECK_EQ(eng::r().IFCR, IP::ctcif<0>.mask);
    commit_w1c<tag>();
    // The half event is still there for its own consumer — as the live flag:
    ALLOY_CHECK((eng::r().ISR & IP::htif<0>.mask) != 0u);
    ALLOY_CHECK(eng::half<1>());
    eng::r().ISR = 0u;

    eng::disable_complete_irq<1>();
}

ALLOY_TEST(st_dma_v1_setup_without_callbacks_folds_no_interrupt_enables) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_ip<tag>;

    // The polled-only consumer: no callbacks registered, so the whole-CCR
    // write must leave every interrupt enable clear — a spurious TCIE with no
    // handler attached would wedge the line.
    eng::setup<3>(eng::dir::periph_to_mem, true, eng::width::b16, eng::width::b16,
                  0x40u, 0x80u, 8u, 3u);
    ALLOY_CHECK((eng::ccr<3>() &
                 (IP::tcie.mask() | IP::teie.mask() | IP::htie.mask())) == 0u);
    // And the request id landed in the MUX row for channel 3 (row Ch-1):
    ALLOY_CHECK_EQ(fake_mux<tag>::mem[2] & 0x7Fu, 3u);
}
