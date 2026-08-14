// The programming sequence of the ST v2 STREAM engine (F2/F4/F7 — the
// pre-DMAMUX parts) — the ENTIRE driver body, split out of st_dma_v2.hpp so a
// host test can reach it, exactly like st_dma_v1_body.hpp: the driver proper
// is constrained on the generated alloy::ip::st::dma_v2 tag, which exists only
// inside a built project; tests/ deliberately never reads generated headers.
// tests/test_st_dma_v2_latch.cpp supplies a hand-written IP double over a
// memory-backed register file and exercises the REAL ISR/latch/poll code —
// the latch witness the v1 engine paid for as phase-1 debt ships with this
// engine from day one.
//
// THE CONTRACT IS st_dma_v1_engine's, verbatim in spirit: same member names,
// same shapes (setup/start/stop/complete/error/remaining/half, enable_*/
// disable_* irq, supports_circular, the ISR->poller LATCH discipline), so
// alloy::dma::channel, alloy::dma::ring, uart.rx_ring, write_dma and the async
// dma_waiter compile against either engine unchanged. What differs is the
// silicon underneath (RM0431/RM0410/RM0385/RM0090 §8.3, semantics recorded in
// registers/st/dma_v2.yaml alongside their provenance):
//
//  - STREAMS, 0-BASED. Template parameter S counts DMA_S0CR..S7CR the way ST
//    numbers them — deliberately unlike dma_v1's 1-based channels. There is no
//    off-by-one adjustment anywhere in this file, and that absence is the
//    design: a 1-based caller fails the S < streams static_assert on S == 8,
//    not silently on the wrong stream.
//  - Flags live in TWO split registers (LISR streams 0-3, HISR 4-7) with an
//    irregular per-stream slot packing; write-1-to-clear via LIFCR/HIFCR.
//    Every mask below comes from the GENERATED explicit field accessors
//    (tcif0..tcif7, ...) — the slot layout is never re-derived by hand here.
//  - No request router: the stream's CHSEL bits select the request source,
//    and the value written is the route's request id — the chip's matched
//    dma_routes triple, never a user-typed number (design §1).
//  - EN=0 IS A REQUEST, NOT A STATE: hardware finishes the in-flight transfer
//    (and FIFO drain) first, so stop() polls EN until it reads back 0 before
//    returning — which is what makes the sequence stop();setup() legal. In
//    non-circular mode hardware ALSO clears EN itself at transfer end, and
//    completion is TCIF with EN already low — the poll falls straight through.
//    (The inverse of dma_v1, where completion leaves EN=1 and only a transfer
//    error clears it.)
//  - Setting EN is IGNORED while any of the stream's five flags is still set,
//    so setup() clears all five through LIFCR/HIFCR before programming.
//  - FIFO: left in DIRECT mode this phase (DMDIS and FEIE explicitly cleared
//    at setup — a previous owner may have set them; reset default is direct).
//    In direct mode MSIZE follows PSIZE in hardware; both are still programmed
//    so the register states what the caller asked. SM1AR/DBM (double-buffer)
//    are not used — rings are half/full events over one buffer, same as v1.
//  - Per-stream NVIC vectors. The line for stream S is looked up in the chip
//    data's irq_lines ranges (the exti_g0 pattern) — NOT hardcoded grouping
//    logic: dma_v1's ch_irqline1/2_3/4_7 member hardcode is exactly why it
//    cannot serve the G4 today, and this engine does not repeat it.
//  - error() also reports DMEIF (direct mode error: a request hit a disabled
//    stream mid-reconfiguration) alongside TEIF — both mean this transfer
//    went wrong. FEIF is not consulted: in direct mode it carries no
//    actionable meaning for this engine's sequences.
//
// The LATCH discipline, restated because it is the witnessed contract: an ISR
// MUST clear the hardware flag it consumed (level-triggered line) and hand the
// fact to a per-stream latch; polled accessors report latch OR live flag;
// setup() resets the latches (a new transfer is not already complete). The ISR
// guard and its clear both read the HARDWARE flags and clear ONLY what the
// handler consumed — a blanket clear would eat the sibling event between the
// read and the write (the v1 test's wrap-instant case, kept here).

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal::detail {

template <class Inst>
struct st_dma_v2_engine {
    using IP = typename Inst::ip;

    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };

    // Capability: SxCR has a CIRC bit — circular streams (and therefore
    // alloy::dma::ring) exist on this engine.
    static constexpr bool supports_circular = true;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    template <unsigned S>
    static rw32& scr() {
        return alloy::reg_at(Inst::base, IP::SCR_offset, IP::SCR_stride, S);
    }
    template <unsigned S>
    static rw32& sndtr() {
        return alloy::reg_at(Inst::base, IP::SNDTR_offset, IP::SNDTR_stride, S);
    }
    template <unsigned S>
    static rw32& spar() {
        return alloy::reg_at(Inst::base, IP::SPAR_offset, IP::SPAR_stride, S);
    }
    template <unsigned S>
    static rw32& sm0ar() {
        return alloy::reg_at(Inst::base, IP::SM0AR_offset, IP::SM0AR_stride, S);
    }
    template <unsigned S>
    static rw32& sfcr() {
        return alloy::reg_at(Inst::base, IP::SFCR_offset, IP::SFCR_stride, S);
    }

    // The status/clear register PAIR this stream's flags live in: LISR/LIFCR
    // for streams 0-3, HISR/HIFCR for 4-7. `auto&` so the engine works over
    // the generated header (LISR is ro32 — this engine never writes status)
    // and over the test double (whose "hardware side" raises flags by writing).
    template <unsigned S>
    static auto& isr_reg() {
        if constexpr (S < 4u) {
            return r().LISR;
        } else {
            return r().HISR;
        }
    }
    template <unsigned S>
    static auto& ifcr_reg() {
        if constexpr (S < 4u) {
            return r().LIFCR;
        } else {
            return r().HIFCR;
        }
    }

    // One stream's masks in its (L/H) pair — read side and write-1-clear side.
    struct stream_flags {
        std::uint32_t tc, ht, te, dme;             // in LISR/HISR
        std::uint32_t ctc, cht, cte, cdme, cfe;    // in LIFCR/HIFCR
    };

    // Every mask taken from the GENERATED explicit fields; the irregular slot
    // packing (per-stream base bits 0/6/16/22, split across two registers) is
    // stated once, in registers/st/dma_v2.yaml, and never re-derived here.
    template <unsigned S>
    static constexpr stream_flags flags_of() {
        constexpr stream_flags table[8] = {
            {IP::tcif0.mask, IP::htif0.mask, IP::teif0.mask, IP::dmeif0.mask,
             IP::ctcif0.mask, IP::chtif0.mask, IP::cteif0.mask, IP::cdmeif0.mask,
             IP::cfeif0.mask},
            {IP::tcif1.mask, IP::htif1.mask, IP::teif1.mask, IP::dmeif1.mask,
             IP::ctcif1.mask, IP::chtif1.mask, IP::cteif1.mask, IP::cdmeif1.mask,
             IP::cfeif1.mask},
            {IP::tcif2.mask, IP::htif2.mask, IP::teif2.mask, IP::dmeif2.mask,
             IP::ctcif2.mask, IP::chtif2.mask, IP::cteif2.mask, IP::cdmeif2.mask,
             IP::cfeif2.mask},
            {IP::tcif3.mask, IP::htif3.mask, IP::teif3.mask, IP::dmeif3.mask,
             IP::ctcif3.mask, IP::chtif3.mask, IP::cteif3.mask, IP::cdmeif3.mask,
             IP::cfeif3.mask},
            {IP::tcif4.mask, IP::htif4.mask, IP::teif4.mask, IP::dmeif4.mask,
             IP::ctcif4.mask, IP::chtif4.mask, IP::cteif4.mask, IP::cdmeif4.mask,
             IP::cfeif4.mask},
            {IP::tcif5.mask, IP::htif5.mask, IP::teif5.mask, IP::dmeif5.mask,
             IP::ctcif5.mask, IP::chtif5.mask, IP::cteif5.mask, IP::cdmeif5.mask,
             IP::cfeif5.mask},
            {IP::tcif6.mask, IP::htif6.mask, IP::teif6.mask, IP::dmeif6.mask,
             IP::ctcif6.mask, IP::chtif6.mask, IP::cteif6.mask, IP::cdmeif6.mask,
             IP::cfeif6.mask},
            {IP::tcif7.mask, IP::htif7.mask, IP::teif7.mask, IP::dmeif7.mask,
             IP::ctcif7.mask, IP::chtif7.mask, IP::cteif7.mask, IP::cdmeif7.mask,
             IP::cfeif7.mask},
        };
        return table[S];
    }

    static void enable_controller() { alloy::gate_on(Inst::gate); }

    // Completion + half-transfer callbacks and their latches. Storage is per
    // (controller, stream): `callback` is a template, so each S gets its own
    // statics without a runtime table. The latch rationale is the v1 engine's,
    // witnessed for THIS engine by test_st_dma_v2_latch.cpp: the ISR must
    // clear the hardware flag (level-triggered line) but wait()/done() poll
    // that same flag, so the ISR hands the fact over here and the polled
    // accessors report either source.
    template <unsigned S>
    struct callback {
        static inline void (*fn)(void*) = nullptr;
        static inline void* ctx = nullptr;
        static inline volatile bool latched = false;
        static inline void (*half_fn)(void*) = nullptr;
        static inline void* half_ctx = nullptr;
        static inline volatile bool half_latched = false;
    };

    // Program one stream (idle after this returns from stop()'s EN poll).
    // Peripheral address is fixed; memory increments. Item count is in ITEMS
    // of `msize`, not bytes. Signature and semantics identical to
    // st_dma_v1_engine::setup so every facade compiles unchanged; `request`
    // lands in CHSEL here instead of a DMAMUX row.
    template <unsigned S>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph_addr, std::uintptr_t mem_addr,
                      std::uint16_t items, std::uint8_t request) {
        static_assert(S < Inst::feat::streams,
                      "stream outside this DMA instance (chip data feat.streams). "
                      "dma_v2 streams are 0-BASED (S0..S7, ST's own numbering) — "
                      "unlike dma_v1's 1-based channels; do not add or drop a 1");
        stop<S>();           // polls EN down: SxCR/SxNDTR/... writable only then
        clear_flags<S>();    // EN set is IGNORED while any of the five flags is up
        callback<S>::latched = false;       // a new transfer is not already complete
        callback<S>::half_latched = false;  // ...nor already half-done
        // Direct mode, explicitly: a previous owner may have enabled the FIFO.
        IP::dmdis.write(sfcr<S>(), 0u);
        IP::feie.write(sfcr<S>(), 0u);
        spar<S>() = static_cast<std::uint32_t>(periph_addr);
        sm0ar<S>() = static_cast<std::uint32_t>(mem_addr);
        sndtr<S>() = items;
        scr<S>() =
            // DIR is 2 bits on this engine (value 2 = mem-to-mem, dma2-only,
            // not offered): p2m = 0, m2p = 1.
            ((d == dir::mem_to_periph ? 0x1u : 0x0u) << IP::dir.pos) |
            (circular ? IP::circ.mask() : 0u) |
            IP::minc.mask() |
            (static_cast<std::uint32_t>(psize) << IP::psize.pos) |
            (static_cast<std::uint32_t>(msize) << IP::msize.pos) |
            (0x2u << IP::pl.pos) |  // high priority, above default traffic
            // The route's request id selects this stream's request SOURCE.
            // Masked to the field: on 3-bit-CHSEL dies bit 28 is reserved, and
            // the chip's validated dma_routes never carry a request past what
            // the die decodes (measured in the register data's provenance).
            ((static_cast<std::uint32_t>(request) & IP::chsel.raw_mask())
             << IP::chsel.pos) |
            // SxCR is written whole here and may only be written while EN=0,
            // so the interrupt enables are folded in at setup rather than
            // switched on later — the v1 fold rule, unchanged. DMEIE rides
            // with TCIE/TEIE: a direct-mode error is a completion-path event.
            (callback<S>::fn != nullptr
                 ? (IP::tcie.mask() | IP::teie.mask() | IP::dmeie.mask())
                 : 0u) |
            (callback<S>::half_fn != nullptr ? IP::htie.mask() : 0u);
    }

    // Facades always call setup() (which cleared the flags) before start() —
    // a bare re-start() of a COMPLETED stream would be silently ignored by
    // hardware, because EN is refused while the stream's flags are set.
    template <unsigned S>
    static void start() {
        scr<S>() = scr<S>() | IP::en.mask();
    }

    // EN=0 requests a stop; hardware completes the in-flight transfer (and
    // FIFO drain) before clearing it, so poll until it reads 0 — reconfiguring
    // earlier is the RM-documented corruption. Falls straight through when
    // hardware already cleared EN itself (non-circular transfer end) or the
    // stream was never started.
    template <unsigned S>
    static void stop() {
        scr<S>() = scr<S>() & ~IP::en.mask();
        while ((scr<S>() & IP::en.mask()) != 0u) {
        }
    }

    template <unsigned S>
    [[nodiscard]] static bool complete() {
        // Either source: the hardware flag, or the latch the ISR set on its
        // way past — without the latch a polled wait() would spin forever on
        // a stream whose interrupt already consumed TCIF.
        return callback<S>::latched ||
               (isr_reg<S>() & flags_of<S>().tc) != 0u;
    }
    template <unsigned S>
    [[nodiscard]] static bool error() {
        constexpr stream_flags fs = flags_of<S>();
        return (isr_reg<S>() & (fs.te | fs.dme)) != 0u;
    }

    // Live remaining-transfer count, in items. Circular mode reloads it to the
    // programmed count at wrap — the transient 0 right at the reload edge is
    // the caller's to fold (alloy::dma::ring does, in cursor()/write_pos()).
    template <unsigned S>
    [[nodiscard]] static std::uint16_t remaining() {
        return static_cast<std::uint16_t>(sndtr<S>());
    }

    // The half-transfer twin of complete<S>().
    template <unsigned S>
    [[nodiscard]] static bool half() {
        return callback<S>::half_latched ||
               (isr_reg<S>() & flags_of<S>().ht) != 0u;
    }

    // All five of the stream's flags — the "EN is ignored while any is set"
    // precondition, cleared in one LIFCR/HIFCR store (write-1-to-clear; whole
    // write, never read-modify-write of a write-only register).
    template <unsigned S>
    static void clear_flags() {
        constexpr stream_flags fs = flags_of<S>();
        ifcr_reg<S>() = fs.ctc | fs.cht | fs.cte | fs.cdme | fs.cfe;
    }

    // The vector is PER STREAM on this engine, but the completion and half
    // handlers still share it, so each returns immediately when its own flag
    // is down — the same safe-no-op contract alloy::irq states for every
    // attachable handler. Guard and clear both read the HARDWARE flags, and
    // the clear names ONLY the flags this handler consumed: at the ring wrap
    // HTIF and TCIF can be up together, and a blanket per-stream clear here
    // would eat the half event before its own handler saw it.
    template <unsigned S>
    static void complete_isr(void*) {
        constexpr stream_flags fs = flags_of<S>();
        const std::uint32_t flags = isr_reg<S>();
        const bool done = (flags & fs.tc) != 0u;
        const bool failed = (flags & (fs.te | fs.dme)) != 0u;
        if (!done && !failed) {
            return;
        }
        ifcr_reg<S>() = (done ? fs.ctc : 0u) |
                        ((flags & fs.te) != 0u ? fs.cte : 0u) |
                        ((flags & fs.dme) != 0u ? fs.cdme : 0u);
        callback<S>::latched = true;
        if (callback<S>::fn != nullptr) {
            // The callback sees a stream whose flags are already cleared, so
            // it may program the next transfer without racing its own
            // completion. (In non-circular mode hardware has ALSO dropped EN
            // by now — setup()'s stop-poll falls straight through.)
            (void)failed;
            callback<S>::fn(callback<S>::ctx);
        }
    }

    // The half-transfer ISR: same contract, same latch discipline. A separate
    // function (not a branch in complete_isr) so each event's attach/detach
    // stays independent — alloy::irq chains both on the stream's line.
    template <unsigned S>
    static void half_isr(void*) {
        constexpr stream_flags fs = flags_of<S>();
        if ((isr_reg<S>() & fs.ht) == 0u) {
            return;
        }
        ifcr_reg<S>() = fs.cht;
        callback<S>::half_latched = true;
        if (callback<S>::half_fn != nullptr) {
            callback<S>::half_fn(callback<S>::half_ctx);
        }
    }

    template <unsigned S>
    static void enable_complete_irq(void (*fn)(void*), void* ctx) {
        callback<S>::fn = fn;
        callback<S>::ctx = ctx;
        alloy::irq::attach(irq_line_of<S>(), &complete_isr<S>);
        alloy::irq::enable(irq_line_of<S>());
    }

    template <unsigned S>
    static void disable_complete_irq() {
        // Leave the NVIC line enabled: the half handler may still be attached
        // to it, and a detached event's handler is simply no longer chained.
        scr<S>() = scr<S>() &
                   ~(IP::tcie.mask() | IP::teie.mask() | IP::dmeie.mask());
        alloy::irq::detach(irq_line_of<S>(), &complete_isr<S>);
        callback<S>::fn = nullptr;
        callback<S>::ctx = nullptr;
    }

    // Register BEFORE the transfer's setup(), like the completion callback and
    // for the same whole-SxCR-write rule — setup() folds HTIE in when this is
    // set.
    template <unsigned S>
    static void enable_half_irq(void (*fn)(void*), void* ctx) {
        callback<S>::half_fn = fn;
        callback<S>::half_ctx = ctx;
        alloy::irq::attach(irq_line_of<S>(), &half_isr<S>);
        alloy::irq::enable(irq_line_of<S>());
    }

    template <unsigned S>
    static void disable_half_irq() {
        scr<S>() = scr<S>() & ~IP::htie.mask();
        alloy::irq::detach(irq_line_of<S>(), &half_isr<S>);
        callback<S>::half_fn = nullptr;
        callback<S>::half_ctx = nullptr;
    }

    // Which vector stream S raises is CHIP DATA — the instance's irq_lines
    // ranges, looked up like st_exti_g0 looks up its line grouping. Nothing
    // about the mapping is written down in this file: the per-member hardcode
    // (dma_v1's ch_irqline1/ch_irqline2_3/ch_irqline4_7) is why that driver
    // cannot serve a die with a different grouping, and it is not repeated.
    template <unsigned S>
    static constexpr int group_of() {
        for (unsigned i = 0; i < Inst::irq_line_count; ++i) {
            if (S >= Inst::irq_lines[i].first && S <= Inst::irq_lines[i].last) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    template <unsigned S>
    static constexpr alloy::irq_line irq_line_of() {
        static_assert(S < Inst::feat::streams);
        constexpr int group = group_of<S>();
        static_assert(group >= 0,
                      "no irq_lines group in the chip data covers this DMA stream");
        return Inst::irq_lines[static_cast<unsigned>(group)].irq;
    }
};

}  // namespace alloy::hal::detail
