// The ISR->poller latch contract of the ST dma_v2 STREAM engine, witnessed on
// the host — the same witness shape as test_st_dma_v1_latch.cpp, shipped WITH
// the driver instead of owed after it (the phase-1 debt, not re-incurred).
//
// WHAT RUNS HERE IS THE REAL DRIVER. detail::st_dma_v2_engine (split out of
// st_dma_v2.hpp for exactly this file) is instantiated over a hand-written IP
// double whose registers are plain host memory, laid out with the dma_v2
// shape: the SPLIT flag pair (LISR streams 0-3 / HISR 4-7) with the irregular
// per-stream slot packing (base bits 0/6/16/22), the 0x18-stride stream
// cluster, CHSEL in SxCR. The tests drive the engine's OWN ISRs and poll its
// OWN accessors; the only thing simulated is the hardware's side of the
// protocol — flags are raised by poking LISR/HISR, and the write-1-to-clear
// behavior of LIFCR/HIFCR is committed by an explicit helper.
//
// THE WITNESS, concretely: the half ISR must clear HTIF (level-triggered
// line) and hand the fact to callback<S>::half_latched, because a poller
// calling half<S>() AFTER the ISR consumed the flag has no other source of
// truth. Revert either half-latch line in st_dma_v2_body.hpp — the
// `half_latched = true` in half_isr, or the `half_latched ||` term in
// half() — and st_dma_v2_half_survives_its_own_isr_for_a_late_poller fails;
// same for the completion pair. (Verified by doing exactly those reverts
// locally while writing this file — all four, one at a time.)
//
// THE ERROR LATCH gets the same treatment, and one step further: because a
// missing error latch does not merely lose a fact, it INVERTS an answer, the
// consequence is witnessed through the SHIPPED caller rather than only through
// the engine accessor. `st_dma_v2_wait_reports_a_failure_its_own_isr_consumed`
// runs the real `alloy::dma::channel::wait()` (this file specializes
// alloy::hal::dma_impl onto the engine, the test_dma_ring.cpp idiom) over a
// stream that failed and whose interrupt ran, and requires FALSE. Before the
// error latch that call returned true — done() from the completion latch,
// error() false because complete_isr had already eaten TEIF/DMEIF.
//
// V2-SPECIFIC EDGES pinned here, beyond the v1 shape:
//  - the half witness runs on stream 5 and completion on stream 6 — HISR/
//    HIFCR territory, so a driver that only ever consulted the LOW register
//    pair fails loudly;
//  - EN AUTO-CLEAR: in non-circular mode hardware clears EN itself at
//    transfer end (the inverse of dma_v1, where completion leaves EN=1) —
//    completion must still be reported with EN already low, and a follow-up
//    setup()+start() must work (stop()'s EN poll falls through, stale
//    latches reset, EN rises again);
//  - CHSEL carries the route's request id, and the five-flag clear that
//    precedes EN (setting EN is IGNORED by hardware while any of the
//    stream's flags is set) names all five clear bits;
//  - TWO FAILURE MODES behind one error(): TEIF and DMEIF. complete_isr's
//    clear is correspondingly wider than v1's, so the engine hides one more
//    mode without the latch — a DIRECT-MODE error (DMEIF alone, no TEIF) is
//    pinned by its own case here.
//
// What this file can NOT witness: EN refusing to fall while a FIFO drains
// (the memory double clears it at the store), real flag timing out of
// silicon, or the F7 Renode model's behavior — those stay the emulation
// legs' claim (the phase-3 model-probe day).

#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/dma/st_dma_v2_body.hpp"
#include "alloy_test.hpp"

namespace {

// The dma_v2 register shape, hand-written (tests never read generated
// headers): same offsets, strides and bit positions as
// registers/st/dma_v2.yaml. LISR/HISR are declared writable — the test IS the
// hardware here, and raising a flag is a write from the hardware's side.
template <class Tag>
struct fake_dma_v2_ip {
    struct regs {
        alloy::rw32 LISR;
        alloy::rw32 HISR;
        alloy::rw32 LIFCR;
        alloy::rw32 HIFCR;
    };

