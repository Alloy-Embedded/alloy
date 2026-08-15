// The RP2040 DMA engine, witnessed on the host — phase 6 of
// docs/design/dma-streams.md. Sibling of test_st_dma_v1_latch.cpp,
// test_st_dma_v2_latch.cpp and test_xdmac_v1_latch.cpp, and the file that
// carries the most weight of the four.
//
// WHAT RUNS HERE IS THE REAL DRIVER. detail::raspberrypi_dma_v1_engine (split
// out of raspberrypi_dma_v1.hpp for exactly this file) is instantiated over a
// hand-written IP double whose registers are host memory laid out with the
// dma_v1 shape — the SAME offsets, strides and bit positions as
// registers/raspberrypi/dma_v1.yaml, so the sequences exercised here are the
// sequences the silicon sees. The tests drive the engine's OWN ISR and poll its
// OWN accessors.
//
// ═══ WHY THIS FILE IS THE ONLY WITNESS THIS ENGINE HAS ═════════════════════
//
// Renode 1.16.1 ships NO RP2040 peripheral of any kind — no DMA model, no
// board, no CPU platform for this part — and alloy's emitter generates none.
// There is no emulation leg for this driver and there cannot be one worth
// having: alloy CAN emit an rp2040 Renode platform (cortex-m0 and UART.PL011
// are generic types Renode has), and Renode reads 0 from unmapped addresses, so
// a DMA leg on such a platform would read CTRL as 0, read TRANS_COUNT as 0 and
// report a finished transfer over a fully-written buffer, having moved nothing.
// That is a FALSE GREEN, not weak evidence. Nobody should add the leg, and the
// reason is written here so the next reader does not add it as an obvious
// improvement.
//
// So: host double, or the board. Nothing in between.
//
// ═══ WHAT THIS FILE CANNOT PROVE, AND IT IS THE IMPORTANT PART ═════════════
//
// 1. IT CANNOT PROVE THAT CH_CTRL_TRIG TRIGGERS. That claim is marked INFERRED
//    in registers/raspberrypi/dma_v1.yaml — the vendor SVD annotates the other
//    three view-0xC registers as trigger registers and does NOT annotate this
//    one; the inference is its name, its offset and those three siblings. This
//    engine's whole configure-through-CH_AL1_CTRL discipline rests on it. What
//    the cases below witness is that the engine never ADDRESSES a trigger
//    register while configuring, which is register-sequence intent. Whether
//    addressing it would have started the channel is a hardware question.
//    THIS FAMILY'S EQUIVALENT OF XDMAC'S UNCITED DESCRIPTOR LAYOUT: if the
//    inference is wrong in the other direction — if CH_AL1_CTRL also triggers —
//    every case here still passes and the first silicon run starts a channel
//    while programming it.
// 2. IT CANNOT PROVE THE THREE DREQ IDS. A wrong TREQ_SEL is a transfer that is
//    paced by the wrong peripheral, and nothing off-target can tell one number
//    from another. §5 of the design records the request id as unwitnessed on
//    G0, unwitnessed on F7 and unwitnessable by construction on SAME70; it is
//    unwitnessed here too.
// 3. IT CANNOT PROVE THE ABORT DRAINS ANYTHING. The double models CHAN_ABORT's
//    poll-to-zero protocol (see abort_reg) so the LOOP is real and a test can
//    count the polls, but whether the hardware's in-flight transfers are
//    actually flushed by then is the SVD's claim, not this file's.
//
// Those three are exactly rows 5, 4 and 6 of
// docs/guide/rp2040-dma-hardware-checklist.md, which carries a falsifier for
// each and the firmware to run them. Nothing on that sheet has been executed,
// and no shipped example even reaches this driver — so today this file is the
// entire evidence base, and it is an evidence base about register sequences.
//
// What it DOES pin is everything a wrong line would break silently: which
// register gets which value, in which order; that configuration never touches a
// trigger; that the start is one store to MULTI_CHAN_TRIGGER; that chaining is
// disabled by naming the channel's own number rather than left at its
// trigger-channel-0 reset value; that stop() aborts and polls instead of merely
// clearing EN; that the completion latch survives its own ISR; and that the
// handler and the arm name the SAME interrupt line.
//
// ═══ THE MUTATION LEDGER ═══════════════════════════════════════════════════
//
// A double nobody has tried to break is not a witness. Twenty-three mutations
// were applied to raspberrypi_dma_v1_body.hpp one at a time — revert the single
// line that carries a claim, rebuild, run this suite, restore. TWENTY WENT RED.
// THREE STAYED GREEN, and those three are the honest edge of this instrument:
//
//   * the ISR's `err_latched = true`, and
//   * error()'s `err_latched ||` term.
//     Not a gap in the test — a property of the silicon. On this IP the failure
//     lives in CTRL's AHB_ERROR, which no read destroys and which this engine's
//     ISR deliberately does not clear (clearing it is not needed to de-assert
//     the line, and inventing a write the hardware does not require would be
//     modelling a hazard rather than the part). So err_latched is DEFENSIVE
//     here, where it is mandatory on XDMAC and load-bearing on both ST engines.
//   * moving setup()'s latch resets ABOVE its clear_flags() call.
//     Also a property of this engine: clear_flags() here only WRITES, so it
//     cannot re-latch a stale event the way XDMAC's harvesting purge can, and
//     the ordering therefore protects nothing. The comment in the engine used
//     to claim otherwise; this run is why it no longer does.
//
// Two of the twenty reds are worth naming because of HOW they fail: reverting
// either completion-latch line does not produce a failed assertion, it produces
// a HANG — channel::wait() spins on a done() that can never become true once
// the ISR has consumed the interrupt bit. That is the bug class the latch
// exists to close, reproduced.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/dma/raspberrypi_dma_v1_body.hpp"
#include "alloy_test.hpp"

