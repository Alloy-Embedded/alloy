// The ISR->poller latch contract of the SAM E70 XDMAC, witnessed on the host —
// the same witness shape as test_st_dma_v1_latch.cpp / test_st_dma_v2_latch.cpp,
// shipped WITH the driver method it guards rather than owed after it.
//
// WHAT RUNS HERE IS THE REAL DRIVER. detail::microchip_xdmac_v1_engine (split
// out of microchip_xdmac_v1.hpp for exactly this file) is instantiated over a
// hand-written IP double whose registers are plain host memory, laid out with
// the XDMAC shape: the global block at 0x00-0x24 with GIE/GID/GIS/GE/GD/GS as
// PER-CHANNEL BIT registers, and the 24-channel interleaved cluster starting at
// 0x50 with stride 0x40. The tests drive the engine's OWN ISR and poll its OWN
// accessors; the only thing simulated is the hardware's side of the protocol.
//
// WHY THIS ENGINE'S LATCH IS A DIFFERENT KIND OF THING FROM ST's. On both ST
// engines the latch is a DISCIPLINE: their ISR must write a flag clear
// (IFCR/LIFCR) that a later poller would otherwise miss, and every accessor is
// spelled `latch || live flag` because their status registers survive being
// read. XDMAC's CIS is CLEARED BY READING (DS60001527). There is no live flag
// to fall back on, there is no second reader, and the loser of the race gets a
// zero. So the contract has two halves and both are witnessed below:
//   1. whoever reads CIS must LATCH everything it held — harvest() is the one
//      reader, and it is idempotent, which also closes a latent bug at HEAD
//      (error() used to read CIS raw, so it answered true and then FALSE);
//   2. while a callback is armed, a POLLER MUST NOT READ CIS AT ALL, because
//      `alloy::dma::channel::wait()` polls in a tight loop and would eat the
//      completion out from under the interrupt that is about to fire.
//
// ── HOW A MEMORY DOUBLE MODELS "CLEARED BY READING", AND WHAT IT CANNOT ──
//
// It cannot destroy the value on read, and that is a measured limitation, not
// an unexamined one. alloy::rw32 is `volatile std::uint32_t` (core/mmio.hpp),
// so every register read is a plain load and no C++ hook exists. The one
// mechanism that does work — mmap a page for the CIS words, mprotect it
// PROT_NONE, and catch SIGSEGV/SIGBUS to count the access and unprotect —
// was PROTOTYPED AND MEASURED before being rejected: standalone it works
// (the faulting read returns the right value and the fault is counted), and
// under `-fsanitize=address` the identical binary produces no output and has
// to be killed. The host suite builds with ASan ON by default wherever CI runs
// (tests/CMakeLists.txt gates it on APPLE), so that instrument would be a leg
// that hangs on the machine nobody watches. It was not taken.
//
// WHAT IS USED INSTEAD, and why it still bites: the LATCH IS THE PROXY FOR THE
// READ. harvest() is the only code in the engine that reads CIS outside
// clear_flags(), and it cannot read without latching — the read and the stores
// are three lines of one function. So `callback<Ch>::latched` and
// `err_latched` staying FALSE after a poller ran is a positive statement that
// the poller did not read CIS, checked directly below. The hardware's own half
// of the protocol — the clear that the read performs — is committed by the
// explicit `hw_read_clears<Ch>()` helper at the instant the test chooses, the
// same idiom test_st_dma_v2_latch.cpp uses for the W1C behavior of LIFCR
// ("the test IS the hardware here").
// The residual gap, stated: a hypothetical CIS read that does NOT latch (an
// accessor rewritten back to a raw read) is invisible to this file. The
// engine has exactly one CIS reader, and that is what makes the proxy sound.

#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/dma/microchip_xdmac_v1_body.hpp"
#include "alloy_test.hpp"

