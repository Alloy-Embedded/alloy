// Socket facade over lwIP's raw (callback) TCP API — the ONE surface user code
// sees, so lwIP stays swappable (CONNECTIVITY.md). tcp_socket satisfies the
// alloy::Socket concept (send/recv/connected/close) by buffering lwIP's
// callback-delivered pbufs and draining them on recv(); tcp_listener turns
// accepted connections into tcp_sockets. Poll-driven, zero-alloc at this seam
// (buffers ride lwIP's pbuf pool). Drive the stack with stack::poll() in a loop.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/concepts.hpp"
#include "alloy/net/lwip.hpp"

#include "lwip/tcp.h"
#include "lwip/udp.h"

namespace alloy::net {

// A connected TCP endpoint (client side after connect(), or a server side handed
// over by tcp_listener::accept()). Non-movable: lwIP keeps a back-pointer to it.
class tcp_socket {
    struct tcp_pcb* pcb_ = nullptr;
    struct pbuf* rx_ = nullptr;   // received-but-unconsumed data (a pbuf chain)
    std::uint16_t rx_off_ = 0;    // bytes already drained from the chain head
    bool established_ = false;    // client: connect completed
    bool peer_closed_ = false;    // remote sent FIN

    static tcp_socket* self(void* arg) { return static_cast<tcp_socket*>(arg); }

    static err_t cb_recv(void* arg, struct tcp_pcb* /*pcb*/, struct pbuf* p, err_t /*err*/) {
        tcp_socket* s = self(arg);
        if (p == nullptr) {  // FIN
            s->peer_closed_ = true;
            return ERR_OK;
        }
        // Queue it; flow control happens on drain (tcp_recved in recv()).
        if (s->rx_ == nullptr) {
            s->rx_ = p;
        } else {
            pbuf_cat(s->rx_, p);
        }
        return ERR_OK;
    }
    static err_t cb_connected(void* arg, struct tcp_pcb* /*pcb*/, err_t err) {
        self(arg)->established_ = (err == ERR_OK);
        return ERR_OK;
    }
    static void cb_err(void* arg, err_t /*err*/) {
        // lwIP already freed the pcb; just mark us dead.
        tcp_socket* s = self(arg);
        s->pcb_ = nullptr;
        s->peer_closed_ = true;
    }

    void arm() {
        tcp_arg(pcb_, this);
        tcp_recv(pcb_, cb_recv);
        tcp_err(pcb_, cb_err);
    }

    // Drop any leftover receive state so a REUSED socket (adopt/connect after a
    // prior close()) starts the new connection clean — otherwise a stale
    // peer_closed_ makes the next connection look instantly EOF, and a stale rx_
    // chain leaks + mixes another peer's bytes in.
    void reset_rx() {
        if (rx_ != nullptr) {
            pbuf_free(rx_);
            rx_ = nullptr;
        }
        rx_off_ = 0;
        peer_closed_ = false;
    }

public:
    tcp_socket() = default;
    ~tcp_socket() {
        if (rx_ != nullptr) pbuf_free(rx_);
        close();
    }
    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

    // Adopt a pcb handed over by a listener (already established).
    void adopt(struct tcp_pcb* pcb) {
        reset_rx();  // clean slate if this socket served a prior connection
        pcb_ = pcb;
        established_ = true;
        arm();
    }

    // Start connecting to a peer. Async: poll the stack until connected().
    [[nodiscard]] bool connect(ipv4 ip, std::uint16_t port) {
        reset_rx();  // clean slate if this socket was reused
        pcb_ = tcp_new();
        if (pcb_ == nullptr) return false;
        arm();
        ip4_addr_t a;
        IP4_ADDR(&a, ip.o[0], ip.o[1], ip.o[2], ip.o[3]);
        return tcp_connect(pcb_, &a, port, cb_connected) == ERR_OK;
    }

    // Queue up to data.size() bytes; returns how many were accepted (0 if the
    // send window is full — retry after more poll()).
    [[nodiscard]] std::uint32_t send(std::span<const std::uint8_t> data) {
        if (pcb_ == nullptr || data.empty()) return 0;
        const std::uint16_t room = tcp_sndbuf(pcb_);
        std::uint16_t n = data.size() < room ? static_cast<std::uint16_t>(data.size()) : room;
        if (n == 0) return 0;
        if (tcp_write(pcb_, data.data(), n, TCP_WRITE_FLAG_COPY) != ERR_OK) return 0;
        tcp_output(pcb_);
        return n;
    }