namespace {

// ── THE REGISTER DOUBLE ───────────────────────────────────────────────────
//
// Hand-written (tests never read generated headers), at the real offsets.
// THREE registers are not plain memory, because plain memory would model the
// wrong protocol, and getting those three right is most of what makes this
// file a witness rather than a re-reading of the engine's own arithmetic.

template <class Tag>
struct fake_rp_ip {
    // INTR is BOTH the status register and the clear register: write-1-to-clear,
    // and there is no separate IFCR the way ST has. A plain word cannot model
    // that at all — the driver's clearing store would SET the bits it means to
    // drop. This does the real thing, and the hardware's side raises bits
    // through raise(), which is not something a driver can spell.
    struct w1c_reg {
        std::uint32_t raw;
        void operator=(std::uint32_t v) { raw = raw & ~v; }  // NOLINT: a register
        operator std::uint32_t() const { return raw; }       // NOLINT: a register
        void raise(std::uint32_t v) { raw = raw | v; }
    };
    static_assert(sizeof(w1c_reg) == 4u);

    // CHAN_ABORT is the one register the driver WRITES and then POLLS THE
    // HARDWARE to take back down. Backed by plain memory it would keep the bit
    // the driver wrote and stop() would spin forever — so the double models the
    // drain, and how many polls it takes is a knob, which is what lets a case
    // prove the loop is really a loop instead of a single read that happened to
    // see zero.
    struct abort_reg {
        std::uint32_t raw;
        static inline unsigned drain_polls = 0u;  // reads that still show the bit
        static inline unsigned reads = 0u;        // polls the driver performed
        static inline unsigned writes = 0u;

        void operator=(std::uint32_t v) {  // NOLINT: a register
            raw = v;
            reads = 0u;
            ++writes;
        }
        operator std::uint32_t() const {  // NOLINT: a register
            ++reads;
            if (reads > drain_polls) {
                const_cast<abort_reg*>(this)->raw = 0u;  // flush finished
            }
            return raw;
        }
        static void reset(unsigned polls) {
            drain_polls = polls;
            reads = 0u;
            writes = 0u;
        }
    };
    static_assert(sizeof(abort_reg) == 4u);

    struct regs {
        std::uint8_t _reserved0[1024];
        w1c_reg INTR;                      // 0x400
        alloy::rw32 INTE0;                 // 0x404
        alloy::rw32 INTF0;                 // 0x408
        alloy::rw32 INTS0;                 // 0x40C
        std::uint8_t _reserved1[4];
        alloy::rw32 INTE1;                 // 0x414
        alloy::rw32 INTF1;                 // 0x418
        alloy::rw32 INTS1;                 // 0x41C
        std::uint8_t _reserved2[16];
        alloy::rw32 MULTI_CHAN_TRIGGER;    // 0x430
        std::uint8_t _reserved3[12];
        alloy::rw32 FIFO_LEVELS;           // 0x440
        abort_reg CHAN_ABORT;              // 0x444
        alloy::rw32 N_CHANNELS;            // 0x448
    };
    static_assert(offsetof(regs, INTR) == 0x400);
    static_assert(offsetof(regs, INTS0) == 0x40C);
    static_assert(offsetof(regs, INTE1) == 0x414);
    static_assert(offsetof(regs, MULTI_CHAN_TRIGGER) == 0x430);
    static_assert(offsetof(regs, CHAN_ABORT) == 0x444);

    // The per-channel block: four registers in four alias views, 0x40 apart,
    // twelve of them. THE DOUBLE DOES NOT ALIAS THE VIEWS, and that is a
    // deliberate, stated infidelity: on silicon all four CTRL offsets are one
    // word, and a write to whichever register lands at view offset 0xC starts
    // the channel. The engine reaches these through alloy::reg_at, which hands
    // back a plain volatile word, so the double cannot execute a trigger. What
    // it CAN do — and what makes the infidelity worth having — is observe WHICH
    // ADDRESS the engine wrote, so "configuration never addresses a trigger
    // register" becomes a checkable fact instead of a comment.
    static constexpr std::uintptr_t CH_READ_ADDR_offset = 0x00;
    static constexpr unsigned CH_READ_ADDR_stride = 64u;
    static constexpr std::uintptr_t CH_WRITE_ADDR_offset = 0x04;
    static constexpr unsigned CH_WRITE_ADDR_stride = 64u;
    static constexpr std::uintptr_t CH_TRANS_COUNT_offset = 0x08;
    static constexpr unsigned CH_TRANS_COUNT_stride = 64u;
    static constexpr std::uintptr_t CH_CTRL_TRIG_offset = 0x0C;  // TRIGGER
    static constexpr unsigned CH_CTRL_TRIG_stride = 64u;
    static constexpr std::uintptr_t CH_AL1_CTRL_offset = 0x10;
    static constexpr unsigned CH_AL1_CTRL_stride = 64u;
    static constexpr std::uintptr_t CH_AL1_TRANS_COUNT_TRIG_offset = 0x1C;  // TRIGGER
    static constexpr unsigned CH_AL1_TRANS_COUNT_TRIG_stride = 64u;
    static constexpr std::uintptr_t CH_AL2_WRITE_ADDR_TRIG_offset = 0x2C;  // TRIGGER
    static constexpr unsigned CH_AL2_WRITE_ADDR_TRIG_stride = 64u;
    static constexpr std::uintptr_t CH_AL3_READ_ADDR_TRIG_offset = 0x3C;  // TRIGGER
    static constexpr unsigned CH_AL3_READ_ADDR_TRIG_stride = 64u;

