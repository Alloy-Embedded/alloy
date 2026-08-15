// The SAM E70 XDMAC linked-list RING, witnessed on the host — phase 5b of
// docs/design/dma-streams.md. Sibling of test_xdmac_v1_latch.cpp, which
// witnesses the completion latch this file builds on; read its header for the
// register double's shape and for why CIS being clear-on-read makes the latch
// mandatory rather than defensive.
//
// WHAT RUNS HERE IS THE REAL ENGINE. detail::microchip_xdmac_v1_engine is
// instantiated over a hand-written IP double, and the double's "hardware" half
// actually FETCHES THE DESCRIPTORS OUT OF HOST MEMORY AND EXECUTES THEM: it
// follows MBR_NDA from one descriptor to the next, loads CUBC from MBR_UBC,
// writes items to MBR_TA, and raises the end-of-block interrupt when the
// microblock runs out. That is what makes the linkage assertions below real
// rather than a re-reading of the engine's own arithmetic — if the engine links
// desc[0] to anywhere but desc[1], the model cannot find the next descriptor and
// the test fails at the fetch.
//
// ═══ WHAT THIS FILE CANNOT PROVE, AND IT IS THE IMPORTANT PART ═══
//
// It cannot prove the DESCRIPTOR LAYOUT. The model reads MBR_NDA / MBR_UBC /
// MBR_TA out of the same `view0_descriptor` struct the engine writes, with the
// same `ubc_*` bit positions, so model and firmware agree BY CONSTRUCTION. That
// layout is curated in no file in either repo (the pinned SVD-derived upstream
// carries every XDMAC register bit and zero descriptor-structure content), so
// it is a datasheet reading held in the driver's header comment and nowhere
// else. If the reading is wrong, every case below still passes and the first
// silicon run takes a bus error or writes to a wild address.
//
// The design's own honesty boundary, one notch worse on this family: on G0 and
// F7 a route is witnessed BY HALVES, because the platform wire and the firmware
// route descend from a common board.json statement while the REQUEST id is
// unwitnessed. Here even the emulation half is missing — Renode 1.16.1 ships no
// XDMAC model of any kind — so the ring has no leg at all, and this file plus a
// datasheet confirmation is the entire evidence base. What it DOES pin is
// everything downstream of the layout: which registers get which values in
// which order, the ping-pong parity, the cursor arithmetic across a half
// boundary, and teardown. Those are the parts a wrong line would break
// silently; the layout is the part that breaks loudly, on hardware, once.

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
// GIS/GE/GD/GS are declared writable — the test is the hardware here.
template <class Tag>
struct fake_xdmac_ip {
    struct regs {
        alloy::rw32 GTYPE;  // 0x00
        alloy::rw32 GCFG;   // 0x04
        alloy::rw32 GWAC;   // 0x08
        alloy::rw32 GIE;    // 0x0C
        alloy::rw32 GID;    // 0x10
        alloy::rw32 GIM;    // 0x14
        alloy::rw32 GIS;    // 0x18
        alloy::rw32 GE;     // 0x1C
        alloy::rw32 GD;     // 0x20
        alloy::rw32 GS;     // 0x24
    };

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

    static constexpr alloy::raw_field bie{0u, 1u};
    static constexpr alloy::raw_field rbie{4u, 1u};
    static constexpr alloy::raw_field wbie{5u, 1u};
    static constexpr alloy::raw_field roie{6u, 1u};
    static constexpr alloy::raw_field bid{0u, 1u};
    static constexpr alloy::raw_field rbeid{4u, 1u};
    static constexpr alloy::raw_field wbeid{5u, 1u};
    static constexpr alloy::raw_field roid{6u, 1u};
    static constexpr alloy::raw_field bis{0u, 1u};
    static constexpr alloy::raw_field lis{1u, 1u};
    static constexpr alloy::raw_field dis{2u, 1u};
    static constexpr alloy::raw_field fis{3u, 1u};
    static constexpr alloy::raw_field rbeis{4u, 1u};
    static constexpr alloy::raw_field wbeis{5u, 1u};
    static constexpr alloy::raw_field rois{6u, 1u};

    // The descriptor-list REGISTERS. Their NDE/NDSUP/NDDUP/NDVIEW sit at the
    // BOTTOM of CNDC; the view-0 descriptor's own UBC word carries the same
    // four names ABOVE its 24-bit length. Two of the cases below exist purely
    // to pin that the engine never confuses them.
    static constexpr alloy::raw_field ndaif{0u, 1u};
    static constexpr alloy::raw_field nda{2u, 30u};
    static constexpr alloy::raw_field nde{0u, 1u};
    static constexpr alloy::raw_field ndsup{1u, 1u};
    static constexpr alloy::raw_field nddup{2u, 1u};
    static constexpr alloy::raw_field ndview{3u, 2u};
    static constexpr alloy::raw_field ublen{0u, 24u};