namespace {

// The XDMAC register shape, hand-written (tests never read generated headers):
// same offsets, strides and bit positions as registers/microchip/xdmac_v1.yaml.
// GIS is declared WRITABLE (silicon says ro) and so are GE/GD/GIE — the test is
// the hardware here, and raising a pending bit is a write from the hardware's
// side, exactly as test_st_dma_v2_latch.cpp declares LISR/HISR writable.
template <class Tag>
struct fake_xdmac_ip {
    struct regs {
        alloy::rw32 GTYPE;  // 0x00
        alloy::rw32 GCFG;   // 0x04
        alloy::rw32 GWAC;   // 0x08
        alloy::rw32 GIE;    // 0x0C — per-channel bit, write-only SET on silicon
        alloy::rw32 GID;    // 0x10 — the CLEAR twin
        alloy::rw32 GIM;    // 0x14
        alloy::rw32 GIS;    // 0x18 — who is pending; NOT clear-on-read
        alloy::rw32 GE;     // 0x1C
        alloy::rw32 GD;     // 0x20
        alloy::rw32 GS;     // 0x24 — 1 while the channel is enabled
    };

    // The channel cluster: channel n's register X at 0x50 + n*0x40 + local.
    static constexpr std::uintptr_t CIE_offset = 0x50;
    static constexpr unsigned CIE_stride = 64u;
    static constexpr std::uintptr_t CID_offset = 0x54;
    static constexpr unsigned CID_stride = 64u;
    static constexpr std::uintptr_t CIM_offset = 0x58;
    static constexpr unsigned CIM_stride = 64u;
    static constexpr std::uintptr_t CIS_offset = 0x5C;
    static constexpr unsigned CIS_stride = 64u;
    static constexpr std::uintptr_t CSA_offset = 0x60;
    static constexpr unsigned CSA_stride = 64u;
    static constexpr std::uintptr_t CDA_offset = 0x64;
    static constexpr unsigned CDA_stride = 64u;
    static constexpr std::uintptr_t CNDA_offset = 0x68;
    static constexpr unsigned CNDA_stride = 64u;
    static constexpr std::uintptr_t CNDC_offset = 0x6C;
    static constexpr unsigned CNDC_stride = 64u;
    static constexpr std::uintptr_t CUBC_offset = 0x70;
    static constexpr unsigned CUBC_stride = 64u;
    static constexpr std::uintptr_t CBC_offset = 0x74;
    static constexpr unsigned CBC_stride = 64u;
    static constexpr std::uintptr_t CC_offset = 0x78;
    static constexpr unsigned CC_stride = 64u;

    // The per-channel bit registers: channel n is bit n of each.
    template <unsigned I>
        requires(I < 24u)
    static constexpr auto ie = alloy::field<&regs::GIE, 0u + I * 1u, 1>;
    template <unsigned I>
        requires(I < 24u)
    static constexpr auto id = alloy::field<&regs::GID, 0u + I * 1u, 1>;
    template <unsigned I>
        requires(I < 24u)
    static constexpr auto is = alloy::field<&regs::GIS, 0u + I * 1u, 1>;
    template <unsigned I>
        requires(I < 24u)
    static constexpr auto en = alloy::field<&regs::GE, 0u + I * 1u, 1>;
    template <unsigned I>
        requires(I < 24u)
    static constexpr auto di = alloy::field<&regs::GD, 0u + I * 1u, 1>;
    template <unsigned I>
        requires(I < 24u)
    static constexpr auto st = alloy::field<&regs::GS, 0u + I * 1u, 1>;

    // CIE / CID / CIS share one bit order (yaml CIE/CID/CIS field lists).
    static constexpr alloy::raw_field bie{0u, 1u};
    static constexpr alloy::raw_field rbie{4u, 1u};
    static constexpr alloy::raw_field wbie{5u, 1u};
    static constexpr alloy::raw_field roie{6u, 1u};
    static constexpr alloy::raw_field bid{0u, 1u};
    static constexpr alloy::raw_field rbeid{4u, 1u};
    static constexpr alloy::raw_field wbeid{5u, 1u};
    static constexpr alloy::raw_field roid{6u, 1u};
    static constexpr alloy::raw_field bis{0u, 1u};   // end of block = completion
    static constexpr alloy::raw_field lis{1u, 1u};   // end of linked list
    static constexpr alloy::raw_field dis{2u, 1u};   // end of disable
    static constexpr alloy::raw_field fis{3u, 1u};   // end of flush
    static constexpr alloy::raw_field rbeis{4u, 1u};  // read bus error
    static constexpr alloy::raw_field wbeis{5u, 1u};  // write bus error
    static constexpr alloy::raw_field rois{6u, 1u};   // request overflow

