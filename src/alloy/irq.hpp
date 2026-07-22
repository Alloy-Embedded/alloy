// User-facing interrupt layer — arch-neutral: line enable/disable go
// through alloy::arch::irq_line_*, implemented per arch (Cortex-M NVIC;
// Xtensa interrupt matrix + level-1 CPU-line allocator + INTENABLE).
//
// Model: the generated vector table emits one weak wrapper per IRQ line
// that calls alloy_irq_dispatch(n); dispatch indexes a RAM slot table of
// {fn, ctx} sized by the chip's IRQ count (also generated). attach() fills
// a slot, enable() arms the NVIC line at a mid-scale default priority.
//
// Two honest v1 traps (deliberate, documented):
//  - attaching to an OCCUPIED line traps: several peripherals can share one
//    vector (G0's USART3_USART4_LPUART1) and silently replacing a handler
//    would drop events. detach() first if replacement is intended.
//  - a strong <NAME>_IRQHandler symbol anywhere in the link still overrides
//    the weak wrapper entirely (modm-style zero-latency expert path).

#pragma once

#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/core/types.hpp"

namespace alloy::irq {

using handler = void (*)(void*);

struct slot {
    handler fn;
    void* ctx;
};

extern "C" {
// Defined in the generated vector_table.c (size = chip IRQ count).
extern slot g_alloy_irq_slots[];
extern const std::uint16_t g_alloy_irq_slot_count;
}

inline void attach(alloy::irq_line line, handler fn, void* ctx = nullptr) {
    if (line.number >= g_alloy_irq_slot_count) {
        __builtin_trap();
    }
    slot& s = g_alloy_irq_slots[line.number];
    const arch::irq_state saved = arch::irq_save();
    if (s.fn != nullptr && s.fn != fn) {
        __builtin_trap();  // occupied (possibly shared) line — detach first
    }
    s.ctx = ctx;
    s.fn = fn;
    arch::irq_restore(saved);
}

inline void enable(alloy::irq_line line) { arch::irq_line_enable(line.number); }

inline void disable(alloy::irq_line line) { arch::irq_line_disable(line.number); }

inline void detach(alloy::irq_line line) {
    disable(line);
    slot& s = g_alloy_irq_slots[line.number];
    const arch::irq_state saved = arch::irq_save();
    s.fn = nullptr;
    s.ctx = nullptr;
    arch::irq_restore(saved);
}

}  // namespace alloy::irq
