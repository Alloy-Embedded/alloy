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

#include "alloy/net/lwip.hpp"

#include "lwip/tcp.h"

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
        pcb_ = pcb;
        established_ = true;
        arm();
    }

    // Start connecting to a peer. Async: poll the stack until connected().
    [[nodiscard]] bool connect(ipv4 ip, std::uint16_t port) {
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

}  // namespace alloy::net