    static constexpr alloy::raw_field type{0u, 1u};
    static constexpr alloy::raw_field dsync{4u, 1u};
    static constexpr alloy::raw_field dwidth{11u, 2u};
    static constexpr alloy::raw_field sif{13u, 1u};
    static constexpr alloy::raw_field dif{14u, 1u};
    static constexpr alloy::raw_field sam{16u, 2u};
    static constexpr alloy::raw_field dam{18u, 2u};
    static constexpr alloy::raw_field perid{24u, 7u};
};

// One fake instance per tag: the engine's per-channel statics are keyed by
// Inst, so a fresh tag is a fresh set of latches, descriptors and claims.
template <class Tag>
struct fake_xdmac_inst {
    using ip = fake_xdmac_ip<Tag>;
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

}  // namespace

// The engine wearing the name alloy::dma::ring looks it up under, so the
// SHIPPED ring<T> — its take()/missed(), its cursor()/readable()/consume(),
// its destructor — runs over this register file unchanged.
namespace alloy::hal {
template <class Tag>
struct dma_impl<fake_xdmac_inst<Tag>>
    : alloy::hal::detail::microchip_xdmac_v1_engine<fake_xdmac_inst<Tag>> {};
}  // namespace alloy::hal

namespace {

// ── THE HARDWARE'S HALF: a descriptor-fetching XDMAC ─────────────────────
//
// Enough of the engine's silicon behavior to run a ring: latch GE into GS,
// fetch the descriptor CNDA points at, move items to MBR_TA counting CUBC down,
// and at each block end raise BIS, follow MBR_NDA, reload, and run the ISR.
//
// Two decoders do the load-bearing work. `which_desc` and `which_half` turn a
// 32-bit register value back into an object, and they do it by COMPARING
// against the addresses the test knows are correct — so an engine that linked
// the list to the wrong place, or pointed a descriptor at the wrong half of the
// buffer, cannot be decoded and the case fails at the fetch instead of quietly
// running on the test's assumptions. (A host pointer does not fit in 32 bits;
// the engine truncates, which is exact on the target, and comparing truncated
// values is what the double can honestly do.)
template <class Tag, unsigned Ch, class T>
struct fake_xdmac_hw {
    using eng = engine<Tag>;
    using IP = fake_xdmac_ip<Tag>;
    using desc_t = typename eng::view0_descriptor;

    std::span<T> buf;
    // Where a descriptor that decoded to nothing writes; see advance().
    static inline T scratch[64]{};
    int cur = -1;            // which descriptor is executing; -1 = idle
    std::uint32_t pos = 0;   // items done within the current microblock
    T next_value{};          // the pattern written into the buffer

    static desc_t* descs() { return eng::template ring_state<Ch>::desc; }

    static alloy::rw32& creg(std::uintptr_t off, unsigned stride) {
        return eng::template chreg<Ch>(off, stride);
    }

    [[nodiscard]] int which_desc(std::uint32_t a) const {
        if (a == eng::addr32(&descs()[0])) {
            return 0;
        }
        if (a == eng::addr32(&descs()[1])) {
            return 1;
        }
        return -1;  // the engine linked the list somewhere we do not own
    }

    [[nodiscard]] T* which_half(std::uint32_t ta) const {
        if (ta == eng::addr32(buf.data())) {
            return buf.data();
        }
        if (ta == eng::addr32(buf.data() + buf.size() / 2u)) {
            return buf.data() + buf.size() / 2u;
        }
        return nullptr;  // a descriptor pointing outside the caller's buffer
    }

    void load(int d) {
        cur = d;
        pos = 0;
        creg(IP::CUBC_offset, IP::CUBC_stride) =
            descs()[d].ubc & eng::ubc_ublen_mask;
        // CNDA reads back as the address of the descriptor to fetch NEXT.
        creg(IP::CNDA_offset, IP::CNDA_stride) = descs()[d].nda;
    }

    // GE was written: the channel goes busy and the first descriptor is fetched
    // from wherever CNDA points.
    void start() {
        ALLOY_CHECK((eng::r().GE & IP::template en<Ch - 1>.mask) != 0u);
        eng::r().GE = 0u;
        eng::r().GS = eng::r().GS | IP::template st<Ch - 1>.mask;
        const int first = which_desc(creg(IP::CNDA_offset, IP::CNDA_stride));
        ALLOY_CHECK_EQ(first, 0);  // desc[0] runs first — the ring's parity base
        load(first);
    }

