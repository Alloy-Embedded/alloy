#pragma once

// alloy::net::LwipAdapter — wires a NetworkInterface into lwIP and exposes
// TcpStream / TcpListener for application use.
//
// Template parameter:
//   Interface — any type satisfying alloy::net::NetworkInterface.
//
// No-OS cooperative model:
//   Call poll() from the main loop at least every ~10 ms.
//   poll() feeds received packets to lwIP and calls sys_check_timeouts().
//
// DHCP / static IP:
//   init(use_dhcp=true)  — starts DHCP; wait for link_up() + dhcp_bound().
//   init(ip, mask, gw)   — static IP configuration.
//
// Usage:
//   EthernetInterface eth{gmac, phy};
//   LwipAdapter<EthernetInterface<...>> adapter{eth};
//   adapter.init();
//   while (true) {
//       eth.poll_link();
//       adapter.poll();
//       // ... application work ...
//   }

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/result.hpp"
#include "drivers/net/lwip/tcp_listener.hpp"
#include "drivers/net/lwip/tcp_stream.hpp"
#include "drivers/net/network_interface.hpp"

// lwIP headers (only pulled in when compiling the adapter; the concept
// headers above compile without them).
#ifdef ALLOY_LWIP_AVAILABLE
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#endif

namespace alloy::net {

template <NetworkInterface Interface>
class LwipAdapter {
   public:
    explicit LwipAdapter(Interface& iface) noexcept : iface_{&iface} {}

    LwipAdapter(const LwipAdapter&)            = delete;
    LwipAdapter& operator=(const LwipAdapter&) = delete;

    // Initialise lwIP, register the netif, and optionally start DHCP.
    // static_ip / static_mask / static_gw: 4-byte big-endian IPv4 addresses.
    // When use_dhcp=true the static parameters are ignored.
    void init(
        std::array<std::uint8_t, 4u> static_ip   = {},
        std::array<std::uint8_t, 4u> static_mask = {},
        std::array<std::uint8_t, 4u> static_gw   = {},
        bool use_dhcp = true) noexcept;

    // Feed RX packets to lwIP and tick all timers.
    // Must be called regularly from the main cooperative loop.
    void poll() noexcept;

    // Returns true when the interface has an IP address.
    [[nodiscard]] bool dhcp_bound() const noexcept;

    // Returns the currently assigned IPv4 address (0.0.0.0 if unassigned).
    [[nodiscard]] std::array<std::uint8_t, 4u> ip_address() const noexcept;

    // -----------------------------------------------------------------------
    // Client connection
    // -----------------------------------------------------------------------

    // Connect to a remote host. Blocks (calls poll() internally) until the
    // TCP handshake completes or timeout_us elapses.
    [[nodiscard]] core::Result<TcpStream, NetError>
    connect(std::array<std::uint8_t, 4u> host,
            std::uint16_t port,
            std::uint32_t timeout_us) noexcept;

    // -----------------------------------------------------------------------
    // Server listener
    // -----------------------------------------------------------------------

    // Bind a passive listener to the given TCP port.
    [[nodiscard]] core::Result<TcpListener, NetError>
    listen(std::uint16_t port) noexcept;

   private:
    Interface* iface_;

#ifdef ALLOY_LWIP_AVAILABLE
    netif netif_{};
    bool  dhcp_started_ = false;

    // lwIP netif low-level output: called by lwIP to send a packet.
    static err_t netif_output(netif* nif, pbuf* p);