    // CNDA / CNDC / CUBC fields — the descriptor-list registers. This file
    // never builds a ring; they are here because the engine's ring path names
    // them and a member has to exist for the class to instantiate at all.
    // NOTE the bit positions: these are the REGISTERS' bits, and the view-0
    // descriptor's UBC word carries the same four names at DIFFERENT positions
    // (the engine spells those `ubc_*` and never mixes the two).
    static constexpr alloy::raw_field ndaif{0u, 1u};
    static constexpr alloy::raw_field nda{2u, 30u};
    static constexpr alloy::raw_field nde{0u, 1u};
    static constexpr alloy::raw_field ndsup{1u, 1u};
    static constexpr alloy::raw_field nddup{2u, 1u};
    static constexpr alloy::raw_field ndview{3u, 2u};
    static constexpr alloy::raw_field ublen{0u, 24u};

    // CC fields the engine folds into its one whole-register write.
    static constexpr alloy::raw_field type{0u, 1u};
    static constexpr alloy::raw_field dsync{4u, 1u};
    static constexpr alloy::raw_field dwidth{11u, 2u};
    static constexpr alloy::raw_field sif{13u, 1u};
    static constexpr alloy::raw_field dif{14u, 1u};
    static constexpr alloy::raw_field sam{16u, 2u};
    static constexpr alloy::raw_field dam{18u, 2u};
    static constexpr alloy::raw_field perid{24u, 7u};
};

// One fake instance per test tag: the engine's callback<Ch> statics are keyed
// by Inst, so a fresh tag is a fresh latch (the ST tests' isolation idiom).
// ONE NVIC line for all 24 channels, which is the silicon fact this engine is
// shaped around; the NUMBER is chip data (58 on a real SAME70) and irrelevant
// here — the host slot table has 8 lines (tests/test_irq.cpp), so the double
// says 5.
template <class Tag>
struct fake_xdmac_inst {
    using ip = fake_xdmac_ip<Tag>;
    // 0x50 + 24*0x40 = 0x650 bytes of register file.
    static inline std::uint32_t mem[512]{};
    static inline std::uint32_t pmc_reg = 0;
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
    static inline const alloy::clock_gate gate{
        reinterpret_cast<std::uintptr_t>(&pmc_reg), 0x1u};
    static constexpr std::uint8_t ch_count = 24u;
    static constexpr alloy::irq_line irq{5};
};

template <class Tag>
using engine = alloy::hal::detail::microchip_xdmac_v1_engine<fake_xdmac_inst<Tag>>;

// ── the hardware's half of the protocol ──────────────────────────────────
//
// Raise a channel's status bits, exactly as the XDMAC would: the bits land in
// CIS, and — since CIE/GIE let them through — the channel's GIS bit goes up
// with them. GIS is what the ISR's shared-line guard reads.
template <class Tag, unsigned Ch>
void hw_raise(std::uint32_t cis_bits) {
    using eng = engine<Tag>;
    using IP = fake_xdmac_ip<Tag>;
    eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) =
        eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) | cis_bits;
    eng::r().GIS = eng::r().GIS | IP::template is<Ch - 1>.mask;
}

// The clear that READING CIS performs, committed at the instant the test
// chooses — the commit_w1c<>() idiom of test_st_dma_v2_latch.cpp, for the
// other clearing discipline. Call it after any engine call that legitimately
// read the register; do NOT call it after a call that must not have read it,
// so that the test's own picture stays honest.
template <class Tag, unsigned Ch>
void hw_read_clears() {
    using eng = engine<Tag>;
    using IP = fake_xdmac_ip<Tag>;
    eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) = 0u;
    eng::r().GIS = eng::r().GIS & ~IP::template is<Ch - 1>.mask;
}

// The transfer end the hardware signals in GS: the channel auto-disables at
// end of block (and on a bus error), so its GS bit falls back to 0.
template <class Tag, unsigned Ch>
void hw_channel_stops() {
    using eng = engine<Tag>;
    using IP = fake_xdmac_ip<Tag>;
    eng::r().GS = eng::r().GS & ~IP::template st<Ch - 1>.mask;
}

// GE/GD are write-only SET/CLEAR registers on silicon; the double turns the
// engine's GE write into the GS bit the engine polls.
template <class Tag, unsigned Ch>
void hw_apply_enable() {
    using eng = engine<Tag>;
    using IP = fake_xdmac_ip<Tag>;
    if ((eng::r().GE & IP::template en<Ch - 1>.mask) != 0u) {
        eng::r().GS = eng::r().GS | IP::template st<Ch - 1>.mask;
        eng::r().GE = 0u;
    }
}