    static constexpr std::uintptr_t SCR_offset = 0x10;
    static constexpr unsigned SCR_stride = 24u;
    static constexpr std::uintptr_t SNDTR_offset = 0x14;
    static constexpr unsigned SNDTR_stride = 24u;
    static constexpr std::uintptr_t SPAR_offset = 0x18;
    static constexpr unsigned SPAR_stride = 24u;
    static constexpr std::uintptr_t SM0AR_offset = 0x1C;
    static constexpr unsigned SM0AR_stride = 24u;
    static constexpr std::uintptr_t SFCR_offset = 0x24;
    static constexpr unsigned SFCR_stride = 24u;

    // The irregular slot packing, restated from the yaml: stream slots at base
    // bits 0/6/16/22 of the L (streams 0-3) or H (4-7) register, flag order
    // within a slot FEIF+0 / DMEIF+2 / TEIF+3 / HTIF+4 / TCIF+5.
    static constexpr auto feif0 = alloy::field<&regs::LISR, 0u, 1>;
    static constexpr auto dmeif0 = alloy::field<&regs::LISR, 2u, 1>;
    static constexpr auto teif0 = alloy::field<&regs::LISR, 3u, 1>;
    static constexpr auto htif0 = alloy::field<&regs::LISR, 4u, 1>;
    static constexpr auto tcif0 = alloy::field<&regs::LISR, 5u, 1>;
    static constexpr auto feif1 = alloy::field<&regs::LISR, 6u, 1>;
    static constexpr auto dmeif1 = alloy::field<&regs::LISR, 8u, 1>;
    static constexpr auto teif1 = alloy::field<&regs::LISR, 9u, 1>;
    static constexpr auto htif1 = alloy::field<&regs::LISR, 10u, 1>;
    static constexpr auto tcif1 = alloy::field<&regs::LISR, 11u, 1>;
    static constexpr auto feif2 = alloy::field<&regs::LISR, 16u, 1>;
    static constexpr auto dmeif2 = alloy::field<&regs::LISR, 18u, 1>;
    static constexpr auto teif2 = alloy::field<&regs::LISR, 19u, 1>;
    static constexpr auto htif2 = alloy::field<&regs::LISR, 20u, 1>;
    static constexpr auto tcif2 = alloy::field<&regs::LISR, 21u, 1>;
    static constexpr auto feif3 = alloy::field<&regs::LISR, 22u, 1>;
    static constexpr auto dmeif3 = alloy::field<&regs::LISR, 24u, 1>;
    static constexpr auto teif3 = alloy::field<&regs::LISR, 25u, 1>;
    static constexpr auto htif3 = alloy::field<&regs::LISR, 26u, 1>;
    static constexpr auto tcif3 = alloy::field<&regs::LISR, 27u, 1>;
    static constexpr auto feif4 = alloy::field<&regs::HISR, 0u, 1>;
    static constexpr auto dmeif4 = alloy::field<&regs::HISR, 2u, 1>;
    static constexpr auto teif4 = alloy::field<&regs::HISR, 3u, 1>;
    static constexpr auto htif4 = alloy::field<&regs::HISR, 4u, 1>;
    static constexpr auto tcif4 = alloy::field<&regs::HISR, 5u, 1>;
    static constexpr auto feif5 = alloy::field<&regs::HISR, 6u, 1>;
    static constexpr auto dmeif5 = alloy::field<&regs::HISR, 8u, 1>;
    static constexpr auto teif5 = alloy::field<&regs::HISR, 9u, 1>;
    static constexpr auto htif5 = alloy::field<&regs::HISR, 10u, 1>;
    static constexpr auto tcif5 = alloy::field<&regs::HISR, 11u, 1>;
    static constexpr auto feif6 = alloy::field<&regs::HISR, 16u, 1>;
    static constexpr auto dmeif6 = alloy::field<&regs::HISR, 18u, 1>;
    static constexpr auto teif6 = alloy::field<&regs::HISR, 19u, 1>;
    static constexpr auto htif6 = alloy::field<&regs::HISR, 20u, 1>;
    static constexpr auto tcif6 = alloy::field<&regs::HISR, 21u, 1>;
    static constexpr auto feif7 = alloy::field<&regs::HISR, 22u, 1>;
    static constexpr auto dmeif7 = alloy::field<&regs::HISR, 24u, 1>;
    static constexpr auto teif7 = alloy::field<&regs::HISR, 25u, 1>;
    static constexpr auto htif7 = alloy::field<&regs::HISR, 26u, 1>;
    static constexpr auto tcif7 = alloy::field<&regs::HISR, 27u, 1>;