    // Copy up to out.size() received bytes; returns how many (0 if nothing buffered).
    [[nodiscard]] std::uint32_t recv(std::span<std::uint8_t> out) {
        if (rx_ == nullptr || out.empty()) return 0;
        const std::uint16_t avail = static_cast<std::uint16_t>(rx_->tot_len - rx_off_);
        std::uint16_t take = out.size() < avail ? static_cast<std::uint16_t>(out.size()) : avail;
        pbuf_copy_partial(rx_, out.data(), take, rx_off_);
        rx_off_ = static_cast<std::uint16_t>(rx_off_ + take);
        if (rx_off_ >= rx_->tot_len) {
            pbuf_free(rx_);
            rx_ = nullptr;
            rx_off_ = 0;
        }
        if (pcb_ != nullptr) tcp_recved(pcb_, take);  // reopen the window as consumed
        return take;
    }

    [[nodiscard]] bool connected() const { return pcb_ != nullptr && established_ && !peer_closed_; }
    // True when the peer closed AND we've drained everything it sent.
    [[nodiscard]] bool eof() const { return peer_closed_ && rx_ == nullptr; }
    // True while the pcb is valid — false only after a peer RST/abort (cb_err)
    // freed it. Unlike connected(), a peer that half-closed (sent FIN) is still
    // alive(): we can finish sending our response before closing.
    [[nodiscard]] bool alive() const { return pcb_ != nullptr; }

    void close() {
        if (pcb_ != nullptr) {
            tcp_arg(pcb_, nullptr);
            tcp_recv(pcb_, nullptr);
            tcp_err(pcb_, nullptr);
            if (tcp_close(pcb_) != ERR_OK) tcp_abort(pcb_);
            pcb_ = nullptr;
        }
        established_ = false;
    }
};

// A TCP listener: binds a port and hands each accepted connection to accept().
template <std::uint32_t Backlog = 4>
class tcp_listener {
    struct tcp_pcb* pcb_ = nullptr;
    struct tcp_pcb* pending_[Backlog]{};
    std::uint32_t n_ = 0;

    static err_t cb_accept(void* arg, struct tcp_pcb* newpcb, err_t /*err*/) {
        auto* l = static_cast<tcp_listener*>(arg);
        if (l->n_ < Backlog) {
            l->pending_[l->n_++] = newpcb;
            return ERR_OK;
        }
        tcp_abort(newpcb);  // backlog full — drop
        return ERR_ABRT;
    }

public:
    tcp_listener() = default;
    ~tcp_listener() {
        if (pcb_ != nullptr) tcp_close(pcb_);
    }
    tcp_listener(const tcp_listener&) = delete;
    tcp_listener& operator=(const tcp_listener&) = delete;

    [[nodiscard]] bool listen(std::uint16_t port) {
        struct tcp_pcb* p = tcp_new();
        if (p == nullptr) return false;
        if (tcp_bind(p, IP_ADDR_ANY, port) != ERR_OK) {
            tcp_close(p);
            return false;
        }
        pcb_ = tcp_listen(p);  // returns a smaller listen pcb (or null)
        if (pcb_ == nullptr) {
            tcp_close(p);
            return false;
        }
        tcp_arg(pcb_, this);
        tcp_accept(pcb_, cb_accept);
        return true;
    }

    // Hand the oldest pending connection to `out`; false if none is waiting.
    [[nodiscard]] bool accept(tcp_socket& out) {
        if (n_ == 0) return false;
        struct tcp_pcb* pcb = pending_[0];
        for (std::uint32_t i = 1; i < n_; ++i) pending_[i - 1] = pending_[i];
        --n_;
        out.adopt(pcb);
        return true;
    }
};

// UDP socket facade over lwIP's raw UDP API — the connectionless, message-framed
// sibling of tcp_socket. bind() claims a local port; send_to() targets any peer
// per datagram; recv_from() yields ONE whole datagram plus its sender. Datagrams
// ride lwIP's pbuf pool (zero-alloc at this seam) and are kept DISCRETE in a small
// fixed ring so message boundaries + per-packet source survive — never pbuf_cat'd
// like the TCP byte stream, which is THE divergence between the two facades.
// Satisfies alloy::DatagramSocket. Non-movable: lwIP keeps a back-pointer
// (udp_recv's arg) to it, so it must live at a stable address (a forever-static on
// firmware). Single-context like tcp_socket: bind/send_to/recv_from/close and the
// callback all run in the thread that drives stack::poll() — never from an ISR.
// Drive with stack::poll() in a loop.
template <std::uint32_t RxDepth = 4>
class udp_socket {
    // One received-but-undrained datagram: its pbuf (chain) + who sent it.
    struct slot {
        struct pbuf* p = nullptr;
        alloy::ip_endpoint from{};
    };

