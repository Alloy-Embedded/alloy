// Host tests for the M2 net stack: vendored lwIP + the netif<->NetDevice glue +
// the Socket facade, exercised WITHOUT hardware. Two angles: (1) a fake
// NetDevice injects a raw ARP frame and captures the reply, proving the
// receive->ethernet_input->transmit glue; (2) a real TCP echo over lwIP's
// loopback (127.0.0.1), driven through tcp_listener/tcp_socket, proving the
// facade + TCP end to end. Together they validate the stack logic the silicon
// test can't reach cheaply.
//
// This TU also supplies lwIP's NO_SYS port hooks for the host (the firmware
// port.cpp needs the systick uptime, absent here): a manually-advanced clock so
// TCP timeouts are deterministic, and a weak RNG for ISNs.
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/net/lwip.hpp"
#include "alloy/net/socket.hpp"
#include "alloy_test.hpp"

extern "C" {
static std::uint32_t g_now_ms = 0;
std::uint32_t sys_now(void) { return g_now_ms; }
std::uint32_t alloy_lwip_rand(void) {
    static std::uint32_t s = 22695477u;
    s = s * 1103515245u + 12345u + g_now_ms;
    return s;
}
}

namespace {
// A NetDevice that drops TX and never receives — enough to bring a netif up.
struct null_dev {
    std::uint32_t receive(std::span<std::uint8_t>) { return 0; }
    bool transmit(std::span<const std::uint8_t>) { return true; }
    [[nodiscard]] bool link_up() const { return true; }
    [[nodiscard]] std::uint32_t mtu() const { return 1500; }
};
static_assert(alloy::NetDevice<null_dev>);

// A NetDevice that lets a test inject one RX frame and capture the last TX frame
// — enough to prove the stack's receive->ethernet_input->reply->transmit path.
struct capture_dev {
    std::uint8_t rx[1536]{};
    std::uint32_t rx_len = 0;
    std::uint8_t tx[1536]{};
    std::uint32_t tx_len = 0;

    void inject(const std::uint8_t* f, std::uint32_t n) {
        std::memcpy(rx, f, n);
        rx_len = n;
    }
    std::uint32_t receive(std::span<std::uint8_t> out) {
        if (rx_len == 0) return 0;
        const std::uint32_t n = rx_len < out.size() ? rx_len : static_cast<std::uint32_t>(out.size());
        std::memcpy(out.data(), rx, n);
        rx_len = 0;
        return n;
    }
    bool transmit(std::span<const std::uint8_t> f) {
        tx_len = f.size() < sizeof(tx) ? static_cast<std::uint32_t>(f.size()) : sizeof(tx);
        std::memcpy(tx, f.data(), tx_len);
        return true;
    }
    [[nodiscard]] bool link_up() const { return true; }
    [[nodiscard]] std::uint32_t mtu() const { return 1500; }
};
static_assert(alloy::NetDevice<capture_dev>);
}  // namespace

ALLOY_TEST(net_answers_arp) {
    capture_dev dev;
    alloy::net::stack<capture_dev> s{dev};
    const std::uint8_t mac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x01};
    s.up({10, 0, 0, 1}, {255, 255, 255, 0}, {10, 0, 0, 1}, mac);

    // ARP "who has 10.0.0.1?" from 10.0.0.99 (02:00:00:00:00:99), broadcast.
    std::uint8_t req[42] = {};
    for (int i = 0; i < 6; ++i) req[i] = 0xFF;             // dst broadcast
    req[6] = 0x02; req[11] = 0x99;                          // src mac 02:..:99
    req[12] = 0x08; req[13] = 0x06;                         // ethertype ARP
    req[15] = 1; req[16] = 0x08; req[18] = 6; req[19] = 4;  // htype/ptype/hlen/plen
    req[21] = 1;                                            // op request
    req[22] = 0x02; req[27] = 0x99;                         // sender mac
    req[28] = 10; req[31] = 99;                             // sender ip 10.0.0.99
    req[38] = 10; req[41] = 1;                              // target ip 10.0.0.1 (us)
    dev.inject(req, 42);

    s.poll();  // receive -> ethernet_input -> ARP -> reply out transmit()

    ALLOY_CHECK(dev.tx_len >= 42);
    ALLOY_CHECK(dev.tx[12] == 0x08 && dev.tx[13] == 0x06);  // ARP
    ALLOY_CHECK_EQ(dev.tx[21], 2);                          // op = reply
    ALLOY_CHECK(std::memcmp(&dev.tx[22], mac, 6) == 0);     // sender mac = us
    ALLOY_CHECK(dev.tx[28] == 10 && dev.tx[31] == 1);       // sender ip = 10.0.0.1
}

