// The programming sequence of the RP2040's DMA — the ENTIRE driver body, split
// out of raspberrypi_dma_v1.hpp so a host test can reach it (the driver proper
// is constrained on the generated alloy::ip::raspberrypi::dma_v1 tag, which
// exists only inside a built project; tests/ deliberately never reads generated
// headers). Same seam as st_dma_v1_body.hpp / st_dma_v2_body.hpp /
// microchip_xdmac_v1_body.hpp: the logic lives once, here; the constrained
// dma_impl<Inst> specialization in raspberrypi_dma_v1.hpp inherits it unchanged.
//
// THE FILE NAME IS NOT THE DESIGN DOC'S. docs/design/dma-streams.md §3.4 calls
// this driver `rp_dma_v1.hpp`. Codegen resolves a HAL driver as
// `alloy/hal/<class>/<vendor>_<ip>.hpp` and the curated IP is
// `raspberrypi/dma_v1`, so the doc's name would never be included — and the
// include is `.exists()`-guarded, so it would fail SILENTLY by simply not being
// there. The file is named for the resolver, not for the doc.
//
// ═══ READ THIS BEFORE TRUSTING ANYTHING THIS FILE CLAIMS ═══════════════════
//
// THIS FAMILY HAS NO EMULATION WITNESS AT ALL. Renode 1.16.1 ships no RP2040
// peripheral of any kind — no DMA model, no board, no CPU platform for this
// part — and alloy's emitter generates none. So the ONLY executable witness
// that exists for this engine is tests/test_rp_dma_v1_latch.cpp, running the
// code below over a hand-written register double. That test carries more
// weight here than its siblings do on G0/F7/SAME70, and it can still only
// prove REGISTER-SEQUENCE INTENT: which register gets which value in which
// order. It proves nothing about behaviour.
//
// AND A DMA LEG MUST NOT BE ADDED TO PAPER OVER THAT. alloy CAN already emit an
// rp2040 Renode platform (cortex-m0 + UART.PL011 are both generic types Renode
// has), and Renode reads 0 from unmapped addresses — so on such a platform this
// engine would read CTRL as 0 (BUSY low, no error), read TRANS_COUNT as 0, and
// report a completed transfer over a buffer nothing wrote. A green DMA leg
// there would be a FALSE GREEN, not weak evidence. Do not add one.
//
// The behavioural claims are therefore owed to hardware, and the one that
// matters most is not even transcribed: registers/raspberrypi/dma_v1.yaml marks
// "CH_CTRL_TRIG itself triggers" as INFERRED (the SVD annotates the other three
// view-0xC registers as triggers and not this one). Everything below about
// configuring through a non-trigger alias rests on that inference.
//
// WHAT WOULD SETTLE IT: docs/guide/rp2040-dma-hardware-checklist.md, which
// names the three unwitnessed claims (that CH_CTRL_TRIG triggers, the three
// DREQ ids, and that CHAN_ABORT flushes anything), gives each a falsifier, and
// carries the firmware to run them — because NO SHIPPED EXAMPLE REACHES THIS
// DRIVER yet: dma_uart and dma_probe both guard on uart.write_dma /
// adc.read_burst, and neither RP2040 facade driver has those hooks. Nothing on
// that sheet has been executed.
//
// ── WHAT THIS SILICON DOES NOT HAVE, and what follows from it ──────────────
//
// Proven by exhaustion over the curated register set (the curation enumerated
// every register and every field of the block from the pinned vendor SVD):
//
//  * NO HALF-TRANSFER EVENT. The seven interrupt registers carry one bit per
//    CHANNEL and nothing else, and CTRL has no half-transfer flag. So this
//    engine has no `half()`, no `enable_half_irq` and no `disable_half_irq` —
//    ABSENT, never a no-op stub, because a no-op half-arm is a lie in the
//    engine contract and `alloy::dma::ring` calls it unconditionally.
//  * NO CIRCULAR / AUTO-RELOAD-ON-WRAP BIT. A channel that runs its count to
//    zero HALTS. TRANS_COUNT does hold a reload value that each TRIGGER copies
//    into the live counter, but nothing issues that trigger by itself, and
//    CHAIN_TO cannot: naming its own channel is how the SVD says chaining is
//    DISABLED.
//  * RING_SIZE/RING_SEL WRAP THE ADDRESS, NOT THE COUNT.
//
// ── THE TWO CAPABILITY FLAGS ARE TWO FACTS AND HERE THEY ARE FALSE FOR
//    DIFFERENT REASONS ─────────────────────────────────────────────────────
//
// `supports_circular` is a statement about HARDWARE; `supports_ring` is a
// statement about THIS ENGINE'S CODE (alloy::dma::ring_capable's own header
// says so). On the two ST engines they are one fact spelled twice. On the SAM
// E70 XDMAC they diverge — no circular bit, but a ring out of linked
// descriptors. Here they are both false, and NOT because one implies the other:
//
//   supports_circular = false   Two independent reasons, either sufficient.
//                               (a) There is no circular/auto-reload bit in
//                               this IP at all. (b) The method it gates,
//                               start_m2p_circular_u16, feeds 16-bit memory
//                               items into 32-bit register writes, and CTRL has
//                               ONE DATA_SIZE field for both sides of a channel
//                               — the same shape as XDMAC's single DWIDTH, and
//                               the same permanent refusal.
//   supports_ring = false       This engine cannot present alloy::dma::ring's
//                               contract BY ANY MEANS on one channel, which is
//                               where it parts company with XDMAC. Two reasons
//                               again: there is no half event to deliver, and a
//                               single channel STOPS at the end of its count —
//                               a ring that silently goes empty forever after
//                               one lap is worse than no ring. Setting this
//                               true with stub events would compile, link, and
//                               then HANG in ring::take(), which spins by
//                               contract; that is the exact inversion of the
//                               compile-error promise the flag exists for.
//
// WHAT A USER LOSES, said plainly rather than left to be discovered: on this
// family `alloy::dma::ring` does not exist. Not degraded — constrained away, so
// a facade's `ring()`/`rx_ring()` is a compile error naming the capability. The
// one-shot surface (claim, start_p2m_u8/u16, start_m2p_u8, on_complete, wait,
// stop) is complete and is what this phase ships.
//
// THE NAMED SUCCESSOR, so the next phase inherits an argument and not a TODO.
// A full ring is reachable on ONE channel with a software ping-pong: program
// half the buffer, and have the completion ISR re-point the write address at
// the other half through CH_AL2_WRITE_ADDR_TRIG — a register at view offset
// 0xC, so that single store sets the address, reloads the count from the reload
// value TRANS_COUNT already holds, and re-triggers. The completion interrupt is
// then the half event and its parity is the wrap. It costs no second channel,
// no board-schema change and no descriptor memory. It is deferred here for a
// WITNESS reason rather than a scope one: every way it can fail — the DREQ
// stall window while the channel is halted between block and re-trigger, and
// the ISR re-arm deadline — is invisible to a host double, which calls the ISR
// synchronously and has no clock. Shipping it on the one family with no
// emulation would put the whole of the risk exactly where nothing in this
// project can see it.
//
// ── THE FOURTH STOP SHAPE ──────────────────────────────────────────────────
//
// docs/design/dma-streams.md §4 lists three: dma_v1 clears EN and is done;
// dma_v2 clears EN and POLLS it to 0; XDMAC writes GD and polls GS. This is the
// fourth. Clearing EN here only PAUSES — the SVD says BUSY stays high if it was
// high. Terminating means writing the channel's bit to CHAN_ABORT and POLLING
// CHAN_ABORT to all-zero, because until it reads zero, in-flight transfers are
// still draining through the address and data FIFOs and "it is unsafe to
// restart the channel". Same use-after-free class as dma_v2's poll: a caller
// that frees a buffer after a bare EN-clear is racing hardware that is still
// writing into it.
//
// ── ONE MORE HAZARD NO OTHER ENGINE IN THIS TREE HAS ───────────────────────
//
// CHAIN_TO's reset value is 0, which means "chain to channel 0". Every channel
// except channel 0 therefore ships out of reset configured to trigger channel 0
// on completion. Chaining is disabled by naming the channel's OWN number, so
// setup() writes CHAIN_TO = Ch — deliberately, every time, on every channel,
// including channel 0 where the value happens to coincide with the reset one.

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal::detail {

template <class Inst>
struct raspberrypi_dma_v1_engine {
    using IP = typename Inst::ip;

    // Direction is NOT a register bit on this IP. There is no DIR field: which
    // way a transfer runs is which side increments (INCR_READ / INCR_WRITE) and
    // which side the request paces. The enum is the portable vocabulary; setup()
    // below turns it into that pair of decisions.
    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    // The enum VALUE is the register encoding, as on both ST engines — CTRL's
    // DATA_SIZE is 0 = byte, 1 = halfword, 2 = word (transcribed in
    // registers/raspberrypi/dma_v1.yaml as BYTE/HALFWORD/WORD). Checked against
    // the curated enumeration rather than inherited from the ST engines.
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };

    static_assert(static_cast<std::uint32_t>(width::b8) == IP::data_size_byte);
    static_assert(static_cast<std::uint32_t>(width::b16) == IP::data_size_halfword);
    static_assert(static_cast<std::uint32_t>(width::b32) == IP::data_size_word);

    // See the two paragraphs in the file header: false for DIFFERENT reasons,
    // and neither is the other's consequence.
    static constexpr bool supports_circular = false;
    static constexpr bool supports_ring = false;

    // CHANNELS ARE 0-BASED, and there is no adjustment anywhere in this file.
    // The chip data records the base rather than the driver assuming it, so an
    // instance that ever said otherwise fails HERE, loudly, instead of quietly
    // addressing the wrong channel.
    static_assert(Inst::ch_first == 0u,
                  "this engine indexes the register arrays with the channel "
                  "number itself (CH0..CH11, the vendor's own numbering); a "
                  "1-based instance would be off by one in every register");

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // One accessor for the whole per-channel block, the XDMAC shape rather than
    // dma_v1's one-function-per-register: this IP has SIXTEEN registers per
    // channel (four real ones in four alias views), so a function apiece would
    // double the file for nothing.
    template <unsigned Ch>
    static rw32& chreg(std::uintptr_t offset, unsigned stride) {
        return alloy::reg_at(Inst::base, offset, stride, Ch);
    }

    // The four the engine actually uses. CTRL is reached through the AL1 view
    // ON PURPOSE and everywhere: CH_CTRL_TRIG at +0x0C is the view-0 TRIGGER,
    // so touching it during configuration would start the channel.
    template <unsigned Ch>
    static rw32& read_addr() {
        return chreg<Ch>(IP::CH_READ_ADDR_offset, IP::CH_READ_ADDR_stride);
    }
    template <unsigned Ch>
    static rw32& write_addr() {
        return chreg<Ch>(IP::CH_WRITE_ADDR_offset, IP::CH_WRITE_ADDR_stride);
    }
    template <unsigned Ch>
    static rw32& trans_count() {
        return chreg<Ch>(IP::CH_TRANS_COUNT_offset, IP::CH_TRANS_COUNT_stride);
    }
    template <unsigned Ch>
    static rw32& ctrl() {
        return chreg<Ch>(IP::CH_AL1_CTRL_offset, IP::CH_AL1_CTRL_stride);
    }

    // Bit `Ch` of every shared per-channel register (INTR, INTE0, INTS0,
    // MULTI_CHAN_TRIGGER, CHAN_ABORT) — one spelling, so a channel can never be
    // named one way in the arm and another in the guard.
    template <unsigned Ch>
    static constexpr std::uint32_t chbit() {
        return std::uint32_t{1} << Ch;
    }

    // THE RESTING CONFIG WORD: disabled, and chained to nobody. Written WHOLE
    // rather than read-modify-written, and that is not a style choice — CTRL
    // carries WRITE_ERROR and READ_ERROR, which are WRITE-1-TO-CLEAR, so a
    // read-modify-write of this register hands the error bits it just read back
    // to the hardware and silently erases the evidence error() reports. Every
    // CTRL write below is therefore a whole-word store.
    //
    // CHAIN_TO = Ch is what disables chaining (see the file header). Leaving it
    // 0 here would mean "when this channel is aborted or reprogrammed, trigger
    // channel 0" on every channel but channel 0.
    template <unsigned Ch>
    static constexpr std::uint32_t idle_ctrl() {
        return static_cast<std::uint32_t>(Ch) << IP::chain_to.pos;
    }

    static void enable_controller() { alloy::gate_on(Inst::gate); }

    // ── THE INTERRUPT LINE POLICY, IN ONE PLACE ────────────────────────────
    //
    // This block raises TWO NVIC vectors (DMA_IRQ_0 and DMA_IRQ_1) and WHICH
    // channel reaches WHICH one is a RUNTIME SOFTWARE CHOICE — set bit n of
    // INTE0, or of INTE1, or of both, or of neither. There is no silicon
    // grouping to look up, which is why the chip data binds one `irq` and does
    // NOT carry `irq_lines` ranges: ranges would assert a mapping this part does
    // not have.
    //
    // So the driver has to HAVE a policy, and this is it: every channel on line
    // 0 — the SVD's own suggestion ("it is also valid to ignore this behaviour
    // and just use INTE0/INTS0/IRQ 0") and pico-sdk's default. The constant
    // exists because the choice appears in THREE places that must agree — the
    // arm writes INTE_n, the handler guards on INTS_n, and the vector is the
    // instance's `irq` — and three copies of a policy is how one of them drifts
    // and the handler silently never runs.
    //
    // NOTHING HERE IS A GROUPING HARDCODE. st_dma_v1_body.hpp writes the G0's
    // channel->vector split into if-constexpr branches over three per-instance
    // members, which is exactly why that driver cannot serve a G4 and is named
    // as a defect twice in this tree. There is no split to write down here.
    static constexpr unsigned kIrqLine = 0u;

    static rw32& inte_reg() {
        if constexpr (kIrqLine == 0u) {
            return r().INTE0;
        } else {
            return r().INTE1;
        }
    }
    static rw32& ints_reg() {
        if constexpr (kIrqLine == 0u) {
            return r().INTS0;
        } else {
            return r().INTS1;
        }
    }

    // Completion callback and its latches. Storage is per (controller,
    // channel): `callback` is a template, so each Ch gets its own statics
    // without a runtime table — the shape all three existing engines use.
    //
    // ONE EVENT, NOT TWO. There is no half-transfer event on this IP, so there
    // is no half callback, no half latch and no half ISR. Their absence is the
    // reason `supports_ring` is false.
    template <unsigned Ch>
    struct callback {
        static inline void (*fn)(void*) = nullptr;
        static inline void* ctx = nullptr;
        // MANDATORY, exactly as on the two ST engines. complete_isr MUST clear
        // the channel's INTR bit or the level-triggered line re-fires forever;
        // complete() polls that same bit; so the ISR hands the fact over here
        // and complete() reports latch OR live bit. Without it a polled wait()
        // spins forever on a transfer whose interrupt already ran.
        static inline volatile bool latched = false;
        // DEFENSIVE HERE, AND SAYING SO IS THE POINT — this is the one place
        // this engine's latch story is WEAKER than its siblings', and pretending
        // otherwise would be the tidy version that rots.
        //
        // On both ST engines the error latch is load-bearing because the ISR's
        // required clear (IFCR/LIFCR) EATS the error flag. On XDMAC it is the
        // only record there will ever be, because CIS is cleared by reading.
        // Here NEITHER is true: the interrupt line is driven by INTR alone, so
        // clearing INTR is all the ISR must do, and the failure itself lives in
        // CTRL's READ_ERROR/WRITE_ERROR/AHB_ERROR — which no read destroys and
        // which this ISR deliberately does not clear (clearing them is not
        // needed to de-assert the line, and inventing a write the hardware does
        // not require would be modelling a hazard rather than the silicon).
        //
        // The latch is kept anyway, for two reasons that are real: it makes
        // error() answer the SAME way on all four engines, and it is the only
        // thing that would survive a future ISR that does clear CTRL. But it is
        // not currently the only source, and tests/test_rp_dma_v1_latch.cpp
        // reports honestly that reverting either error-latch line alone leaves
        // the suite GREEN — this engine's known dead spot, named the way
        // microchip_xdmac_v1_body.hpp names its 1-of-40.
        static inline volatile bool err_latched = false;
    };

    // Program one channel (idle after this returns from stop()'s abort poll).
    // Signature and semantics identical to the three existing engines, so
    // alloy::dma::channel, alloy::dma::pair and every facade compile unchanged.
    //
    // `request` lands in CTRL's TREQ_SEL — a 6-bit field, so unlike dma_v2's
    // CHSEL there is real masking to get right, and unlike dma_v1 there is no
    // separate router row to write.
    //
    // ITEMS vs the register: TRANS_COUNT is 32-BIT on this part while the
    // portable contract is uint16_t (dma_v1/dma_v2 count registers are 16-bit
    // and the whole stack is typed for them). The register is wider than the
    // parameter; the parameter is not widened here, because a wider count would
    // not port.
    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph_addr, std::uintptr_t mem_addr,
                      std::uint16_t items, std::uint8_t request) {
        static_assert(Ch < Inst::ch_count,
                      "channel outside this DMA instance (chip data "
                      "channels.count). RP2040 channels are 0-BASED (CH0..CH11, "
                      "the vendor's own numbering) — do not add or drop a 1");
        if (msize != psize) {
            __builtin_trap();  // ONE DATA_SIZE field for both sides
        }
        if (circular) {
            // Unreachable through the shipped surface — supports_circular and
            // supports_ring are both false, so neither start_m2p_circular_u16
            // nor alloy::dma::ring exists on this engine — but the parameter is
            // in the portable signature, so a hand-wired caller can still pass
            // it. A trap is the honest answer: this IP has no circular mode and
            // a channel programmed as if it did would run once and stop.
            __builtin_trap();
        }
        stop<Ch>();        // abort-and-drain; see the fourth-shape note
        clear_flags<Ch>();
        // The latch resets go AFTER the purge, matching all three sibling
        // engines — and on THIS engine that order is a convention, not a
        // mechanism. Saying so because the first draft of this comment claimed
        // it was load-bearing and the revert bar caught it: swapping these two
        // lines above clear_flags() leaves the whole suite GREEN (measured).
        // XDMAC must order them this way because its clear_flags() harvests a
        // clear-on-read register and therefore SETS the latches; this one only
        // writes, so it cannot re-latch anything and there is nothing for the
        // ordering to protect. It is kept so that one reading of setup() serves
        // all four engines — which is worth a line of comment and not a claim.
        callback<Ch>::latched = false;      // a new transfer is not already complete
        callback<Ch>::err_latched = false;  // ...and does not inherit the last
                                            // transfer's failure
        const bool to_periph = d == dir::mem_to_periph;
        // No DIR bit: the direction IS this pair of decisions. The peripheral
        // side never moves; the memory side walks.
        read_addr<Ch>() =
            static_cast<std::uint32_t>(to_periph ? mem_addr : periph_addr);
        write_addr<Ch>() =
            static_cast<std::uint32_t>(to_periph ? periph_addr : mem_addr);
        // Writing TRANS_COUNT sets the RELOAD value; the trigger start() issues
        // copies it into the live counter. This view (+0x08) is NOT a trigger —
        // the AL1 copy at +0x1C is, and writing that one here would start the
        // channel before its control word existed.
        trans_count<Ch>() = items;
        // ...and the control word, LAST and through the NON-TRIGGER alias.
        // This is the single biggest sequencing divergence from the three
        // engines already in the tree: they configure and set the enable inside
        // ONE register, because on their silicon writing that register does not
        // start anything. Here CH_CTRL_TRIG at +0x0C does, so EN goes in here
        // (through CH_AL1_CTRL, +0x10) and the START is a separate store.
        ctrl<Ch>() =
            IP::en.mask() |
            IP::high_priority.mask() |  // above default traffic, like ST's PL=2
            (static_cast<std::uint32_t>(msize) << IP::data_size.pos) |
            // Exactly one side increments (see above); the other is a fixed
            // peripheral register.
            (to_periph ? IP::incr_read.mask() : IP::incr_write.mask()) |
            // Chaining OFF, spelled as this channel's own number — the reset
            // value 0 would mean "trigger channel 0" on every other channel.
            idle_ctrl<Ch>() |
            // The route's request id paces the channel. Masked to the field:
            // TREQ_SEL is 6 bits and `request` is a uint8_t, so an id that does
            // not fit is truncated here rather than smeared into IRQ_QUIET.
            ((static_cast<std::uint32_t>(request) & IP::treq_sel.raw_mask())
             << IP::treq_sel.pos);
        // Interrupt arming, folded into setup() exactly as all three existing
        // engines do — and here, as on XDMAC, that is a CHOICE and not a
        // constraint. ST must fold: CCR/SxCR carry the enables, may only be
        // written while the channel is disabled, and are written whole. INTE0
        // is a separate register writable at any time. The fold is kept so that
        // one caller sequence (on_complete() then the transfer) means the same
        // thing on all four controllers. Do not "simplify" it into
        // enable_complete_irq without deciding, on purpose, to change WHEN
        // interrupts start being delivered relative to the transfer.
        //
        // ONE BIT SERVES BOTH OUTCOMES: a channel raises its INTR bit at
        // transfer complete AND when it halts on a bus error, so there is no
        // separate error enable to ride along the way ST's TEIE rides TCIE.
        if (callback<Ch>::fn != nullptr) {
            inte_reg() = inte_reg() | chbit<Ch>();
        }
    }

    // Start. MULTI_CHAN_TRIGGER is bit-per-channel and "bit n is exactly a
    // write to channel n's trigger register" — so this is a single global store
    // that starts the channel WITHOUT rewriting any configuration, which is
    // what the three trigger aliases cannot do. (It is also why a future pair
    // could start two channels in one store.) The channel starts only if EN is
    // set, which setup() did.
    template <unsigned Ch>
    static void start() {
        r().MULTI_CHAN_TRIGGER = chbit<Ch>();
    }

    // THE FOURTH STOP SHAPE (file header, and docs/design/dma-streams.md §4).
    // Clearing EN does NOT abort — it stops the channel responding to triggers
    // and PAUSES the sequence, with BUSY still high if it was high. CHAN_ABORT
    // is the abort, and the SVD's instruction is explicit: poll it to all-zero,
    // because until then in-flight transfers are still draining through the
    // address and data FIFOs and it is unsafe to restart the channel.
    //
    // THE POLL IS NOT OPTIONAL AND IT IS NOT DECORATION. alloy::dma::channel's
    // stop() and every scoped owner call this and then hand the buffer back;
    // returning while hardware is still writing into it is a use-after-free
    // with the DMA as the writer — the same class as dma_v2's poll-EN-to-0.
    //
    // ORDER: EN down FIRST, so a trigger that is already pending cannot restart
    // the channel in the middle of the drain; the abort second; the poll last.
    // Both CTRL writes are WHOLE-WORD (see idle_ctrl) and neither carries a
    // write-1-to-clear bit, so stopping a channel does not destroy the error
    // evidence error() reports — clearing that is clear_flags()'s job, and the
    // split matters because channel::wait() consults error() after done().
    // Each poll reads the register into a LOCAL before testing it, rather than
    // spelling `(r().CHAN_ABORT & bit) != 0`. That is not style: it keeps every
    // status read in this engine a plain load whose result is a value, which is
    // what lets the host double give CHAN_ABORT and INTR the read/write
    // protocols the silicon has (a drain that finishes, a write-1-to-clear)
    // instead of leaving them as plain memory words that no test can animate.
    template <unsigned Ch>
    static void stop() {
        ctrl<Ch>() = idle_ctrl<Ch>();
        r().CHAN_ABORT = chbit<Ch>();
        for (;;) {
            const std::uint32_t draining = r().CHAN_ABORT;
            if ((draining & chbit<Ch>()) == 0u) {
                return;
            }
        }
    }

    // Either source: the latch the ISR set on its way past, or the live bit for
    // the polled-only case — the ST spelling, available here because nothing in
    // this IP's status path is destroyed by a read.
    //
    // THE SOURCE IS INTR, NOT BUSY, and that is a decision worth a line. BUSY
    // (CTRL bit 24) is this engine's nearest analogue of XDMAC's GS, and using
    // it would inherit GS's flaw — a channel that never started also reads BUSY
    // low, so complete() would be true before start() — plus one of its own: on
    // silicon BUSY does not necessarily rise in the same cycle as the trigger
    // store, so a wait() could fall straight through a transfer that had not
    // begun. INTR is the real event ("transfer complete, or halted on a bus
    // error"), it is sticky until written, and it is raised regardless of
    // whether any INTE bit routes it anywhere — so a polled-only consumer sees
    // it with no interrupt configured at all.
    template <unsigned Ch>
    [[nodiscard]] static bool complete() {
        static_assert(Ch < Inst::ch_count);
        const std::uint32_t raw = r().INTR;
        return callback<Ch>::latched || (raw & chbit<Ch>()) != 0u;
    }

    // Either source, like complete(). AHB_ERROR is the read-only OR of
    // READ_ERROR and WRITE_ERROR, so one non-destructive read answers for both
    // failure modes — and reading CTRL through the AL1 alias cannot start
    // anything, which is why even the accessors use it. Transfer-scoped:
    // setup() clears both the bits and the latch, so error() reports THIS
    // transfer and never the last one.
    template <unsigned Ch>
    [[nodiscard]] static bool error() {
        static_assert(Ch < Inst::ch_count);
        return callback<Ch>::err_latched ||
               (ctrl<Ch>() & IP::ahb_error.mask()) != 0u;
    }

    // Live remaining-transfer count, in items. A plain register read, like both
    // ST engines and unlike XDMAC's synthesis — TRANS_COUNT counts down through
    // the whole programmed transfer and reads as the transfers REMAINING. There
    // is no ring on this engine, so there is no wrap to fold and no parity flag
    // to sample: the retry loop XDMAC needs is absent on purpose, not by
    // omission. The register is 32-bit and the contract is 16-bit; the cast is
    // the same narrowing the portable count type already implies.
    template <unsigned Ch>
    [[nodiscard]] static std::uint16_t remaining() {
        static_assert(Ch < Inst::ch_count);
        return static_cast<std::uint16_t>(trans_count<Ch>());
    }

    // TWO HOMES, NOT ONE — say it out loud, because every other engine in this
    // tree clears its whole status in a single store.
    //   * The channel's interrupt bit lives in INTR and is write-1-to-clear.
    //     Clearing it THERE rather than through INTS0 clears the raw source, so
    //     it is right whichever line (or neither) the channel was routed to.
    //   * The bus-error bits live in CTRL and are separately write-1-to-clear.
    // Both stores are whole-word writes to write-1-to-clear registers: never a
    // read-modify-write, which on CTRL would hand the error bits straight back.
    template <unsigned Ch>
    static void clear_flags() {
        static_assert(Ch < Inst::ch_count);
        r().INTR = chbit<Ch>();
        ctrl<Ch>() =
            idle_ctrl<Ch>() | IP::write_error.mask() | IP::read_error.mask();
    }

    // Every channel shares ONE NVIC line here (all twelve, by this driver's
    // policy — see kIrqLine), so this handler runs for interrupts belonging to
    // other channels: returning immediately when our own bit is down is the
    // shared-line contract alloy::irq states for every chained handler.
    //
    // THE GUARD READS INTS_n, NOT INTR, AND THAT IS THE TWO-LINE DISPATCH. INTR
    // is the raw status of every channel regardless of routing; INTS0 is
    // "channels currently asserting DMA_IRQ_0" (INTR masked by INTE0, plus the
    // INTF0 force). A handler on line 0 that guarded on INTR would consume — and
    // clear — the completion of a channel deliberately routed to DMA_IRQ_1,
    // whose own handler would then find nothing. Guarding on the SAME line
    // register the arm wrote is what keeps the two lines independent.
    //
    // THE CLEAR is INTR, and it names ONLY this channel's bit: a wider store
    // would eat a sibling channel's completion that rose between the read and
    // the write. (Writing INTS0 would clear the same underlying bit; INTR is
    // used for symmetry with clear_flags(), which must work for a channel that
    // was never routed to a line at all.)
    template <unsigned Ch>
    static void complete_isr(void*) {
        if ((ints_reg() & chbit<Ch>()) == 0u) {
            return;
        }
        r().INTR = chbit<Ch>();
        // The error latch is written BEFORE the completion latch and before the
        // callback runs: a consumer that sees done() must already be able to see
        // why, and a callback calling error() from interrupt context gets the
        // truth about the transfer that just ended. (Here the live CTRL bits
        // would answer too — see the err_latched note — but the ORDER is the
        // portable contract and is kept identical on all four engines.)
        if ((ctrl<Ch>() & IP::ahb_error.mask()) != 0u) {
            callback<Ch>::err_latched = true;
        }
        callback<Ch>::latched = true;
        if (callback<Ch>::fn != nullptr) {
            // The callback sees a channel whose interrupt bit is already
            // cleared, so it may start the next transfer without racing its own
            // completion.
            callback<Ch>::fn(callback<Ch>::ctx);
        }
    }

    template <unsigned Ch>
    static void enable_complete_irq(void (*fn)(void*), void* ctx) {
        callback<Ch>::fn = fn;
        callback<Ch>::ctx = ctx;
        // INTE_n is NOT written here — setup() folds it in, see the note there.
        // Registering after a transfer has started therefore reports from the
        // NEXT setup(), exactly as on the three existing engines.
        alloy::irq::attach(irq_line_of<Ch>(), &complete_isr<Ch>);
        alloy::irq::enable(irq_line_of<Ch>());
    }

    template <unsigned Ch>
    static void disable_complete_irq() {
        // Leave the shared NVIC line enabled: eleven other channels may still
        // be using it, and every handler is a no-op for interrupts that are not
        // its own. Only this channel's routing bit comes down. INTE_n has no
        // write-1-to-clear bits, so a read-modify-write is correct here — and
        // it is the ONE register in this engine that is written that way.
        inte_reg() = inte_reg() & ~chbit<Ch>();
        alloy::irq::detach(irq_line_of<Ch>(), &complete_isr<Ch>);
        callback<Ch>::fn = nullptr;
        callback<Ch>::ctx = nullptr;
    }

    // NO enable_half_irq / disable_half_irq / half(). Their ABSENCE is the
    // engine contract's answer to "this IP has no half-transfer event", and it
    // is what makes alloy::dma::ring_capable false in a second, independent way
    // (the first is supports_ring). A no-op stub here would satisfy the concept
    // and hang in ring::take(); see the file header.

    // ONE line for the whole block, from CHIP DATA. Nothing about the
    // channel->vector map is written down in this file, because on this part
    // there is nothing to write down: the map is INTE0/INTE1, a runtime choice,
    // and this driver's choice is kIrqLine. The instance's `irq` is the vector
    // that line raises — and the chip data deliberately does NOT carry
    // `irq_lines` ranges, which would assert a silicon grouping this part does
    // not have.
    template <unsigned Ch>
    static constexpr alloy::irq_line irq_line_of() {
        static_assert(Ch < Inst::ch_count);
        static_assert(kIrqLine == 0u,
                      "the chip data binds ONE vector for this controller and "
                      "kIrqLine says which INTE/INTS pair the driver arms and "
                      "guards; a second line needs a second curated vector "
                      "before this returns anything else");
        return Inst::irq;
    }
};

}  // namespace alloy::hal::detail