    struct udp_pcb* pcb_ = nullptr;
    slot rx_[RxDepth]{};        // ring of pending datagrams (static, no malloc)
    std::uint32_t head_ = 0;    // oldest undrained slot
    std::uint32_t count_ = 0;   // datagrams waiting (0..RxDepth)

    static udp_socket* self(void* arg) { return static_cast<udp_socket*>(arg); }

    // lwIP source ip_addr_t + host-order port -> our packed host-order endpoint.
    // Octet-wise via ip4_addr1..4 so it is endianness-safe; ip_2_ip4 is a no-op in
    // this v4-only build and keeps it correct if IPv6 is ever enabled.
    static alloy::ip_endpoint endpoint_of(const ip_addr_t* addr, std::uint16_t port) {
        const ip4_addr_t* v4 = ip_2_ip4(addr);
        const std::uint32_t packed =
            (static_cast<std::uint32_t>(ip4_addr1(v4)) << 24) |
            (static_cast<std::uint32_t>(ip4_addr2(v4)) << 16) |
            (static_cast<std::uint32_t>(ip4_addr3(v4)) << 8) |
            static_cast<std::uint32_t>(ip4_addr4(v4));
        return {packed, port};
    }

    // The udp_recv callback: lwIP hands us ONE datagram and we OWN p. Unlike the
    // TCP recv cb this returns void, and p is NEVER a null FIN sentinel. Copy the
    // sender out BEFORE keeping p (addr may alias into p), then transfer ownership
    // of p into the ring (freed later in recv_from/close). Ring full -> tail-drop
    // (UDP is lossy; no backpressure). We still own p on the drop path, so free it.
    static void cb_recv(void* arg, struct udp_pcb* /*pcb*/, struct pbuf* p,
                        const ip_addr_t* addr, u16_t port) {
        udp_socket* s = self(arg);
        if (p == nullptr) return;              // never for UDP; cheap + safe
        if (s->count_ >= RxDepth) {            // ring full: drop; we still own p
            pbuf_free(p);
            return;
        }
        const std::uint32_t i = (s->head_ + s->count_) % RxDepth;
        s->rx_[i].from = endpoint_of(addr, port);  // COPY sender before we keep p
        s->rx_[i].p = p;                            // ownership transfers into ring
        ++s->count_;
    }

    // Free every buffered datagram (close/dtor). Leaves the socket rebindable.
    void drain_rx() {
        while (count_ != 0) {
            pbuf_free(rx_[head_].p);
            rx_[head_].p = nullptr;
            head_ = (head_ + 1) % RxDepth;
            --count_;
        }
        head_ = 0;
    }

    // The pbuf-alloc idiom that replaces tcp_write(COPY). The caller keeps the
    // pbuf, so we ALWAYS free it (udp_sendto never consumes p). All-or-nothing:
    // full size on ERR_OK, else 0.
    std::uint32_t emit(std::span<const std::uint8_t> data,
                       const ip4_addr_t& dst, std::uint16_t port) {
        if (pcb_ == nullptr || data.empty() || data.size() > 0xFFFF) return 0;
        const std::uint16_t len = static_cast<std::uint16_t>(data.size());
        struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
        if (p == nullptr) return 0;            // out of PBUF_RAM
        if (pbuf_take(p, data.data(), len) != ERR_OK) {
            pbuf_free(p);
            return 0;
        }
        const err_t e = udp_sendto(pcb_, p, &dst, port);
        pbuf_free(p);                          // always — success OR failure
        return e == ERR_OK ? len : 0;
    }

public:
    udp_socket() = default;
    ~udp_socket() { close(); }                 // close() drains ring + frees pcb
    udp_socket(const udp_socket&) = delete;
    udp_socket& operator=(const udp_socket&) = delete;

