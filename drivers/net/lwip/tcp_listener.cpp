// TcpListener implementation (requires lwIP).
// Included from tcp_stream.cpp — not compiled independently.

#pragma once

#ifdef ALLOY_LWIP_AVAILABLE

#include "drivers/net/lwip/tcp_listener.hpp"

#include "lwip/tcp.h"
#include "lwip/timeouts.h"

namespace alloy::net {

static err_t tcp_listener_accept_cb(void* arg, tcp_pcb* new_pcb, err_t err) {
    auto* listener = static_cast<TcpListener*>(arg);
    if (err == ERR_OK && new_pcb) {
        listener->pending_ = new_pcb;
    }
    return ERR_OK;
}

TcpListener::~TcpListener() noexcept {
    if (pcb_) {
        tcp_arg(pcb_, nullptr);
        tcp_close(pcb_);
        pcb_ = nullptr;
    }
}

core::Result<TcpStream, NetError>
TcpListener::accept(std::uint32_t timeout_us) noexcept {
    tcp_arg(pcb_, this);
    tcp_accept(pcb_, tcp_listener_accept_cb);

    std::uint32_t waited = 0u;
    constexpr std::uint32_t kTickUs = 1000u;
    while (!pending_ && waited < timeout_us) {
        sys_check_timeouts();
        waited += kTickUs;
    }
    if (!pending_) return core::Err(NetError::Busy);

    tcp_pcb* conn = pending_;
    pending_ = nullptr;
    return core::Ok(TcpStream{conn});
}

}  // namespace alloy::net

#endif  // ALLOY_LWIP_AVAILABLE