    static constexpr auto cfeif0 = alloy::field<&regs::LIFCR, 0u, 1>;
    static constexpr auto cdmeif0 = alloy::field<&regs::LIFCR, 2u, 1>;
    static constexpr auto cteif0 = alloy::field<&regs::LIFCR, 3u, 1>;
    static constexpr auto chtif0 = alloy::field<&regs::LIFCR, 4u, 1>;
    static constexpr auto ctcif0 = alloy::field<&regs::LIFCR, 5u, 1>;
    static constexpr auto cfeif1 = alloy::field<&regs::LIFCR, 6u, 1>;
    static constexpr auto cdmeif1 = alloy::field<&regs::LIFCR, 8u, 1>;
    static constexpr auto cteif1 = alloy::field<&regs::LIFCR, 9u, 1>;
    static constexpr auto chtif1 = alloy::field<&regs::LIFCR, 10u, 1>;
    static constexpr auto ctcif1 = alloy::field<&regs::LIFCR, 11u, 1>;
    static constexpr auto cfeif2 = alloy::field<&regs::LIFCR, 16u, 1>;
    static constexpr auto cdmeif2 = alloy::field<&regs::LIFCR, 18u, 1>;
    static constexpr auto cteif2 = alloy::field<&regs::LIFCR, 19u, 1>;
    static constexpr auto chtif2 = alloy::field<&regs::LIFCR, 20u, 1>;
    static constexpr auto ctcif2 = alloy::field<&regs::LIFCR, 21u, 1>;
    static constexpr auto cfeif3 = alloy::field<&regs::LIFCR, 22u, 1>;
    static constexpr auto cdmeif3 = alloy::field<&regs::LIFCR, 24u, 1>;
    static constexpr auto cteif3 = alloy::field<&regs::LIFCR, 25u, 1>;
    static constexpr auto chtif3 = alloy::field<&regs::LIFCR, 26u, 1>;
    static constexpr auto ctcif3 = alloy::field<&regs::LIFCR, 27u, 1>;
    static constexpr auto cfeif4 = alloy::field<&regs::HIFCR, 0u, 1>;
    static constexpr auto cdmeif4 = alloy::field<&regs::HIFCR, 2u, 1>;
    static constexpr auto cteif4 = alloy::field<&regs::HIFCR, 3u, 1>;
    static constexpr auto chtif4 = alloy::field<&regs::HIFCR, 4u, 1>;
    static constexpr auto ctcif4 = alloy::field<&regs::HIFCR, 5u, 1>;
    static constexpr auto cfeif5 = alloy::field<&regs::HIFCR, 6u, 1>;
    static constexpr auto cdmeif5 = alloy::field<&regs::HIFCR, 8u, 1>;
    static constexpr auto cteif5 = alloy::field<&regs::HIFCR, 9u, 1>;
    static constexpr auto chtif5 = alloy::field<&regs::HIFCR, 10u, 1>;
    static constexpr auto ctcif5 = alloy::field<&regs::HIFCR, 11u, 1>;
    static constexpr auto cfeif6 = alloy::field<&regs::HIFCR, 16u, 1>;
    static constexpr auto cdmeif6 = alloy::field<&regs::HIFCR, 18u, 1>;
    static constexpr auto cteif6 = alloy::field<&regs::HIFCR, 19u, 1>;
    static constexpr auto chtif6 = alloy::field<&regs::HIFCR, 20u, 1>;
    static constexpr auto ctcif6 = alloy::field<&regs::HIFCR, 21u, 1>;
    static constexpr auto cfeif7 = alloy::field<&regs::HIFCR, 22u, 1>;
    static constexpr auto cdmeif7 = alloy::field<&regs::HIFCR, 24u, 1>;
    static constexpr auto cteif7 = alloy::field<&regs::HIFCR, 25u, 1>;
    static constexpr auto chtif7 = alloy::field<&regs::HIFCR, 26u, 1>;
    static constexpr auto ctcif7 = alloy::field<&regs::HIFCR, 27u, 1>;