    // Claim a local port (0 = ephemeral) and start receiving. One call arms both
    // the callback and the back-pointer (udp_recv sets both; no separate arg call).
    // false on pcb-alloc failure (MEMP_NUM_UDP_PCB exhausted) or bind failure
    // (ERR_USE: port already in use).
    [[nodiscard]] bool bind(std::uint16_t port) {
        if (pcb_ != nullptr) return false;     // already bound
        pcb_ = udp_new();                      // IPv4 pcb; NULL on OOM
        if (pcb_ == nullptr) return false;
        if (udp_bind(pcb_, IP_ADDR_ANY, port) != ERR_OK) {
            udp_remove(pcb_);                  // undo the half-built pcb
            pcb_ = nullptr;
            return false;
        }
        udp_recv(pcb_, cb_recv, this);         // arg = this (the back-pointer)
        return true;
    }

    // Send one datagram to an explicit peer (connectionless; dest is per-call).
    // Concept form (ip_endpoint) + an ergonomic ipv4/port overload mirroring the
    // octet style of stack::up()/tcp connect({...}, port). Returns bytes sent
    // (== data.size()) on success, 0 otherwise.
    [[nodiscard]] std::uint32_t send_to(std::span<const std::uint8_t> data,
                                        const alloy::ip_endpoint& to) {
        ip4_addr_t dst;
        IP4_ADDR(&dst, (to.addr >> 24) & 0xFF, (to.addr >> 16) & 0xFF,
                 (to.addr >> 8) & 0xFF, to.addr & 0xFF);
        return emit(data, dst, to.port);
    }
    [[nodiscard]] std::uint32_t send_to(std::span<const std::uint8_t> data,
                                        ipv4 ip, std::uint16_t port) {
        ip4_addr_t dst;
        IP4_ADDR(&dst, ip.o[0], ip.o[1], ip.o[2], ip.o[3]);
        return emit(data, dst, port);
    }

    // Copy ONE whole datagram (if any) into out; report its sender. Returns bytes
    // copied (0 if none queued). The datagram is fully consumed even when out is
    // too small — the excess is discarded (BSD datagram-truncation semantics), so
    // size out to the max expected payload (e.g. 1472 over a 1500 MTU).
    [[nodiscard]] std::uint32_t recv_from(std::span<std::uint8_t> out,
                                          alloy::ip_endpoint& from) {
        if (count_ == 0 || out.empty()) return 0;
        slot& sl = rx_[head_];
        const std::uint16_t take = out.size() < sl.p->tot_len
            ? static_cast<std::uint16_t>(out.size())
            : sl.p->tot_len;
        pbuf_copy_partial(sl.p, out.data(), take, 0);  // walks the chain, offset 0
        from = sl.from;                                // sender captured in cb_recv
        pbuf_free(sl.p);                               // whole datagram consumed
        sl.p = nullptr;
        head_ = (head_ + 1) % RxDepth;
        --count_;
        return take;
    }
    [[nodiscard]] std::uint32_t recv_from(std::span<std::uint8_t> out,
                                          ipv4& fip, std::uint16_t& fport) {
        alloy::ip_endpoint from{};
        const std::uint32_t n = recv_from(out, from);
        if (n != 0) {
            fip = ipv4{static_cast<std::uint8_t>((from.addr >> 24) & 0xFF),
                       static_cast<std::uint8_t>((from.addr >> 16) & 0xFF),
                       static_cast<std::uint8_t>((from.addr >> 8) & 0xFF),
                       static_cast<std::uint8_t>(from.addr & 0xFF)};
            fport = from.port;
        }
        return n;
    }

    [[nodiscard]] bool bound() const { return pcb_ != nullptr; }
    // Distinguishes a queued zero-length datagram from "nothing waiting" (recv_from
    // returns 0 for both).
    [[nodiscard]] bool pending() const { return count_ != 0; }

    // Idempotent. Unhook the callback FIRST so no late dispatch touches a freed
    // object; udp_remove unbinds AND frees the pcb (no udp_close/udp_abort exist).
    // THEN drain buffered pbufs (independent pool allocations, not owned by the
    // pcb) — this ordering matters; do not reorder into a use-after-free.
    void close() {
        if (pcb_ != nullptr) {
            udp_recv(pcb_, nullptr, nullptr);
            udp_remove(pcb_);
            pcb_ = nullptr;
        }
        drain_rx();
    }
};

static_assert(alloy::DatagramSocket<udp_socket<>>);

}  // namespace alloy::net