    // CTRL's bits, transcribed from the curation. WRITE_ERROR/READ_ERROR are
    // write-1-to-clear and AHB_ERROR is a read-only OR of the two — semantics
    // this double cannot give a plain word (see hw_commit_ctrl_w1c).
    static constexpr alloy::raw_field en{0u, 1u};
    static constexpr alloy::raw_field high_priority{1u, 1u};
    static constexpr alloy::raw_field data_size{2u, 2u};
    static constexpr alloy::raw_field incr_read{4u, 1u};
    static constexpr alloy::raw_field incr_write{5u, 1u};
    static constexpr alloy::raw_field ring_size{6u, 4u};
    static constexpr alloy::raw_field ring_sel{10u, 1u};
    static constexpr alloy::raw_field chain_to{11u, 4u};
    static constexpr alloy::raw_field treq_sel{15u, 6u};
    static constexpr alloy::raw_field irq_quiet{21u, 1u};
    static constexpr alloy::raw_field bswap{22u, 1u};
    static constexpr alloy::raw_field sniff_en{23u, 1u};
    static constexpr alloy::raw_field busy{24u, 1u};
    static constexpr alloy::raw_field write_error{29u, 1u};
    static constexpr alloy::raw_field read_error{30u, 1u};
    static constexpr alloy::raw_field ahb_error{31u, 1u};

    static constexpr std::uint32_t data_size_byte = 0u;
    static constexpr std::uint32_t data_size_halfword = 1u;
    static constexpr std::uint32_t data_size_word = 2u;
};

// One fake instance per test tag: the engine's callback<Ch> statics are keyed
// by Inst, so a fresh tag is a fresh latch (the test_st_dma_v1_latch idiom).
//
// The vector number is 6 and not the RP2040's real DMA_IRQ_0 = 11 because the
// host harness sizes its chain table at 8 slots and alloy::irq::attach traps
// above it. Nothing here depends on the value: what the cases check is that
// irq_line_of() returns THE INSTANCE'S number for every channel — chip data,
// no grouping written into the driver.
template <class Tag>
struct fake_rp_inst {
    using ip = fake_rp_ip<Tag>;
    // 0x84C bytes of register space (channels, shared block, debug window).
    static inline std::uint32_t mem[560]{};
    static inline std::uint32_t resets_reg = 0u;
    // reset_release polls the DONE register until the bit reads 1; the double
    // says the block came out of reset immediately.
    static inline std::uint32_t reset_done = std::uint32_t{1} << 2;
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
    static inline const alloy::clock_gate gate{
        reinterpret_cast<std::uintptr_t>(&resets_reg), std::uint32_t{1} << 2,
        alloy::clock_gate::style::reset_release,
        reinterpret_cast<std::uintptr_t>(&reset_done)};
    static constexpr std::uint8_t ch_count = 12u;
    static constexpr std::uint8_t ch_first = 0u;
    static constexpr alloy::irq_line irq{6};
};

template <class Tag>
using engine = alloy::hal::detail::raspberrypi_dma_v1_engine<fake_rp_inst<Tag>>;

template <class Tag>
std::uint32_t& raw_at(std::uintptr_t offset, unsigned ch) {
    return fake_rp_inst<Tag>::mem[(offset + std::uintptr_t{64} * ch) / 4u];
}

// ── THE HARDWARE'S HALF ───────────────────────────────────────────────────

// INTS0/INTS1 are DERIVED on this IP — "INTR masked by INTEn, plus INTFn" — not
// independent state. Recomputing them here rather than letting a test poke them
// is what makes the two-line dispatch a real witness: an ISR that clears INTR
// really does take its own line down, and a channel routed only to INTE1 really
// is invisible on INTS0.
template <class Tag>
void hw_settle() {
    auto& r = engine<Tag>::r();
    const std::uint32_t raw = r.INTR;
    r.INTS0 = (raw & r.INTE0) | r.INTF0;
    r.INTS1 = (raw & r.INTE1) | r.INTF1;
}

// A channel finished (or halted on a bus error): its INTR bit rises.
template <class Tag>
void hw_raise_intr(unsigned ch) {
    engine<Tag>::r().INTR.raise(std::uint32_t{1} << ch);
    hw_settle<Tag>();
}

// A bus error: the two error bits are in CTRL, and AHB_ERROR is their OR.
// Raised on the AL1 view because that is the word the silicon's four CTRL
// offsets all name (see the double's note) and the one the engine reads.
template <class Tag>
void hw_raise_read_error(unsigned ch) {
    using IP = fake_rp_ip<Tag>;
    raw_at<Tag>(IP::CH_AL1_CTRL_offset, ch) |=
        IP::read_error.mask() | IP::ahb_error.mask();
}

// CTRL is a MIXED register: mostly ordinary read-write, but WRITE_ERROR and
// READ_ERROR are write-1-to-clear and AHB_ERROR is a read-only OR of the two.
// The engine reaches it through alloy::reg_at, which hands back a plain
// volatile word, so — unlike INTR and CHAN_ABORT, which are members of `regs`
// and carry their real protocols — the double CANNOT give this register those
// semantics. The test commits the hardware's half by hand instead, exactly as
// test_st_dma_v1_latch.cpp commits IFCR: a store that carried those bits as 1
// really does take them (and their OR) down. The direction it cannot model is
// the other one — a store carrying them as 0 must PRESERVE them on silicon,
// and here it simply overwrites — so the cases that care about that assert on
// the WORD THE ENGINE WROTE rather than on the register's later value.
template <class Tag>
void hw_commit_ctrl_w1c(unsigned ch) {
    using IP = fake_rp_ip<Tag>;
    std::uint32_t& c = raw_at<Tag>(IP::CH_AL1_CTRL_offset, ch);
    if ((c & (IP::write_error.mask() | IP::read_error.mask())) != 0u) {
        c &= ~(IP::write_error.mask() | IP::read_error.mask() |
               IP::ahb_error.mask());
    }
}

// "Does this engine offer half-buffer events at all?" — asked through a
// concept, because the members are genuinely ABSENT rather than constrained
// away, and a `requires` naming a missing member of a CONCRETE type is a hard
// error, not a false. The concept makes the engine dependent, which is the
// whole trick.
template <class E, unsigned Ch>
concept has_half_events = requires {
    E::template enable_half_irq<Ch>(nullptr, nullptr);
    E::template half<Ch>();
};

int g_calls = 0;
void count_calls(void*) { ++g_calls; }

// A child that must NOT exit cleanly: the guard under test has to fire.
// (test_claim.cpp's idiom; a trap-expecting case cannot run in-process because
// the whole suite shares one.)
template <class Fn>
bool refuses(Fn body) {
    const pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);  // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

}  // namespace