    // Move `n` items. Returns how many block ends (half boundaries) happened.
    int advance(unsigned n) {
        int ends = 0;
        for (unsigned i = 0; i < n; ++i) {
            ALLOY_CHECK(cur >= 0);
            const std::uint32_t ublen = descs()[cur].ubc & eng::ubc_ublen_mask;
            T* dst = which_half(descs()[cur].ta);
            // A descriptor pointing outside the caller's buffer is the shape a
            // stride or linkage mistake takes, and getting the FAILURE MODE
            // right here mattered more than it looks. ALLOY_CHECK records and
            // returns; it does not abort. Writing through the null decode
            // killed the process before the assertion that had already fired
            // could reach the log, and bailing out of advance() instead left
            // ring<T>::take() spinning forever on boundaries that never came —
            // a crash and a hang, neither of them a readable red. So the model
            // keeps running into a scratch sink and lets the assertions speak.
            ALLOY_CHECK(dst != nullptr);
            if (dst == nullptr) {
                dst = scratch;
            }
            dst[pos] = next_value;
            next_value = static_cast<T>(next_value + T{1});
            ++pos;
            creg(IP::CUBC_offset, IP::CUBC_stride) = ublen - pos;
            if (pos == ublen) {
                // End of block. The status bit rises, the channel STAYS
                // ENABLED (NDE said there is another descriptor), the next one
                // is fetched, and the shared line is taken.
                eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) =
                    eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) |
                    IP::bis.mask();
                eng::r().GIS = eng::r().GIS | IP::template is<Ch - 1>.mask;
                const int nxt = which_desc(descs()[cur].nda);
                ALLOY_CHECK(nxt >= 0);
                load(nxt);
                eng::template complete_isr<Ch>(nullptr);
                // The read the ISR performed IS the clear, committed here.
                eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) = 0u;
                eng::r().GIS = eng::r().GIS & ~IP::template is<Ch - 1>.mask;
                ++ends;
            }
        }
        return ends;
    }

    // A bus error: it lands in CIS with no BIS beside it, and the line is taken.
    void raise_error() {
        eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) =
            eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) |
            IP::rbeis.mask();
        eng::r().GIS = eng::r().GIS | IP::template is<Ch - 1>.mask;
        eng::template complete_isr<Ch>(nullptr);
        eng::template chreg<Ch>(IP::CIS_offset, IP::CIS_stride) = 0u;
        eng::r().GIS = eng::r().GIS & ~IP::template is<Ch - 1>.mask;
    }

    // GD was written: the engine's stop() spins on GS until this happens.
    static void apply_disable() {
        if ((eng::r().GD & IP::template di<Ch - 1>.mask) != 0u) {
            eng::r().GS = eng::r().GS & ~IP::template st<Ch - 1>.mask;
            eng::r().GD = 0u;
        }
    }

    // Bring the channel to rest BEFORE handing control to code that stops it.
    // The engine's stop() writes GD and then SPINS on GS, and a host double
    // cannot answer a register write from inside somebody else's busy-wait —
    // so the model is stepped one beat early and the engine's own GD write then
    // finds a channel that has already come to rest. (Skip this and the ring
    // destructor hangs, which is how this comment came to be written.)
    static void settle() {
        eng::r().GD = IP::template di<Ch - 1>.mask;
        apply_disable();
    }
};

int g_half_calls = 0;
int g_full_calls = 0;
void count_half(void*) { ++g_half_calls; }
void count_full(void*) { ++g_full_calls; }

}  // namespace

// ── 1. THE DESCRIPTOR LIST: linkage, and the bits it is spelled in ───────