int g_calls = 0;
void count_calls(void*) { ++g_calls; }

}  // namespace

// The engine, wearing the name alloy::dma::channel looks it up under — so the
// SHIPPED user-facing wait()/done()/error() can be run over the same memory-
// backed register file (the test_st_dma_v2_latch.cpp idiom). Nothing here
// overrides behavior: it is the engine, renamed.
namespace alloy::hal {
template <class Tag>
struct dma_impl<fake_xdmac_inst<Tag>>
    : alloy::hal::detail::microchip_xdmac_v1_engine<fake_xdmac_inst<Tag>> {};
}  // namespace alloy::hal

// ── THE WITNESS: a completion its own ISR consumed is still reportable ────

ALLOY_TEST(xdmac_completion_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<3>(&count_calls, nullptr);
    eng::setup<3>(eng::dir::mem_to_periph, /*circular=*/false, eng::width::b8,
                  eng::width::b8, 0x40u, 0x80u, 8u, 9u);
    // Register-level witness of the fold: setup() armed BOTH halves of the
    // interrupt path — the channel's own event select (CIE) and the channel's
    // permission to reach the NVIC line at all (GIE).
    ALLOY_CHECK((eng::chreg<3>(IP::CIE_offset, IP::CIE_stride) & IP::bie.mask()) != 0u);
    ALLOY_CHECK((eng::r().GIE & IP::template ie<2>.mask) != 0u);
    // ...and the request id landed in PERID (9 = USART1_TX on the real chip).
    ALLOY_CHECK_EQ((eng::chreg<3>(IP::CC_offset, IP::CC_stride) & IP::perid.mask()) >>
                       IP::perid.pos,
                   9u);
    eng::start<3>();
    hw_apply_enable<tag, 3>();
    ALLOY_CHECK(!eng::complete<3>());  // negative control: still running

    // Hardware: the block finishes. BIS rises, the channel auto-disables.
    hw_raise<tag, 3>(IP::bis.mask());
    hw_channel_stops<tag, 3>();
    eng::complete_isr<3>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 1);
    // The ISR read CIS — which on this IP IS the clear. Commit it.
    hw_read_clears<tag, 3>();
    ALLOY_CHECK_EQ(eng::chreg<3>(IP::CIS_offset, IP::CIS_stride), 0u);

    // THE WITNESS. A poller arriving after the ISR has no CIS left to read —
    // complete<3>() must report from the latch. Revert the `latched = true`
    // line in harvest() and this fails.
    ALLOY_CHECK(eng::complete<3>());
    // ...and it succeeded: the control that keeps the error latch honest in
    // the other direction.
    ALLOY_CHECK(!eng::error<3>());

    eng::disable_complete_irq<3>();
}

// The case above passes on the GS source alone, and saying so is the point of
// this one. On a SINGLE-MICROBLOCK transfer — all setup() programs today, CBC
// is written 0 — end-of-block IS end-of-transfer, so the channel auto-disables
// at the same instant BIS rises and complete() can answer from GS without ever
// consulting the latch. The completion latch is therefore belt-and-braces for
// what ships TODAY, and load-bearing for the state this IP reaches as soon as
// the transfer is more than one block: BIS is END OF BLOCK, and with a block
// count or a descriptor list behind it the channel stays ENABLED across the
// event. Then GS says "still running", the ISR has already eaten CIS, and the
// latch is the only thing left that knows a block finished. That is the phase
// 5b (ring) shape, pinned here so the contract cannot be quietly dropped
// between now and then — this is the case that goes red if the
// `latched = true` in harvest() or the `latched ||` term in complete() is
// reverted.
ALLOY_TEST(xdmac_completion_latch_is_the_only_source_while_the_channel_runs_on) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<5>(&count_calls, nullptr);
    eng::setup<5>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 9u);
    eng::start<5>();
    hw_apply_enable<tag, 5>();

    // Hardware: end of BLOCK, channel still enabled (multi-block / linked
    // list). Note what is NOT called here: hw_channel_stops.
    hw_raise<tag, 5>(IP::bis.mask());
    eng::complete_isr<5>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 1);
    hw_read_clears<tag, 5>();

    // GS still says "running", CIS is empty, and complete<5>() must STILL be
    // true — there is no third place for that fact to live.
    ALLOY_CHECK((eng::r().GS & IP::template st<4>.mask) != 0u);
    ALLOY_CHECK_EQ(eng::chreg<5>(IP::CIS_offset, IP::CIS_stride), 0u);
    ALLOY_CHECK(eng::complete<5>());

    eng::disable_complete_irq<5>();
}