    // SxCR fields the engine names (registers/st/dma_v2.yaml SCR).
    static constexpr alloy::raw_field en{0u, 1u};
    static constexpr alloy::raw_field dmeie{1u, 1u};
    static constexpr alloy::raw_field teie{2u, 1u};
    static constexpr alloy::raw_field htie{3u, 1u};
    static constexpr alloy::raw_field tcie{4u, 1u};
    static constexpr alloy::raw_field dir{6u, 2u};
    static constexpr alloy::raw_field circ{8u, 1u};
    static constexpr alloy::raw_field pinc{9u, 1u};
    static constexpr alloy::raw_field minc{10u, 1u};
    static constexpr alloy::raw_field psize{11u, 2u};
    static constexpr alloy::raw_field msize{13u, 2u};
    static constexpr alloy::raw_field pl{16u, 2u};
    static constexpr alloy::raw_field chsel{25u, 4u};
    // SxFCR fields the engine names.
    static constexpr alloy::raw_field fth{0u, 2u};
    static constexpr alloy::raw_field dmdis{2u, 1u};
    static constexpr alloy::raw_field feie{7u, 1u};
};

// One fake instance per test tag: the engine's callback<S> statics are keyed
// by Inst, so a fresh tag is a fresh latch (the v1 test's isolation idiom).
// Per-stream vectors, stated as chip data (irq_lines ranges) the way
// device.py emits them — the host suite's slot table has 8 lines, so stream S
// simply raises line S here.
template <class Tag>
struct fake_dma_v2_inst {
    using ip = fake_dma_v2_ip<Tag>;
    // 64 words cover LISR..HIFCR + 8 streams of SCR..SFCR at stride 24
    // (0x10 + 8*0x18 = 0xD0 bytes).
    static inline std::uint32_t mem[64]{};
    static inline std::uint32_t rcc_reg = 0;
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
    static inline const alloy::clock_gate gate{
        reinterpret_cast<std::uintptr_t>(&rcc_reg), 0x1u};
    struct feat {
        static constexpr std::uint32_t streams = 8u;
    };
    static constexpr unsigned irq_line_count = 8u;
    static constexpr alloy::irq_line_range irq_lines[8] = {
        {0u, 0u, alloy::irq_line{0}}, {1u, 1u, alloy::irq_line{1}},
        {2u, 2u, alloy::irq_line{2}}, {3u, 3u, alloy::irq_line{3}},
        {4u, 4u, alloy::irq_line{4}}, {5u, 5u, alloy::irq_line{5}},
        {6u, 6u, alloy::irq_line{6}}, {7u, 7u, alloy::irq_line{7}},
    };
};

template <class Tag>
using engine = alloy::hal::detail::st_dma_v2_engine<fake_dma_v2_inst<Tag>>;

// The hardware's half of LIFCR/HIFCR: write-1-to-clear, committed on demand so
// a test chooses the exact instant the flag disappears (clear positions match
// flag positions register-for-register, which is what makes the mask-out
// below the yaml's own statement of W1C).
template <class Tag>
void commit_w1c() {
    auto& r = engine<Tag>::r();
    r.LISR = r.LISR & ~r.LIFCR;
    r.LIFCR = 0u;
    r.HISR = r.HISR & ~r.HIFCR;
    r.HIFCR = 0u;
}

int g_half_calls = 0;
void count_half(void*) { ++g_half_calls; }
int g_full_calls = 0;
void count_full(void*) { ++g_full_calls; }

}  // namespace

// The engine, wearing the name alloy::dma::channel looks it up under — so the
// SHIPPED user-facing wait()/done()/error() can be run over the same memory-
// backed register file (test_dma_ring.cpp's idiom, pointed at the real driver
// instead of a recording fake). Nothing here overrides behavior: it is the
// engine, renamed.
namespace alloy::hal {
template <class Tag>
struct dma_impl<fake_dma_v2_inst<Tag>>
    : alloy::hal::detail::st_dma_v2_engine<fake_dma_v2_inst<Tag>> {};
}  // namespace alloy::hal

// ── THE WITNESS: ISR-then-poll on the HALF flag, in HISR territory ────────

