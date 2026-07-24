// User-facing interrupt layer — arch-neutral: line enable/disable go
// through alloy::arch::irq_line_*, implemented per arch (Cortex-M NVIC;
// Xtensa interrupt matrix + level-1 CPU-line allocator + INTENABLE).
//
// Model: the generated vector table emits one weak wrapper per IRQ line that
// calls alloy_irq_dispatch(n); dispatch walks a per-line CHAIN of handlers and
// calls each. The chain heads live in the generated g_alloy_irq_heads[] (sized
// by the chip's IRQ count); the chain nodes come from a small static pool here.
//
// Several peripherals can share one vector (STM32G0's USART3_USART4_LPUART1,
// G0B1's five-way UART vector). Each attaches its own handler and dispatch runs
// them all — so two UARTs that share a vector both work. The contract each
// attachable ISR must honor: be a safe no-op when its own peripheral has no
// pending status (every in-tree driver ISR already checks its RXNE/status
// first). A strong <NAME>_IRQHandler symbol anywhere in the link still overrides
// the weak wrapper entirely (modm-style zero-latency expert path).
//
// Two honest traps (deliberate, documented):
//  - attaching the SAME (line, fn) twice without detaching traps: it would run
//    that ISR twice per interrupt. detach(line, fn) first to re-attach.
//  - the handler pool (kMaxHandlers) exhausting traps: raise it if a design
//    legitimately needs more concurrently-attached IRQ handlers.

#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/core/types.hpp"

namespace alloy::irq {

using handler = void (*)(void*);

// One handler in a per-line chain. A free pool node has fn == nullptr.
struct node {
    handler fn;
    void* ctx;
    node* next;
};

// Upper bound on concurrently-attached IRQ handlers across the whole firmware.
// Generous for a superloop app; exhaustion traps in attach() rather than
// silently dropping an interrupt.
inline constexpr std::size_t kMaxHandlers = 16;
inline node g_pool[kMaxHandlers]{};

extern "C" {
// Per-line chain heads, defined in the generated vector_table.c / irq_data.c
// (sized by the chip IRQ count). Declared there as void*[] — a chain head is
// opaque to the C codegen; same pointer layout, reinterpreted here as node*.
extern node* g_alloy_irq_heads[];
extern const std::uint16_t g_alloy_irq_slot_count;
}

namespace detail {
inline node* alloc_node() {
    for (node& n : g_pool) {
        if (n.fn == nullptr) {
            return &n;
        }
    }
    return nullptr;
}
}  // namespace detail

// Link a handler into `line`'s chain. Distinct handlers on one shared line
// coexist; a duplicate (line, fn) traps.
inline void attach(alloy::irq_line line, handler fn, void* ctx = nullptr) {
    if (line.number >= g_alloy_irq_slot_count || fn == nullptr) {
        __builtin_trap();
    }
    const arch::irq_state saved = arch::irq_save();
    for (node* p = g_alloy_irq_heads[line.number]; p != nullptr; p = p->next) {
        if (p->fn == fn) {
            arch::irq_restore(saved);
            __builtin_trap();  // already attached — detach before re-attaching
        }
    }
    node* n = detail::alloc_node();
    if (n == nullptr) {
        arch::irq_restore(saved);
        __builtin_trap();  // handler pool exhausted — raise kMaxHandlers
    }
    n->fn = fn;
    n->ctx = ctx;
    n->next = g_alloy_irq_heads[line.number];
    g_alloy_irq_heads[line.number] = n;
    arch::irq_restore(saved);
}

inline void enable(alloy::irq_line line) { arch::irq_line_enable(line.number); }

inline void disable(alloy::irq_line line) { arch::irq_line_disable(line.number); }

// Set a line's interrupt priority (call after enable(), which sets a mid-scale
// default). level 0 == most urgent; higher == less urgent. Effective resolution
// depends on the part's implemented priority bits.
inline void set_priority(alloy::irq_line line, std::uint8_t level) {
    if (line.number >= g_alloy_irq_slot_count) {
        __builtin_trap();
    }
    arch::irq_line_priority(line.number, level);
}

// Unlink `fn` from `line`'s chain. The NVIC/matrix line is disabled only when
// the chain empties — a co-sharing peripheral may still need the vector armed.
inline void detach(alloy::irq_line line, handler fn) {
    if (line.number >= g_alloy_irq_slot_count) {
        __builtin_trap();
    }
    const arch::irq_state saved = arch::irq_save();
    node** link = &g_alloy_irq_heads[line.number];
    while (*link != nullptr && (*link)->fn != fn) {
        link = &(*link)->next;
    }
    if (*link != nullptr) {
        node* found = *link;
        *link = found->next;
        found->fn = nullptr;  // return the node to the pool
        found->ctx = nullptr;
        found->next = nullptr;
    }
    const bool empty = g_alloy_irq_heads[line.number] == nullptr;
    arch::irq_restore(saved);
    if (empty) {
        arch::irq_line_disable(line.number);
    }
}

// Run every handler attached to `line`; returns true if any ran. Called from
// the arch vector wrappers. Caller guarantees line < g_alloy_irq_slot_count
// (the generated wrapper only ever passes valid line numbers).
inline bool dispatch(unsigned line) {
    node* p = g_alloy_irq_heads[line];
    const bool ran = p != nullptr;
    while (p != nullptr) {
        node* next = p->next;  // cache: a handler may detach itself mid-dispatch
        p->fn(p->ctx);
        p = next;
    }
    return ran;
}

// RAII critical section that masks only interrupts at `level` OR LESS urgency,
// leaving MORE-urgent lines (e.g. a control-loop ISR) live — the real-time
// alternative to a full irq_save. On ARMv7-M this raises BASEPRI; on ARMv6-M and
// Xtensa (no priority mask) it degrades to a full mask. Nesting only tightens.
//
//   {
//       alloy::irq::critical_section cs{4};   // mask level 4 and any less-urgent line
//       shared = update();                    // a level-0 control ISR still fires here
//   }
class critical_section {
    arch::irq_state saved_;

public:
    explicit critical_section(std::uint8_t level) : saved_(arch::irq_save_below(level)) {}
    ~critical_section() { arch::irq_restore_below(saved_); }
    critical_section(const critical_section&) = delete;
    critical_section& operator=(const critical_section&) = delete;
    critical_section(critical_section&&) = delete;
    critical_section& operator=(critical_section&&) = delete;
};

}  // namespace alloy::irq