ALLOY_TEST(xdmac_ring_links_two_view0_descriptors_into_a_loop) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 3;
    static std::uint16_t buf[8]{};

    eng::setup<Ch>(eng::dir::periph_to_mem, /*circular=*/true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 8u, 35u);

    auto* d = eng::ring_state<Ch>::desc;

    // THE LOOP. Each descriptor points at the other, and nothing points at
    // zero — that is the whole of "circular" on an IP with no circular bit.
    ALLOY_CHECK_EQ(d[0].nda, eng::addr32(&d[1]));
    ALLOY_CHECK_EQ(d[1].nda, eng::addr32(&d[0]));

    // THE HALVES. desc[0] covers items [0,4), desc[1] items [4,8) — and the
    // second address is scaled by the TRANSFER WIDTH, not by the item count,
    // which is the one arithmetic slip that would still look plausible.
    ALLOY_CHECK_EQ(d[0].ta, eng::addr32(&buf[0]));
    ALLOY_CHECK_EQ(d[1].ta, eng::addr32(&buf[4]));

    // THE CONTROL WORD, asserted as a whole value rather than bit by bit, so
    // that a stray extra bit fails too. Length in the low 24, NDE at 24 (there
    // IS a next descriptor), NDDUP at 26 (the next fetch updates the
    // DESTINATION — this is peripheral-to-memory, so memory is what moves),
    // NDSUP clear (the peripheral register never moves), NVIEW = 0 at 27.
    const std::uint32_t expect_ubc =
        4u | (std::uint32_t{1} << 24u) | (std::uint32_t{1} << 26u);
    ALLOY_CHECK_EQ(d[0].ubc, expect_ubc);
    ALLOY_CHECK_EQ(d[1].ubc, expect_ubc);
    // ...and the trap that makes the struct exist, stated as the two halves of
    // that word rather than as a "these bits are clear" negative — because the
    // negative CANNOT be written here, and finding that out is worth the four
    // lines. IP::nddup is bit 2 of CNDC, and the microblock length is 4, which
    // IS bit 2. So a descriptor built out of the register's fields would carry
    // 4|1|4 = 5 and no bitwise probe of the low bits could tell that apart from
    // a length that happens to look like flags. The whole-word check above is
    // the only guard, and these two say what it is guarding: the control bits
    // live ABOVE the length, and the length is only a length.
    ALLOY_CHECK_EQ(d[0].ubc >> 24u, 0b101u);  // NDE at 24, NVIEW 0, NDDUP at 26
    ALLOY_CHECK_EQ(d[0].ubc & eng::ubc_ublen_mask, 4u);

    // THE REGISTERS THAT POINT AT THE LIST, in their OWN bit positions.
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CNDA_offset, IP::CNDA_stride),
                   eng::addr32(&d[0]));
    // NDAIF = 0: the fetch goes through the same interface as memory.
    ALLOY_CHECK_EQ(
        eng::chreg<Ch>(IP::CNDA_offset, IP::CNDA_stride) & IP::ndaif.mask(), 0u);
    const std::uint32_t cndc = eng::chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride);
    ALLOY_CHECK((cndc & IP::nde.mask()) != 0u);
    ALLOY_CHECK((cndc & IP::nddup.mask()) != 0u);
    ALLOY_CHECK_EQ(cndc & IP::ndsup.mask(), 0u);
    ALLOY_CHECK_EQ((cndc & IP::ndview.mask()) >> IP::ndview.pos, 0u);  // view 0

    // CUBC is left to the fetch; CBC says one microblock per block.
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride), 0u);
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CBC_offset, IP::CBC_stride), 0u);
    // The peripheral side is programmed the ordinary way and does not move.
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CSA_offset, IP::CSA_stride), 0x40u);
    // The request rode into PERID exactly as it does for a one-shot.
    ALLOY_CHECK_EQ((eng::chreg<Ch>(IP::CC_offset, IP::CC_stride) & IP::perid.mask()) >>
                       IP::perid.pos,
                   35u);
}

// The mirror direction. Memory-to-peripheral moves the SOURCE, so the update
// bit swaps in BOTH the descriptor word and CNDC — two places, one decision,
// and this is the case that fails if they ever disagree.
ALLOY_TEST(xdmac_ring_m2p_updates_the_source_side_instead) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 1;
    static std::uint8_t buf[6]{};

    eng::setup<Ch>(eng::dir::mem_to_periph, /*circular=*/true, eng::width::b8,
                   eng::width::b8, 0x1Cu,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 6u, 9u);

    auto* d = eng::ring_state<Ch>::desc;
    const std::uint32_t expect_ubc =
        3u | (std::uint32_t{1} << 24u) | (std::uint32_t{1} << 25u);  // NDSUP
    ALLOY_CHECK_EQ(d[0].ubc, expect_ubc);
    ALLOY_CHECK_EQ(d[1].ta, eng::addr32(&buf[3]));  // byte items: stride 1
    const std::uint32_t cndc = eng::chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride);
    ALLOY_CHECK((cndc & IP::ndsup.mask()) != 0u);
    ALLOY_CHECK_EQ(cndc & IP::nddup.mask(), 0u);
    // The fixed side is the peripheral, and for m2p that is the DESTINATION.
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CDA_offset, IP::CDA_stride), 0x1Cu);
}

// ── 2. THE PING-PONG: one hardware event, two boundaries ─────────────────