// The engine wearing the name alloy::dma::channel looks it up under, so the
// SHIPPED user-facing claim()/wait()/done()/error() run over the same
// memory-backed register file. Nothing here overrides behavior: it is the
// engine, renamed.
namespace alloy::hal {
template <class Tag>
struct dma_impl<fake_rp_inst<Tag>>
    : alloy::hal::detail::raspberrypi_dma_v1_engine<fake_rp_inst<Tag>> {};
}  // namespace alloy::hal

// ── THE CAPABILITY GAP, AS A COMPILE-TIME FACT ────────────────────────────
//
// The phase ends with a real gap by design; this is the gap spelled so it
// cannot be mistaken for coverage. Both flags are false, for DIFFERENT reasons
// (see the engine's header), and the consequence is that alloy::dma::ring does
// not EXIST on this family — a facade's ring()/rx_ring() is a compile error
// naming the capability, never a runtime surprise and never a silent hang.
ALLOY_TEST(rp_dma_v1_has_neither_a_circular_mode_nor_a_ring_nor_a_half_event) {
    struct tag {};
    using eng = engine<tag>;
    using inst = fake_rp_inst<tag>;

    static_assert(!eng::supports_circular,
                  "no circular/auto-reload bit exists in this IP, and CTRL has "
                  "ONE DATA_SIZE for both sides");
    static_assert(!eng::supports_ring,
                  "no half event exists, and a single channel HALTS at the end "
                  "of its count");
    // The half-event members are ABSENT, not stubs. A no-op stub would satisfy
    // ring_capable, compile, link, and then hang in ring::take(), which spins
    // by contract — the exact inversion of the compile-error promise.
    static_assert(!has_half_events<eng, 0>);
    static_assert(!has_half_events<eng, 11>);
    // ...and therefore the ring type is constrained away for every channel.
    static_assert(!alloy::dma::ring_capable<inst, 0>);
    static_assert(!alloy::dma::ring_capable<inst, 11>);
    // The one-shot surface, by contrast, is complete.
    static_assert(requires {
        eng::template setup<0>(eng::dir::periph_to_mem, false, eng::width::b8,
                               eng::width::b8, 0u, 0u, 1u, 0u);
        eng::template start<0>();
        eng::template stop<0>();
        eng::template complete<0>();
        eng::template error<0>();
        eng::template remaining<0>();
        eng::template clear_flags<0>();
        eng::template enable_complete_irq<0>(nullptr, nullptr);
    });
    ALLOY_CHECK(true);  // the assertions above are the test
}

// ── WITNESS (i): CONFIGURATION NEVER ADDRESSES A TRIGGER REGISTER ─────────
//
// The sequencing divergence from all three engines already in the tree. They
// configure and set the enable inside ONE register because on their silicon
// writing that register starts nothing. Here the register at view offset 0xC
// is a TRIGGER, so the control word goes in through CH_AL1_CTRL (+0x10) and the
// start is a separate store to MULTI_CHAN_TRIGGER.
ALLOY_TEST(rp_dma_v1_setup_configures_without_touching_any_trigger_register) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    constexpr unsigned kCh = 5;
    IP::abort_reg::reset(0u);

    eng::setup<kCh>(eng::dir::periph_to_mem, /*circular=*/false, eng::width::b16,
                    eng::width::b16, 0x40u, 0x80u, 32u, 36u);
    hw_commit_ctrl_w1c<tag>(kCh);

    // ALL FOUR trigger registers are untouched. Revert the engine's ctrl<Ch>()
    // to CH_CTRL_TRIG, or its trans_count<Ch>() to the AL1 view, and one of
    // these fails.
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_CTRL_TRIG_offset, kCh), 0u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_AL1_TRANS_COUNT_TRIG_offset, kCh), 0u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_AL2_WRITE_ADDR_TRIG_offset, kCh), 0u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_AL3_READ_ADDR_TRIG_offset, kCh), 0u);
    // ...and nothing was started.
    ALLOY_CHECK_EQ(eng::r().MULTI_CHAN_TRIGGER, 0u);

    // The control word did land, in the non-trigger alias, with EN set: this
    // IP's enable is part of the config, and it is the TRIGGER that starts a
    // channel, not the enable.
    const std::uint32_t ctrl = raw_at<tag>(IP::CH_AL1_CTRL_offset, kCh);
    ALLOY_CHECK((ctrl & IP::en.mask()) != 0u);
    ALLOY_CHECK_EQ(IP::data_size.read(ctrl), IP::data_size_halfword);
    ALLOY_CHECK_EQ(IP::treq_sel.read(ctrl), 36u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_TRANS_COUNT_offset, kCh), 32u);

    // THE START: one store, to MULTI_CHAN_TRIGGER, naming exactly this channel
    // — and it rewrites no configuration, which is what the trigger aliases
    // cannot do.
    eng::start<kCh>();
    ALLOY_CHECK_EQ(eng::r().MULTI_CHAN_TRIGGER, std::uint32_t{1} << kCh);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_CTRL_TRIG_offset, kCh), 0u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_AL1_CTRL_offset, kCh), ctrl);
}