ALLOY_TEST(st_dma_v2_half_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    g_half_calls = 0;

    // Stream 5 on purpose: its flags live in HISR/HIFCR — a driver that only
    // ever consulted the LOW register pair cannot pass this test.
    eng::enable_half_irq<5>(&count_half, nullptr);
    eng::setup<5>(eng::dir::periph_to_mem, /*circular=*/true, eng::width::b8,
                  eng::width::b8, 0x40u, 0x80u, 8u, 3u);
    eng::start<5>();
    // Register-level witness of the fold: HTIE went in with the whole-SxCR
    // write, EN with start().
    ALLOY_CHECK((eng::scr<5>() & IP::htie.mask()) != 0u);
    ALLOY_CHECK((eng::scr<5>() & IP::en.mask()) != 0u);
    ALLOY_CHECK(!eng::half<5>());  // negative control: nothing latched yet

    // Hardware: the first half fills — HTIF5 rises (in HISR), the ISR runs.
    eng::r().HISR = eng::r().HISR | IP::htif5.mask;
    eng::half_isr<5>(nullptr);
    ALLOY_CHECK_EQ(g_half_calls, 1);
    // The ISR consumed the flag: it wrote CHTIF5 (it must — level-triggered
    // line), and nothing leaked into the LOW pair...
    ALLOY_CHECK((eng::r().HIFCR & IP::chtif5.mask) != 0u);
    ALLOY_CHECK_EQ(eng::r().LIFCR, 0u);
    commit_w1c<tag>();
    // ...and the hardware flag is really gone:
    ALLOY_CHECK((eng::r().HISR & IP::htif5.mask) == 0u);

    // THE WITNESS. A poller arriving after the ISR has no hardware flag left
    // to read — half<5>() must report from the latch. Revert either half-latch
    // line in st_dma_v2_body.hpp and this check fails.
    ALLOY_CHECK(eng::half<5>());

    eng::disable_half_irq<5>();  // unhook from the host irq chain
}

// The completion twin, also on a high stream.
ALLOY_TEST(st_dma_v2_completion_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    g_full_calls = 0;

    eng::enable_complete_irq<6>(&count_full, nullptr);
    eng::setup<6>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    eng::start<6>();
    ALLOY_CHECK((eng::scr<6>() & IP::tcie.mask()) != 0u);
    ALLOY_CHECK((eng::scr<6>() & IP::teie.mask()) != 0u);
    ALLOY_CHECK((eng::scr<6>() & IP::dmeie.mask()) != 0u);
    ALLOY_CHECK(!eng::complete<6>());

    eng::r().HISR = eng::r().HISR | IP::tcif6.mask;
    eng::complete_isr<6>(nullptr);
    ALLOY_CHECK_EQ(g_full_calls, 1);
    ALLOY_CHECK((eng::r().HIFCR & IP::ctcif6.mask) != 0u);
    commit_w1c<tag>();
    ALLOY_CHECK((eng::r().HISR & IP::tcif6.mask) == 0u);

    ALLOY_CHECK(eng::complete<6>());  // the latch, or nothing
    // ...and it succeeded. The control that keeps the error latch honest in
    // the other direction: a clean completion must never raise error().
    ALLOY_CHECK(!eng::error<6>());

    eng::disable_complete_irq<6>();
}