ALLOY_TEST(net_stack_comes_up) {
    null_dev dev;
    alloy::net::stack<null_dev> s{dev};
    const std::uint8_t mac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x05};
    s.up({10, 0, 0, 5}, {255, 255, 255, 0}, {10, 0, 0, 1}, mac);
    ALLOY_CHECK(netif_is_up(s.netif_handle()));
    ALLOY_CHECK(netif_is_link_up(s.netif_handle()));
}

// Drive lwIP one step: run timers, process the loopback queue, advance the clock.
static void pump() {
    sys_check_timeouts();
    netif_poll_all();
    g_now_ms += 5;
}

// The real thing: a TCP echo over lwIP's loopback (127.0.0.1), driven entirely
// through the alloy::net Socket facade — connect, accept, send, echo, recv — with
// no hardware. Exercises tcp_listener + tcp_socket + lwIP's TCP end to end.
ALLOY_TEST(net_socket_tcp_echo_loopback) {
    alloy::net::ensure_init();  // lwip_init + the loopif (127.0.0.1)

    alloy::net::tcp_listener<> srv;
    ALLOY_CHECK(srv.listen(7));

    alloy::net::tcp_socket client;
    ALLOY_CHECK(client.connect({127, 0, 0, 1}, 7));

    alloy::net::tcp_socket conn;  // server-side accepted connection
    bool accepted = false;
    for (int i = 0; i < 3000 && !(client.connected() && accepted); ++i) {
        pump();
        if (!accepted) accepted = srv.accept(conn);
    }
    ALLOY_CHECK(accepted);
    ALLOY_CHECK(client.connected());

    const char msg[] = "hello alloy lwip";
    const std::uint32_t mlen = sizeof(msg) - 1;
    ALLOY_CHECK_EQ(client.send({reinterpret_cast<const std::uint8_t*>(msg), mlen}), mlen);

    std::uint8_t got[64] = {};
    std::uint32_t got_n = 0;
    for (int i = 0; i < 3000 && got_n < mlen; ++i) {
        pump();
        std::uint8_t buf[64];
        const std::uint32_t r = conn.recv(buf);  // server drains...
        if (r != 0) (void)conn.send({buf, r});   // ...and echoes back
        got_n += client.recv({got + got_n, sizeof(got) - got_n});
    }
    ALLOY_CHECK_EQ(got_n, mlen);
    ALLOY_CHECK(std::memcmp(got, msg, mlen) == 0);
}

// The UDP facade satisfies the datagram contract (compile-time, like NetDevice).
static_assert(alloy::DatagramSocket<alloy::net::udp_socket<>>);

