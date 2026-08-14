// alloy::lib::bus — the bridge: local topics extended over a byte link.
// One declared route per forwarded message, one owning TX task, one owning
// RX feeder — and the same publish() call site whether the consumer sits in
// this image or on the other end of a uart.
//
// The design decision that carries this file is ENCODE-AT-PUBLISH: a
// bridge_route<B> hangs a wire-node on topic<T>, and delivery ENCODES the
// message straight into the bridge's byte ring, inside the publish walk.
// That dissolves the wait-any problem (the framework has no when_any, and
// N forwarded topics would otherwise need N parked subscribers): the TX
// task awaits ONE tx_pending() regardless of how many routes feed it, and
// ordering across topics is publication order, which N separate queues
// could not promise.
//
// The costs are bounded by doctrine, not hope: a body is capped at
// wire_max_body (validated at compile time in the binding), so the masked
// window per publish is one encode plus one ≤wire_max_frame copy into the
// ring. The ring accepts a frame WHOLE OR NOT AT ALL — a partial frame in
// the ring would corrupt the link; a dropped one is tx_missed() evidence.
//
// Republish from the wire skips wire nodes (detail::publish_filtered):
// a message cannot echo back out its own link, and cannot hop onward
// through a second bridge — single-hop is structure, not configuration.
// At-most-once end to end; every failure mode has a counter, not a retry.
//
// Sans-IO like everything at this boundary: the bridge owns no uart and no
// clock. Bytes leave via tx_take() into the caller's staging buffer (the
// caller knows which RAM its DMA can see); bytes arrive via on_bytes()
// with the caller's now_us. bridge_core is the non-templated seam so a
// route links against ANY ring size — the executor_core/executor split.

#pragma once

#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/arch/irq.hpp"
#include "alloy/async/waiter.hpp"
#include "bus/topic.hpp"
#include "bus/wire.hpp"

namespace alloy::lib::bus {

static_assert(wire_max_frame <= 0xFF, "tx ring length prefix is one byte");

namespace detail {

// RX dispatch entry — one per bridge_route, resident in the route object.
struct rx_route {
    std::uint16_t id = 0;
    bool (*dispatch)(void* owner, const msg_view& v) = nullptr;
    void* owner = nullptr;
    rx_route* next = nullptr;
};

}  // namespace detail

class bridge_core {
public:
    bridge_core(const bridge_core&) = delete;
    bridge_core& operator=(const bridge_core&) = delete;

    // ---- TX: publish contexts in, one owning task out ----------------

    // Encode m as one frame and enqueue it whole. Safe from thread or ISR
    // context (self-masked, so it nests inside publish's mask and stands
    // alone for a message that has no local topic). False = ring full,
    // frame dropped whole, tx_missed() incremented.
    template <WireBinding B>
    bool send(const typename B::message& m) noexcept {
        std::uint8_t frame[wire_max_frame];
        const arch::irq_state s = arch::irq_save();
        const std::size_t n = encode_datagram<B>(m, tx_seq_++, frame);
        const bool ok = push_frame(frame, n);
        arch::irq_restore(s);
        if (ok) {
            tx_slot_.wake();
        }
        return ok;
    }

    // Pop ONE whole frame into `staging`, returning the frame view (empty =
    // nothing queued). staging must hold wire_max_frame bytes — it is the
    // caller's, because the caller knows which RAM its DMA can read.
    [[nodiscard]] std::span<const std::uint8_t> tx_take(std::span<std::uint8_t> staging) noexcept {
        if (staging.size() < wire_max_frame) {
            __builtin_trap();  // contract: a staging buffer that fits any frame
        }
        const arch::irq_state s = arch::irq_save();
        if (count_ == 0) {
            arch::irq_restore(s);
            return {};
        }
        const std::size_t n = ring_pop_byte();
        for (std::size_t i = 0; i < n; ++i) {
            staging[i] = ring_pop_byte();
        }
        arch::irq_restore(s);
        return staging.first(n);
    }

    [[nodiscard]] bool tx_empty() const noexcept { return count_ == 0; }

    // co_await link.tx_pending(): parks the ONE owning TX task until a frame
    // is queued. Same single-waiter contract as every slot in the house.
    [[nodiscard]] auto tx_pending() noexcept {
        struct awaiter {
            bridge_core& c;
            [[nodiscard]] bool await_ready() const noexcept { return !c.tx_empty(); }
            bool await_suspend(std::coroutine_handle<> h) noexcept {
                return c.tx_slot_.park(h, [this] { return !c.tx_empty(); });
            }
            void await_resume() noexcept {}
        };
        return awaiter{*this};
    }

    // ---- RX: the link's byte feeder in, local republish out ----------

    // Feed received bytes (thread context — the rx task that owns the uart).
    // Complete datagrams are decoded through the registered routes and
    // republished locally, SKIPPING wire nodes. Returns messages delivered.
    std::size_t on_bytes(std::span<const std::uint8_t> bytes, std::uint32_t now_us) noexcept {
        std::size_t delivered = 0;
        for (const std::uint8_t b : bytes) {
            if (!rx_.feed(b, now_us)) {
                continue;
            }
            if (rx_.type() != wire_type_datagram) {
                ++rx_unknown_;  // a future frame type: not ours, not an error storm
                continue;
            }
            msg_view v{};
            if (!parse_datagram(rx_.payload(), v)) {
                ++rx_dropped_;  // shorter than its own header
                continue;
            }
            detail::rx_route* r = routes_;
            while (r != nullptr && r->id != v.id) {
                r = r->next;
            }
            if (r == nullptr) {
                ++rx_unknown_;  // the peer's bus.toml knows ids ours does not
                continue;
            }
            if (r->dispatch(r->owner, v)) {
                ++delivered;
            } else {
                ++rx_dropped_;  // id matched, layout/ver did not — stale peer
            }
        }
        return delivered;
    }

