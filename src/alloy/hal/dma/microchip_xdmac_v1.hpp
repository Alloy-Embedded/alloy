// DMA driver for the SAM E70 XDMAC — v1 one-shot single-microblock
// transfers (XDMAC has NO circular bit; rings need linked-list
// descriptors via CNDA/CNDC — a future phase).
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

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/dma/dma_impl.hpp"
#include "alloy/ip/microchip/xdmac_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::xdmac_v1>
struct dma_impl<Inst> {
    using IP = typename Inst::ip;

    enum class dir : std::uint8_t { periph_to_mem, mem_to_periph };
    enum class width : std::uint8_t { b8 = 0, b16 = 1, b32 = 2 };

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    template <unsigned Ch>
    static rw32& chreg(std::uintptr_t offset, unsigned stride) {
        return alloy::reg_at(Inst::base, offset, stride, Ch - 1);
    }

    static void enable_controller() { alloy::gate_on(Inst::gate); }

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

    // The channel auto-disables at end of block: GS bit falls back to 0.
    // NOTE: a bus error also auto-disables the channel (GS -> 0), so
    // "complete" alone cannot distinguish success — the L3 wait() does a
    // FINAL error() check after the GS poll for exactly this reason.
    template <unsigned Ch>
    [[nodiscard]] static bool complete() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        return (r().GS & IP::template st<Ch - 1>.mask) == 0u;
    }

    // Bus errors / request overflow surface in CIS; reading clears, which
    // is fine — completion is tracked via GS, never via CIS.BIS here.
    template <unsigned Ch>
    [[nodiscard]] static bool error() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        const std::uint32_t cis = chreg<Ch>(IP::CIS_offset, IP::CIS_stride);
        return (cis & (IP::rbeis.mask() | IP::wbeis.mask() | IP::rois.mask())) != 0u;
    }

    template <unsigned Ch>
    static void clear_flags() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        // A discarded FUNCTION-CALL returning volatile& performs NO read
        // ([expr.context]/2) — bind to a local so the clear-by-read is real.
        const std::uint32_t stale = chreg<Ch>(IP::CIS_offset, IP::CIS_stride);
        (void)stale;
    }

    // One NVIC line for the whole block (chip data irq).
    template <unsigned Ch>
    static constexpr alloy::irq_line irq_line_of() {
        static_assert(Ch >= 1 && Ch <= Inst::ch_count);
        return Inst::irq;
    }
};

}  // namespace alloy::hal