// The hazard no other engine in this tree has. CHAIN_TO's reset value is 0,
// which means "trigger channel 0 on completion" — so every channel but channel
// 0 ships out of reset chained to channel 0. Chaining is disabled by naming the
// channel's OWN number, so setup(), stop() and clear_flags() all write it.
ALLOY_TEST(rp_dma_v1_every_control_write_disables_chaining_by_self_reference) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);

    eng::setup<7>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 4u, 20u);
    hw_commit_ctrl_w1c<tag>(7);
    ALLOY_CHECK_EQ(IP::chain_to.read(raw_at<tag>(IP::CH_AL1_CTRL_offset, 7)), 7u);

    // ...and the resting states too: a channel torn down with CHAIN_TO = 0
    // would fire channel 0 the next time anything triggered it.
    eng::stop<7>();
    ALLOY_CHECK_EQ(IP::chain_to.read(raw_at<tag>(IP::CH_AL1_CTRL_offset, 7)), 7u);
    eng::clear_flags<7>();
    ALLOY_CHECK_EQ(IP::chain_to.read(raw_at<tag>(IP::CH_AL1_CTRL_offset, 7)), 7u);

    // Channel 0 is the case where the safe value coincides with the reset one,
    // so it is checked separately rather than assumed.
    eng::setup<0>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 4u, 20u);
    hw_commit_ctrl_w1c<tag>(0);
    ALLOY_CHECK_EQ(IP::chain_to.read(raw_at<tag>(IP::CH_AL1_CTRL_offset, 0)), 0u);
}

// There is no DIR bit on this IP: direction IS which side increments and which
// side the request paces. Both directions, because getting one right and the
// other backwards is a transfer that runs and corrupts.
ALLOY_TEST(rp_dma_v1_direction_is_which_side_increments) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);

    // peripheral -> memory: read the fixed register, walk the buffer.
    eng::setup<1>(eng::dir::periph_to_mem, false, eng::width::b8, eng::width::b8,
                  0x4004C00Cu, 0x20001000u, 8u, 36u);
    hw_commit_ctrl_w1c<tag>(1);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_READ_ADDR_offset, 1), 0x4004C00Cu);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_WRITE_ADDR_offset, 1), 0x20001000u);
    {
        const std::uint32_t c = raw_at<tag>(IP::CH_AL1_CTRL_offset, 1);
        ALLOY_CHECK((c & IP::incr_read.mask()) == 0u);   // the register never moves
        ALLOY_CHECK((c & IP::incr_write.mask()) != 0u);  // the buffer does
    }

    // memory -> peripheral: exactly the mirror.
    eng::setup<2>(eng::dir::mem_to_periph, false, eng::width::b8, eng::width::b8,
                  0x40034000u, 0x20002000u, 8u, 20u);
    hw_commit_ctrl_w1c<tag>(2);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_READ_ADDR_offset, 2), 0x20002000u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_WRITE_ADDR_offset, 2), 0x40034000u);
    {
        const std::uint32_t c = raw_at<tag>(IP::CH_AL1_CTRL_offset, 2);
        ALLOY_CHECK((c & IP::incr_read.mask()) != 0u);
        ALLOY_CHECK((c & IP::incr_write.mask()) == 0u);
        ALLOY_CHECK_EQ(IP::treq_sel.read(c), 20u);
    }

    // remaining() is the live count register, straight — no synthesis, no
    // parity flag, because there is no ring to place a cursor in.
    raw_at<tag>(IP::CH_TRANS_COUNT_offset, 2) = 3u;
    ALLOY_CHECK_EQ(eng::remaining<2>(), 3u);
}

// ── WITNESS (ii): STOP ABORTS AND POLLS ───────────────────────────────────
//
// The fourth stop shape in this tree (§4 lists three). Clearing EN only PAUSES
// — BUSY stays high if it was high — so a teardown that stops there and hands
// the buffer back is racing hardware that is still writing into it. CHAN_ABORT
// is the abort, and the SVD's instruction is to poll it to all-zero because
// until then in-flight transfers are still draining through the FIFOs.
ALLOY_TEST(rp_dma_v1_stop_aborts_the_channel_and_polls_the_drain_to_zero) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    constexpr unsigned kCh = 3;

    IP::abort_reg::reset(0u);
    eng::setup<kCh>(eng::dir::periph_to_mem, false, eng::width::b32,
                    eng::width::b32, 0x40u, 0x80u, 16u, 36u);
    hw_commit_ctrl_w1c<tag>(kCh);
    eng::start<kCh>();

    // Three polls' worth of in-flight transfers still draining.
    IP::abort_reg::reset(3u);
    eng::stop<kCh>();

    // EN came down FIRST (through the non-trigger alias, so the teardown cannot
    // itself start anything)...
    const std::uint32_t ctrl = raw_at<tag>(IP::CH_AL1_CTRL_offset, kCh);
    ALLOY_CHECK((ctrl & IP::en.mask()) == 0u);
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_CTRL_TRIG_offset, kCh), 0u);
    // ...the abort was requested for exactly this channel...
    ALLOY_CHECK_EQ(IP::abort_reg::writes, 1u);
    // ...and the driver really POLLED, four times: three that still saw the bit
    // and the one that saw it gone. Delete the while-loop in stop() and this
    // drops to 1; delete the CHAN_ABORT write and `writes` drops to 0. Either
    // revert fails here.
    ALLOY_CHECK_EQ(IP::abort_reg::reads, 4u);

    // STOPPING MUST NOT DESTROY THE ERROR EVIDENCE, and clearing must. The
    // double cannot animate this one: CTRL is reached through alloy::reg_at,
    // which hands back a plain word, so WRITE_ERROR/READ_ERROR being
    // write-1-to-clear (a written 0 PRESERVES them) and AHB_ERROR being a
    // read-only OR are not reproducible here — unlike INTR and CHAN_ABORT,
    // which are members of `regs` and do carry their real protocols above.
    //
    // What IS checkable is the sequence intent, which is the part a wrong line
    // gets wrong: stop()'s control store carries those bits CLEAR, so on
    // silicon it leaves the failure standing for the error() that
    // channel::wait() consults AFTER done(); clear_flags()'s carries them SET,
    // so on silicon it is the one call that drops them. Read-modify-writing
    // CTRL anywhere would hand the bits it just read straight back to the
    // hardware and error() would go blind — which is why idle_ctrl() exists and
    // why every CTRL store in this engine is a whole word.
    constexpr std::uint32_t kErrW1c =
        IP::write_error.mask() | IP::read_error.mask();
    eng::stop<kCh>();
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_AL1_CTRL_offset, kCh) & kErrW1c, 0u);
    eng::clear_flags<kCh>();
    ALLOY_CHECK_EQ(raw_at<tag>(IP::CH_AL1_CTRL_offset, kCh) & kErrW1c, kErrW1c);
    hw_commit_ctrl_w1c<tag>(kCh);
}