ALLOY_TEST(xdmac_ring_block_end_alternates_half_and_full) {
    struct tag {};
    using eng = engine<tag>;
    constexpr unsigned Ch = 4;
    static std::uint16_t buf[8]{};
    g_half_calls = 0;
    g_full_calls = 0;

    eng::enable_half_irq<Ch>(&count_half, nullptr);
    eng::enable_complete_irq<Ch>(&count_full, nullptr);
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 8u, 35u);
    eng::start<Ch>();

    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{std::span<std::uint16_t>{buf}};
    hw.start();

    // desc[0] is live: the FIRST boundary is the first half going stable.
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::live, 0u);
    ALLOY_CHECK_EQ(hw.advance(4), 1);
    ALLOY_CHECK_EQ(g_half_calls, 1);
    ALLOY_CHECK_EQ(g_full_calls, 0);
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::live, 1u);
    ALLOY_CHECK(eng::half<Ch>());

    // desc[1] is live: the SECOND boundary is the wrap.
    ALLOY_CHECK_EQ(hw.advance(4), 1);
    ALLOY_CHECK_EQ(g_half_calls, 1);
    ALLOY_CHECK_EQ(g_full_calls, 1);
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::live, 0u);

    // ...and it keeps alternating, which is what "the list has no end" means.
    ALLOY_CHECK_EQ(hw.advance(8), 2);
    ALLOY_CHECK_EQ(g_half_calls, 2);
    ALLOY_CHECK_EQ(g_full_calls, 2);

    // THE CHANNEL NEVER STOPPED, and that is the state that makes the
    // completion latch load-bearing rather than belt-and-braces: GS still says
    // busy, so complete() has no second source and answers from the latch the
    // ISR filled — the case test_xdmac_v1_latch.cpp said it was pinning for
    // this phase.
    ALLOY_CHECK((eng::r().GS & fake_xdmac_ip<tag>::template st<Ch - 1>.mask) != 0u);
    ALLOY_CHECK(eng::complete<Ch>());
    ALLOY_CHECK(!eng::error<Ch>());

    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    eng::stop<Ch>();
    eng::disable_half_irq<Ch>();
    eng::disable_complete_irq<Ch>();
}

// One hardware event means ONE handler on the shared line, and the two enables
// must not fight over it: alloy::irq::attach TRAPS on a duplicate, so an engine
// that attached from both would have died at construction on every ring ever
// built. Disarming one half must also leave the other's BIE alone — they ARE
// the same bit.
ALLOY_TEST(xdmac_ring_half_and_full_share_one_handler_and_one_enable_bit) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 7;
    static std::uint16_t buf[4]{};
    g_half_calls = 0;
    g_full_calls = 0;

    // Both armed, then setup: BIE is on and the channel can reach the line.
    eng::enable_half_irq<Ch>(&count_half, nullptr);
    eng::enable_complete_irq<Ch>(&count_full, nullptr);
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 4u, 35u);
    ALLOY_CHECK((eng::chreg<Ch>(IP::CIE_offset, IP::CIE_stride) & IP::bie.mask()) !=
                0u);
    ALLOY_CHECK((eng::r().GIE & IP::template ie<Ch - 1>.mask) != 0u);

    // Drop the FULL callback. The half event still has to be delivered, so
    // nothing may be disarmed: the CID/GID writes must not have happened.
    eng::chreg<Ch>(IP::CID_offset, IP::CID_stride) = 0u;
    eng::r().GID = 0u;
    eng::disable_complete_irq<Ch>();
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CID_offset, IP::CID_stride), 0u);
    ALLOY_CHECK_EQ(eng::r().GID, 0u);

    eng::start<Ch>();
    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{std::span<std::uint16_t>{buf}};
    hw.start();
    ALLOY_CHECK_EQ(hw.advance(2), 1);
    ALLOY_CHECK_EQ(g_half_calls, 1);  // still delivered with `fn` gone
    ALLOY_CHECK_EQ(g_full_calls, 0);

    // Now drop the half callback too. NOTHING is armed, so the channel stops
    // being an interrupt source and the handler comes off the chain.
    eng::disable_half_irq<Ch>();
    ALLOY_CHECK((eng::chreg<Ch>(IP::CID_offset, IP::CID_stride) & IP::bid.mask()) !=
                0u);
    ALLOY_CHECK((eng::r().GID & IP::template id<Ch - 1>.mask) != 0u);

    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    eng::stop<Ch>();
}

