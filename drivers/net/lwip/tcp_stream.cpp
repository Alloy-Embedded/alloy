// TcpStream implementation (requires lwIP).
// Only compiled when ALLOY_LWIP_AVAILABLE is defined via the alloy::net_lwip target.

#ifdef ALLOY_LWIP_AVAILABLE

#include "drivers/net/lwip/tcp_stream.hpp"

#include <cstring>

#include "lwip/tcp.h"
#include "lwip/timeouts.h"

namespace alloy::net {

// ============================================================================
// lwIP callbacks
// ============================================================================

// Called by lwIP when data is received on the PCB.
static err_t tcp_stream_recv_cb(void* arg, tcp_pcb* /*pcb*/,
                                 pbuf* p, err_t /*err*/) {
    auto* stream = static_cast<TcpStream*>(arg);
    if (!p) {
        // Connection closed by remote.
        stream->pcb_ = nullptr;
        return ERR_OK;
    }
    for (pbuf* q = p; q; q = q->next) {
        stream->push_rx(static_cast<const std::byte*>(q->payload), q->len);
        tcp_recved(stream->pcb_, q->len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_stream_err_cb(void* arg, err_t /*err*/) {
    auto* stream = static_cast<TcpStream*>(arg);
    stream->pcb_ = nullptr;  // PCB already freed by lwIP on error
}

// ============================================================================
// TcpStream
// ============================================================================

TcpStream::~TcpStream() noexcept {
    if (pcb_) {
        tcp_arg(pcb_, nullptr);
        tcp_close(pcb_);
        pcb_ = nullptr;
    }
}

core::Result<std::size_t, modbus::StreamError>
TcpStream::read(std::span<std::byte> buf, std::uint32_t timeout_us) noexcept {
    if (!pcb_) return core::Err(modbus::StreamError::HardwareError);

    // Register receive callback once.
    tcp_arg(pcb_, this);
    tcp_recv(pcb_, tcp_stream_recv_cb);
    tcp_err(pcb_, tcp_stream_err_cb);

    // Wait for data or timeout.
    std::uint32_t waited = 0u;
    constexpr std::uint32_t kTickUs = 1000u;
    while (rx_head_ == rx_tail_ && waited < timeout_us) {
        sys_check_timeouts();
        waited += kTickUs;
    }

    const std::size_t n = pop_rx(buf);
    return core::Ok(n);
}

core::Result<void, modbus::StreamError>
TcpStream::write(std::span<const std::byte> buf) noexcept {
    if (!pcb_) return core::Err(modbus::StreamError::HardwareError);
    if (buf.empty()) return core::Ok();

    const err_t e = tcp_write(pcb_,
                               buf.data(),
                               static_cast<std::uint16_t>(buf.size()),
                               TCP_WRITE_FLAG_COPY);
    if (e != ERR_OK) return core::Err(modbus::StreamError::HardwareError);
    return core::Ok();
}

core::Result<void, modbus::StreamError>
TcpStream::flush(std::uint32_t /*timeout_us*/) noexcept {
    if (!pcb_) return core::Err(modbus::StreamError::HardwareError);
    tcp_output(pcb_);
    return core::Ok();
}

void TcpStream::push_rx(const std::byte* data, std::size_t len) noexcept {
    for (std::size_t i = 0u; i < len; ++i) {
        const std::size_t next = (rx_tail_ + 1u) % kRxBufCap;
        if (next == rx_head_) break;  // drop on overrun
        rx_buf_[rx_tail_] = data[i];
        rx_tail_ = next;
    }
}

std::size_t TcpStream::pop_rx(std::span<std::byte> out) noexcept {
    std::size_t n = 0u;
    while (n < out.size() && rx_head_ != rx_tail_) {
        out[n++] = rx_buf_[rx_head_];
        rx_head_ = (rx_head_ + 1u) % kRxBufCap;
    }
    return n;
}

// ============================================================================
// TcpListener implementation
// ============================================================================

}  // namespace alloy::net

// Include tcp_listener.cpp here to keep the build simple (one .cpp per target).
#include "drivers/net/lwip/tcp_listener.cpp"

#endif  // ALLOY_LWIP_AVAILABLE
