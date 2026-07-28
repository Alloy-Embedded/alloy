// lwIP NO_SYS stack bound to an alloy NetDevice (the GMAC). Owns the single
// netif and pumps frames both ways: RX from the NetDevice into lwIP, and lwIP's
// TX back out the NetDevice. Static IP for v1 (DHCP is a follow-up). The vendored
// lwIP (fs/vendor... net/vendor/lwip) is the TCP/IP behavior; this header is the
// per-device seam. User code binds sockets on top via the raw lwIP API (a
// Socket facade lands next); zero chip addresses here.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"

namespace alloy::net {

// A dotted-quad IPv4 address as four octets (host order in source).
struct ipv4 {
    std::uint8_t o[4];
};

namespace detail {
// lwIP's core is global state: init it exactly once even if several stacks
// (e.g. two netifs cabled together in a host test) come up.
inline bool& lwip_inited() {
    static bool v = false;
    return v;
}
}  // namespace detail

// Initialize lwIP's core exactly once (idempotent). stack::up() calls this; a
// test using lwIP directly (e.g. the loopback socket test) can call it too.
inline void ensure_init() {
    if (!detail::lwip_inited()) {
        lwip_init();
        detail::lwip_inited() = true;
    }
}

template <alloy::NetDevice Dev>
class stack {
    Dev& dev_;
    struct netif netif_ {};
    std::uint8_t mac_[6]{};
    bool up_ = false;
    // One TX staging buffer: lwIP pbuf chains are gathered here before the
    // single-shot NetDevice.transmit(). Sized for a full frame.
    alignas(4) std::uint8_t txbuf_[1536]{};

    static stack* self(struct netif* n) { return static_cast<stack*>(n->state); }

    // lwIP -> wire: flatten the pbuf chain and hand it to the NetDevice.
    static err_t link_output(struct netif* n, struct pbuf* p) {
        stack* s = self(n);
        const std::uint16_t len = pbuf_copy_partial(p, s->txbuf_, sizeof(s->txbuf_), 0);
        return s->dev_.transmit({s->txbuf_, len}) ? ERR_OK : ERR_IF;
    }

    // Called by netif_add: wire the interface up (ethernet, ARP, our outputs).
    static err_t init_cb(struct netif* n) {
        stack* s = self(n);
        n->name[0] = 'e';
        n->name[1] = 'n';
        n->hwaddr_len = 6;
        for (int i = 0; i < 6; ++i) n->hwaddr[i] = s->mac_[i];
        n->mtu = static_cast<std::uint16_t>(s->dev_.mtu());
        n->output = etharp_output;         // IP tx -> ARP resolve -> linkoutput
        n->linkoutput = link_output;       // raw frame tx
        n->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
        return ERR_OK;
    }

public:
    explicit stack(Dev& dev) : dev_(dev) {}

    // Remove our netif from lwIP's global list when destroyed — matters for host
    // tests that spin stacks up and down; on firmware the stack is a forever
    // static and this never runs.
    ~stack() {
        if (up_) netif_remove(&netif_);
    }

    stack(const stack&) = delete;
    stack& operator=(const stack&) = delete;

    // Bring the stack + interface up with a static address. Call once after the
    // link is up (board::eth_phy negotiated) and lwIP has a valid time source.
    void up(ipv4 addr, ipv4 mask, ipv4 gw, const std::uint8_t mac[6]) {
        for (int i = 0; i < 6; ++i) mac_[i] = mac[i];
        ensure_init();
        ip4_addr_t a, m, g;
        IP4_ADDR(&a, addr.o[0], addr.o[1], addr.o[2], addr.o[3]);
        IP4_ADDR(&m, mask.o[0], mask.o[1], mask.o[2], mask.o[3]);
        IP4_ADDR(&g, gw.o[0], gw.o[1], gw.o[2], gw.o[3]);
        netif_add(&netif_, &a, &m, &g, this, init_cb, ethernet_input);
        netif_set_default(&netif_);
        netif_set_up(&netif_);
        netif_set_link_up(&netif_);
        up_ = true;
    }

    // Drive the stack: feed one received frame (if any) and run timeouts.
    // Call in a tight loop.
    void poll() {
        alignas(4) static std::uint8_t rxbuf[1536];
        const std::uint32_t n = dev_.receive({rxbuf, sizeof(rxbuf)});
        if (n != 0) {
            struct pbuf* p = pbuf_alloc(PBUF_RAW, static_cast<std::uint16_t>(n), PBUF_POOL);
            if (p != nullptr) {
                pbuf_take(p, rxbuf, static_cast<std::uint16_t>(n));
                if (netif_.input(p, &netif_) != ERR_OK) {
                    pbuf_free(p);
                }
            }
        }
        sys_check_timeouts();
    }

    [[nodiscard]] struct netif* netif_handle() { return &netif_; }
};

}  // namespace alloy::net