// UDP echo over lwIP's loopback (127.0.0.1), driven through udp_socket — bind,
// send_to, recv_from (with the sender), echo back. No stack<Dev>/netif needed:
// ensure_init() brings up the loopif and pump()'s netif_poll_all() drains it.
// Exercises BOTH the ipv4 send overload and the ip_endpoint recv form.
ALLOY_TEST(net_socket_udp_echo_loopback) {
    alloy::net::ensure_init();

    alloy::net::udp_socket<> srv;
    ALLOY_CHECK(srv.bind(7));
    alloy::net::udp_socket<> client;
    ALLOY_CHECK(client.bind(0));  // ephemeral local port

    const char msg[] = "hello alloy udp";
    const std::uint32_t mlen = sizeof(msg) - 1;
    // ipv4 ergonomic overload: 127.0.0.1:7
    ALLOY_CHECK_EQ(client.send_to({reinterpret_cast<const std::uint8_t*>(msg), mlen},
                                  alloy::net::ipv4{127, 0, 0, 1}, 7), mlen);

    alloy::ip_endpoint from{};
    std::uint8_t rx[64];
    std::uint32_t rn = 0;
    for (int i = 0; i < 3000 && rn == 0; ++i) {
        pump();
        rn = srv.recv_from(rx, from);           // server drains one datagram...
        if (rn != 0) (void)srv.send_to({rx, rn}, from);  // ...and echoes to its source
    }
    ALLOY_CHECK_EQ(rn, mlen);
    ALLOY_CHECK_EQ(from.addr, (std::uint32_t{127} << 24) | 1u);  // 127.0.0.1 packed
    ALLOY_CHECK(from.port != 0);                // client's ephemeral port recorded

    alloy::ip_endpoint echo_from{};
    std::uint8_t got[64];
    std::uint32_t gn = 0;
    for (int i = 0; i < 3000 && gn == 0; ++i) {
        pump();
        gn = client.recv_from(got, echo_from);
    }
    ALLOY_CHECK_EQ(gn, mlen);
    ALLOY_CHECK(std::memcmp(got, msg, mlen) == 0);
    ALLOY_CHECK_EQ(echo_from.port, static_cast<std::uint16_t>(7));  // echo came from srv:7
}

// UDP is message-oriented: three datagrams sent back-to-back must come back as
// three DISCRETE datagrams, each with its own length — never coalesced the way
// the TCP byte stream would merge them. Also proves the ring buffers depth > 1.
ALLOY_TEST(net_socket_udp_preserves_datagram_boundaries) {
    alloy::net::ensure_init();

    alloy::net::udp_socket<> srv;
    ALLOY_CHECK(srv.bind(7001));  // distinct port from the echo test
    alloy::net::udp_socket<> client;
    ALLOY_CHECK(client.bind(0));

    const char* msgs[3] = {"one", "twotwo", "three-3"};
    for (int k = 0; k < 3; ++k) {
        const auto len = static_cast<std::uint32_t>(std::strlen(msgs[k]));
        ALLOY_CHECK_EQ(client.send_to({reinterpret_cast<const std::uint8_t*>(msgs[k]), len},
                                      alloy::net::ipv4{127, 0, 0, 1}, 7001), len);
    }
    for (int i = 0; i < 300; ++i) pump();  // deliver all three

    // Three recv_from calls return the three datagrams individually, in order.
    for (int k = 0; k < 3; ++k) {
        alloy::ip_endpoint f{};
        std::uint8_t b[64];
        const std::uint32_t n = srv.recv_from(b, f);
        const auto len = static_cast<std::uint32_t>(std::strlen(msgs[k]));
        ALLOY_CHECK_EQ(n, len);
        ALLOY_CHECK(std::memcmp(b, msgs[k], len) == 0);
    }
    // Ring is drained.
    alloy::ip_endpoint f{};
    std::uint8_t b[64];
    ALLOY_CHECK_EQ(srv.recv_from(b, f), 0u);
    ALLOY_CHECK(!srv.pending());
}