// ── THE ERROR TWIN: a failure the ISR consumed is still reportable ────────
//
// The defect this pins: complete_isr clears TEIF *and* DMEIF (it must —
// level-triggered line) and used to discard the fact with `(void)failed;`,
// while error() read only the live bits. A failed stream therefore reported
// complete()==true and error()==false to anything that polled after the
// interrupt — and this engine hid BOTH of its failure modes that way.
ALLOY_TEST(st_dma_v2_error_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    g_full_calls = 0;

    // Stream 4: HISR/HIFCR territory again, so a low-pair-only error path
    // cannot pass this.
    eng::enable_complete_irq<4>(&count_full, nullptr);
    eng::setup<4>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    eng::start<4>();
    ALLOY_CHECK(!eng::error<4>());  // negative control: nothing latched yet

    // Hardware: the stream faults. TEIF4 rises WITHOUT TCIF, and hardware
    // clears EN itself (RM0431 §9.3.18 — a transfer error disables the stream).
    eng::r().HISR = eng::r().HISR | IP::teif4.mask;
    eng::scr<4>() = eng::scr<4>() & ~IP::en.mask();
    eng::complete_isr<4>(nullptr);
    // A failure IS a completion-path event: the callback runs (TEIE and DMEIE
    // rode into SxCR with TCIE) and the transfer is over.
    ALLOY_CHECK_EQ(g_full_calls, 1);
    // The ISR consumed the flag — and ONLY that flag: no CTCIF (no completion
    // to clear) and no CDMEIF (DMEIF was never up). The clear-only-what-you-
    // consumed rule, on the error side, where this engine's clear is widest.
    ALLOY_CHECK_EQ(eng::r().HIFCR, IP::cteif4.mask);
    commit_w1c<tag>();
    ALLOY_CHECK((eng::r().HISR & IP::teif4.mask) == 0u);

    // THE WITNESS. A poller arriving after the ISR has no hardware flag left to
    // read — error<4>() must report from the latch. Revert either error-latch
    // line in st_dma_v2_body.hpp (the `err_latched = true` in complete_isr, or
    // the `err_latched ||` term in error()) and this check fails.
    ALLOY_CHECK(eng::error<4>());
    // And the transfer is finished either way, which is what makes wait() fall
    // out of its loop and consult error() at all.
    ALLOY_CHECK(eng::complete<4>());

    eng::disable_complete_irq<4>();
}

// The v2-only second failure mode: a DIRECT-MODE error (a request arrived at a
// stream that could not service it) raises DMEIF with TEIF clear. error() has
// always reported both; the latch must carry both, and the ISR must clear only
// the one that was up.
ALLOY_TEST(st_dma_v2_direct_mode_error_survives_its_own_isr) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    g_full_calls = 0;

    eng::enable_complete_irq<0>(&count_full, nullptr);
    eng::setup<0>(eng::dir::periph_to_mem, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    eng::start<0>();
    commit_w1c<tag>();
    ALLOY_CHECK(!eng::error<0>());

    eng::r().LISR = eng::r().LISR | IP::dmeif0.mask;
    eng::complete_isr<0>(nullptr);
    ALLOY_CHECK_EQ(g_full_calls, 1);
    ALLOY_CHECK_EQ(eng::r().LIFCR, IP::cdmeif0.mask);  // CDMEIF only
    commit_w1c<tag>();
    ALLOY_CHECK((eng::r().LISR & IP::dmeif0.mask) == 0u);

    ALLOY_CHECK(eng::error<0>());  // the latch carries the direct-mode error too

    eng::disable_complete_irq<0>();
}

// The CONSEQUENCE, through the shipped caller rather than the engine accessor:
// alloy::dma::channel::wait() ends in `return !error()`, so a lost error is a
// wait() that returns SUCCESS for a transfer that failed — and anchor 2.4's
// spi.transfer_dma() returns a bool sourced from exactly this call.
ALLOY_TEST(st_dma_v2_wait_reports_a_failure_its_own_isr_consumed) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    using inst = fake_dma_v2_inst<tag>;
    static const std::uint8_t payload[4] = {1, 2, 3, 4};
    g_full_calls = 0;

    // POSITIVE CONTROL FIRST, on its own stream: a transfer that completes
    // cleanly, whose interrupt also ran, must still wait() TRUE — otherwise
    // this test could "pass" by breaking every DMA transfer in the framework.
    {
        auto ok = alloy::dma::channel<inst, 2>::claim();
        ok.on_complete(&count_full, nullptr);
        ok.start_m2p_u8(std::span<const std::uint8_t>(payload), 0x40u, 3u);
        eng::r().LISR = eng::r().LISR | IP::tcif2.mask;
        eng::scr<2>() = eng::scr<2>() & ~IP::en.mask();  // v2: EN self-clears
        eng::complete_isr<2>(nullptr);
        commit_w1c<tag>();
        ALLOY_CHECK(ok.wait());
        ok.clear_on_complete();
    }

    // THE CASE. Same shape, but hardware raises TEIF instead of TCIF.
    {
        auto bad = alloy::dma::channel<inst, 3>::claim();
        bad.on_complete(&count_full, nullptr);
        bad.start_m2p_u8(std::span<const std::uint8_t>(payload), 0x40u, 3u);
        eng::r().LISR = eng::r().LISR | IP::teif3.mask;
        eng::scr<3>() = eng::scr<3>() & ~IP::en.mask();
        eng::complete_isr<3>(nullptr);
        commit_w1c<tag>();  // the flag the ISR consumed is really gone

        // The shipped answer FIRST, so that it is this check — the one a
        // caller's `bool ok` comes from — that trips when the latch is
        // reverted, not a diagnostic further down.
        ALLOY_CHECK(!bad.wait());
        ALLOY_CHECK(bad.done());   // the transfer is over...
        ALLOY_CHECK(bad.error());  // ...and it failed.
        bad.clear_on_complete();
    }
    ALLOY_CHECK_EQ(g_full_calls, 2);
}

