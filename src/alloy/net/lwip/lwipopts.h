/* lwIP configuration for alloy (bare-metal NO_SYS, IPv4 + TCP, static pools).
 *
 * This is the hand-written v1; the plan (CONNECTIVITY.md) is to GENERATE it from
 * board data (pool sizes, features). Everything not set here takes lwIP's
 * default from src/include/lwip/opt.h. */
#ifndef ALLOY_LWIPOPTS_H
#define ALLOY_LWIPOPTS_H

/* ── System model ──────────────────────────────────────────────────────── */
#define NO_SYS                     1  /* bare-metal, single-threaded poll loop */
#define SYS_LIGHTWEIGHT_PROT       0  /* no ISR/thread contention on lwIP state */
#define LWIP_NETCONN               0  /* sequential API needs an RTOS */
#define LWIP_SOCKET                0  /* BSD sockets need an RTOS */
#define LWIP_TIMERS                1  /* sys_check_timeouts() driven from the loop */

/* ── Memory: static, no libc malloc ────────────────────────────────────── */
#define MEM_LIBC_MALLOC            0
#define MEMP_MEM_MALLOC            0
#define MEM_ALIGNMENT              4
#define MEM_SIZE                   (12 * 1024)
#define MEMP_NUM_PBUF              16
#define MEMP_NUM_TCP_PCB           8
#define MEMP_NUM_TCP_PCB_LISTEN    4
#define MEMP_NUM_TCP_SEG           24
#define PBUF_POOL_SIZE             16
#define PBUF_POOL_BUFSIZE          1536

/* ── Protocols ─────────────────────────────────────────────────────────── */
#define LWIP_IPV4                  1
#define LWIP_IPV6                  0
#define LWIP_ARP                   1
#define LWIP_ETHERNET              1
#define LWIP_ICMP                  1
#define LWIP_RAW                   1
#define LWIP_UDP                   1
#define LWIP_TCP                   1
#define LWIP_DHCP                  0  /* static IP for v1; DHCP is a follow-up */
#define LWIP_DNS                   0
#define LWIP_IGMP                  0
#define LWIP_AUTOIP                0

/* ── TCP tuning ────────────────────────────────────────────────────────── */
#define TCP_MSS                    1460
#define TCP_WND                    (4 * TCP_MSS)
#define TCP_SND_BUF                (4 * TCP_MSS)
#define TCP_SND_QUEUELEN           ((4 * TCP_SND_BUF) / TCP_MSS)

/* ── netif ─────────────────────────────────────────────────────────────── */
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK   0
#define LWIP_NETIF_HOSTNAME        0

/* Loopback (127.0.0.1) is enabled ONLY for the host tests (ALLOY_LWIP_HOST), so
 * the Socket facade gets a real TCP handshake with no hardware; firmware stays
 * lean with a single netif (the GMAC). NO_SYS drains loopback via
 * netif_poll_all() from the poll loop. */
#ifdef ALLOY_LWIP_HOST
#define LWIP_HAVE_LOOPIF                    1
#define LWIP_NETIF_LOOPBACK                 1
#define LWIP_NETIF_LOOPBACK_MULTITHREADING  0
#define LWIP_LOOPBACK_MAX_PBUFS             8
#define LWIP_SINGLE_NETIF                   0  /* eth + loopif */
#else
#define LWIP_SINGLE_NETIF                   1  /* just the GMAC */
#endif

/* ── Checksums: computed in software (no MAC offload wired yet) ─────────── */
#define CHECKSUM_GEN_IP            1
#define CHECKSUM_GEN_UDP           1
#define CHECKSUM_GEN_TCP           1
#define CHECKSUM_GEN_ICMP          1
#define CHECKSUM_CHECK_IP          1
#define CHECKSUM_CHECK_UDP         1
#define CHECKSUM_CHECK_TCP         1
#define CHECKSUM_CHECK_ICMP        1

/* ── Diagnostics: off (no printf on firmware) ──────────────────────────── */
#define LWIP_STATS                 0
#define LWIP_DEBUG                 0

#endif /* ALLOY_LWIPOPTS_H */
