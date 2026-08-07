// Pin-interrupt driver for the ST exti_g0 IP (STM32G0-era EXTI).
//
// BEHAVIOR only: every address, offset and vector number comes from the
// generated alloy::ip::st::exti_g0 header and the generated instance
// descriptor. Configurable line n is pin n of whichever port EXTICR selects,
// so ONE line number is shared by all six ports — PA13 and PC13 cannot both
// have a pin interrupt at the same time. That is silicon, not a driver limit.
//
// THREE things here are the whole job, and each is a trap this project has
// already paid for:
//
// 1. THE PENDING REGISTERS ARE WRITE-1-TO-CLEAR, AND THE GENERATED FIELD
//    ACCESSORS MUST NOT TOUCH THEM. alloy::field_t::set() is a read-modify-
//    write (`r = r | mask`): on RPR1/FPR1 that reads every OTHER line's pending
//    bit and writes it back as a 1, silently eating edges belonging to other
//    pins. field_t::clear() writes zeros, which on write-1-to-clear clears
//    NOTHING and leaves the handler re-entering forever. So the ISR does a BARE
//    STORE of its own bit and nothing else.
//
// 2. THE PENDING REGISTER IS SPLIT. A rising edge latches in RPR1 and a falling
//    edge in FPR1; clearing one does not clear the other. `pin_edge::both` arms
//    both triggers, so the ISR must clear whichever half is set.
//
// 3. NO POLLED API MAY READ RPR1/FPR1. st_dma_v1 needs a software latch because
//    its ISR eats the very flag wait() polls. There is no such conflict here —
//    gpio::input::is_high() reads the port's IDR, not EXTI — and this driver
//    keeps it that way on purpose: `edges<Line>()` counts in SOFTWARE, in the
//    ISR, so nothing outside the handler ever depends on the hardware bit. A
//    `pending()` that read RPR1/FPR1 would recreate the DMA hazard exactly.
//
// WHAT IS AND IS NOT CHECKED. The emulation leg (tests/emulation/pin_irq.robot)
// genuinely exercises the port select, the trigger selection and delivery to the
// right shared vector — each was broken in turn and the leg failed. TWO THINGS
// ARE NOT CHECKED BY ANYTHING, and no silicon was available to check them on:
//   - THE IMR1 UNMASK. Renode's stand-in model leaves IMR1 unimplemented and
//     delivers the interrupt whether or not it is written, so a driver that
//     forgot to unmask would still pass. This is the opposite of the SPI
//     precedent, where forgetting RXNEIE makes the test fail.
//   - WHICH PENDING HALF IS CLEARED. That model latches BOTH edge directions in
//     FPR1 and deasserts on a write to EITHER register, so clearing the wrong
//     one would pass here and hang on real hardware. This driver clears each
//     half it finds set, which is correct for the silicon described in the
//     reference manual — but that is reading, not measurement.
// For the same reason this driver deliberately exposes NO "which edge fired"
// API: it could not be validated, and an unvalidated direction report is worse
// than none.
//
// COST, said out loud rather than left to be discovered: one alloy::irq::node
// per ARMED PIN, out of a firmware-wide pool of alloy::irq::kMaxHandlers (16 at
// the time of writing) shared with UART, SPI, I2C and DMA — and attach() TRAPS
// when it runs out. The alternative, one handler per vector that scans the
// pending register and dispatches, would spend fewer nodes but force a
// 16-entry RAM callback table that exists whether or not any pin is armed. A
// per-pin handler costs exactly nothing when the pin is not armed, because
// `slot<Line>`'s members are only instantiated on odr-use.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/exti/exti_impl.hpp"
#include "alloy/ip/st/exti_g0.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::exti_g0>
struct exti_impl<Inst> {
    using IP = typename Inst::ip;

    // Lines 16 and up are internal/direct sources with no port select; this
    // driver serves the GPIO range only.
    static constexpr unsigned line_count = 16u;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // Per-line callback storage. `slot` is a TEMPLATE, so each line gets its
    // own statics with no runtime table and an unarmed line costs zero bytes.
    template <unsigned Line>
    struct slot {
        static inline void (*fn)(void*) = nullptr;
        static inline void* ctx = nullptr;
        // Software edge counter, bumped by the ISR. NOT the hardware pending
        // bit — see note 3 in the header comment. Monotonic, so a caller that
        // was away can see it missed edges rather than being told "one".
        static inline volatile std::uint32_t count = 0u;
        static inline volatile std::uint32_t taken = 0u;
    };