// ── the v2 silicon edge: EN auto-clears at non-circular transfer end ──────

ALLOY_TEST(st_dma_v2_completion_with_en_self_cleared_and_restart) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;

    // A polled one-shot (no callbacks): program, start, let it finish.
    eng::setup<2>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    eng::start<2>();

    // Hardware at transfer end, per RM: TCIF2 rises AND the stream clears its
    // own EN (the inverse of dma_v1, where completion leaves EN=1).
    eng::r().LISR = eng::r().LISR | IP::tcif2.mask;
    eng::scr<2>() = eng::scr<2>() & ~IP::en.mask();

    // Completion must be reported from the flag with EN already low...
    ALLOY_CHECK(eng::complete<2>());
    ALLOY_CHECK((eng::scr<2>() & IP::en.mask()) == 0u);

    // ...and the stream must be reusable: setup()'s stop-poll falls straight
    // through the already-clear EN (a hang here is this test timing out),
    // the stale TCIF is cleared so hardware will accept EN again, and
    // start() re-raises EN.
    eng::setup<2>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 4u, 3u);
    commit_w1c<tag>();
    ALLOY_CHECK((eng::r().LISR & IP::tcif2.mask) == 0u);
    ALLOY_CHECK(!eng::complete<2>());  // the new transfer is not already done
    eng::start<2>();
    ALLOY_CHECK((eng::scr<2>() & IP::en.mask()) != 0u);
    ALLOY_CHECK_EQ(eng::remaining<2>(), 4u);
}

// ── the latch's lifecycle and the ISRs' precision (the v1 shape, kept) ────

ALLOY_TEST(st_dma_v2_setup_resets_stale_latches) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;

    eng::enable_half_irq<1>(&count_half, nullptr);
    eng::enable_complete_irq<1>(&count_full, nullptr);
    eng::setup<1>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();
    // A full lap of the previous transfer latched both events...
    eng::r().LISR = eng::r().LISR | IP::htif1.mask | IP::tcif1.mask |
                    IP::teif1.mask;
    eng::half_isr<1>(nullptr);
    commit_w1c<tag>();
    eng::complete_isr<1>(nullptr);
    commit_w1c<tag>();
    ALLOY_CHECK(eng::half<1>());
    ALLOY_CHECK(eng::complete<1>());
    ALLOY_CHECK(eng::error<1>());  // all THREE latches are up

    // ...and programming a NEW transfer must forget them: a fresh transfer is
    // not already half-done, is not already complete, and — the reason the
    // error latch is transfer-scoped rather than sticky — has not failed
    // because its predecessor did.
    eng::setup<1>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();
    ALLOY_CHECK(!eng::half<1>());
    ALLOY_CHECK(!eng::complete<1>());
    ALLOY_CHECK(!eng::error<1>());

    eng::disable_half_irq<1>();
    eng::disable_complete_irq<1>();
}

ALLOY_TEST(st_dma_v2_isrs_are_no_ops_for_foreign_interrupts) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    g_half_calls = 0;
    g_full_calls = 0;

    eng::enable_half_irq<0>(&count_half, nullptr);
    eng::enable_complete_irq<0>(&count_full, nullptr);
    eng::setup<0>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();  // absorb setup's own clear_flags store

    // A SIBLING stream's flags are up in the SAME register; stream 0's are
    // all clear. The vector is per-stream on this engine, but the safe-no-op
    // contract still holds (alloy::irq chains, spurious NVIC pends): both
    // handlers run and must touch NOTHING — no IFCR write (a clear here would
    // eat the sibling's event), no callback, no latch.
    eng::r().LISR = eng::r().LISR | IP::htif2.mask | IP::tcif2.mask |
                    IP::teif2.mask | IP::dmeif2.mask;
    eng::half_isr<0>(nullptr);
    eng::complete_isr<0>(nullptr);
    ALLOY_CHECK_EQ(eng::r().LIFCR, 0u);
    ALLOY_CHECK_EQ(g_half_calls, 0);
    ALLOY_CHECK_EQ(g_full_calls, 0);
    ALLOY_CHECK(!eng::half<0>());
    ALLOY_CHECK(!eng::complete<0>());
    // ...including the error latch: a SIBLING stream's failure is not ours.
    ALLOY_CHECK(!eng::error<0>());
    eng::r().LISR = 0u;

    eng::disable_half_irq<0>();
    eng::disable_complete_irq<0>();
}