// setup() drops the half latch with the other two, and the ordering rule is the
// same one the completion latch already lives under: the resets go AFTER the
// flag purge, because on this IP the purge HARVESTS and would otherwise re-latch
// the previous transfer's block end into the fresh one. A ring that reported a
// stable half before the engine had written a byte would hand the consumer the
// last stream's data.
ALLOY_TEST(xdmac_ring_setup_drops_a_stale_half_latch) {
    struct tag {};
    using eng = engine<tag>;
    constexpr unsigned Ch = 21;
    static std::uint16_t buf[4]{};
    g_half_calls = 0;
    g_full_calls = 0;

    eng::enable_half_irq<Ch>(&count_half, nullptr);
    eng::enable_complete_irq<Ch>(&count_full, nullptr);
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 4u, 35u);
    eng::start<Ch>();
    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{std::span<std::uint16_t>{buf}};
    hw.start();
    hw.advance(2);
    ALLOY_CHECK(eng::half<Ch>());

    // Tear the stream down and start a new one over the same channel.
    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    eng::stop<Ch>();
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 4u, 35u);
    ALLOY_CHECK(!eng::half<Ch>());

    eng::disable_half_irq<Ch>();
    eng::disable_complete_irq<Ch>();
}

// BIE is armed for whoever wants the event, and on this IP the two callbacks
// share it. A ring registered for HALF boundaries only — no completion callback
// at all — must still be an interrupt source, or its every boundary is lost and
// take() spins forever on a channel that is working perfectly.
ALLOY_TEST(xdmac_a_half_only_consumer_still_arms_the_block_interrupt) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 23;
    static std::uint16_t buf[4]{};
    g_half_calls = 0;

    eng::enable_half_irq<Ch>(&count_half, nullptr);
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 4u, 35u);
    ALLOY_CHECK((eng::chreg<Ch>(IP::CIE_offset, IP::CIE_stride) & IP::bie.mask()) !=
                0u);
    ALLOY_CHECK((eng::r().GIE & IP::template ie<Ch - 1>.mask) != 0u);

    eng::start<Ch>();
    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{std::span<std::uint16_t>{buf}};
    hw.start();
    hw.advance(2);
    ALLOY_CHECK_EQ(g_half_calls, 1);

    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    eng::stop<Ch>();
    eng::disable_half_irq<Ch>();
}

// An error while ringing is LATCHED and DISPATCHES NOTHING. Calling the full
// callback for it would move alloy::dma::ring's wrap counter — and therefore
// its cursor — a whole buffer forward on the strength of an event that moved no
// data at all.
ALLOY_TEST(xdmac_ring_error_only_event_latches_but_moves_no_boundary) {
    struct tag {};
    using eng = engine<tag>;
    constexpr unsigned Ch = 9;
    static std::uint16_t buf[4]{};
    g_half_calls = 0;
    g_full_calls = 0;

    eng::enable_half_irq<Ch>(&count_half, nullptr);
    eng::enable_complete_irq<Ch>(&count_full, nullptr);
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 4u, 35u);
    eng::start<Ch>();
    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{std::span<std::uint16_t>{buf}};
    hw.start();

    hw.raise_error();
    ALLOY_CHECK_EQ(g_half_calls, 0);
    ALLOY_CHECK_EQ(g_full_calls, 0);
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::live, 0u);  // parity did not move
    // ...but the failure is not lost: CIS is gone and the latch is the record.
    ALLOY_CHECK(eng::error<Ch>());

    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    eng::stop<Ch>();
    eng::disable_half_irq<Ch>();
    eng::disable_complete_irq<Ch>();
}

// ── 3. THE CURSOR: CUBC counts a HALF, remaining() must count the whole ──

ALLOY_TEST(xdmac_ring_remaining_spans_the_buffer_not_the_microblock) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 11;
    static std::uint16_t buf[8]{};
    g_half_calls = 0;
    g_full_calls = 0;

    eng::enable_half_irq<Ch>(&count_half, nullptr);
    eng::enable_complete_irq<Ch>(&count_full, nullptr);
    eng::setup<Ch>(eng::dir::periph_to_mem, true, eng::width::b16,
                   eng::width::b16, 0x40u,
                   reinterpret_cast<std::uintptr_t>(&buf[0]), 8u, 35u);
    eng::start<Ch>();
    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{std::span<std::uint16_t>{buf}};
    hw.start();

    // Nothing written: 8 of 8 left, though CUBC only ever says 4.
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride), 4u);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 8u);

    // Inside the FIRST half: remaining = CUBC + half.
    hw.advance(1);
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride), 3u);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 7u);
    hw.advance(2);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 5u);

    // ACROSS THE BOUNDARY — the one place a raw CUBC read is not merely
    // imprecise but wrong by half a buffer. The 4th item ends the block: CUBC
    // reloads to 4 and the parity flips, so remaining must fall from 5 to 4
    // and NOT jump back to 8.
    ALLOY_CHECK_EQ(hw.advance(1), 1);
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride), 4u);
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::live, 1u);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 4u);

    // Inside the SECOND half: remaining = CUBC alone.
    hw.advance(3);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 1u);
    // ...and the wrap puts it back to a full buffer.
    ALLOY_CHECK_EQ(hw.advance(1), 1);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 8u);

    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    eng::stop<Ch>();
    eng::disable_half_irq<Ch>();
    eng::disable_complete_irq<Ch>();
}

