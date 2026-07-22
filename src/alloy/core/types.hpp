// Core value types shared between generated device descriptors and
// hand-written drivers. Generated code carries FACTS in these types;
// drivers supply BEHAVIOR that consumes them.

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"

namespace alloy {

// A peripheral clock-enable bit: absolute register address + mask + write
// style, all resolved by codegen from device data.
//   rmw       — read-modify-write the enable bit (ST RCC *ENR registers)
//   write_set — the register is a write-only SET register; write the mask
//               directly (Microchip PMC_PCERx — an RMW would read garbage)
struct clock_gate {
    enum class style : std::uint8_t { rmw, write_set };
    std::uintptr_t reg;
    std::uint32_t mask;
    style kind = style::rmw;
};

inline void gate_on(clock_gate g) {
    auto& r = *reinterpret_cast<rw32*>(g.reg);
    if (g.kind == clock_gate::style::write_set) {
        r = g.mask;
        return;  // set-registers are write-only; nothing meaningful to read back
    }
    r = r | g.mask;
    // Read back to guarantee the enable has taken effect before the first
    // peripheral access (required on several vendors' bus bridges).
    const std::uint32_t readback = r;
    (void)readback;
}

struct irq_line {
    std::uint16_t number;
};

// Which clock a peripheral's kernel runs from. Codegen maps the chip data's
// kernel_clock string onto this enum; drivers resolve it against the board's
// clock profile.
enum class clock_node : std::uint8_t { sysclk, ahb, apb };

// Pin-mux signals. Codegen validates chip-data signal names against this
// list and fails generation on unknown names (facts fail at generation,
// never at compile).
enum class signal : std::uint8_t {
    tx, rx, cts, rts, ck,
    sck, miso, mosi, cs,
    scl, sda,
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