ALLOY_TEST(st_dma_v2_complete_isr_clears_only_what_it_consumed) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;
    g_full_calls = 0;

    eng::enable_complete_irq<3>(&count_full, nullptr);
    eng::setup<3>(eng::dir::periph_to_mem, true, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    commit_w1c<tag>();

    // The wrap instant on a ring: HTIF and TCIF up together. The completion
    // handler runs first (attach order — see alloy::dma::ring) and must clear
    // ONLY the flags it consumed — a blanket per-stream clear here would eat
    // the half event before its own handler saw it.
    eng::r().LISR = eng::r().LISR | IP::htif3.mask | IP::tcif3.mask;
    eng::complete_isr<3>(nullptr);
    ALLOY_CHECK_EQ(eng::r().LIFCR, IP::ctcif3.mask);
    commit_w1c<tag>();
    // The half event is still there for its own consumer — as the live flag:
    ALLOY_CHECK((eng::r().LISR & IP::htif3.mask) != 0u);
    ALLOY_CHECK(eng::half<3>());
    eng::r().LISR = 0u;

    eng::disable_complete_irq<3>();
}

ALLOY_TEST(st_dma_v2_setup_without_callbacks_folds_no_interrupt_enables) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;

    // The polled-only consumer: no callbacks registered, so the whole-SxCR
    // write must leave every interrupt enable clear — a spurious TCIE with no
    // handler attached would wedge the line.
    eng::setup<4>(eng::dir::periph_to_mem, true, eng::width::b16,
                  eng::width::b16, 0x40u, 0x80u, 8u, 9u);
    ALLOY_CHECK((eng::scr<4>() &
                 (IP::tcie.mask() | IP::teie.mask() | IP::htie.mask() |
                  IP::dmeie.mask())) == 0u);
    // The request id landed in CHSEL — the stream's request-source selector,
    // fed by the route triple (9 exercises the 4-bit width):
    ALLOY_CHECK_EQ((eng::scr<4>() & IP::chsel.mask()) >> IP::chsel.pos, 9u);
    // And the whole-register write left EN low until start():
    ALLOY_CHECK((eng::scr<4>() & IP::en.mask()) == 0u);
}

ALLOY_TEST(st_dma_v2_setup_clears_all_five_flags_and_forces_direct_mode) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_dma_v2_ip<tag>;

    // Hardware refuses EN while ANY of the stream's five flags is set, so
    // setup()'s clear must name all five — including FEIF, which the engine
    // never otherwise consults.
    eng::r().HISR = IP::feif7.mask | IP::dmeif7.mask | IP::teif7.mask |
                    IP::htif7.mask | IP::tcif7.mask;
    // A previous owner left the FIFO enabled; setup must put the stream back
    // in direct mode (this phase's honest scope).
    IP::dmdis.write(eng::sfcr<7>(), 1u);
    IP::feie.write(eng::sfcr<7>(), 1u);

    eng::setup<7>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 3u);
    ALLOY_CHECK_EQ(eng::r().HIFCR,
                   IP::ctcif7.mask | IP::chtif7.mask | IP::cteif7.mask |
                       IP::cdmeif7.mask | IP::cfeif7.mask);
    commit_w1c<tag>();
    ALLOY_CHECK_EQ(eng::r().HISR, 0u);
    ALLOY_CHECK_EQ(IP::dmdis.read(eng::sfcr<7>()), 0u);
    ALLOY_CHECK_EQ(IP::feie.read(eng::sfcr<7>()), 0u);
}