// Two homes for a channel's status, which no other engine in this tree has:
// the interrupt bit in INTR (write-1-to-clear) and the bus-error bits in CTRL
// (separately write-1-to-clear).
ALLOY_TEST(rp_dma_v1_clear_flags_clears_both_status_homes) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);

    hw_raise_intr<tag>(4);
    hw_raise_read_error<tag>(4);
    ALLOY_CHECK(eng::complete<4>());  // from the live INTR bit, no ISR involved
    ALLOY_CHECK(eng::error<4>());     // from the live AHB_ERROR bit

    eng::clear_flags<4>();
    hw_commit_ctrl_w1c<tag>(4);
    hw_settle<tag>();
    ALLOY_CHECK(!eng::complete<4>());
    ALLOY_CHECK(!eng::error<4>());

    // ...and it named ONLY channel 4: a wider store would eat a sibling's
    // completion that rose between the read and the write.
    hw_raise_intr<tag>(4);
    hw_raise_intr<tag>(9);
    eng::clear_flags<4>();
    hw_settle<tag>();
    ALLOY_CHECK(!eng::complete<4>());
    ALLOY_CHECK(eng::complete<9>());
    eng::clear_flags<9>();
}

// ── WITNESS (iii): THE COMPLETION LATCH ───────────────────────────────────
//
// MANDATORY on this engine, exactly as on the two ST ones: complete_isr must
// clear the channel's INTR bit (the line is level-triggered off it), and
// complete() polls that same bit, so without the latch a polled wait() spins
// forever on a transfer whose interrupt already ran.
ALLOY_TEST(rp_dma_v1_completion_survives_its_own_isr_for_a_late_poller) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    constexpr unsigned kCh = 8;
    IP::abort_reg::reset(0u);
    g_calls = 0;

    eng::enable_complete_irq<kCh>(&count_calls, nullptr);
    eng::setup<kCh>(eng::dir::mem_to_periph, false, eng::width::b8,
                    eng::width::b8, 0x40u, 0x80u, 8u, 20u);
    hw_commit_ctrl_w1c<tag>(kCh);
    eng::start<kCh>();
    // Register-level witness of the FOLD: the routing bit went into INTE0 at
    // setup, because a callback was registered before it.
    ALLOY_CHECK((eng::r().INTE0 & (std::uint32_t{1} << kCh)) != 0u);
    ALLOY_CHECK(!eng::complete<kCh>());  // negative control: nothing latched yet

    // Hardware: the transfer finishes, the channel's INTR bit rises, and
    // because INTE0 routes it, INTS0 shows it and the line is taken.
    hw_raise_intr<tag>(kCh);
    ALLOY_CHECK((eng::r().INTS0 & (std::uint32_t{1} << kCh)) != 0u);
    eng::complete_isr<kCh>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 1);

    // The ISR consumed the flag — it MUST, or the line re-fires forever — and
    // the derived status register followed it down.
    hw_settle<tag>();
    ALLOY_CHECK_EQ(eng::r().INTR & (std::uint32_t{1} << kCh), 0u);
    ALLOY_CHECK_EQ(eng::r().INTS0 & (std::uint32_t{1} << kCh), 0u);

    // THE WITNESS. A poller arriving after the ISR has no hardware bit left to
    // read — complete() must report from the latch. Revert either completion
    // latch line in raspberrypi_dma_v1_body.hpp (the `latched = true` in
    // complete_isr, or the `latched ||` term in complete()) and this fails.
    ALLOY_CHECK(eng::complete<kCh>());
    // ...and a clean completion never raises error(): the control that keeps
    // the error path honest in the other direction.
    ALLOY_CHECK(!eng::error<kCh>());

    eng::disable_complete_irq<kCh>();
    // Disarming takes this channel's routing bit down and leaves the line up
    // for the other eleven.
    ALLOY_CHECK_EQ(eng::r().INTE0 & (std::uint32_t{1} << kCh), 0u);
}

