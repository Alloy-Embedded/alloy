// ESP32 (Xtensa LX6) interrupt-line control + level-1 dispatch.
//
// The chip-data irq number is a MATRIX SOURCE (0..68). irq_line_enable
// allocates a free level-1 CPU interrupt from a fixed pool, routes the
// source to it through the DPORT interrupt matrix (address/stride come
// from the GENERATED irq_data.c — facts stay generated), and sets the
// INTENABLE bit. The asm dispatch loop (vectors.S) calls
// alloy_irq_dispatch(cpu_int); the source mapping lives here.
//
// Pool rationale (esp-idf int_desc): 12,13,17,18,9,2,3 are level-1
// external level-triggered lines; 0,1,4,5,8 stay radio-reserved, 10 is
// edge-type, 6/7/11/15/16/29 are internal, 14 is the NMI, 19+ need
// medium/high-level vectors (v1 traps them).

#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/core/mmio.hpp"
#include "alloy/irq.hpp"

extern "C" {
// Generated (emit/vectors.py xtensa branch): DPORT PRO_INTR_MAP array.
extern const unsigned long g_alloy_intmap_addr;
extern const unsigned short g_alloy_intmap_stride;
}

namespace {

constexpr unsigned char kPool[] = {12, 13, 17, 18, 9, 2, 3};

// cpu_int -> source + 1 (0 = free). Sized for all 32 CPU lines.
unsigned char g_cpu2src[32];

std::uint32_t intenable_read() {
    std::uint32_t v;
    __asm volatile("rsr %0, intenable" : "=a"(v));
    return v;
}

void intenable_write(std::uint32_t v) {
    __asm volatile("wsr %0, intenable\n\trsync" ::"a"(v));
}

}  // namespace

namespace alloy::arch {

void irq_line_enable(unsigned source) {
    if (source >= alloy::irq::g_alloy_irq_slot_count) {
        __builtin_trap();  // not a matrix source of this chip
    }
    const irq_state saved = irq_save();
    // Reuse an existing route for this source, else take a free pool line.
    unsigned cpu_int = 0xFF;
    for (unsigned char cand : kPool) {
        if (g_cpu2src[cand] == source + 1u) {
            cpu_int = cand;
            break;
        }
        if (cpu_int == 0xFF && g_cpu2src[cand] == 0u) {
            cpu_int = cand;
        }
    }
    if (cpu_int == 0xFF) {
        __builtin_trap();  // level-1 pool exhausted (7 concurrent sources)
    }
    g_cpu2src[cpu_int] = static_cast<unsigned char>(source + 1u);
    *reinterpret_cast<rw32*>(g_alloy_intmap_addr +
                             static_cast<std::uintptr_t>(g_alloy_intmap_stride) * source) =
        cpu_int;
    intenable_write(intenable_read() | (1u << cpu_int));
    irq_restore(saved);
}

void irq_line_disable(unsigned source) {
    if (source >= alloy::irq::g_alloy_irq_slot_count) {
        __builtin_trap();
    }
    const irq_state saved = irq_save();
    for (unsigned char cand : kPool) {
        if (g_cpu2src[cand] == source + 1u) {
            intenable_write(intenable_read() & ~(1u << cand));
            g_cpu2src[cand] = 0u;
            // Detach: route to CPU int 16 (internal CCOMPARE2, never
            // enabled by alloy) — the reset-state convention.
            *reinterpret_cast<rw32*>(
                g_alloy_intmap_addr +
                static_cast<std::uintptr_t>(g_alloy_intmap_stride) * source) = 16u;
            break;
        }
    }
    irq_restore(saved);
}

}  // namespace alloy::arch

// Called by the vectors.S dispatch loop, once per pending+enabled level-1
// CPU line, lowest first, re-polling INTERRUPT after each call. A handler
// that fails to clear its level-triggered source at the peripheral would
// livelock that loop — the driver ISRs own that contract.
extern "C" void alloy_irq_dispatch(unsigned cpu_int) {
    const unsigned mapped = g_cpu2src[cpu_int];
    if (mapped != 0u) {
        const alloy::irq::slot s = alloy::irq::g_alloy_irq_slots[mapped - 1u];
        if (s.fn != nullptr) {
            s.fn(s.ctx);
            return;
        }
    }
    // Unattached line fired: mask it so the dispatch loop cannot livelock.
    intenable_write(intenable_read() & ~(1u << cpu_int));
}
