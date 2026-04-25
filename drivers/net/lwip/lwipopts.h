/*
 * lwipopts.h — Alloy lwIP configuration for bare-metal no-OS targets.
 *
 * Configures lwIP for:
 *   - NO_SYS = 1 (cooperative, no RTOS)
 *   - Raw API (not socket API)
 *   - TCP + UDP + DHCP enabled; ICMP/ARP included by default
 *   - Static pool allocator (no malloc) with fixed-size .bss pool
 *   - Statistics disabled to save flash
 *
 * Footprint guidance (with default pool sizes):
 *   ~12 KB flash + ~8 KB RAM for the pool allocator at these settings.
 *   Tune MEM_SIZE, MEMP_NUM_TCP_PCB, TCP_MSS as needed.
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* ---- System ------------------------------------------------------------ */
#define NO_SYS              1     /* no RTOS — cooperative polling model   */
#define SYS_LIGHTWEIGHT_PROT 0    /* no critical-section hooks needed      */

/* ---- Memory allocator -------------------------------------------------- */
#define MEM_LIBC_MALLOC     0     /* use lwIP's own pool allocator         */
#define MEM_ALIGNMENT       4
#define MEM_SIZE            (8 * 1024)   /* 8 KB heap for pbuf chains      */

/* ---- Mbufs / pbufs ----------------------------------------------------- */
#define PBUF_POOL_SIZE      16
#define PBUF_POOL_BUFSIZE   1536

/* ---- Protocols --------------------------------------------------------- */
#define LWIP_TCP            1
#define LWIP_UDP            1
#define LWIP_DHCP           1
#define LWIP_ICMP           1
#define LWIP_ARP            1
#define LWIP_ETHERNET       1
#define LWIP_IPV4           1
#define LWIP_IPV6           0     /* IPv6 not needed for Modbus/control    */

/* ---- TCP tuning -------------------------------------------------------- */
#define TCP_MSS             1460
#define TCP_SND_BUF         (4 * TCP_MSS)
#define TCP_WND             (4 * TCP_MSS)
#define MEMP_NUM_TCP_PCB    4     /* max simultaneous TCP connections      */
#define MEMP_NUM_TCP_PCB_LISTEN 2

/* ---- UDP --------------------------------------------------------------- */
#define MEMP_NUM_UDP_PCB    4

/* ---- DHCP -------------------------------------------------------------- */
#define DHCP_DOES_ARP_CHECK 0

/* ---- Loopback interface (used for host tests) -------------------------- */
#define LWIP_HAVE_LOOPIF    1
#define LWIP_LOOPBACK_MAX_PBUFS 8

/* ---- Debugging / stats ------------------------------------------------- */
#define LWIP_STATS          0
#define LWIP_DEBUG          0

/* ---- Checksum ---------------------------------------------------------- */
/* Hardware checksum offload can be enabled per-netif; disabled by default. */
#define CHECKSUM_GEN_IP     1
#define CHECKSUM_GEN_UDP    1
#define CHECKSUM_GEN_TCP    1
#define CHECKSUM_CHECK_IP   1
#define CHECKSUM_CHECK_UDP  1
#define CHECKSUM_CHECK_TCP  1

/* ---- Netif ------------------------------------------------------------- */
#define LWIP_NETIF_API      0     /* raw netif API only                    */
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK   1

#endif /* LWIPOPTS_H */