// The error path, through the SHIPPED caller rather than the engine accessor:
// alloy::dma::channel::wait() ends in `return !error()`, and anchor 2.4's
// spi.transfer_dma() returns a bool sourced from exactly that call.
//
// NOTE ON THIS ENGINE'S ERROR LATCH, stated where it can be checked: unlike its
// three siblings, err_latched here is DEFENSIVE, not load-bearing. The failure
// lives in CTRL's AHB_ERROR, which no read destroys and which this ISR
// deliberately does not clear (clearing it is not needed to de-assert the
// line). So this case passes from the live bit even with the latch reverted —
// measured, and named as this engine's dead spot rather than left to be
// discovered. What it DOES pin is that the ISR consuming the completion does
// not also lose the failure, which is the bug the latch closed on ST.
ALLOY_TEST(rp_dma_v1_wait_reports_a_failure_its_own_isr_consumed) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    using inst = fake_rp_inst<tag>;
    static const std::uint8_t payload[4] = {1, 2, 3, 4};
    IP::abort_reg::reset(0u);
    g_calls = 0;

    // POSITIVE CONTROL FIRST, on its own channel: a transfer that completes
    // cleanly, whose interrupt also ran, must still wait() TRUE — otherwise
    // this test could "pass" by breaking every DMA transfer in the framework.
    {
        auto ok = alloy::dma::channel<inst, 10>::claim();
        ok.on_complete(&count_calls, nullptr);
        ok.start_m2p_u8(std::span<const std::uint8_t>(payload), 0x40u, 20u);
        hw_commit_ctrl_w1c<tag>(10);
        hw_raise_intr<tag>(10);
        eng::complete_isr<10>(nullptr);
        hw_settle<tag>();
        ALLOY_CHECK(ok.wait());
        ok.clear_on_complete();
    }

    // THE CASE. Same shape, but the channel halts on a read bus error — which
    // on this IP raises the SAME INTR bit a completion does (one bit, both
    // outcomes; there is no separate error enable to arm).
    {
        auto bad = alloy::dma::channel<inst, 11>::claim();
        bad.on_complete(&count_calls, nullptr);
        bad.start_m2p_u8(std::span<const std::uint8_t>(payload), 0x40u, 20u);
        hw_commit_ctrl_w1c<tag>(11);
        hw_raise_read_error<tag>(11);
        hw_raise_intr<tag>(11);
        eng::complete_isr<11>(nullptr);
        hw_settle<tag>();

        // The shipped answer FIRST, so that it is this check — the one a
        // caller's `bool ok` comes from — that trips, not a diagnostic below.
        ALLOY_CHECK(!bad.wait());
        ALLOY_CHECK(bad.done());   // the transfer is over...
        ALLOY_CHECK(bad.error());  // ...and it failed.
        bad.clear_on_complete();
    }
    ALLOY_CHECK_EQ(g_calls, 2);
}

ALLOY_TEST(rp_dma_v1_setup_resets_stale_latches) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);
    g_calls = 0;

    eng::enable_complete_irq<6>(&count_calls, nullptr);
    eng::setup<6>(eng::dir::periph_to_mem, false, eng::width::b16,
                  eng::width::b16, 0x40u, 0x80u, 8u, 36u);
    hw_commit_ctrl_w1c<tag>(6);

    // The previous transfer failed and its interrupt ran: both latches are up.
    hw_raise_read_error<tag>(6);
    hw_raise_intr<tag>(6);
    eng::complete_isr<6>(nullptr);
    hw_settle<tag>();
    ALLOY_CHECK(eng::complete<6>());
    ALLOY_CHECK(eng::error<6>());

    // Programming a NEW transfer must forget them: it is not already complete,
    // and — the reason the error latch is transfer-scoped rather than sticky —
    // it has not failed because its predecessor did.
    eng::setup<6>(eng::dir::periph_to_mem, false, eng::width::b16,
                  eng::width::b16, 0x40u, 0x80u, 8u, 36u);
    hw_commit_ctrl_w1c<tag>(6);
    hw_settle<tag>();
    ALLOY_CHECK(!eng::complete<6>());
    ALLOY_CHECK(!eng::error<6>());

    eng::disable_complete_irq<6>();
}

// ── THE TWO-LINE DISPATCH ─────────────────────────────────────────────────
//
// This block raises TWO vectors, and which channel reaches which one is a
// RUNTIME SOFTWARE CHOICE (bit n of INTE0, or INTE1, or both, or neither) —
// not silicon geometry, which is why the chip data binds one vector and carries
// no `irq_lines` ranges. The driver's policy is kIrqLine, and it must be the
// same line in all three places it appears: the arm writes INTE_n, the handler
// guards on INTS_n, and the vector is the instance's `irq`. If the arm and the
// guard ever name different lines, the handler simply never runs.
ALLOY_TEST(rp_dma_v1_the_handler_guards_the_same_line_the_arm_routed_to) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    constexpr unsigned kCh = 2;
    IP::abort_reg::reset(0u);
    g_calls = 0;

    eng::enable_complete_irq<kCh>(&count_calls, nullptr);
    eng::setup<kCh>(eng::dir::periph_to_mem, false, eng::width::b8,
                    eng::width::b8, 0x40u, 0x80u, 8u, 36u);
    hw_commit_ctrl_w1c<tag>(kCh);

    // The arm touched line 0 and ONLY line 0. A driver that routed to INTE1
    // while guarding INTS0 would leave the interrupt undeliverable.
    ALLOY_CHECK_EQ(eng::r().INTE0, std::uint32_t{1} << kCh);
    ALLOY_CHECK_EQ(eng::r().INTE1, 0u);

    // Now the adversarial half: another agent routes THIS channel to the SECOND
    // line as well, and a transfer finishes. INTS1 shows it; INTS0 does not,
    // because we then remove the first-line routing. Our handler is line 0's,
    // so it must return having touched nothing — no clear (which would consume
    // the other line's event), no callback, no latch.
    eng::r().INTE0 = 0u;
    eng::r().INTE1 = std::uint32_t{1} << kCh;
    hw_raise_intr<tag>(kCh);
    ALLOY_CHECK_EQ(eng::r().INTS0 & (std::uint32_t{1} << kCh), 0u);
    ALLOY_CHECK((eng::r().INTS1 & (std::uint32_t{1} << kCh)) != 0u);

    eng::complete_isr<kCh>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 0);
    // The raw bit is untouched — the other line's handler will still find it.
    ALLOY_CHECK((eng::r().INTR & (std::uint32_t{1} << kCh)) != 0u);
    // Change the ISR's guard from INTS0 to INTR and this check fails: the raw
    // register does not know about routing, so a guard on it consumes every
    // channel's completion regardless of which line it belongs to.

    eng::r().INTE1 = 0u;
    eng::clear_flags<kCh>();
    hw_settle<tag>();
    eng::disable_complete_irq<kCh>();
}

