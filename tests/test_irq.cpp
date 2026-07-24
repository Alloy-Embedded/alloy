// Unit tests for the interrupt layer's per-line handler chain (src/alloy/irq.hpp).
// The chain is what lets peripherals that SHARE a vector (STM32G0's
// USART3_USART4_LPUART1, G0B1's five-way UART line) each attach their own
// handler and all run. On silicon the head table + count come from the
// generated vector_table.c; here the host build supplies them directly.

#include <cstdint>

#include "alloy/irq.hpp"
#include "alloy_test.hpp"

// Stand in for the generated g_alloy_irq_heads[]/count (vector_table.c on
// silicon). Declared extern "C" node*[] in irq.hpp; defined here for the host.
// Non-braced extern "C" so the const keeps external linkage (a const inside a
// braced extern "C" {} would fall back to internal linkage and not match).
extern "C" alloy::irq::node* g_alloy_irq_heads[8]{};
extern "C" const std::uint16_t g_alloy_irq_slot_count = 8;

namespace {
int g_a = 0;
int g_b = 0;
void handler_a(void*) { ++g_a; }
void handler_b(void*) { ++g_b; }

int g_ctx_seen = 0;
void handler_ctx(void* ctx) { g_ctx_seen = *static_cast<int*>(ctx); }
}  // namespace

ALLOY_TEST(irq_shared_line_runs_every_handler) {
    const alloy::irq_line line{3};
    g_a = 0;
    g_b = 0;

    alloy::irq::attach(line, &handler_a);
    alloy::irq::attach(line, &handler_b);  // same vector — the shared-line case
    alloy::irq::dispatch(line.number);
    ALLOY_CHECK_EQ(g_a, 1);
    ALLOY_CHECK_EQ(g_b, 1);  // BOTH ran, not just the first attacher

    // Detaching one leaves the co-sharer attached and the line still armed.
    alloy::irq::detach(line, &handler_a);
    alloy::irq::dispatch(line.number);
    ALLOY_CHECK_EQ(g_a, 1);  // gone
    ALLOY_CHECK_EQ(g_b, 2);  // still fires

    // Detaching the last empties the chain.
    alloy::irq::detach(line, &handler_b);
    alloy::irq::dispatch(line.number);
    ALLOY_CHECK_EQ(g_b, 2);
    ALLOY_CHECK(g_alloy_irq_heads[line.number] == nullptr);
}

ALLOY_TEST(irq_lines_are_independent) {
    const alloy::irq_line l1{5};
    const alloy::irq_line l2{6};
    g_a = 0;
    g_b = 0;

    alloy::irq::attach(l1, &handler_a);
    alloy::irq::attach(l2, &handler_b);
    alloy::irq::dispatch(l1.number);
    ALLOY_CHECK_EQ(g_a, 1);
    ALLOY_CHECK_EQ(g_b, 0);  // l2's handler did not run

    alloy::irq::detach(l1, &handler_a);
    alloy::irq::detach(l2, &handler_b);
}

ALLOY_TEST(irq_dispatch_empty_line_is_noop) {
    // dispatch() on a never-attached line reports "nothing ran" (false) — the
    // Xtensa path relies on this to mask a spuriously-pending source.
    ALLOY_CHECK(!alloy::irq::dispatch(7u));
}

ALLOY_TEST(irq_handler_receives_its_ctx) {
    const alloy::irq_line line{2};
    int token = 0xABC;
    alloy::irq::attach(line, &handler_ctx, &token);
    g_ctx_seen = 0;
    alloy::irq::dispatch(line.number);
    ALLOY_CHECK_EQ(g_ctx_seen, 0xABC);
    alloy::irq::detach(line, &handler_ctx);
}

ALLOY_TEST(irq_detach_returns_nodes_to_the_pool) {
    // Attach+detach far more times than the pool holds: if detach did not free
    // its node the pool (kMaxHandlers) would exhaust and attach would trap.
    const alloy::irq_line line{4};
    for (int i = 0; i < 100; ++i) {
        alloy::irq::attach(line, &handler_a);
        alloy::irq::detach(line, &handler_a);
    }
    ALLOY_CHECK(g_alloy_irq_heads[line.number] == nullptr);
}