// An oversized datagram is truncated to the caller's buffer (BSD semantics) AND
// still fully consumed — the ownership-critical branch (pbuf_free + ring advance
// on take != tot_len) that guards against a leaked/stuck datagram.
ALLOY_TEST(net_socket_udp_truncates_oversized_datagram) {
    alloy::net::ensure_init();

    alloy::net::udp_socket<> srv;
    ALLOY_CHECK(srv.bind(7002));
    alloy::net::udp_socket<> client;
    ALLOY_CHECK(client.bind(0));

    const char msg[] = "0123456789";  // 10 bytes
    const std::uint32_t mlen = sizeof(msg) - 1;
    ALLOY_CHECK_EQ(client.send_to({reinterpret_cast<const std::uint8_t*>(msg), mlen},
                                  alloy::net::ipv4{127, 0, 0, 1}, 7002), mlen);

    alloy::ip_endpoint from{};
    std::uint8_t out4[4];  // deliberately smaller than the 10-byte datagram
    std::uint32_t n = 0;
    for (int i = 0; i < 3000 && n == 0; ++i) {
        pump();
        n = srv.recv_from(out4, from);
    }
    ALLOY_CHECK_EQ(n, 4u);                          // clamped to out.size(), not tot_len
    ALLOY_CHECK(std::memcmp(out4, msg, 4) == 0);    // first 4 bytes delivered
    ALLOY_CHECK(!srv.pending());                    // whole datagram consumed despite truncation
    std::uint8_t b[64];
    ALLOY_CHECK_EQ(srv.recv_from(b, from), 0u);     // ring not stuck: nothing left
}

// When the RX ring is full, the newest datagram is tail-dropped IN the callback
// (pbuf freed there) — the other ownership-critical branch. A depth-2 socket fed
// three datagrams keeps the first two and drops the third.
ALLOY_TEST(net_socket_udp_ring_full_drops_newest) {
    alloy::net::ensure_init();

    alloy::net::udp_socket<2> srv;  // ring depth 2
    ALLOY_CHECK(srv.bind(7003));
    alloy::net::udp_socket<> client;
    ALLOY_CHECK(client.bind(0));

    const char* msgs[3] = {"aa", "bbb", "cccc"};
    for (int k = 0; k < 3; ++k) {
        const auto len = static_cast<std::uint32_t>(std::strlen(msgs[k]));
        ALLOY_CHECK_EQ(client.send_to({reinterpret_cast<const std::uint8_t*>(msgs[k]), len},
                                      alloy::net::ipv4{127, 0, 0, 1}, 7003), len);
    }
    for (int i = 0; i < 300; ++i) pump();  // deliver all three at the ring

    // Only the first two survive; the third was dropped when count_ hit RxDepth.
    for (int k = 0; k < 2; ++k) {
        alloy::ip_endpoint f{};
        std::uint8_t b[64];
        const std::uint32_t n = srv.recv_from(b, f);
        const auto len = static_cast<std::uint32_t>(std::strlen(msgs[k]));
        ALLOY_CHECK_EQ(n, len);
        ALLOY_CHECK(std::memcmp(b, msgs[k], len) == 0);
    }
    alloy::ip_endpoint f{};
    std::uint8_t b[64];
    ALLOY_CHECK_EQ(srv.recv_from(b, f), 0u);  // third was dropped, not queued
    ALLOY_CHECK(!srv.pending());
}

// close() leaves the socket rebindable: bind -> close -> bind the same port, and
// the rebound socket still receives. Proves close() clears pcb_ and the ring.
ALLOY_TEST(net_socket_udp_rebinds_after_close) {
    alloy::net::ensure_init();

    alloy::net::udp_socket<> s;
    ALLOY_CHECK(s.bind(7004));
    ALLOY_CHECK(s.bound());
    s.close();
    ALLOY_CHECK(!s.bound());
    ALLOY_CHECK(s.bind(7004));  // rebind the same port

    alloy::net::udp_socket<> client;
    ALLOY_CHECK(client.bind(0));
    const char msg[] = "reb";
    const std::uint32_t mlen = sizeof(msg) - 1;
    ALLOY_CHECK_EQ(client.send_to({reinterpret_cast<const std::uint8_t*>(msg), mlen},
                                  alloy::net::ipv4{127, 0, 0, 1}, 7004), mlen);

    alloy::ip_endpoint f{};
    std::uint8_t b[16];
    std::uint32_t n = 0;
    for (int i = 0; i < 3000 && n == 0; ++i) {
        pump();
        n = s.recv_from(b, f);
    }
    ALLOY_CHECK_EQ(n, mlen);
    ALLOY_CHECK(std::memcmp(b, msg, mlen) == 0);
}

