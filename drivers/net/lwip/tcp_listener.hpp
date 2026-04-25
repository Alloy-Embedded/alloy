#pragma once

// alloy::net::TcpListener — passive TCP listener that produces TcpStreams.
//
// Created by LwipAdapter::listen(port). Call accept(timeout_us) from the
// main loop to receive incoming connections.
//
// Usage (Modbus TCP slave):
//   auto listener = adapter.listen(502u);
//   while (true) {
//       auto conn = listener.unwrap().accept(5'000'000u);
//       if (conn.is_ok()) {
//           Slave slave{conn.unwrap(), kSlaveId, registry};
//           while (slave.poll(10'000u).is_ok()) { adapter.poll(); }
//       }
//       adapter.poll();
//   }

#include <cstdint>

#include "core/result.hpp"
#include "drivers/net/lwip/tcp_stream.hpp"
#include "drivers/net/network_interface.hpp"

struct tcp_pcb;

namespace alloy::net {

class TcpListener {
   public:
    explicit TcpListener(tcp_pcb* pcb) noexcept : pcb_{pcb} {}

    TcpListener(const TcpListener&)            = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    TcpListener(TcpListener&& o) noexcept : pcb_{o.pcb_} { o.pcb_ = nullptr; }
    TcpListener& operator=(TcpListener&& o) noexcept {
        pcb_ = o.pcb_; o.pcb_ = nullptr; return *this;
    }

    ~TcpListener() noexcept;

    // Block until a client connects or timeout_us elapses.
    // The caller must continue calling LwipAdapter::poll() for timers to tick.
    // Returns a connected TcpStream or NetError::Busy on timeout.
    [[nodiscard]] core::Result<TcpStream, NetError>
    accept(std::uint32_t timeout_us) noexcept;

   private:
    tcp_pcb*  pcb_     = nullptr;
    tcp_pcb*  pending_ = nullptr;  // set by lwIP accept callback

    friend std::int8_t tcp_listener_accept_callback(void* arg, tcp_pcb* new_pcb,
                                                     std::int8_t err);
};

}  // namespace alloy::net