// The other side of the same accessor: with no ring, remaining() is the plain
// CUBC read both ST engines have, so one-shot transfers are untouched by any
// of this.
ALLOY_TEST(xdmac_remaining_without_a_ring_is_a_plain_cubc_read) {
    struct tag {};
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 13;
    static std::uint8_t src[16]{};

    eng::setup<Ch>(eng::dir::mem_to_periph, /*circular=*/false, eng::width::b8,
                   eng::width::b8, 0x1Cu,
                   reinterpret_cast<std::uintptr_t>(&src[0]), 16u, 9u);
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::half_items, 0u);
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 16u);
    eng::chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride) = 5u;
    ALLOY_CHECK_EQ(eng::remaining<Ch>(), 5u);
}

// ── 4. THE SHIPPED ring<T> OVER THIS ENGINE ──────────────────────────────
//
// Everything above drives the engine directly. This one builds the REAL
// alloy::dma::ring — the same type an adc_stream on a G0 builds — and runs both
// of its consumption disciplines over the XDMAC's descriptor list. It is the
// case that says the design's one-vocabulary claim is true rather than aspired
// to: no XDMAC-shaped parameter, no second ring type, no descriptor argument.

ALLOY_TEST(xdmac_ring_carries_the_shipped_ring_take_discipline) {
    struct tag {};
    using inst = fake_xdmac_inst<tag>;
    using eng = engine<tag>;
    constexpr unsigned Ch = 15;
    using route = alloy::dma::route<inst, Ch, 35>;
    static alloy::dma::ring_storage<std::uint16_t, 8> storage{};

    {
        alloy::dma::ring<std::uint16_t, route> r{route{}, 0x40u, storage};
        fake_xdmac_hw<tag, Ch, std::uint16_t> hw{
            std::span<std::uint16_t>{storage.data, 8}};
        hw.next_value = 100;
        hw.start();
        // The shipped ring really did take the descriptor path — not a
        // one-shot that happens to fit — and it did so without being told
        // anything about descriptors.
        ALLOY_CHECK_EQ(eng::ring_state<Ch>::half_items, 4u);

        hw.advance(4);
        std::span<const std::uint16_t> first = r.take();
        ALLOY_CHECK_EQ(first.size(), 4u);
        ALLOY_CHECK_EQ(first.data(), &storage.data[0]);
        ALLOY_CHECK_EQ(first[0], 100u);
        ALLOY_CHECK_EQ(first[3], 103u);

        hw.advance(4);
        std::span<const std::uint16_t> second = r.take();
        ALLOY_CHECK_EQ(second.data(), &storage.data[4]);
        ALLOY_CHECK_EQ(second[0], 104u);
        ALLOY_CHECK_EQ(r.missed(), 0u);

        // Fall behind by a whole lap: both halves go stable before take() runs,
        // so it resynchronizes to the newest and counts the skipped one.
        hw.advance(8);
        std::span<const std::uint16_t> third = r.take();
        ALLOY_CHECK_EQ(third.data(), &storage.data[4]);
        ALLOY_CHECK_EQ(r.missed(), 1u);

        fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    }
    // Destroyed: see the teardown case for what that had to do.
}

ALLOY_TEST(xdmac_ring_carries_the_shipped_cursor_discipline_across_a_wrap) {
    struct tag {};
    using inst = fake_xdmac_inst<tag>;
    constexpr unsigned Ch = 17;
    using route = alloy::dma::route<inst, Ch, 35>;
    static alloy::dma::ring_storage<std::uint16_t, 8> storage{};

    alloy::dma::ring<std::uint16_t, route> r{route{}, 0x40u, storage};
    fake_xdmac_hw<tag, Ch, std::uint16_t> hw{
        std::span<std::uint16_t>{storage.data, 8}};
    hw.next_value = 1;
    hw.start();

    ALLOY_CHECK_EQ(r.cursor(), 0u);
    hw.advance(3);
    ALLOY_CHECK_EQ(r.cursor(), 3u);  // inside the first half
    ALLOY_CHECK_EQ(r.readable().size(), 3u);
    ALLOY_CHECK_EQ(r.readable()[0], 1u);

    // Straight through the half boundary. A raw CUBC read here would report 1
    // item written instead of 5 — the cursor would go BACKWARDS.
    hw.advance(2);
    ALLOY_CHECK_EQ(r.cursor(), 5u);
    ALLOY_CHECK_EQ(r.readable().size(), 5u);

    r.consume(5);
    ALLOY_CHECK_EQ(r.readable().size(), 0u);

    // Past the wrap: readable() hands back the tail run first, then the head.
    hw.advance(5);
    ALLOY_CHECK_EQ(r.cursor(), 10u);
    std::span<const std::uint16_t> tail = r.readable();
    ALLOY_CHECK_EQ(tail.size(), 3u);  // items 5..7
    ALLOY_CHECK_EQ(tail.data(), &storage.data[5]);
    r.consume(3);
    std::span<const std::uint16_t> head = r.readable();
    ALLOY_CHECK_EQ(head.size(), 2u);  // items 0..1 of the new lap
    ALLOY_CHECK_EQ(head.data(), &storage.data[0]);

    fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
}

