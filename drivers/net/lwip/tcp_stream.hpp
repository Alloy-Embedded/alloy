#pragma once

// alloy::net::TcpStream — lwIP TCP connection wrapped as a Modbus ByteStream.
//
// TcpStream is produced by LwipAdapter::connect() (client) or
// TcpListener::accept() (server). It satisfies alloy::modbus::ByteStream so
// it can be passed directly to Slave{tcp_stream, id, registry} or Master{}.
//
// Important: TcpStream is NOT safe to use from multiple threads or ISRs.
// All interaction must happen from the cooperative main loop that also calls
// LwipAdapter::poll().
//
// The underlying lwIP PCB is owned by LwipAdapter; TcpStream holds a
// non-owning reference. After the connection closes (either side), further
// read/write calls return StreamError::HardwareError.
//
// Availability: this header compiles on host (no lwIP needed) but the
// implementation (.cpp) requires lwIP. Link `alloy::net_lwip` target.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/protocol/modbus/include/alloy/modbus/byte_stream.hpp"

// Forward-declare lwIP PCB so this header does not pull in all of lwIP.
struct tcp_pcb;

namespace alloy::net {

class TcpStream {
   public:
    // Constructs from an already-connected lwIP PCB.
    // The PCB must remain valid until the stream is destroyed.
    explicit TcpStream(tcp_pcb* pcb) noexcept : pcb_{pcb} {}

    TcpStream(const TcpStream&)            = delete;
    TcpStream& operator=(const TcpStream&) = delete;
    TcpStream(TcpStream&& o) noexcept : pcb_{o.pcb_} { o.pcb_ = nullptr; }
    TcpStream& operator=(TcpStream&& o) noexcept {
        pcb_ = o.pcb_; o.pcb_ = nullptr; return *this;
    }

    ~TcpStream() noexcept;

    // -----------------------------------------------------------------------
    // modbus::ByteStream implementation
    // -----------------------------------------------------------------------

    // Reads up to buf.size() bytes; blocks until timeout_us elapses or data
    // arrives. Returns the number of bytes actually read (may be < buf.size()).
    [[nodiscard]] core::Result<std::size_t, modbus::StreamError>
    read(std::span<std::byte> buf, std::uint32_t timeout_us) noexcept;

    // Writes all bytes in buf. Blocks until the TCP send buffer accepts them
    // or a write error occurs.
    [[nodiscard]] core::Result<void, modbus::StreamError>
    write(std::span<const std::byte> buf) noexcept;

    // Flushes the TCP send buffer (calls tcp_output on the PCB).
    [[nodiscard]] core::Result<void, modbus::StreamError>
    flush(std::uint32_t timeout_us) noexcept;

    // For TCP, wait_idle is a no-op (there is no inter-frame silence concept).
    // Returns Ok immediately.
    [[nodiscard]] core::Result<void, modbus::StreamError>
    wait_idle(std::uint32_t /*silence_us*/) noexcept {
        return core::Ok();
    }

    // Returns true if the underlying PCB is valid and connected.
    [[nodiscard]] bool connected() const noexcept { return pcb_ != nullptr; }

   private:
    tcp_pcb* pcb_ = nullptr;

    // Receive buffer — data pushed by lwIP's tcp_recv callback is queued here.
    static constexpr std::size_t kRxBufCap = 2048u;
    std::byte  rx_buf_[kRxBufCap]{};
    std::size_t rx_head_ = 0u;
    std::size_t rx_tail_ = 0u;

    friend void tcp_stream_recv_callback(void* arg, tcp_pcb* pcb,
                                         void* pbuf, std::int8_t err);
    void push_rx(const std::byte* data, std::size_t len) noexcept;
    std::size_t pop_rx(std::span<std::byte> out) noexcept;
};

static_assert(modbus::ByteStream<TcpStream>);

}  // namespace alloy::net