// ── THE ERROR TWIN ───────────────────────────────────────────────────────
//
// Worse here than on ST, and for a structural reason: completion has a second,
// non-destructive source in GS, so a lost BIS is survivable on a one-shot.
// A bus error has NO second source — CIS is the only place RBEIS/WBEIS/ROIS
// are ever visible. If the ISR reads CIS and does not latch, the failure is
// not merely late, it is GONE, and wait() (`return !error()`) reports SUCCESS
// for a transfer that faulted.
ALLOY_TEST(xdmac_error_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<7>(&count_calls, nullptr);
    eng::setup<7>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 9u);
    eng::start<7>();
    hw_apply_enable<tag, 7>();
    ALLOY_CHECK(!eng::error<7>());  // negative control

    // Hardware: the source address was not readable by the memory master —
    // the SILICON-FOUND case this driver's header records, a .rodata source on
    // a real SAME70 Xplained. RBEIS rises WITHOUT BIS, and the channel
    // auto-disables just as it does on success.
    hw_raise<tag, 7>(IP::rbeis.mask());
    hw_channel_stops<tag, 7>();
    eng::complete_isr<7>(nullptr);
    // A failure IS a completion-path event: the error enables rode into CIE
    // with BIE, the transfer is over, and the callback runs.
    ALLOY_CHECK_EQ(g_calls, 1);
    hw_read_clears<tag, 7>();

    // THE WITNESS. Revert the `err_latched = true` line in harvest() and this
    // fails — with nothing else to read, the failure would simply vanish.
    ALLOY_CHECK(eng::error<7>());
    // The transfer is over either way, which is what makes wait() fall out of
    // its loop and consult error() at all.
    ALLOY_CHECK(eng::complete<7>());

    eng::disable_complete_irq<7>();
}

// The CONSEQUENCE, through the shipped caller rather than the engine accessor:
// alloy::dma::channel::wait() ends in `return !error()`, and uart::write_dma()
// returns a bool sourced from exactly that call.
ALLOY_TEST(xdmac_wait_reports_a_failure_its_own_isr_consumed) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    using inst = fake_xdmac_inst<tag>;
    static const std::uint8_t payload[4] = {1, 2, 3, 4};
    g_calls = 0;

    // POSITIVE CONTROL FIRST, on its own channel: a transfer that completes
    // cleanly, whose interrupt also ran, must still wait() TRUE — otherwise
    // this test could "pass" by breaking every DMA transfer in the framework.
    {
        auto ok = alloy::dma::channel<inst, 1>::claim();
        ok.on_complete(&count_calls, nullptr);
        ok.start_m2p_u8(std::span<const std::uint8_t>(payload), 0x40u, 9u);
        hw_apply_enable<tag, 1>();
        hw_raise<tag, 1>(IP::bis.mask());
        hw_channel_stops<tag, 1>();
        eng::complete_isr<1>(nullptr);
        hw_read_clears<tag, 1>();
        ALLOY_CHECK(ok.wait());
        ok.clear_on_complete();
    }

    // THE CASE. Same shape, but the hardware raises a write bus error.
    {
        auto bad = alloy::dma::channel<inst, 2>::claim();
        bad.on_complete(&count_calls, nullptr);
        bad.start_m2p_u8(std::span<const std::uint8_t>(payload), 0x40u, 9u);
        hw_apply_enable<tag, 2>();
        hw_raise<tag, 2>(IP::wbeis.mask());
        hw_channel_stops<tag, 2>();
        eng::complete_isr<2>(nullptr);
        hw_read_clears<tag, 2>();  // the flag the ISR consumed is really gone

        // The shipped answer FIRST, so that it is this check — the one a
        // caller's `bool ok` comes from — that trips when the latch is
        // reverted, not a diagnostic further down.
        ALLOY_CHECK(!bad.wait());
        ALLOY_CHECK(bad.done());   // the transfer is over...
        ALLOY_CHECK(bad.error());  // ...and it failed.
        bad.clear_on_complete();
    }
    ALLOY_CHECK_EQ(g_calls, 2);
}