    // Which of the die's vectors a line raises. EXTI does not have one vector,
    // and WHICH grouping it has is a per-die fact (G0: 0-1, 2-3, 4-15; F4
    // splits the same 16 across seven), so unlike st_dma_v1's hand-written
    // if-constexpr chain the GROUPING ITSELF is chip data here — nothing about
    // the split is written down in this file.
    template <unsigned Line>
    static constexpr int group_of() {
        for (unsigned i = 0; i < Inst::irq_line_count; ++i) {
            if (Line >= Inst::irq_lines[i].first && Line <= Inst::irq_lines[i].last) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    template <unsigned Line>
    static constexpr alloy::irq_line irq_line_of() {
        constexpr int group = group_of<Line>();
        static_assert(group >= 0,
                      "no irq_lines group in the chip data covers this EXTI line");
        return Inst::irq_lines[static_cast<unsigned>(group)].irq;
    }

    // Up to twelve pins SHARE one vector on this IP, so this runs for edges
    // belonging to other lines. Returning immediately when neither of our own
    // pending bits is set is what makes several pins attachable to one line —
    // the same contract alloy::irq states for every shared handler, and the
    // same reason the DMA channels can share theirs.
    template <unsigned Line>
    static void edge_isr(void*) {
        constexpr std::uint32_t bit = 1u << Line;
        auto& regs = r();
        const std::uint32_t rising = regs.RPR1 & bit;
        const std::uint32_t falling = regs.FPR1 & bit;
        if ((rising | falling) == 0u) {
            return;  // not our line
        }
        // Bare stores of OUR bit only — see note 1. Cleared BEFORE the callback
        // so the callback may re-arm this pin without racing its own re-entry,
        // the identical ordering st_dma_v1 and st_spi_v2 both document.
        if (rising != 0u) {
            regs.RPR1 = bit;
        }
        if (falling != 0u) {
            regs.FPR1 = bit;
        }
        slot<Line>::count = slot<Line>::count + 1u;
        if (slot<Line>::fn != nullptr) {
            slot<Line>::fn(slot<Line>::ctx);
        }
    }

    // EXTICR: one BYTE per line, `exti` usable bits wide, register Line/4
    // covering lines 4i..4i+3 with line L at bit (L % 4) * 8.
    template <unsigned Line>
    static void select_port(unsigned port_index) {
        rw32& cr = alloy::reg_at(Inst::base, IP::EXTICR_offset, IP::EXTICR_stride, Line / 4u);
        const unsigned shift = (Line % 4u) * 8u;
        const std::uint32_t mask = IP::exti.raw_mask() << shift;
        cr = (cr & ~mask) | ((port_index & IP::exti.raw_mask()) << shift);
    }

    // Route port `PortIndex`'s pin `Line` into this controller and report its
    // edges. Re-arming an already-armed line is allowed and replaces the edge
    // and the callback (alloy::irq::attach traps on a duplicate, so the detach
    // below is load-bearing, not defensive).
    template <unsigned PortIndex, unsigned Line>
    static void arm(pin_edge e, void (*fn)(void*), void* ctx) {
        static_assert(Line < line_count,
                      "st/exti_g0 routes GPIO pins on lines 0..15 only");
        constexpr std::uint32_t bit = 1u << Line;
        auto& regs = r();

        // Mask and disarm both triggers first: nothing may be delivered while
        // the port select and the callback slot are being changed.
        regs.IMR1 = regs.IMR1 & ~bit;
        regs.RTSR1 = regs.RTSR1 & ~bit;
        regs.FTSR1 = regs.FTSR1 & ~bit;

        select_port<Line>(PortIndex);
        slot<Line>::fn = fn;
        slot<Line>::ctx = ctx;
        slot<Line>::count = 0u;
        slot<Line>::taken = 0u;

        // Drop anything latched while the pin was being configured, so the
        // first edge reported is one the caller could actually have caused.
        regs.RPR1 = bit;
        regs.FPR1 = bit;

        alloy::irq::detach(irq_line_of<Line>(), &edge_isr<Line>);
        alloy::irq::attach(irq_line_of<Line>(), &edge_isr<Line>);
        alloy::irq::enable(irq_line_of<Line>());

        // Trigger and unmask LAST — the slot and the vector are ready now.
        if (e != pin_edge::falling) {
            regs.RTSR1 = regs.RTSR1 | bit;
        }
        if (e != pin_edge::rising) {
            regs.FTSR1 = regs.FTSR1 | bit;
        }
        regs.IMR1 = regs.IMR1 | bit;
    }

    template <unsigned Line>
    static void disarm() {
        constexpr std::uint32_t bit = 1u << Line;
        auto& regs = r();
        regs.IMR1 = regs.IMR1 & ~bit;
        regs.RTSR1 = regs.RTSR1 & ~bit;
        regs.FTSR1 = regs.FTSR1 & ~bit;
        // Leave the shared vector ENABLED: alloy::irq::detach disables it only
        // when the chain empties, and another pin on the same group may still
        // need it — its handler is a no-op for edges that are not its own.
        alloy::irq::detach(irq_line_of<Line>(), &edge_isr<Line>);
        slot<Line>::fn = nullptr;
        slot<Line>::ctx = nullptr;
        regs.RPR1 = bit;
        regs.FPR1 = bit;
    }

    template <unsigned Line>
    [[nodiscard]] static std::uint32_t edges() {
        return slot<Line>::count;
    }

    // Consume ONE counted edge. Returns false once the caller has caught up,
    // so a superloop drains exactly as many edges as happened instead of
    // collapsing a burst into a single "something happened".
    template <unsigned Line>
    static bool take_edge() {
        if (slot<Line>::taken == slot<Line>::count) {
            return false;
        }
        slot<Line>::taken = slot<Line>::taken + 1u;
        return true;
    }
};

}  // namespace alloy::hal