ALLOY_TEST(rp_dma_v1_isr_is_a_no_op_for_a_foreign_channels_interrupt) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);
    g_calls = 0;

    eng::enable_complete_irq<1>(&count_calls, nullptr);
    eng::setup<1>(eng::dir::periph_to_mem, false, eng::width::b8, eng::width::b8,
                  0x40u, 0x80u, 8u, 36u);
    hw_commit_ctrl_w1c<tag>(1);
    // A sibling channel is routed to the same line and finishes; ours has not.
    eng::r().INTE0 = eng::r().INTE0 | (std::uint32_t{1} << 9);
    hw_raise_intr<tag>(9);

    eng::complete_isr<1>(nullptr);
    ALLOY_CHECK_EQ(g_calls, 0);
    ALLOY_CHECK(!eng::complete<1>());
    ALLOY_CHECK(!eng::error<1>());
    // ...and the sibling's event is still there for its own handler. A blanket
    // clear here would have eaten it.
    ALLOY_CHECK((eng::r().INTR & (std::uint32_t{1} << 9)) != 0u);

    eng::clear_flags<9>();
    hw_settle<tag>();
    eng::disable_complete_irq<1>();
}

// A polled-only consumer registers no callback, so nothing may be routed to a
// vector: an INTE bit with no handler attached wedges the line.
ALLOY_TEST(rp_dma_v1_setup_without_a_callback_routes_nothing_to_a_vector) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);

    eng::setup<4>(eng::dir::periph_to_mem, false, eng::width::b32,
                  eng::width::b32, 0x40u, 0x80u, 8u, 36u);
    hw_commit_ctrl_w1c<tag>(4);
    ALLOY_CHECK_EQ(eng::r().INTE0, 0u);
    ALLOY_CHECK_EQ(eng::r().INTE1, 0u);

    // ...and the polled path still works, off the raw INTR bit, which rises
    // whatever the routing says.
    hw_raise_intr<tag>(4);
    ALLOY_CHECK(eng::complete<4>());
    eng::clear_flags<4>();
    hw_settle<tag>();
}

// The vector comes from CHIP DATA and is the same for every channel: there is
// no grouping to look up on this part, because the channel-to-line map is a
// pair of mask registers rather than silicon geometry. st_dma_v1_body.hpp's
// per-member hardcode is named as a defect twice in this tree and is not
// repeated here.
ALLOY_TEST(rp_dma_v1_the_vector_is_chip_data_and_carries_no_grouping) {
    struct tag {};
    using eng = engine<tag>;
    using inst = fake_rp_inst<tag>;

    static_assert(eng::template irq_line_of<0>().number == inst::irq.number);
    static_assert(eng::template irq_line_of<1>().number == inst::irq.number);
    static_assert(eng::template irq_line_of<11>().number == inst::irq.number);
    ALLOY_CHECK_EQ(eng::template irq_line_of<7>().number, inst::irq.number);
}

// ── THE RUNTIME REFUSALS ──────────────────────────────────────────────────
//
// Both traps guard a physical impossibility rather than a policy, and both run
// in a forked child because the suite shares one process.
ALLOY_TEST(rp_dma_v1_refuses_what_this_silicon_cannot_do) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_rp_ip<tag>;
    IP::abort_reg::reset(0u);

    // THE NEGATIVE CONTROL for the idiom, first: a legal setup must NOT trap,
    // or every refusal below would "pass" for the wrong reason.
    ALLOY_CHECK(!refuses([] {
        engine<tag>::template setup<0>(engine<tag>::dir::periph_to_mem, false,
                                       engine<tag>::width::b16,
                                       engine<tag>::width::b16, 0x40u, 0x80u, 8u,
                                       36u);
    }));

    // ONE DATA_SIZE for both sides of a channel — the same limitation XDMAC's
    // single DWIDTH has. A caller asking for 16-bit memory items into 32-bit
    // register writes is asking for something this IP cannot express.
    ALLOY_CHECK(refuses([] {
        engine<tag>::template setup<0>(engine<tag>::dir::mem_to_periph, false,
                                       engine<tag>::width::b16,
                                       engine<tag>::width::b32, 0x40u, 0x80u, 8u,
                                       20u);
    }));

    // There is no circular mode in this IP at all. Unreachable through the
    // shipped surface (both capability flags are false, so neither
    // start_m2p_circular_u16 nor alloy::dma::ring exists here), but the
    // parameter is in the portable signature and a hand-wired caller can still
    // pass it — a channel programmed as if it were circular would run once and
    // stop, silently.
    ALLOY_CHECK(refuses([] {
        engine<tag>::template setup<0>(engine<tag>::dir::periph_to_mem,
                                       /*circular=*/true, engine<tag>::width::b8,
                                       engine<tag>::width::b8, 0x40u, 0x80u, 8u,
                                       36u);
    }));
    ALLOY_CHECK_EQ(eng::r().MULTI_CHAN_TRIGGER, 0u);
}