// ── THE CLEAR-ON-READ HAZARD ITSELF ──────────────────────────────────────
//
// The one this IP has and neither ST engine does. `channel::wait()` is
//     while (!done()) { if (error()) return false; }
// so a polled wait calls error() thousands of times while the transfer runs.
// If those calls read CIS, the poller EATS the completion: the interrupt then
// fires, reads zero, latches nothing and calls nothing. On a one-shot the
// caller is saved by GS; on anything that leaves the channel enabled the spin
// never ends, and the callback is swallowed either way.
//
// THE LATCH IS THE PROXY FOR THE READ (see the file header): harvest() cannot
// read CIS without setting a latch, so latches still FALSE after the polling
// is a positive statement that CIS was never touched. Delete the
// `if (poller_owns_cis<Ch>())` guard from complete() or from error() — making
// them harvest unconditionally, the ST spelling — and this test goes red on
// the very next line.
ALLOY_TEST(xdmac_a_polling_waiter_does_not_eat_the_isrs_completion) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<4>(&count_calls, nullptr);
    eng::setup<4>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 9u);
    eng::start<4>();
    hw_apply_enable<tag, 4>();

    // The block finishes. CIS.BIS is up and the NVIC line is asserted, but the
    // interrupt has not been serviced yet — the window every real transfer
    // passes through, and the one a spinning wait() lives inside.
    hw_raise<tag, 4>(IP::bis.mask());
    hw_channel_stops<tag, 4>();

    // The poller spins here, exactly as channel::wait() does.
    for (int i = 0; i < 32; ++i) {
        (void)eng::complete<4>();
        (void)eng::error<4>();
    }

    // THE WITNESS, in two halves. First: the polling did not read CIS. The
    // latches are the proxy — harvest() cannot read without setting one, and
    // the completion is plainly there to be latched...
    ALLOY_CHECK(!eng::callback<4>::latched);
    ALLOY_CHECK(!eng::callback<4>::err_latched);
    // ...second: the register the interrupt needs is still intact.
    ALLOY_CHECK((eng::chreg<4>(IP::CIS_offset, IP::CIS_stride) & IP::bis.mask()) != 0u);
    ALLOY_CHECK((eng::r().GIS & IP::template is<3>.mask) != 0u);

    // So the interrupt, when it finally runs, still finds its evidence and the
    // callback fires. This is the line the whole guard exists for.
    eng::complete_isr<4>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 1);
    hw_read_clears<tag, 4>();
    ALLOY_CHECK(eng::complete<4>());

    eng::disable_complete_irq<4>();
}

// The other side of the same guard: with NO callback armed, the poller IS the
// only reader and must harvest for itself — otherwise a polled-only owner
// could never see a bus error at all, since CIS is the only place one appears.
// And having harvested, it must keep answering: the read emptied the register,
// so a second call has nothing left to consult but the latch.
ALLOY_TEST(xdmac_a_polled_only_owner_harvests_and_error_is_idempotent) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;

    // No enable_complete_irq: callback<Ch>::fn stays null, so this channel is
    // the poller's.
    eng::setup<6>(eng::dir::periph_to_mem, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 10u);
    eng::start<6>();
    hw_apply_enable<tag, 6>();
    ALLOY_CHECK(!eng::error<6>());  // negative control

    hw_raise<tag, 6>(IP::rois.mask());  // request overflow
    hw_channel_stops<tag, 6>();

    // First look: the poller reads CIS itself and sees the failure.
    ALLOY_CHECK(eng::error<6>());
    hw_read_clears<tag, 6>();  // that read cleared the register

    // THE IDEMPOTENCE WITNESS, and a latent bug at HEAD that this closes: the
    // old error() was a raw CIS read with no latch, so the SECOND call — the
    // one channel::wait() makes on its way out, `return !error()` — got the
    // register the first call had already emptied and answered FALSE. A
    // failed transfer reported success. Ask twice, three times: same answer.
    ALLOY_CHECK(eng::error<6>());
    ALLOY_CHECK(eng::error<6>());
    ALLOY_CHECK(eng::complete<6>());
}

// ── the latch lifecycle and the shared-line contract ─────────────────────

