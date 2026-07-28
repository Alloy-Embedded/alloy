"""Generate lwIP's ``lwipopts.h`` from board + project data.

lwIP's entire configuration is a single header it ``#include``s (``lwipopts.h``);
every knob left unset there takes lwIP's own default from ``lwip/opt.h``. Under
alloy's contract that header is a FACT SHEET, not hand-tuned C: the feature set,
pool sizes, and checksum policy are DERIVED from what the board provides (does
the MAC offload checksums? what MTU?) and what the project declares it needs
(the optional ``[net]`` table in ``alloy.toml``). So a second Ethernet chip, or
a project that wants DHCP, costs data — not an edit to a header shared by every
target.

One renderer, two callers:
  * firmware (``emit/__init__.generate``) — per board+project, a single netif;
  * host tests (``cmd_test``) — a fixed profile with lwIP's loopback (127.0.0.1)
    turned on, so the Socket facade gets a real TCP handshake with no hardware.

The defaults below reproduce the v1 hand-written config exactly, so turning this
on changes no bytes for an existing firmware target — only the SOURCE of truth.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

# IP (20) + TCP (20) headers: the classic MSS = MTU - 40.
_TCP_TCPIP_OVERHEAD = 40


@dataclass(frozen=True)
class NetProfile:
    """The knobs a ``lwipopts.h`` is rendered from. Defaults == the v1 config."""

    # Target context.
    host: bool = False              # host tests: loopback on, dual netif.

    # Board/chip facts (the per-chip seam — today's data uses the defaults).
    mtu: int = 1500                 # link MTU; TCP_MSS derives from it.
    checksum_offload: bool = False  # MAC computes IP/TCP/UDP/ICMP checksums.

    # Protocol features (project policy).
    udp: bool = True
    tcp: bool = True
    dhcp: bool = False              # static IP for v1; opt in per project.
    dns: bool = False
    igmp: bool = False
    autoip: bool = False

    # Static memory budget (project policy — no libc malloc, ever).
    mem_size_kb: int = 12
    memp_num_pbuf: int = 16
    tcp_pcb: int = 8                # concurrent TCP connections.
    tcp_pcb_listen: int = 4         # listening sockets.
    tcp_seg: int = 24
    pbuf_pool: int = 16
    pbuf_pool_bufsize: int = 1536

    # TCP window / send buffer, as multiples of MSS.
    tcp_wnd_mss: int = 4
    tcp_snd_mss: int = 4

    @property
    def tcp_mss(self) -> int:
        return self.mtu - _TCP_TCPIP_OVERHEAD


def net_profile(board: dict[str, Any], chip: dict[str, Any],
                net_opts: dict[str, Any]) -> NetProfile:
    """Assemble a firmware :class:`NetProfile` from board/chip facts + ``[net]``.

    Called only for boards that declare an ethernet role. Precedence: an explicit
    ``[net]`` key in the project's ``alloy.toml`` overrides the board/chip fact,
    which overrides the built-in default.
    """
    eth = board.get("roles", {}).get("ethernet", {})
    periph = chip.get("peripherals", {}).get(eth.get("peripheral"), {})

    def pick(key: str, fact: Any, default: Any) -> Any:
        return net_opts.get(key, fact if fact is not None else default)

    return NetProfile(
        host=False,
        mtu=int(pick("mtu", eth.get("mtu"), 1500)),
        checksum_offload=bool(pick("checksum_offload",
                                   periph.get("checksum_offload"), False)),
        udp=bool(net_opts.get("udp", True)),
        tcp=bool(net_opts.get("tcp", True)),
        dhcp=bool(net_opts.get("dhcp", False)),
        dns=bool(net_opts.get("dns", False)),
        igmp=bool(net_opts.get("igmp", False)),
        autoip=bool(net_opts.get("autoip", False)),
        mem_size_kb=int(net_opts.get("mem_size_kb", 12)),
        memp_num_pbuf=int(net_opts.get("memp_num_pbuf", 16)),
        tcp_pcb=int(net_opts.get("tcp_pcb", 8)),
        tcp_pcb_listen=int(net_opts.get("tcp_pcb_listen", 4)),
        tcp_seg=int(net_opts.get("tcp_seg", 24)),
        pbuf_pool=int(net_opts.get("pbuf_pool", 16)),
        pbuf_pool_bufsize=int(net_opts.get("pbuf_pool_bufsize", 1536)),
        tcp_wnd_mss=int(net_opts.get("tcp_wnd_mss", 4)),
        tcp_snd_mss=int(net_opts.get("tcp_snd_mss", 4)),
    )


def _b(value: bool) -> int:
    return 1 if value else 0


def render_lwipopts(p: NetProfile) -> str:
    """Render a complete ``lwipopts.h`` for the given profile."""
    csum = _b(not p.checksum_offload)  # 1 = compute in software.
    if p.host:
        netif_block = (
            "/* Host tests: lwIP's loopback (127.0.0.1) is on so the Socket facade\n"
            " * gets a real TCP handshake with no hardware; NO_SYS drains it via\n"
            " * netif_poll_all() from the test's pump loop. */\n"
            "#define LWIP_HAVE_LOOPIF                   1\n"
            "#define LWIP_NETIF_LOOPBACK                1\n"
            "#define LWIP_NETIF_LOOPBACK_MULTITHREADING 0\n"
            "#define LWIP_LOOPBACK_MAX_PBUFS            8\n"
            "#define LWIP_SINGLE_NETIF                  0  /* loopif + (test) netif */\n"
        )
    else:
        netif_block = (
            "#define LWIP_SINGLE_NETIF                  1  /* just the MAC */\n"
        )

    origin = ("the host-test profile (loopback on)" if p.host
              else "board + project data ([net] in alloy.toml)")
    return f"""/* GENERATED by alloy (emit/lwipopts.py) from {origin} — DO NOT EDIT.
 *
 * lwIP configuration for alloy's NO_SYS net stack. Every knob NOT set here takes
 * lwIP's default from vendor/lwip/src/include/lwip/opt.h. Under alloy's contract
 * this is a fact sheet derived from data, not a hand-tuned header — regenerate,
 * don't edit. */