#if LWIP_DHCP
// --- DHCP client host test -------------------------------------------------
// Drive a full DHCP handshake with no hardware: bring the stack up via DHCP,
// capture the DISCOVER it emits, then inject a crafted OFFER and ACK and assert
// the netif leases the offered address. ACD is ON (matching firmware), so the ACK
// puts the client into CHECKING and the lease binds only after the RFC5227 ARP-probe
// window — the test advances the clock to drive that exact path. UDP checksum is
// left 0 (lwIP skips it); the IP header checksum is computed (lwIP verifies it).

// 16-bit one's-complement checksum over a header (its own checksum field zeroed).
static std::uint16_t ip_checksum(const std::uint8_t* h, std::size_t len) {
    std::uint32_t sum = 0;
    for (std::size_t i = 0; i + 1 < len; i += 2)
        sum += (std::uint32_t(h[i]) << 8) | h[i + 1];
    if (len & 1) sum += std::uint32_t(h[len - 1]) << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<std::uint16_t>(~sum);
}

// The DHCP message-type (option 53) in a captured frame, or -1 if absent. Options
// start after Ethernet(14) + IP(20) + UDP(8) + BOOTP-fixed(236) + cookie(4) = 282.
static int dhcp_msgtype(const std::uint8_t* f, std::uint32_t len) {
    std::uint32_t i = 282;
    while (i + 1 < len) {
        const std::uint8_t code = f[i];
        if (code == 0) { ++i; continue; }   // PAD
        if (code == 255) break;             // END
        if (code == 53 && f[i + 1] >= 1 && i + 2 < len) return f[i + 2];
        i += 2u + f[i + 1];
    }
    return -1;
}

// Build a broadcast BOOTREPLY (OFFER msg_type=2 / ACK=5) leasing 10.0.0.50 from
// server 10.0.0.1, echoing the client's xid. Returns the frame length (342).
static std::uint32_t build_dhcp_reply(std::uint8_t msg_type, const std::uint8_t xid[4],
                                      const std::uint8_t client_mac[6], std::uint8_t out[342]) {
    std::memset(out, 0, 342);
    // Ethernet: dst broadcast, src server 02:..:01, ethertype IPv4.
    for (int i = 0; i < 6; ++i) out[i] = 0xFF;
    out[6] = 0x02; out[11] = 0x01;
    out[12] = 0x08; out[13] = 0x00;
    // IPv4: ver/ihl, total_len 328, ttl 64, proto UDP, 10.0.0.1 -> 255.255.255.255.
    out[14] = 0x45; out[16] = 0x01; out[17] = 0x48;
    out[22] = 0x40; out[23] = 0x11;
    out[26] = 10; out[27] = 0; out[28] = 0; out[29] = 1;
    out[30] = out[31] = out[32] = out[33] = 0xFF;
    const std::uint16_t c = ip_checksum(&out[14], 20);
    out[24] = static_cast<std::uint8_t>(c >> 8); out[25] = static_cast<std::uint8_t>(c & 0xFF);
    // UDP: src 67, dst 68, len 308, checksum 0 (skipped by lwIP).
    out[34] = 0x00; out[35] = 0x43; out[36] = 0x00; out[37] = 0x44;
    out[38] = 0x01; out[39] = 0x34;
    // BOOTP/DHCP (payload at 42): op REPLY, htype/hlen, xid, yiaddr, siaddr, chaddr.
    out[42] = 0x02; out[43] = 0x01; out[44] = 0x06;
    std::memcpy(&out[46], xid, 4);
    out[58] = 10; out[59] = 0; out[60] = 0; out[61] = 50;  // yiaddr 10.0.0.50
    out[62] = 10; out[63] = 0; out[64] = 0; out[65] = 1;   // siaddr 10.0.0.1
    std::memcpy(&out[70], client_mac, 6);                  // chaddr
    out[278] = 0x63; out[279] = 0x82; out[280] = 0x53; out[281] = 0x63;  // magic cookie
    std::uint8_t* o = &out[282];
    *o++ = 53; *o++ = 1; *o++ = msg_type;                  // 53 message type
    *o++ = 54; *o++ = 4; *o++ = 10; *o++ = 0; *o++ = 0; *o++ = 1;              // 54 server id (required in OFFER)
    *o++ = 51; *o++ = 4; *o++ = 0x00; *o++ = 0x01; *o++ = 0x51; *o++ = 0x80;   // 51 lease 86400
    *o++ = 1;  *o++ = 4; *o++ = 255; *o++ = 255; *o++ = 255; *o++ = 0;         // 1 subnet /24
    *o++ = 3;  *o++ = 4; *o++ = 10; *o++ = 0; *o++ = 0; *o++ = 1;              // 3 router
    *o++ = 255;                                            // END
    return 342;
}