// clear_flags() is the engine's OTHER destructive read — the stale-status
// purge — and it goes through harvest() so that the engine has exactly one
// piece of code that touches CIS. That is not bookkeeping: alloy::dma::
// channel::stop() calls clear_flags(), so on a raw-read purge, tearing a
// channel down would DESTROY the only record that it had failed, and the
// owner asking error() afterwards would be told the transfer was fine. Revert
// clear_flags() to a raw discard-read and this test goes red.
ALLOY_TEST(xdmac_the_status_purge_latches_instead_of_destroying) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;

    eng::setup<20>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    eng::start<20>();
    hw_apply_enable<tag, 20>();

    // The transfer faults; the owner tears the channel down without ever
    // having polled (the alloy::dma::channel::stop() path: stop + clear_flags).
    hw_raise<tag, 20>(IP::wbeis.mask());
    hw_channel_stops<tag, 20>();
    eng::clear_flags<20>();
    hw_read_clears<tag, 20>();  // the purge read it; hardware clears

    // The evidence is gone from the register and must still be answerable.
    ALLOY_CHECK_EQ(eng::chreg<20>(IP::CIS_offset, IP::CIS_stride), 0u);
    ALLOY_CHECK(eng::error<20>());

    // ...and a fresh setup() still forgets it — the purge latches, the reset
    // that FOLLOWS the purge drops it, in that order.
    eng::setup<20>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    ALLOY_CHECK(!eng::error<20>());
}

ALLOY_TEST(xdmac_setup_resets_stale_latches_after_purging_the_flags) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<9>(&count_calls, nullptr);
    eng::setup<9>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 9u);
    eng::start<9>();
    hw_apply_enable<tag, 9>();

    // A previous transfer that finished AND faulted latched both events.
    hw_raise<tag, 9>(IP::bis.mask() | IP::rbeis.mask());
    hw_channel_stops<tag, 9>();
    eng::complete_isr<9>(nullptr);
    hw_read_clears<tag, 9>();
    ALLOY_CHECK(eng::complete<9>());
    ALLOY_CHECK(eng::error<9>());

    // Programming a NEW transfer must forget them: a fresh transfer is not
    // already complete, and — the reason the error latch is transfer-scoped
    // rather than sticky — has not failed because its predecessor did.
    // Hardware also left a stale flag behind, which setup()'s clear_flags()
    // must eat BEFORE the latch resets run; reversing those two lines would
    // re-set `latched` from the very read that was meant to purge it.
    hw_raise<tag, 9>(IP::bis.mask());
    eng::setup<9>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 4u, 9u);
    hw_read_clears<tag, 9>();  // setup's clear_flags() read it
    eng::start<9>();
    hw_apply_enable<tag, 9>();
    ALLOY_CHECK(!eng::complete<9>());
    ALLOY_CHECK(!eng::error<9>());

    eng::disable_complete_irq<9>();
}

// 24 channels share ONE NVIC line, so every armed channel's handler runs on
// every XDMAC interrupt. A handler that skipped its GIS guard would read a
// FOREIGN channel's... no — worse: it would read its OWN CIS on somebody
// else's interrupt, which on this IP is not a wasted cycle but a destructive
// act, and would also run its callback for a transfer that never finished.
ALLOY_TEST(xdmac_isr_is_a_no_op_for_another_channels_interrupt) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<11>(&count_calls, nullptr);
    eng::setup<11>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    eng::start<11>();
    hw_apply_enable<tag, 11>();

    // A SIBLING channel is the one that finished. Ours is still running, and
    // its CIS happens to hold nothing.
    hw_raise<tag, 12>(IP::bis.mask());
    eng::complete_isr<11>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 0);
    ALLOY_CHECK(!eng::callback<11>::latched);
    ALLOY_CHECK(!eng::callback<11>::err_latched);
    // ...and the sibling's evidence is untouched, waiting for its own handler.
    ALLOY_CHECK((eng::chreg<12>(IP::CIS_offset, IP::CIS_stride) & IP::bis.mask()) != 0u);

    eng::disable_complete_irq<11>();
}

