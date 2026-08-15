// The programming sequence of the SAM E70 XDMAC — the ENTIRE driver body,
// split out of microchip_xdmac_v1.hpp so a host test can reach it (the driver
// proper is constrained on the generated alloy::ip::microchip::xdmac_v1 tag,
// which exists only inside a built project; tests/ deliberately never reads
// generated headers). Same seam as st_dma_v1_body.hpp / st_dma_v2_body.hpp:
// the logic lives once, here; the constrained dma_impl<Inst> specialization in
// microchip_xdmac_v1.hpp inherits it unchanged.
//
// Everything below reaches the register layout through `typename Inst::ip`, so
// a host test supplies a hand-written IP double over a memory-backed register
// file and exercises the REAL ISR/latch/poll code — which is what makes the
// latch contract (§4 of docs/design/dma-streams.md) witnessable off-target.
// On THIS controller that matters more than on the two ST engines: CIS is
// CLEARED BY READING, so the double must destroy the value on read to
// reproduce the hazard at all (tests/test_xdmac_v1_latch.cpp does).
//
// One-shot single-microblock transfers, plus the linked-list RING: XDMAC has
// no circular bit, so alloy::dma::ring is built here from two view-0
// descriptors that point at each other, each covering one half of the caller's
// buffer, with the END-OF-BLOCK interrupt as the half event.
//
// ═══ READ THIS BEFORE TRUSTING A RING FROM THIS ENGINE ON SILICON ═══
//
// THE RING PATH HAS NO WITNESS. Not a weak one — none. Stating it here, at the
// top, because every other honesty note in this file sits on top of a
// measurement and this one does not:
//
//  * NO EMULATION. Renode 1.16.1 ships no XDMAC model and no Microchip DMA
//    model of any kind (measured two independent ways: every plausible type
//    name fails to resolve, and the complete SAM* type set in its
//    Infrastructure assembly is 19 peripherals, none of them a DMA
//    controller). alloy's own emitter generates none either. So there is no
//    leg, and inventing one would prove nothing anyway — see the next point.
//  * NO CURATED DESCRIPTOR LAYOUT. What a view-0 descriptor CONTAINS in
//    memory — the word order, and the bit packing of its own control word —
//    is stated in NO file in either repo. The register curation covers the
//    registers that POINT at descriptors (CNDA's NDA/NDAIF, CNDC's
//    NDE/NDSUP/NDDUP/NDVIEW, CUBC's UBLEN) exhaustively and with datasheet
//    provenance; the descriptor STRUCTURE is not a register and is not there.
//    The pinned upstream does not carry it either: the SVD-derived device file
//    in same70-dfp-4.9.129.atpack (sha-locked in the builder's sources.lock)
//    has the CNDC NDVIEW value group and every register bit, and ZERO
//    descriptor-structure content. So a Renode model, if one were written,
//    would encode the SAME unverified reading this file does, and a green leg
//    would prove only that the model and the firmware agree with each other.
//    CORROBORATED SINCE, BY A SECOND IMPLEMENTATION RATHER THAN BY THE
//    DATASHEET — which lowers the risk without closing the citation. Zephyr's
//    Atmel SAM XDMAC driver header (drivers/dma/dma_sam_xdmac.h, Apache-2.0,
//    (c) 2017 comsuisse AG) declares its view-0 linked-list descriptor as the
//    same THREE words in the same order (next-descriptor address, microblock
//    control, transfer address) and puts the control word's fetch-enable at
//    bit 24, its source-update at bit 25, its destination-update at bit 26 and
//    the two-bit view selector at bit 27 — position for position what the five
//    declarations below say, with the descriptor's update bits spelled NSEN /
//    NDEN there and NDSUP / NDDUP here (the CNDC register's names for the same
//    two decisions). Two independent readings agreeing is evidence, not a
//    citation: they can share a misreading of one figure. The instruction
//    below is unchanged.
//  * NO SILICON. Nobody has run this.
//
// WHAT THE HOST TEST DOES AND DOES NOT PROVE, so the next reader does not
// over-read it: tests/test_xdmac_v1_ring.cpp runs the real engine over a
// register double that FETCHES the descriptors out of host memory and executes
// them. It proves the bookkeeping — linkage, ping-pong parity, cursor
// arithmetic across the half boundary, teardown — against THIS FILE'S reading
// of the layout. It cannot prove the reading. If the layout below is wrong,
// every test in that file still passes and the first silicon run takes a bus
// error or writes to a wild address.
//
// THE ONE DISCREPANCY ALREADY KNOWN, and it is in the design doc rather than
// in the silicon: docs/design/dma-streams.md §3.3 says "16 bytes of RAM per
// descriptor" for view 0. This file says 12 — three words, MBR_NDA / MBR_UBC /
// MBR_TA — because view 0 carries ONE transfer address (which is exactly why
// it is the right view for a ring, where only the memory side moves) and 16
// bytes is the four-word view 1 (NDA, UBC, SA, DA). That correction is stated
// in the changelog and in the design's own status table. It is a reading, not
// a citation: CONFIRM THE STRUCTURE AND THE UBC BIT POSITIONS AGAINST
// DS60001527 (the XDMAC chapter, "Linked List Descriptor View 0") BEFORE
// CURATING THEM OR SHIPPING A RING TO HARDWARE. Everything the confirmation
// touches is in one struct and four named constants, immediately below
// `supports_ring`.
//
// Model honored (DS60001527): program CSA/CDA/CUBC/CC with the channel
// disabled (GS bit 0), read CIS once to clear stale flags (CIS is
// CLEARED BY READING), then GE arms it; the channel AUTO-DISABLES at end
// of block, so completion is "GS bit back to 0". Peripheral APB targets
// sit behind AHB IF1, memory behind IF0 (SIF/DIF per direction). L3
// channels are 1-based (ST-doc style); XDMAC registers are 0-based.
// M7 caches are never enabled by alloy startup — no cache maintenance.
//
// SILICON-FOUND: the memory-side master (IF0) reads SRAM but NOT the
// embedded flash — a .rodata source raised a read bus error on the real
// SAME70 Xplained. Memory-to-peripheral sources must live in RAM (the ST
// dma_v1, by contrast, reads flash fine).

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal::detail {

template <class Inst>
struct microchip_xdmac_v1_engine {
    using IP = typename Inst::ip;

    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };

    // Capability: XDMAC has NO circular bit — the register curation says so
    // exhaustively — so dma::channel refuses start_m2p_circular_u16 at COMPILE
    // time on this controller instead of trapping at runtime. That refusal is
    // permanent and independent of rings: that method feeds 16-bit memory items
    // into 32-bit register writes, and this engine has ONE transfer width for
    // both sides of a channel.
    static constexpr bool supports_circular = false;
    // The ring capability is the SEPARATE question (alloy::dma::ring_capable):
    // can this engine present alloy::dma::ring's contract by any means. It can
    // — two linked view-0 descriptors ping-ponging the halves — so the SAME
    // alloy::dma::ring<T, Route> a G0 or an F7 project writes compiles here,
    // with the same take()/missed() and cursor()/readable()/consume()
    // disciplines. Read the witness warning in the file header first.
    static constexpr bool supports_ring = true;

    // ── THE VIEW-0 LINKED-LIST DESCRIPTOR ────────────────────────────────
    //
    // THIS IS THE UNVERIFIED PART. Everything a datasheet confirmation would
    // touch is these five declarations; nothing else in the file depends on the
    // memory layout. See the file header for what is and is not known.
    //
    // A view-0 descriptor is three 32-bit words:
    //   MBR_NDA  address of the next descriptor (word-aligned; CNDA's NDA field
    //            is bits 31:2, which is what pins the alignment)
    //   MBR_UBC  microblock length in DATA UNITS of CC.DWIDTH, plus the control
    //            bits that say what the NEXT fetch does
    //   MBR_TA   the transfer address this descriptor supplies — SOURCE or
    //            DESTINATION, whichever of NDSUP/NDDUP is set. Exactly one is,
    //            here: the peripheral side of a ring never moves, so the ring
    //            updates the memory side and leaves the other to the CC/CSA/CDA
    //            programming that setup() already did.
    //
    // THE TRAP THAT MAKES THIS A STRUCT AND NOT A REUSE OF THE GENERATED
    // FIELDS: the descriptor's UBC word carries NDE / NDSUP / NDDUP / NVIEW at
    // DIFFERENT BIT POSITIONS from the CNDC register's identically-named bits.
    // CNDC has them low (IP::nde / ndsup / nddup / ndview, generated from the
    // curation); the UBC word has them above the 24-bit length. Reaching for
    // IP::nde when writing a descriptor is a silent wrong-bits bug, so the two
    // sets are deliberately spelled differently — `ubc_*` below is memory,
    // `IP::*` is the register, and no line mixes them.
    static constexpr std::uint32_t ubc_ublen_mask = 0xFFFFFFu;  // 24-bit length
    static constexpr unsigned ubc_nde_pos = 24u;    // fetch a next descriptor
    static constexpr unsigned ubc_ndsup_pos = 25u;  // ...updating the source
    static constexpr unsigned ubc_nddup_pos = 26u;  // ...updating the dest
    static constexpr unsigned ubc_nview_pos = 27u;  // NVIEW; 0 = view 0

    struct view0_descriptor {
        std::uint32_t nda;
        std::uint32_t ubc;
        std::uint32_t ta;
    };
    static_assert(sizeof(view0_descriptor) == 12u,
                  "a view-0 descriptor is THREE words (NDA, UBC, TA) — the "
                  "four-word 16-byte shape is view 1, which carries SA and DA "
                  "separately; docs/design/dma-streams.md §3.3 says 16 and is "
                  "corrected here");
    static_assert(alignof(view0_descriptor) == 4u,
                  "CNDA's NDA field is bits 31:2, so a descriptor address must "
                  "be word-aligned and the low two bits are not ours to use");

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    template <unsigned Ch>
    static rw32& chreg(std::uintptr_t offset, unsigned stride) {
        return alloy::reg_at(Inst::base, offset, stride, Ch - 1);
    }

    static void enable_controller() { alloy::gate_on(Inst::gate); }

    // Completion callback + the latches. Storage is per (controller, channel):
    // `callback` is a template, so each Ch gets its own statics without a
    // runtime table — the st_dma_v1_body.hpp shape, one event short (the half
    // event needs linked-list descriptors on this IP and is a later phase).
    template <unsigned Ch>
    struct callback {
        static inline void (*fn)(void*) = nullptr;
        static inline void* ctx = nullptr;
        // ON THIS CONTROLLER THE LATCH IS MANDATORY, NOT DEFENSIVE, and that
        // is the one sentence to carry away from this file. Both ST engines
        // latch because their ISR must WRITE a clear (IFCR/LIFCR) that a
        // later poller would otherwise miss — a discipline. Here CIS IS
        // CLEARED BY READING (DS60001527): the act of looking destroys the
        // evidence, for the ISR and for a poller alike. So there is no
        // "latch OR live flag" spelling to fall back on the way ST has —
        // whoever reads first is the only one who will ever see the bits, and
        // if they do not hand them over here, they are gone. Everything below
        // follows from that: ONE reader (harvest), latches only ever cleared
        // by setup(), and pollers that keep their hands off CIS entirely
        // while an interrupt owns the channel.
        //
        // THE TWO LATCHES ARE NOT EQUALLY LOAD-BEARING TODAY, and pretending
        // otherwise would be the kind of tidy story that rots. `err_latched`
        // is the only record of a failure there will ever be: RBEIS/WBEIS/ROIS
        // live in CIS and nowhere else, so an unlatched error is a LOST error
        // and wait() answers true for a transfer that faulted. `latched` has a
        // second source for what ships today — a single-microblock transfer
        // auto-disables at end of block, so GS answers complete() on its own.
        // It becomes the only source the moment a transfer spans more than one
        // block (block count, or the descriptor list phase 5b brings), because
        // BIS is END OF BLOCK and the channel runs on through it. The contract
        // is written for that, and tests/test_xdmac_v1_latch.cpp pins it there
        // rather than on a case GS would carry anyway.
        static inline volatile bool latched = false;
        static inline volatile bool err_latched = false;

        // The half event, which on THIS IP is the same hardware event as the
        // full one. Both ST engines have two distinct status flags with two
        // independently attachable handlers; XDMAC has ONE — end of block —
        // and the two halves are told apart by WHICH DESCRIPTOR just finished
        // (ring_state<Ch>::live). So `half_fn` and `fn` are two callbacks fed
        // by one interrupt, fanned out by parity in complete_isr, and the
        // handler chain carries ONE entry for this channel rather than two.
        // Do not add a `half_isr<Ch>` to attach beside it: alloy::irq runs
        // every handler on the line, the first one to reach CIS empties it,
        // and the second would see zero.
        static inline void (*half_fn)(void*) = nullptr;
        static inline void* half_ctx = nullptr;
        static inline volatile bool half_latched = false;
        // Whether this channel's one handler is on the chain. alloy::irq::attach
        // TRAPS on a duplicate, and both enable_*_irq() want the same handler
        // there, so the attach is idempotent through this flag and the detach
        // waits until neither callback is armed.
        static inline bool attached = false;
    };

    // Ring bookkeeping, per (controller, channel) like the callbacks and for
    // the same reason: a template's statics cost nothing for channels that
    // never ring, so 24 channels do not pay for the two that do.
    template <unsigned Ch>
    struct ring_state {
        // DMA-VISIBLE RAM BY CONSTRUCTION, which is the §4 safety rule this IP
        // enforces the hard way. These are mutable statics, so they land in
        // .bss — SRAM — and the XDMAC's memory master can read them. A
        // `constexpr`/`const` descriptor pair would land in flash, which this
        // engine's master CANNOT read (SILICON-FOUND; see the file header), and
        // the very first descriptor fetch would be a read bus error.
        //
        // ENGINE-OWNED, NOT CALLER-OWNED, and that is a deliberate divergence
        // from the ring_storage precedent that the data buffer follows. The
        // descriptors are not the caller's data — they are this engine's
        // private encoding of "ring", they do not exist on either ST engine,
        // and threading them through alloy::dma::ring's constructor would put
        // an XDMAC-shaped parameter into the one portable type the whole design
        // exists to keep portable. Static storage also outlives every ring by
        // construction, which is strictly safer than a caller-provided object
        // for something an interrupt may still be pointing at. The lifetime
        // rule the caller does own is unchanged: the DATA buffer is still
        // theirs.
        //
        // WHAT IT COSTS A FIRMWARE THAT NEVER RINGS, measured rather than
        // assumed — the first draft of this comment said "a channel that never
        // rings pays nothing, because the statics are a template's", and that
        // is FALSE. `circular` is a RUNTIME parameter of setup() (it is in the
        // engine contract that way, so alloy::dma::channel and alloy::dma::ring
        // can share one entry point), so link_ring() is emitted and these
        // statics are allocated for every channel whose setup() is
        // instantiated, ring or not. Measured on examples/dma_uart for
        // same70_xplained — one channel, one-shot only, no ring anywhere in the
        // program: text 3656 -> 3768, bss 2592 -> 2608. Twenty-four bytes of
        // descriptors and about a hundred of code, per one-shot channel, for a
        // feature it does not use. Small, real, and the honest number to quote.
        static inline view0_descriptor desc[2]{};
        // Items per half, and the flag that says a ring is running at all:
        // ZERO means no ring, which is what makes remaining() fall back to a
        // plain CUBC read for one-shot transfers.
        static inline volatile std::uint16_t half_items = 0u;
        // Which descriptor is EXECUTING. Toggled by the ISR at each block end,
        // and read by remaining() to know whether CUBC is counting down through
        // the first half or the second — the design's "cursor() degrades"
        // sentence, made concrete.
        static inline volatile std::uint8_t live = 0u;
    };

    // THE ONE CIS READER. Reads CIS exactly once — which CLEARS it — and
    // folds everything it found into the latches, so the value is never lost
    // no matter who got there first. Two properties this buys, both pinned by
    // tests/test_xdmac_v1_latch.cpp:
    //  * IDEMPOTENT. Before the latch, error() read CIS raw, so calling it
    //    twice on a failed transfer answered true, then FALSE — the second
    //    read got the register the first one had already emptied. A latent
    //    bug at HEAD, closed here rather than inherited.
    //  * LOSSLESS BETWEEN READERS. The latches are set, never cleared, by
    //    harvest; only setup() resets them (and only after clear_flags(), so
    //    the stale-flag purge cannot un-latch the transfer it is arming).
    // Returns the captured word so a caller that must DECIDE — the ISR — can
    // do so from the copy instead of asking the hardware a second time.
    template <unsigned Ch>
    static std::uint32_t harvest() {
        const std::uint32_t cis = chreg<Ch>(IP::CIS_offset, IP::CIS_stride);
        if ((cis & IP::bis.mask()) != 0u) {
            callback<Ch>::latched = true;
        }
        if ((cis & (IP::rbeis.mask() | IP::wbeis.mask() | IP::rois.mask())) != 0u) {
            callback<Ch>::err_latched = true;
        }
        return cis;
    }

    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph_addr, std::uintptr_t mem_addr,
                      std::uint16_t items, std::uint8_t request) {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count,
                      "channel outside this XDMAC instance (chip data ch_count)");
        if (msize != psize) {
            __builtin_trap();  // XDMAC has ONE DWIDTH for both sides
        }
        if (circular && (items < 2u || (items % 2u) != 0u)) {
            __builtin_trap();  // a ring is two descriptors over two equal halves
        }
        stop<Ch>();
        clear_flags<Ch>();
        // AFTER the flag purge, never before, and on this engine that ordering
        // is load-bearing rather than tidy: clear_flags() harvests, so the
        // purge itself SETS the latches from the previous transfer's status.
        // Reset them first and the very next line re-latches the stale
        // completion, and the fresh transfer reports done() before it has
        // moved a byte. (st_dma_v1_body.hpp orders its resets the same way,
        // for the milder reason that its purge is a write.)
        callback<Ch>::latched = false;      // a new transfer is not already complete
        callback<Ch>::err_latched = false;  // ...and does not inherit the last
                                            // transfer's failure
        callback<Ch>::half_latched = false;  // ...nor is it already half-done
        const bool to_periph = d == dir::mem_to_periph;
        chreg<Ch>(IP::CSA_offset, IP::CSA_stride) =
            static_cast<std::uint32_t>(to_periph ? mem_addr : periph_addr);
        chreg<Ch>(IP::CDA_offset, IP::CDA_stride) =
            static_cast<std::uint32_t>(to_periph ? periph_addr : mem_addr);
        chreg<Ch>(IP::CBC_offset, IP::CBC_stride) = 0;  // one microblock per block
        if (circular) {
            // The moving side's address comes from the descriptors from here
            // on; CUBC is loaded by the fetch, so the value written above would
            // be overwritten anyway. Zero says so out loud.
            chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride) = 0;
            link_ring<Ch>(to_periph, mem_addr, items, msize);
        } else {
            chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride) = items;  // units of DWIDTH
            chreg<Ch>(IP::CNDA_offset, IP::CNDA_stride) = 0;      // no descriptors
            chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride) = 0;
        }
        // Peripheral-synchronized, hw request, chunk 1, single-beat bursts.
        // Memory side increments on IF0; peripheral side fixed on IF1.
        chreg<Ch>(IP::CC_offset, IP::CC_stride) =
            IP::type.mask() |
            (to_periph ? IP::dsync.mask() : 0u) |
            (static_cast<std::uint32_t>(msize) << IP::dwidth.pos) |
            (to_periph ? 0u : IP::sif.mask()) |   // src = periph (IF1) on rx
            (to_periph ? IP::dif.mask() : 0u) |   // dst = periph (IF1) on tx
            ((to_periph ? 1u : 0u) << IP::sam.pos) |
            ((to_periph ? 0u : 1u) << IP::dam.pos) |
            (static_cast<std::uint32_t>(request) << IP::perid.pos);
        // Interrupt arming, folded into setup() exactly as the two ST engines
        // do — and here that is a CHOICE, not a constraint. ST must fold:
        // CCR/SxCR carry TCIE/HTIE, may only be written while the channel is
        // disabled, and are written whole, so "enable it later" would silently
        // erase the other enables. XDMAC has NO interrupt-enable bit in CC at
        // all; CIE and GIE are separate write-only SET registers, writable at
        // any time. The fold is kept anyway so that one caller sequence
        // (on_complete() then the transfer) means the same thing on all three
        // controllers. Do not "simplify" it into enable_complete_irq without
        // deciding, on purpose, to change WHEN interrupts start being
        // delivered relative to the transfer.
        //
        // TWO REGISTERS, both needed: CIE selects which of the channel's
        // events propagate, GIE lets that channel reach the NVIC line at all.
        // The error events ride with the completion event, like ST's TEIE
        // rides with TCIE, so a failure is delivered rather than waited out.
        //
        // BIE serves BOTH callbacks — it is end of block, which is the whole
        // transfer for a one-shot and one half for a ring — so the condition
        // asks whether ANYONE wants to hear about it.
        if (callback<Ch>::fn != nullptr || callback<Ch>::half_fn != nullptr) {
            chreg<Ch>(IP::CIE_offset, IP::CIE_stride) =
                IP::bie.mask() | IP::rbie.mask() | IP::wbie.mask() |
                IP::roie.mask();
            r().GIE = IP::template ie<Ch - 1>.mask;
        }
    }

    // Build the two-descriptor loop and point the channel at it. Called only
    // from setup(circular=true), with the channel already stopped and its flags
    // already purged.
    //
    // THE SHAPE, which is the whole of "a ring on an IP with no circular bit":
    //
    //     desc[0]: TA = &buf[0]          NDA -> desc[1]   UBLEN = N/2
    //     desc[1]: TA = &buf[N/2]        NDA -> desc[0]   UBLEN = N/2
    //
    // Each descriptor's UBC sets NDE (there IS a next one) and exactly one of
    // NDSUP/NDDUP (the next fetch updates the MEMORY side and leaves the
    // peripheral side alone — a ring's peripheral register never moves). The
    // list has no end: desc[1] points back at desc[0], so the engine runs until
    // stop(). That is the circular bit, spelled in RAM.
    //
    // CNDA gets desc[0]'s address with NDAIF = 0 — the descriptor fetch goes
    // through the same AHB interface as memory, which is where .bss is. The
    // address is word-aligned (static_assert above) so writing it whole leaves
    // NDAIF clear without masking.
    template <unsigned Ch>
    static void link_ring(bool to_periph, std::uintptr_t mem_addr,
                          std::uint16_t items, width w) {
        const std::uint32_t half = static_cast<std::uint32_t>(items) / 2u;
        // Bytes per half = items per half scaled by the transfer width, and the
        // width enum IS the shift (b8=0, b16=1, b32=2) rather than a table.
        const std::uint32_t half_bytes = half << static_cast<unsigned>(w);
        // The UBC control word. NVIEW = 0 contributes nothing, and is left
        // unwritten rather than OR-ed with a zero so that a future view change
        // has to touch this line on purpose.
        const std::uint32_t ctl =
            (half & ubc_ublen_mask) | (std::uint32_t{1} << ubc_nde_pos) |
            (std::uint32_t{1} << (to_periph ? ubc_ndsup_pos : ubc_nddup_pos));
        static_assert(ubc_nview_pos > ubc_nddup_pos,
                      "NVIEW sits above the update bits in the UBC word");
        view0_descriptor* const d = ring_state<Ch>::desc;
        d[0].nda = addr32(&d[1]);
        d[0].ubc = ctl;
        d[0].ta = static_cast<std::uint32_t>(mem_addr);
        d[1].nda = addr32(&d[0]);
        d[1].ubc = ctl;
        d[1].ta = static_cast<std::uint32_t>(mem_addr + half_bytes);
        // Only now does the bookkeeping go live: desc[0] runs first, so the
        // first block end is the FIRST half going stable.
        ring_state<Ch>::live = 0u;
        ring_state<Ch>::half_items = static_cast<std::uint16_t>(half);
        chreg<Ch>(IP::CNDA_offset, IP::CNDA_stride) = addr32(&d[0]);
        // CNDC's OWN bits, which are NOT the UBC word's bits — see the trap
        // note at the descriptor struct. NDVIEW = 0 (view 0) is the reset
        // value and is left alone for the same reason as above.
        chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride) =
            IP::nde.mask() | (to_periph ? IP::ndsup.mask() : IP::nddup.mask());
    }

    // The descriptor pointer as the 32-bit register sees it. On the target this
    // is exact; on a host double the descriptors live wherever the loader put
    // them and only the low half round-trips, which is what the test compares.
    static std::uint32_t addr32(const void* p) {
        return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));
    }

    template <unsigned Ch>
    static void start() {
        r().GE = IP::template en<Ch - 1>.mask;
    }

    template <unsigned Ch>
    static void stop() {
        r().GD = IP::template di<Ch - 1>.mask;
        while ((r().GS & IP::template st<Ch - 1>.mask) != 0u) {
        }
        // BREAKING THE LOOP IS PART OF STOPPING, on this engine only. A ring's
        // descriptors point at each other forever, so a disabled channel whose
        // CNDC still has NDE set is one stray GE write away from chasing them
        // into a buffer whose owner has been destroyed — and alloy::dma::ring's
        // destructor releases the channel claim immediately afterwards, so the
        // next claimant could be the one to write that GE. Clearing NDE here
        // makes "the destructor stops the hardware" (§4) true of the descriptor
        // list too, not just of the channel enable. Guarded so a one-shot's
        // setup(), which calls stop() first and writes CNDC = 0 anyway, is not
        // given an extra register write it does not need.
        if (ring_state<Ch>::half_items != 0u) {
            chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride) = 0u;
            ring_state<Ch>::half_items = 0u;
        }
    }

    // WHO MAY TOUCH CIS — the rule that keeps a poller and an ISR from
    // destroying each other's evidence, and the reason both accessors below
    // are shaped the way they are rather than the ST "latch || live flag" way.
    //
    // When a callback is armed, the ISR owns CIS and these accessors must not
    // read it AT ALL. Not a style preference: `alloy::dma::channel::wait()` is
    // `while (!done()) { if (error()) return false; }`, so a poller that read
    // CIS here would sit in a tight loop EATING the channel's status, and the
    // interrupt — which fires after the flag it needs was already consumed —
    // would read zero, latch nothing and call nothing. On a one-shot transfer
    // the caller survives that by luck, because completion is also visible in
    // GS (a non-destructive register the poller may read freely); on anything
    // that keeps the channel enabled the spin never ends. So: callback armed
    // -> the latch is the only source; no callback -> the poller is the only
    // reader and harvests for itself.
    //
    // The residual gap, stated rather than papered over: with a callback armed
    // and the NVIC line masked by the caller, an error is latched by nobody
    // and error() stays false until the interrupt is allowed to run. The
    // shipped paths do not do that — enable_complete_irq() enables the line.
    template <unsigned Ch>
    static constexpr bool poller_owns_cis() { return callback<Ch>::fn == nullptr; }

    // The channel auto-disables at end of block: GS bit falls back to 0.
    // NOTE: a bus error also auto-disables the channel (GS -> 0), so
    // "complete" alone cannot distinguish success — the L3 wait() does a
    // FINAL error() check after the GS poll for exactly this reason.
    //
    // GS is read either way: it is not clear-on-read, so it costs a poller
    // nothing and it is what makes a polled one-shot work even before the
    // interrupt has been serviced.
    template <unsigned Ch>
    [[nodiscard]] static bool complete() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        if (poller_owns_cis<Ch>()) {
            harvest<Ch>();
        }
        return callback<Ch>::latched ||
               (r().GS & IP::template st<Ch - 1>.mask) == 0u;
    }

    // Bus errors (RBEIS/WBEIS) and request overflow (ROIS) surface in CIS and
    // nowhere else — unlike completion, there is no GS-shaped second source.
    // The answer therefore comes from the latch, filled either by this call's
    // own harvest (polled-only owner) or by the ISR's.
    template <unsigned Ch>
    [[nodiscard]] static bool error() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        if (poller_owns_cis<Ch>()) {
            harvest<Ch>();
        }
        return callback<Ch>::err_latched;
    }

    // Items left to write, over the WHOLE buffer — which on this engine is a
    // SYNTHESIS, not a register read, and that is the design's "cursor()
    // degrades" sentence cashed out.
    //
    // Both ST engines have one countdown register spanning the whole circular
    // buffer, so their remaining() is a load. XDMAC's CUBC counts down through
    // the CURRENT MICROBLOCK, which here is ONE HALF: it runs N/2 -> 0, reloads
    // from the next descriptor, and runs N/2 -> 0 again. Read raw it would make
    // alloy::dma::ring's cursor arithmetic wrong by half a buffer for half of
    // every lap — silently, and in the direction that says the writer is behind
    // where it actually is. The missing bit is WHICH HALF, and the only source
    // for it is the descriptor parity the ISR maintains:
    //
    //     filling half 0:  N/2 - CUBC  items written  ->  remaining = CUBC + N/2
    //     filling half 1:  N/2 + (N/2 - CUBC) written ->  remaining = CUBC
    //
    // THE RETRY LOOP IS NOT DECORATION. Two independent reads — a software
    // variable and a hardware register — that a block-end interrupt can land
    // between: sample `live`, then CUBC, then `live` again, and start over if
    // it moved. Without it the interleaving `live == 0` / boundary / small CUBC
    // reports a position most of a buffer ahead of the truth, which is the one
    // direction a cursor must never be wrong in (readable() would hand out
    // items the DMA has not written). alloy::dma::ring's own cursor() pins the
    // wrap count across this call for the same reason one level up.
    template <unsigned Ch>
    [[nodiscard]] static std::uint16_t remaining() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        const std::uint32_t half = ring_state<Ch>::half_items;
        if (half == 0u) {
            // No ring: CUBC is the whole transfer, exactly like the ST engines.
            //
            // THIS BRANCH IS CLARITY, NOT CORRECTNESS, and saying so is the
            // honest version — the revert bar caught it. With half_items zero
            // the synthesis below degenerates to `cubc + 0` or `cubc`, which is
            // the same answer either way, so deleting this branch changes no
            // result and no test goes red. It stays because a one-shot transfer
            // should not read a parity flag it does not have, and because the
            // reader who asks "what does remaining() mean when there is no
            // ring" deserves the answer in one line rather than in the
            // arithmetic.
            return static_cast<std::uint16_t>(
                chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride) & ubc_ublen_mask);
        }
        for (;;) {
            const std::uint8_t live = ring_state<Ch>::live;
            const std::uint32_t cubc =
                chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride) & ubc_ublen_mask;
            if (ring_state<Ch>::live == live) {
                return static_cast<std::uint16_t>(live == 0u ? cubc + half
                                                             : cubc);
            }
        }
    }

    // The half-transfer twin of complete<Ch>(). ON THIS ENGINE IT IS THE ISR'S
    // LATCH AND NOTHING ELSE, which is a real divergence from ST and is stated
    // rather than smoothed over: both ST engines can answer half() by polling a
    // live HTIF that survives being read, so their rings work (badly, but
    // correctly) with interrupts off. XDMAC signals both halves with the SAME
    // end-of-block event, so the parity — which half just went stable — exists
    // only in the ISR's toggle. A polled ring cannot tell the halves apart
    // here, which is exactly why alloy::dma::ring_capable demands
    // enable_half_irq and why alloy::dma::ring arms it before setup().
    template <unsigned Ch>
    [[nodiscard]] static bool half() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        return callback<Ch>::half_latched;
    }

    template <unsigned Ch>
    static void clear_flags() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        // The purge goes THROUGH harvest, so this engine has exactly ONE piece
        // of code that reads CIS. That is worth more than the two lines it
        // costs: "the latches are the only record of a CIS read" is then a
        // property of the engine rather than a habit, which is what lets the
        // host witness assert a NEGATIVE — that a poller did not read the
        // register — from the latches alone (a memory double cannot see a
        // read; see tests/test_xdmac_v1_latch.cpp).
        // Purging is destructive by definition here, and the caller wants it
        // that way: setup() drops the latches immediately afterwards, and
        // channel::stop() has just torn the transfer down. A discarded
        // FUNCTION-CALL returning volatile& would perform NO read
        // ([expr.context]/2); harvest returns by VALUE, so the clear-by-read
        // really happens.
        (void)harvest<Ch>();
    }

    // ONE NVIC line for all 24 channels, so this runs for interrupts that
    // belong to other channels — the shared-line contract alloy::irq states
    // for every chained handler, and the same shape st_dma_v1_body.hpp uses
    // for its 2-3 / 4-7 groupings.
    //
    // THE GUARD READS GIS, AND THAT CHOICE IS THE WHOLE DESIGN. GIS is the
    // per-channel "who is pending" register and it is NOT clear-on-read, so
    // asking it is free; CIS is, so asking it is a commitment. Reading GIS
    // first means a foreign channel's interrupt leaves our CIS untouched
    // instead of emptying it into a handler that would then discard it.
    //
    // A NOTE ON THE DESIGN TEXT (docs/design/dma-streams.md §3.3), so that the
    // difference is a decision and not a drift: the design says "the ISR reads
    // GIS once and dispatches to per-channel latches", i.e. ONE handler with a
    // runtime table. This is the per-channel spelling instead — each armed
    // channel attaches its own complete_isr<Ch>, each reads GIS and returns
    // when the bit is not its own. It costs one extra GIS read per attached
    // channel and buys three things: no runtime dispatch table, an
    // attach/detach granularity identical to both ST engines (so
    // enable_complete_irq means the same thing on all three), and a handler
    // that a host test can call directly for ONE channel — which is what
    // tests/test_xdmac_v1_latch.cpp does.
    template <unsigned Ch>
    static void complete_isr(void*) {
        if ((r().GIS & IP::template is<Ch - 1>.mask) == 0u) {
            return;
        }
        // The one read. It clears CIS in hardware; every bit it held is now
        // in the latches, and the decision below is made from the copy.
        const std::uint32_t cis = harvest<Ch>();
        const bool done = (cis & IP::bis.mask()) != 0u;
        const bool failed =
            (cis & (IP::rbeis.mask() | IP::wbeis.mask() | IP::rois.mask())) != 0u;
        if (!done && !failed) {
            return;  // an event we never enabled (end-of-flush, end-of-disable)
        }
        // ── ONE HARDWARE EVENT, TWO CALLBACKS: the ring fan-out ──────────
        //
        // On both ST engines a half-buffer boundary and a wrap are two distinct
        // status flags with two independently attached handlers. XDMAC has ONE:
        // end of block, raised once per descriptor. Which BOUNDARY it is comes
        // from which DESCRIPTOR just finished — desc[0] covers the first half,
        // desc[1] the second — so the toggle below IS the half/full
        // distinction, and it is the same variable remaining() reads to place
        // the cursor. Advance it BEFORE calling out: a callback may look at the
        // cursor, and by the time it runs the engine is already filling the
        // other half.
        //
        // AN ERROR-ONLY EVENT DISPATCHES NOTHING WHILE RINGING, deliberately.
        // It is already in err_latched — the only record there will ever be —
        // but neither callback may run for it: alloy::dma::ring's full handler
        // increments the wrap count, so calling it for a bus error would move
        // the cursor a whole buffer forward on the strength of an event that
        // moved no data. (The one-shot path below does call fn on an error, and
        // should: there the callback means "the transfer is over", which is
        // true either way.)
        if (ring_state<Ch>::half_items != 0u) {
            if (!done) {
                return;
            }
            const std::uint8_t just = ring_state<Ch>::live;
            ring_state<Ch>::live = static_cast<std::uint8_t>(just ^ 1u);
            if (just == 0u) {
                callback<Ch>::half_latched = true;
                if (callback<Ch>::half_fn != nullptr) {
                    callback<Ch>::half_fn(callback<Ch>::half_ctx);
                }
                return;
            }
        }
        if (callback<Ch>::fn != nullptr) {
            // The callback sees a channel whose flags are already consumed and
            // whose latches already answer done()/error(), so it may start the
            // next transfer without racing its own completion.
            callback<Ch>::fn(callback<Ch>::ctx);
        }
    }

    // ONE handler per channel on the shared line, attached at most once. Both
    // enable_complete_irq and enable_half_irq want complete_isr<Ch> on the
    // chain and alloy::irq::attach TRAPS on a duplicate, so the attach is
    // idempotent here rather than at either call site.
    template <unsigned Ch>
    static void arm_line() {
        if (!callback<Ch>::attached) {
            alloy::irq::attach(irq_line_of<Ch>(), &complete_isr<Ch>);
            callback<Ch>::attached = true;
        }
        alloy::irq::enable(irq_line_of<Ch>());
    }

    // ...and the mirror: the channel stops being an interrupt source only when
    // NEITHER callback is armed. Disarming the ring's half event while its full
    // event is still registered must not take BIE away — they are the same bit.
    template <unsigned Ch>
    static void disarm_line_if_idle() {
        if (callback<Ch>::fn != nullptr || callback<Ch>::half_fn != nullptr) {
            return;
        }
        // CID and GID are the write-only CLEAR twins of CIE and GIE (writing a
        // 1 disables that bit; there is no read-modify-write to get wrong).
        chreg<Ch>(IP::CID_offset, IP::CID_stride) =
            IP::bid.mask() | IP::rbeid.mask() | IP::wbeid.mask() | IP::roid.mask();
        r().GID = IP::template id<Ch - 1>.mask;
        if (callback<Ch>::attached) {
            // The shared NVIC line stays enabled: 23 other channels may be
            // using it, and every handler is a no-op for interrupts that are
            // not its own.
            alloy::irq::detach(irq_line_of<Ch>(), &complete_isr<Ch>);
            callback<Ch>::attached = false;
        }
    }

    template <unsigned Ch>
    static void enable_complete_irq(void (*fn)(void*), void* ctx) {
        callback<Ch>::fn = fn;
        callback<Ch>::ctx = ctx;
        // CIE/GIE are NOT written here — setup() folds them in, see the note
        // there. Registering after a transfer has started therefore reports
        // from the NEXT setup(), exactly as on the ST engines.
        arm_line<Ch>();
    }

    template <unsigned Ch>
    static void disable_complete_irq() {
        callback<Ch>::fn = nullptr;
        callback<Ch>::ctx = nullptr;
        disarm_line_if_idle<Ch>();
    }

    // The ring's half-buffer callback. Register BEFORE setup(), like the
    // completion one and for the same reason — setup() decides then whether to
    // arm BIE at all. alloy::dma::ring calls this first, then
    // enable_complete_irq, then setup: on ST that order is load-bearing because
    // the two handlers land on one chain and dispatch runs them newest-first;
    // here the order is irrelevant because there is only ever ONE handler, and
    // saying so is worth a line so nobody "fixes" the caller.
    template <unsigned Ch>
    static void enable_half_irq(void (*fn)(void*), void* ctx) {
        callback<Ch>::half_fn = fn;
        callback<Ch>::half_ctx = ctx;
        arm_line<Ch>();
    }

    template <unsigned Ch>
    static void disable_half_irq() {
        callback<Ch>::half_fn = nullptr;
        callback<Ch>::half_ctx = nullptr;
        disarm_line_if_idle<Ch>();
    }

    // One NVIC line for the whole block (chip data irq).
    template <unsigned Ch>
    static constexpr alloy::irq_line irq_line_of() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        return Inst::irq;
    }
};

}  // namespace alloy::hal::detail