#ifndef ALLOY_LWIPOPTS_H
#define ALLOY_LWIPOPTS_H

/* -- System model: bare-metal, single-threaded poll loop -- */
#define NO_SYS                     1
#define SYS_LIGHTWEIGHT_PROT       0
#define LWIP_NETCONN               0  /* sequential API needs an RTOS */
#define LWIP_SOCKET                0  /* BSD sockets need an RTOS */
#define LWIP_TIMERS                1  /* sys_check_timeouts() from the loop */

/* -- Memory: static pools, no libc malloc -- */
#define MEM_LIBC_MALLOC            0
#define MEMP_MEM_MALLOC            0
#define MEM_ALIGNMENT              4
#define MEM_SIZE                   ({p.mem_size_kb} * 1024)
#define MEMP_NUM_PBUF              {p.memp_num_pbuf}
#define MEMP_NUM_TCP_PCB           {p.tcp_pcb}
#define MEMP_NUM_TCP_PCB_LISTEN    {p.tcp_pcb_listen}
#define MEMP_NUM_TCP_SEG           {p.tcp_seg}
#define PBUF_POOL_SIZE             {p.pbuf_pool}
#define PBUF_POOL_BUFSIZE          {p.pbuf_pool_bufsize}

/* -- Protocols -- */
#define LWIP_IPV4                  1
#define LWIP_IPV6                  0
#define LWIP_ARP                   1
#define LWIP_ETHERNET              1
#define LWIP_ICMP                  1
#define LWIP_RAW                   1
#define LWIP_UDP                   {_b(p.udp)}
#define LWIP_TCP                   {_b(p.tcp)}
#define LWIP_DHCP                  {_b(p.dhcp)}
#define LWIP_DNS                   {_b(p.dns)}
#define LWIP_IGMP                  {_b(p.igmp)}
#define LWIP_AUTOIP                {_b(p.autoip)}

/* -- TCP tuning (MSS = MTU {p.mtu} - 40) -- */
#define TCP_MSS                    {p.tcp_mss}
#define TCP_WND                    ({p.tcp_wnd_mss} * TCP_MSS)
#define TCP_SND_BUF                ({p.tcp_snd_mss} * TCP_MSS)
#define TCP_SND_QUEUELEN           ((4 * TCP_SND_BUF) / TCP_MSS)

/* -- netif -- */
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK   0
#define LWIP_NETIF_HOSTNAME        0
{netif_block}
/* -- Checksums: {'MAC hardware offload' if p.checksum_offload else 'computed in software'} -- */
#define CHECKSUM_GEN_IP            {csum}
#define CHECKSUM_GEN_UDP           {csum}
#define CHECKSUM_GEN_TCP           {csum}
#define CHECKSUM_GEN_ICMP          {csum}
#define CHECKSUM_CHECK_IP          {csum}
#define CHECKSUM_CHECK_UDP         {csum}
#define CHECKSUM_CHECK_TCP         {csum}
#define CHECKSUM_CHECK_ICMP        {csum}

/* -- Diagnostics: off (no printf on firmware) -- */
#define LWIP_STATS                 0
#define LWIP_DEBUG                 0

#endif /* ALLOY_LWIPOPTS_H */
"""