// The sharper half of the shared-line guard, and the one that says why the
// guard reads GIS rather than just reading CIS and filtering. GIS is the
// MASKED status: a channel's bit is up only while one of the events it
// ENABLED is pending. So "CIS holds BIS but GIS is clear" is a real state —
// it is what a channel looks like from the instant its interrupt is disabled
// (CID/GID written) until somebody consumes the flag, which includes the
// window inside disable_complete_irq() itself, where the hardware is already
// disarmed but the handler is still on the chain and fn is still set.
//
// A sibling's interrupt in that window runs our handler. Without the GIS
// guard it would harvest a channel that is not pending — destroying a flag
// the owner asked to be left alone, and running a callback the owner just
// cancelled. Revert the guard and this test goes red on both counts.
ALLOY_TEST(xdmac_isr_leaves_a_masked_channels_flag_alone) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<17>(&count_calls, nullptr);
    eng::setup<17>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    eng::start<17>();
    hw_apply_enable<tag, 17>();

    // The block finished, so CIS.BIS is up — but the channel's interrupt has
    // been masked, so its GIS bit is NOT. (Raised by hand, not through
    // hw_raise, because that helper models the enabled case.)
    eng::chreg<17>(IP::CIS_offset, IP::CIS_stride) = IP::bis.mask();
    ALLOY_CHECK((eng::r().GIS & IP::template is<16>.mask) == 0u);

    // A sibling channel is the one actually interrupting.
    hw_raise<tag, 18>(IP::bis.mask());
    eng::complete_isr<17>(nullptr);

    ALLOY_CHECK_EQ(g_calls, 0);                    // no callback for a masked channel
    ALLOY_CHECK(!eng::callback<17>::latched);      // ...and nothing was read
    ALLOY_CHECK((eng::chreg<17>(IP::CIS_offset, IP::CIS_stride) & IP::bis.mask()) != 0u);

    eng::disable_complete_irq<17>();
}

// An event this driver never acts on (end-of-flush / end-of-disable / end-of-
// list) must be harvested — the read is what clears the line — but must NOT be
// reported as a completion or a failure, and must not run the callback.
// Reachable because CIE is a write-only SET register that setup() never
// clears: a channel handed over from another owner (or a future phase that
// enables the end-of-list event for descriptors) can arrive pending on an
// event this handler has no business reporting.
ALLOY_TEST(xdmac_isr_ignores_events_it_never_enabled) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    g_calls = 0;

    eng::enable_complete_irq<13>(&count_calls, nullptr);
    eng::setup<13>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    eng::start<13>();
    hw_apply_enable<tag, 13>();

    hw_raise<tag, 13>(IP::fis.mask() | IP::dis.mask() | IP::lis.mask());
    eng::complete_isr<13>(nullptr);
    hw_read_clears<tag, 13>();
    ALLOY_CHECK_EQ(g_calls, 0);
    ALLOY_CHECK(!eng::complete<13>());  // still running: GS up, nothing latched
    ALLOY_CHECK(!eng::error<13>());

    eng::disable_complete_irq<13>();
}

// disable_complete_irq() must actually disarm the hardware, both halves: the
// channel's event select (CID, the write-only clear twin of CIE) and its
// permission to reach the NVIC line (GID, the twin of GIE). Leaving either set
// on a shared line means a detached channel keeps asserting a vector whose
// handler is gone.
ALLOY_TEST(xdmac_disable_complete_irq_disarms_both_halves) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;

    eng::enable_complete_irq<15>(&count_calls, nullptr);
    eng::setup<15>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    ALLOY_CHECK((eng::chreg<15>(IP::CIE_offset, IP::CIE_stride) & IP::bie.mask()) != 0u);
    ALLOY_CHECK((eng::r().GIE & IP::template ie<14>.mask) != 0u);

    eng::disable_complete_irq<15>();
    ALLOY_CHECK((eng::chreg<15>(IP::CID_offset, IP::CID_stride) & IP::bid.mask()) != 0u);
    ALLOY_CHECK((eng::chreg<15>(IP::CID_offset, IP::CID_stride) & IP::rbeid.mask()) != 0u);
    ALLOY_CHECK((eng::r().GID & IP::template id<14>.mask) != 0u);

    // And the fold is now conditional the other way: a setup() after the
    // callback is gone must arm nothing, or a channel with no handler would
    // wedge the shared line.
    eng::chreg<15>(IP::CIE_offset, IP::CIE_stride) = 0u;
    eng::r().GIE = 0u;
    eng::setup<15>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                   0x40u, 0x80u, 8u, 9u);
    ALLOY_CHECK_EQ(eng::chreg<15>(IP::CIE_offset, IP::CIE_stride), 0u);
    ALLOY_CHECK_EQ(eng::r().GIE, 0u);
}