    // Housekeeping passthrough: abandon a stalled half frame (see wire.hpp).
    void tick(std::uint32_t now_us) noexcept { rx_.tick(now_us); }

    // ---- witnesses ---------------------------------------------------
    [[nodiscard]] std::uint32_t tx_missed() const noexcept { return tx_missed_; }
    [[nodiscard]] std::uint32_t rx_frames() const noexcept { return rx_.frames(); }
    [[nodiscard]] std::uint32_t rx_bad_frames() const noexcept { return rx_.bad_frames(); }
    [[nodiscard]] std::uint32_t rx_lost() const noexcept { return rx_.lost(); }
    [[nodiscard]] std::uint32_t rx_unknown() const noexcept { return rx_unknown_; }
    [[nodiscard]] std::uint32_t rx_dropped() const noexcept { return rx_dropped_; }

protected:
    bridge_core(std::uint8_t* buf, std::size_t cap) noexcept : buf_(buf), cap_(cap) {}
    ~bridge_core() = default;

private:
    template <WireBinding B>
    friend class bridge_route;

    void route_link(detail::rx_route& r) noexcept {
        const arch::irq_state s = arch::irq_save();
        r.next = routes_;
        routes_ = &r;
        arch::irq_restore(s);
    }
    void route_unlink(detail::rx_route& r) noexcept {
        const arch::irq_state s = arch::irq_save();
        detail::rx_route** p = &routes_;
        while (*p != nullptr && *p != &r) {
            p = &(*p)->next;
        }
        if (*p == &r) {
            *p = r.next;
        }
        arch::irq_restore(s);
        r.next = nullptr;
    }

    // Byte ring, length-prefixed frames. ALL access is masked (the producer
    // is the publish walk, the consumer is tx_take) — plain indices, whole-
    // frame granularity. Simplicity over lock-freedom here is deliberate:
    // the masked window is the same order as the publish copy itself.
    bool push_frame(const std::uint8_t* frame, std::size_t n) noexcept {
        if (n == 0 || cap_ - count_ < n + 1) {
            ++tx_missed_;
            return false;  // whole or not at all
        }
        ring_push_byte(static_cast<std::uint8_t>(n));
        for (std::size_t i = 0; i < n; ++i) {
            ring_push_byte(frame[i]);
        }
        return true;
    }
    void ring_push_byte(std::uint8_t b) noexcept {
        buf_[head_] = b;
        head_ = (head_ + 1 == cap_) ? 0 : head_ + 1;
        ++count_;
    }
    [[nodiscard]] std::uint8_t ring_pop_byte() noexcept {
        const std::uint8_t b = buf_[tail_];
        tail_ = (tail_ + 1 == cap_) ? 0 : tail_ + 1;
        --count_;
        return b;
    }

    std::uint8_t* buf_;
    std::size_t cap_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t count_ = 0;
    async::waiter_slot tx_slot_;
    wire_receiver<> rx_;
    detail::rx_route* routes_ = nullptr;
    std::uint32_t tx_missed_ = 0;
    std::uint32_t rx_unknown_ = 0;
    std::uint32_t rx_dropped_ = 0;
    std::uint8_t tx_seq_ = 0;
};

// The storage half: RingBytes of length-prefixed encoded frames. Size it
// for the burst the link must absorb while the TX task drains — a frame
// costs its length + 1.
template <std::size_t RingBytes = 512>
class bridge : public bridge_core {
    static_assert(RingBytes > wire_max_frame + 1,
                  "a ring that cannot hold one full frame forwards nothing");

public:
    bridge() noexcept : bridge_core(storage_, RingBytes) {}

private:
    std::uint8_t storage_[RingBytes]{};
};

// One forwarded message: hangs a wire-node on topic<B::message> (outbound,
// encode-at-publish) and an RX dispatch entry on the bridge (inbound,
// decode-and-republish). Declared statically, one per message per link —
// the task_storage spirit: whoever wants the resource declares the storage.
template <WireBinding B>
class bridge_route {
    using T = typename B::message;

public:
    explicit bridge_route(bridge_core& link) noexcept : link_(link) {
        topic_node_.deliver = &deliver_thunk;
        topic_node_.owner = this;
        topic_node_.wire = true;
        detail::topic<T>::link(topic_node_);
        rx_node_.id = B::id;
        rx_node_.dispatch = &dispatch_thunk;
        rx_node_.owner = this;
        link_.route_link(rx_node_);
    }
    ~bridge_route() {
        detail::topic<T>::unlink(topic_node_);
        link_.route_unlink(rx_node_);
    }
    bridge_route(const bridge_route&) = delete;
    bridge_route& operator=(const bridge_route&) = delete;
    bridge_route(bridge_core&&) = delete;  // a temporary link would dangle

private:
    // Outbound: runs inside the publish walk. send() self-masks (nested).
    static bool deliver_thunk(void* owner, const void* msg) noexcept {
        auto& self = *static_cast<bridge_route*>(owner);
        return self.link_.template send<B>(*static_cast<const T*>(msg));
    }
    // Inbound: runs in the rx feeder's thread context, from on_bytes().
    static bool dispatch_thunk(void* owner, const msg_view& v) noexcept {
        (void)owner;
        T m{};
        if (!decode_as<B>(v, m)) {
            return false;
        }
        (void)detail::publish_filtered(m, /*include_wire=*/false);
        return true;
    }

    detail::node topic_node_{};
    detail::rx_route rx_node_{};
    bridge_core& link_;
};

}  // namespace alloy::lib::bus