// ── 5. TEARDOWN ─────────────────────────────────────────────────────────
//
// The destructor stops the channel, BREAKS THE DESCRIPTOR LOOP, disarms the
// interrupt and releases the claim — in that order. The loop-breaking is the
// one this IP adds to the §4 sequence and the one a reader would not expect:
// the descriptors point at each other forever, so a disabled channel whose CNDC
// still says NDE is one stray GE away from chasing them into a buffer whose
// owner is gone — and the claim is released immediately afterwards, so the next
// claimant is exactly who would write that GE.
ALLOY_TEST(xdmac_ring_teardown_stops_the_channel_and_breaks_the_loop) {
    struct tag {};
    using inst = fake_xdmac_inst<tag>;
    using eng = engine<tag>;
    using IP = fake_xdmac_ip<tag>;
    constexpr unsigned Ch = 19;
    using route = alloy::dma::route<inst, Ch, 35>;
    static alloy::dma::ring_storage<std::uint16_t, 4> storage{};

    {
        alloy::dma::ring<std::uint16_t, route> r{route{}, 0x40u, storage};
        fake_xdmac_hw<tag, Ch, std::uint16_t> hw{
            std::span<std::uint16_t>{storage.data, 4}};
        hw.start();
        hw.advance(2);
        ALLOY_CHECK((eng::r().GS & IP::template st<Ch - 1>.mask) != 0u);
        ALLOY_CHECK((eng::chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride) &
                     IP::nde.mask()) != 0u);
        // stop() spins on GS, so the hardware has to answer the GD write. The
        // engine's own loop cannot run on the host without this, which is why
        // the destructor is entered with the disable pre-applied.
        fake_xdmac_hw<tag, Ch, std::uint16_t>::settle();
    }

    // Channel disabled, list broken, bookkeeping cleared.
    ALLOY_CHECK_EQ(eng::r().GS & IP::template st<Ch - 1>.mask, 0u);
    ALLOY_CHECK_EQ(eng::chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride), 0u);
    ALLOY_CHECK_EQ(eng::ring_state<Ch>::half_items, 0u);
    // Interrupts disarmed at both ends...
    ALLOY_CHECK((eng::chreg<Ch>(IP::CID_offset, IP::CID_stride) & IP::bid.mask()) !=
                0u);
    ALLOY_CHECK((eng::r().GID & IP::template id<Ch - 1>.mask) != 0u);
    // ...and the claim released, so the channel can be taken again. A ring that
    // released without disarming would leave a handler pointing at a destroyed
    // object; a ring that stopped without releasing would trap right here.
    auto again = alloy::dma::channel<inst, Ch>::claim();
    (void)again;
}

// ── 6. THE VOCABULARY ────────────────────────────────────────────────────
//
// The capability that lets all of the above exist is supports_ring, and it is
// NOT supports_circular — this engine has no circular bit and must never grow
// one, because start_m2p_circular_u16 feeds 16-bit memory items into 32-bit
// register writes and XDMAC has a single transfer width for both sides of a
// channel. Setting one flag for both would have made that method visible and
// trapping on this controller: a runtime surprise where dma.hpp promises a
// compile error.
namespace {
struct vocab_tag {};
}  // namespace
static_assert(alloy::dma::ring_capable<fake_xdmac_inst<vocab_tag>, 1>);
static_assert(!engine<vocab_tag>::supports_circular);
static_assert(engine<vocab_tag>::supports_ring);
// The refusal itself is asserted through the flag rather than through a
// `requires` probe naming the method: Apple clang hard-errors on naming a
// constraint-failing member inside a requires-expression instead of folding it
// to false, the same footnote test_dma_ring.cpp records for ring<>.
