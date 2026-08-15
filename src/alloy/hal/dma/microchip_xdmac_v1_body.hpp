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
// v1 one-shot single-microblock transfers (XDMAC has NO circular bit; rings
// need linked-list descriptors via CNDA/CNDC — a future phase).
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
    // — two linked view-0 descriptors ping-ponging the halves — but that is the
    // next commit; this one only splits the question in two.
    static constexpr bool supports_ring = false;

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
        if (circular) {
            __builtin_trap();  // XDMAC circular = linked-list descriptors, not in v1
        }
        if (msize != psize) {
            __builtin_trap();  // XDMAC has ONE DWIDTH for both sides
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
        const bool to_periph = d == dir::mem_to_periph;
        chreg<Ch>(IP::CSA_offset, IP::CSA_stride) =
            static_cast<std::uint32_t>(to_periph ? mem_addr : periph_addr);
        chreg<Ch>(IP::CDA_offset, IP::CDA_stride) =
            static_cast<std::uint32_t>(to_periph ? periph_addr : mem_addr);
        chreg<Ch>(IP::CUBC_offset, IP::CUBC_stride) = items;  // units of DWIDTH
        chreg<Ch>(IP::CBC_offset, IP::CBC_stride) = 0;        // single microblock
        chreg<Ch>(IP::CNDA_offset, IP::CNDA_stride) = 0;      // no descriptors
        chreg<Ch>(IP::CNDC_offset, IP::CNDC_stride) = 0;
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
        if (callback<Ch>::fn != nullptr) {
            chreg<Ch>(IP::CIE_offset, IP::CIE_stride) =
                IP::bie.mask() | IP::rbie.mask() | IP::wbie.mask() |
                IP::roie.mask();
            r().GIE = IP::template ie<Ch - 1>.mask;
        }
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
        if (callback<Ch>::fn != nullptr) {
            // The callback sees a channel whose flags are already consumed and
            // whose latches already answer done()/error(), so it may start the
            // next transfer without racing its own completion.
            callback<Ch>::fn(callback<Ch>::ctx);
        }
    }

    template <unsigned Ch>
    static void enable_complete_irq(void (*fn)(void*), void* ctx) {
        callback<Ch>::fn = fn;
        callback<Ch>::ctx = ctx;
        // CIE/GIE are NOT written here — setup() folds them in, see the note
        // there. Registering after a transfer has started therefore reports
        // from the NEXT setup(), exactly as on the ST engines.
        alloy::irq::attach(irq_line_of<Ch>(), &complete_isr<Ch>);
        alloy::irq::enable(irq_line_of<Ch>());
    }

    template <unsigned Ch>
    static void disable_complete_irq() {
        // CID and GID are the write-only CLEAR twins of CIE and GIE (writing a
        // 1 disables that bit; there is no read-modify-write to get wrong).
        // The shared NVIC line stays enabled: 23 other channels may be using
        // it, and every handler is a no-op for interrupts that are not its own.
        chreg<Ch>(IP::CID_offset, IP::CID_stride) =
            IP::bid.mask() | IP::rbeid.mask() | IP::wbeid.mask() |
            IP::roid.mask();
        r().GID = IP::template id<Ch - 1>.mask;
        alloy::irq::detach(irq_line_of<Ch>(), &complete_isr<Ch>);
        callback<Ch>::fn = nullptr;
        callback<Ch>::ctx = nullptr;
    }

    // One NVIC line for the whole block (chip data irq).
    template <unsigned Ch>
    static constexpr alloy::irq_line irq_line_of() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        return Inst::irq;
    }
};

}  // namespace alloy::hal::detail
