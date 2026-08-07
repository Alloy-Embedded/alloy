// DMA driver for the ST v1 channel engine + DMAMUX request router
// (G0-era pairing; the mux is the instance's `mux` companion).
//
// Rules honored (RM0444 §10/§11): CCR/CNDTR/CPAR/CMAR may only be written
// with EN=0; hardware clears EN ONLY on a transfer error — completion
// leaves EN=1, so completion is TCIF, never EN; IFCR is write-1-to-clear;
// DMAMUX channel (Ch-1 + Inst::ch_mux_offset) feeds this instance's
// channel Ch (offset 0 for DMA1; 7 for a future G0B1 DMA2). Channels are
// 1-based like ST documentation and are TEMPLATE parameters — the per-
// channel flag positions come from the generated repeat-field accessors,
// which need a compile-time index.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/dma/dma_impl.hpp"
#include "alloy/ip/st/dma_v1.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// Constrained on the IP tag AND the mux companion: this driver models the
// DMAMUX pairing specifically (the dma_v1 layout also exists on families
// with fixed or CSELR routing — those get their own driver when curated).
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::dma_v1> &&
             requires { typename Inst::mux_t; }
struct dma_impl<Inst> {
    using IP = typename Inst::ip;
    using MUX = typename Inst::mux_t;

    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };

    // Capability: this IP has a CIRC bit, so circular streams are supported.
    // dma::channel gates start_m2p_circular_u16 on this at compile time.
    static constexpr bool supports_circular = true;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    template <unsigned Ch>
    static rw32& ccr() {
        return alloy::reg_at(Inst::base, IP::CCR_offset, IP::CCR_stride, Ch - 1);
    }
    template <unsigned Ch>
    static rw32& cndtr() {
        return alloy::reg_at(Inst::base, IP::CNDTR_offset, IP::CNDTR_stride, Ch - 1);
    }
    template <unsigned Ch>
    static rw32& cpar() {
        return alloy::reg_at(Inst::base, IP::CPAR_offset, IP::CPAR_stride, Ch - 1);
    }
    template <unsigned Ch>
    static rw32& cmar() {
        return alloy::reg_at(Inst::base, IP::CMAR_offset, IP::CMAR_stride, Ch - 1);
    }
    template <unsigned Ch>
    static rw32& mux_ccr() {
        return alloy::reg_at(MUX::base, MUX::ip::CCR_offset, MUX::ip::CCR_stride,
                             Ch - 1 + Inst::ch_mux_offset);
    }

    static void enable_controller() { alloy::gate_on(Inst::gate); }

    // Program one channel (must be idle). Peripheral address is fixed;
    // memory increments. Item count is in ITEMS of `msize`, not bytes.
    // msize/psize are separate: a halfword WRITE through the APB bridge is
    // duplicated across the word, which corrupts 32-bit-capable peripheral
    // registers (TIM2 CCR1 read back with the halfword repeated in BOTH
    // halves on real silicon — compare value in the millions, LED stuck on) — peripheral-bound streams use psize=b32 and the engine
    // zero-extends each 16-bit memory item (RM0444 width/endianness table).
    template <unsigned Ch>
    static void setup(dir d, bool circular, width msize, width psize,
                      std::uintptr_t periph_addr, std::uintptr_t mem_addr,
                      std::uint16_t items, std::uint8_t request) {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count,
                      "channel outside this DMA instance (chip data ch_count)");
        stop<Ch>();
        clear_flags<Ch>();
        MUX::ip::dmareq_id.write(mux_ccr<Ch>(), request);
        cpar<Ch>() = static_cast<std::uint32_t>(periph_addr);
        cmar<Ch>() = static_cast<std::uint32_t>(mem_addr);
        cndtr<Ch>() = items;
        ccr<Ch>() = (d == dir::mem_to_periph ? IP::dir.mask() : 0u) |
                    (circular ? IP::circ.mask() : 0u) |
                    IP::minc.mask() |
                    (static_cast<std::uint32_t>(psize) << IP::psize.pos) |
                    (static_cast<std::uint32_t>(msize) << IP::msize.pos) |
                    (0x2u << IP::pl.pos) |  // high priority, above default traffic
                    // CCR is written whole here and may only be written while
                    // EN=0, so the interrupt enables have to be folded in at
                    // setup rather than switched on later — otherwise every
                    // start_*() would silently clear the callback's TCIE.
                    (callback<Ch>::fn != nullptr
                         ? (IP::tcie.mask() | IP::teie.mask())
                         : 0u);
    }

    template <unsigned Ch>
    static void start() {
        ccr<Ch>() = ccr<Ch>() | IP::en.mask();
    }

    template <unsigned Ch>
    static void stop() {
        ccr<Ch>() = ccr<Ch>() & ~IP::en.mask();
    }

    template <unsigned Ch>
    [[nodiscard]] static bool complete() {
        return (r().ISR & IP::template tcif<Ch - 1>.mask) != 0u;
    }
    template <unsigned Ch>
    [[nodiscard]] static bool error() {
        return (r().ISR & IP::template teif<Ch - 1>.mask) != 0u;
    }

    template <unsigned Ch>
    static void clear_flags() {
        r().IFCR = IP::template cgif<Ch - 1>.mask;  // clears the channel's 4 flags
    }

    // Completion callbacks. Storage is per (controller, channel): `callback` is
    // a template, so each Ch gets its own statics without a runtime table.
    template <unsigned Ch>
    struct callback {
        static inline void (*fn)(void*) = nullptr;
        static inline void* ctx = nullptr;
    };

    // Channels SHARE NVIC lines on this IP (2-3 together, 4-7 together), so this
    // runs for interrupts belonging to other channels. Returning immediately
    // when our own TCIF is clear is what makes several channels attachable to
    // one line — the same contract alloy::irq states for every shared handler.
    template <unsigned Ch>
    static void complete_isr(void*) {
        if (!complete<Ch>() && !error<Ch>()) {
            return;
        }
        const bool failed = error<Ch>();
        clear_flags<Ch>();
        if (callback<Ch>::fn != nullptr) {
            // The callback sees a channel whose flags are already cleared, so it
            // may start the next transfer without racing its own completion.
            (void)failed;
            callback<Ch>::fn(callback<Ch>::ctx);
        }
    }

    template <unsigned Ch>
    static void enable_complete_irq(void (*fn)(void*), void* ctx) {
        callback<Ch>::fn = fn;
        callback<Ch>::ctx = ctx;
        alloy::irq::attach(irq_line_of<Ch>(), &complete_isr<Ch>);
        alloy::irq::enable(irq_line_of<Ch>());
    }

    template <unsigned Ch>
    static void disable_complete_irq() {
        // Leave the shared NVIC line enabled: another channel may still be using
        // it, and its handler is a no-op for interrupts that are not its own.
        ccr<Ch>() = ccr<Ch>() & ~(IP::tcie.mask() | IP::teie.mask());
        alloy::irq::detach(irq_line_of<Ch>(), &complete_isr<Ch>);
        callback<Ch>::fn = nullptr;
        callback<Ch>::ctx = nullptr;
    }

    // IRQ line grouping is IP-version behavior; the numbers are chip data.
    template <unsigned Ch>
    static constexpr alloy::irq_line irq_line_of() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        if constexpr (Ch == 1) {
            return alloy::irq_line{Inst::ch_irqline1};
        } else if constexpr (Ch <= 3) {
            return alloy::irq_line{Inst::ch_irqline2_3};
        } else {
            return alloy::irq_line{Inst::ch_irqline4_7};
        }
    }
};

}  // namespace alloy::hal
