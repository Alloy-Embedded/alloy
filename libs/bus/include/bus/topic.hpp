// alloy::lib::bus — typed publish/subscribe over intrusive per-type lists.
//
// The topic IS the C++ type: publish(temp_reading{...}) reaches every live
// subscriber<temp_reading> in the program, resolved at compile time — no
// registry, no string names, no IDs for purely-local messages. (Crossing a
// wire to another board is where declared IDs appear; that lives in
// bus/wire.hpp, a later phase, not here.)
//
// Fan-out is genuinely new surface in alloy: every existing layer is
// single-owner by construction (one rx callback per uart instance, one
// waiter per slot, one claim per DMA channel). This module is the one place
// that multiplexes, so its concurrency story is spelled out precisely:
//
//   - The subscriber list is intrusive and static-storage: nodes live inside
//     subscriber/watch_route objects, linked at construction, UNLINKED IN THE
//     DESTRUCTOR (same obligation delay's timer_node carries — a dead node
//     left linked would make the next publish walk freed memory).
//   - publish() walks the list under arch::irq_save()/irq_restore() — the
//     same full-mask idiom executor_core::schedule() uses. That serializes
//     any mix of thread-context and ISR-context publishers on a single core,
//     and makes link/unlink safe against a publish landing mid-walk.
//   - The mask is held for the WHOLE walk: per subscriber, one copy of
//     sizeof(T) plus ring-index bookkeeping plus (at most) one executor
//     schedule. Keep messages small; the bus is the slow plane (events,
//     telemetry, mode/config changes) — a 20 kHz control loop calls a
//     function, it does not publish. Measured costs are in the README.
//   - Multi-core targets: irq_save masks THIS core only. The local bus is
//     single-core by contract; a cross-core transport is a future bridge,
//     not a property of these lists.
//
// Delivery order is reverse registration order (LIFO link) and is NOT a
// contract — code that needs ordering between consumers has one consumer.

#pragma once

#include <type_traits>

#include "alloy/arch/irq.hpp"

namespace alloy::lib::bus {

namespace detail {

// Type-erased delivery hook. One per subscriber/watch_route/bridge_route,
// resident in that object. `wire` marks bridge/sniffer nodes: a message
// republished FROM the wire skips them (single-hop, structural anti-echo —
// consumed by the bridge phase; local publish delivers to everyone).
struct node {
    bool (*deliver)(void* owner, const void* msg) = nullptr;
    void* owner = nullptr;
    node* next = nullptr;
    bool wire = false;
};

template <class T>
struct topic {
    static_assert(std::is_trivially_copyable_v<T>,
                  "bus messages are plain data — they are copied into per-"
                  "subscriber queues under an irq mask");
    static_assert(std::is_default_constructible_v<T>,
                  "ring_buffer slots are default-constructed");

    // One list per message type across the whole program (inline variable:
    // every TU sees the same head).
    inline static node* head = nullptr;

    static void link(node& n) noexcept {
        const arch::irq_state s = arch::irq_save();
        n.next = head;
        head = &n;
        arch::irq_restore(s);
    }

    static void unlink(node& n) noexcept {
        const arch::irq_state s = arch::irq_save();
        node** p = &head;
        while (*p != nullptr && *p != &n) {
            p = &(*p)->next;
        }
        if (*p == &n) {
            *p = n.next;
        }
        arch::irq_restore(s);
        n.next = nullptr;
    }
};

}  // namespace detail

// Publish one message to every live subscriber of T. Safe from thread or ISR
// context (the walk is masked). Returns true when every subscriber accepted
// it; false when at least one dropped (its own missed() counter says which —
// a fire-and-forget call site may ignore the return, the witness remains).
template <class T>
bool publish(const T& msg) noexcept {
    bool ok = true;
    const arch::irq_state s = arch::irq_save();
    for (detail::node* n = detail::topic<T>::head; n != nullptr; n = n->next) {
        ok = n->deliver(n->owner, &msg) && ok;
    }
    arch::irq_restore(s);
    return ok;
}

}  // namespace alloy::lib::bus
