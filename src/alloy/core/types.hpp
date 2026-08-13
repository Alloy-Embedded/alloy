// Core value types shared between generated device descriptors and
// hand-written drivers. Generated code carries FACTS in these types;
// drivers supply BEHAVIOR that consumes them.

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"

namespace alloy {

// A peripheral clock-enable/reset-release bit: absolute register address(es)
// + mask + style, all resolved by codegen from device data.
//   rmw           — read-modify-write the enable bit (ST RCC *ENR registers)
//   write_set     — write-only SET register; write the mask directly
//                   (Microchip PMC_PCERx — an RMW would read garbage)
//   reset_release — CLEAR the bit to release the block from reset, then poll
//                   the same bit high in done_reg (RP2040 RESETS/RESET_DONE)
struct clock_gate {
    enum class style : std::uint8_t { rmw, write_set, reset_release };
    std::uintptr_t reg;
    std::uint32_t mask;
    style kind = style::rmw;
    std::uintptr_t done_reg = 0;
};

inline void gate_on(clock_gate g) {
    auto& r = *reinterpret_cast<rw32*>(g.reg);
    switch (g.kind) {
        case clock_gate::style::write_set:
            r = g.mask;  // write-only set-register; nothing meaningful to read back
            return;
        case clock_gate::style::reset_release: {
            r = r & ~g.mask;
            auto& done = *reinterpret_cast<ro32*>(g.done_reg);
            while ((done & g.mask) == 0u) {
            }
            return;
        }
        case clock_gate::style::rmw: {
            r = r | g.mask;
            // Read back to guarantee the enable has taken effect before the
            // first peripheral access (several vendors' bus bridges need it).
            const std::uint32_t readback = r;
            (void)readback;
            return;
        }
    }
}

struct irq_line {
    std::uint16_t number;
};

// One inclusive range of a multi-line controller's lines, and the vector that
// range raises. Some controllers do not have an `irq` — they have several, and
// which one a line reaches is a per-die fact: an STM32G0's EXTI groups its 16
// pin lines onto three vectors (0-1, 2-3, 4-15) while an F4 splits the same 16
// across seven. Codegen emits the chip's `irq_lines` as an array of these, so a
// driver LOOKS THE GROUPING UP instead of writing the split down.
struct irq_line_range {
    std::uint16_t first;  // inclusive
    std::uint16_t last;   // inclusive
    irq_line irq;
};

// Which clock a peripheral's kernel runs from. Codegen maps the chip data's
// kernel_clock string onto this enum; drivers resolve it against the board's
// clock profile.
enum class clock_node : std::uint8_t { sysclk, ahb, apb, apb2 };

// Pin-mux signals. Codegen validates chip-data signal names against this
// list and fails generation on unknown names (facts fail at generation,
// never at compile).
enum class signal : std::uint8_t {
    tx, rx, cts, rts, ck,
    sck, miso, mosi, cs,
    scl, sda,
    ch1, ch2, ch3, ch4,
    // The COMPLEMENTARY halves of the first three channels, and the break
    // input. Only an advanced-control timer has them, which is why they were
    // not here until a driver needed them: a pin routed to TIM1_CH1N is a
    // different route from the same pin routed to TIM1_CH1, and without an
    // enumerator of its own it could not be told apart at compile time.
    // ch4n, bk2 and etr stay unmodelled — the routes are in the chip database
    // and alloy/routes_gen.hpp lists what it skipped.
    ch1n, ch2n, ch3n, bk,
    in, out,
};

// One step of a data-driven clock program. Emitted by codegen with all
// addresses/masks/values fully resolved; executed by run_clock_program().
struct clock_step {
    enum class op : std::uint8_t { write, rmw, poll, delay } kind;
    std::uintptr_t addr;      // unused for delay
    std::uint32_t mask;       // rmw: field mask; poll: field mask
    std::uint32_t value;      // write/rmw: pre-shifted value; poll: expected; delay: microseconds
    std::uint32_t timeout_us; // poll only
};

}  // namespace alloy