ALLOY_TEST(net_dhcp_client_leases) {
    capture_dev dev;
    alloy::net::stack<capture_dev> s{dev};
    const std::uint8_t mac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x0D};

    s.up_dhcp(mac);  // dhcp_start emits DISCOVER synchronously (link is up)
    ALLOY_CHECK(!s.has_lease());

    // DISCOVER captured: BOOTREQUEST, UDP 68->67, magic cookie, message-type 1.
    ALLOY_CHECK(dev.tx_len >= 282u);
    ALLOY_CHECK_EQ(dev.tx[42], 0x01);                       // op = BOOTREQUEST
    ALLOY_CHECK(dev.tx[34] == 0x00 && dev.tx[35] == 0x44);  // UDP src 68 (client)
    ALLOY_CHECK(dev.tx[36] == 0x00 && dev.tx[37] == 0x43);  // UDP dst 67 (server)
    ALLOY_CHECK(dev.tx[278] == 0x63 && dev.tx[279] == 0x82 &&
                dev.tx[280] == 0x53 && dev.tx[281] == 0x63);
    ALLOY_CHECK_EQ(dhcp_msgtype(dev.tx, dev.tx_len), 1);    // DHCPDISCOVER

    std::uint8_t xid[4];
    std::memcpy(xid, &dev.tx[46], 4);  // echo the client's xid verbatim

    // Inject OFFER -> client sends REQUEST.
    std::uint8_t frame[342];
    dev.inject(frame, build_dhcp_reply(2, xid, mac, frame));  // 2 = OFFER
    s.poll();
    ALLOY_CHECK_EQ(dhcp_msgtype(dev.tx, dev.tx_len), 3);    // DHCPREQUEST

    // Inject ACK -> client accepts the lease and enters CHECKING (ACD probes for
    // an address conflict before binding — the firmware path).
    dev.inject(frame, build_dhcp_reply(5, xid, mac, frame));  // 5 = ACK
    s.poll();
    ALLOY_CHECK(!s.has_lease());  // not bound yet: ACD is still probing

    // Drive ACD to completion by advancing the clock (100 ms per acd_tmr tick).
    // No conflicting ARP is injected, so after the probe/announce window the client
    // binds. Bounded well past the ~9 s RFC5227 window; exits as soon as it binds.
    for (int i = 0; i < 400 && !s.has_lease(); ++i) {
        g_now_ms += 100;
        s.poll();
    }

    ALLOY_CHECK(s.has_lease());
    const alloy::net::ipv4 ip = s.ip_address();
    ALLOY_CHECK(ip.o[0] == 10 && ip.o[1] == 0 && ip.o[2] == 0 && ip.o[3] == 50);
}
#endif  // LWIP_DHCP