    // lwIP netif init callback.
    static err_t netif_init_cb(netif* nif);
#endif
};

// ============================================================================
// Implementation
// ============================================================================

#ifdef ALLOY_LWIP_AVAILABLE

template <NetworkInterface Interface>
void LwipAdapter<Interface>::init(
    std::array<std::uint8_t, 4u> static_ip,
    std::array<std::uint8_t, 4u> static_mask,
    std::array<std::uint8_t, 4u> static_gw,
    bool use_dhcp) noexcept {

    lwip_init();

    ip4_addr_t ip{}, mask{}, gw{};
    if (!use_dhcp) {
        IP4_ADDR(&ip,   static_ip[0],   static_ip[1],   static_ip[2],   static_ip[3]);
        IP4_ADDR(&mask, static_mask[0], static_mask[1], static_mask[2], static_mask[3]);
        IP4_ADDR(&gw,   static_gw[0],   static_gw[1],   static_gw[2],   static_gw[3]);
    }

    netif_add(&netif_, &ip, &mask, &gw, this, netif_init_cb, ethernet_input);
    netif_set_default(&netif_);
    netif_set_up(&netif_);

    if (use_dhcp) {
        dhcp_start(&netif_);
        dhcp_started_ = true;
    }
}

template <NetworkInterface Interface>
void LwipAdapter<Interface>::poll() noexcept {
    // Feed received packets into lwIP.
    std::byte rx_buf[1536]{};
    auto r = iface_->recv_packet(std::span{rx_buf});
    if (r.is_ok() && r.unwrap() > 0u) {
        pbuf* p = pbuf_alloc(PBUF_RAW, static_cast<std::uint16_t>(r.unwrap()), PBUF_POOL);
        if (p) {
            pbuf_take(p, rx_buf, static_cast<std::uint16_t>(r.unwrap()));
            if (netif_.input(p, &netif_) != ERR_OK) pbuf_free(p);
        }
    }
    sys_check_timeouts();
}

template <NetworkInterface Interface>
bool LwipAdapter<Interface>::dhcp_bound() const noexcept {
#ifdef LWIP_DHCP
    return dhcp_supplied_address(&netif_) != 0;
#else
    return false;
#endif
}

template <NetworkInterface Interface>
std::array<std::uint8_t, 4u> LwipAdapter<Interface>::ip_address() const noexcept {
    const std::uint32_t addr = netif_.ip_addr.addr;
    return {
        static_cast<std::uint8_t>(addr        & 0xFFu),
        static_cast<std::uint8_t>((addr >>  8) & 0xFFu),
        static_cast<std::uint8_t>((addr >> 16) & 0xFFu),
        static_cast<std::uint8_t>((addr >> 24) & 0xFFu),
    };
}

template <NetworkInterface Interface>
core::Result<TcpStream, NetError>
LwipAdapter<Interface>::connect(std::array<std::uint8_t, 4u> host,
                                 std::uint16_t port,
                                 std::uint32_t timeout_us) noexcept {
    ip4_addr_t remote{};
    IP4_ADDR(&remote, host[0], host[1], host[2], host[3]);

    tcp_pcb* pcb = tcp_new();
    if (!pcb) return core::Err(NetError::HardwareError);

    struct ConnState { bool connected = false; bool failed = false; };
    ConnState state;
    tcp_arg(pcb, &state);
    tcp_connect(pcb, &remote, port,
        [](void* arg, tcp_pcb*, err_t err) -> err_t {
            auto* s = static_cast<ConnState*>(arg);
            if (err == ERR_OK) s->connected = true;
            else               s->failed    = true;
            return ERR_OK;
        });

    // Spin-wait with poll() until connected or timeout.
    std::uint32_t waited = 0u;
    constexpr std::uint32_t kTickUs = 1000u;
    while (!state.connected && !state.failed && waited < timeout_us) {
        poll();
        waited += kTickUs;
    }
    if (!state.connected) {
        tcp_abort(pcb);
        return core::Err(NetError::Busy);
    }
    return core::Ok(TcpStream{pcb});
}

template <NetworkInterface Interface>
core::Result<TcpListener, NetError>
LwipAdapter<Interface>::listen(std::uint16_t port) noexcept {
    tcp_pcb* pcb = tcp_new();
    if (!pcb) return core::Err(NetError::HardwareError);
    if (tcp_bind(pcb, IP4_ADDR_ANY, port) != ERR_OK) {
        tcp_abort(pcb);
        return core::Err(NetError::HardwareError);
    }
    tcp_pcb* lpcb = tcp_listen(pcb);
    if (!lpcb) {
        tcp_abort(pcb);
        return core::Err(NetError::HardwareError);
    }
    return core::Ok(TcpListener{lpcb});
}

template <NetworkInterface Interface>
err_t LwipAdapter<Interface>::netif_output(netif* nif, pbuf* p) {
    auto* self = static_cast<LwipAdapter<Interface>*>(nif->state);
    std::byte tmp[1518]{};
    std::size_t total = 0u;
    for (pbuf* q = p; q; q = q->next) {
        if (total + q->len > sizeof(tmp)) break;
        const auto* src = static_cast<const std::byte*>(q->payload);
        for (std::size_t i = 0u; i < q->len; ++i) tmp[total + i] = src[i];
        total += q->len;
    }
    (void)self->iface_->send_packet(std::span{tmp, total});
    return ERR_OK;
}

template <NetworkInterface Interface>
err_t LwipAdapter<Interface>::netif_init_cb(netif* nif) {
    auto* self = static_cast<LwipAdapter<Interface>*>(nif->state);
    nif->name[0] = 'e'; nif->name[1] = '0';
    nif->output     = etharp_output;
    nif->linkoutput = netif_output;
    nif->mtu        = 1500u;
    nif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    const auto mac = self->iface_->mac_address();
    for (std::size_t i = 0u; i < 6u; ++i) nif->hwaddr[i] = mac[i];
    nif->hwaddr_len = 6u;
    return ERR_OK;
}

#endif  // ALLOY_LWIP_AVAILABLE

}  // namespace alloy::net
