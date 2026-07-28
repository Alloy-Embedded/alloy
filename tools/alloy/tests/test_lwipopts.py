"""Unit tests for the generated lwipopts.h (emit/lwipopts.py).

The whole point of generating this header is that the emitted #defines are DERIVED
from board/chip facts + the project's [net] table — a wrong-but-valid config (a
feature left off, a checksum offloaded when the MAC won't, a pool sized from the
wrong MTU) is exactly what the cross-compile matrix can't catch, so it's pinned
here instead.
"""

from __future__ import annotations

import re

from alloy_cli.emit.lwipopts import NetProfile, net_profile, render_lwipopts


def _defines(text: str) -> dict[str, str]:
    """Parse `#define NAME value` -> {NAME: value}, comments stripped."""
    out: dict[str, str] = {}
    for line in text.splitlines():
        m = re.match(r"\s*#define\s+(\w+)\s+(.*)", line)
        if not m:
            continue
        value = m.group(2).split("/*")[0].strip()
        out[m.group(1)] = value
    return out


def test_default_firmware_profile_matches_v1_config() -> None:
    d = _defines(render_lwipopts(NetProfile()))
    # System model + static memory (the v1 hand-written values).
    assert d["NO_SYS"] == "1"
    assert d["LWIP_SOCKET"] == "0"
    assert d["MEM_LIBC_MALLOC"] == "0"
    assert d["MEM_SIZE"] == "(12 * 1024)"
    assert d["MEMP_NUM_TCP_PCB"] == "8"
    assert d["PBUF_POOL_BUFSIZE"] == "1536"
    # Protocols: TCP/UDP on, DHCP/DNS off for v1.
    assert d["LWIP_TCP"] == "1"
    assert d["LWIP_UDP"] == "1"
    assert d["LWIP_DHCP"] == "0"
    assert d["LWIP_DNS"] == "0"
    # MSS = MTU 1500 - 40.
    assert d["TCP_MSS"] == "1460"
    assert d["TCP_WND"] == "(4 * TCP_MSS)"
    # Firmware is a single netif (just the MAC); no loopback.
    assert d["LWIP_SINGLE_NETIF"] == "1"
    assert "LWIP_HAVE_LOOPIF" not in d
    # Checksums computed in software by default (no offload wired).
    assert d["CHECKSUM_GEN_TCP"] == "1"
    assert d["CHECKSUM_CHECK_IP"] == "1"


def test_host_profile_enables_loopback() -> None:
    d = _defines(render_lwipopts(NetProfile(host=True)))
    assert d["LWIP_HAVE_LOOPIF"] == "1"
    assert d["LWIP_NETIF_LOOPBACK"] == "1"
    assert d["LWIP_SINGLE_NETIF"] == "0"


def test_feature_toggles_flip_the_right_defines() -> None:
    d = _defines(render_lwipopts(NetProfile(dhcp=True, dns=True, udp=False)))
    assert d["LWIP_DHCP"] == "1"
    assert d["LWIP_DNS"] == "1"
    assert d["LWIP_UDP"] == "0"


def test_dhcp_acd_follows_dhcp_and_its_own_knob() -> None:
    # ACD is only meaningful with DHCP, and defaults on (matches lwIP).
    assert _defines(render_lwipopts(NetProfile()))["LWIP_DHCP_DOES_ACD_CHECK"] == "0"
    assert _defines(render_lwipopts(NetProfile(dhcp=True)))["LWIP_DHCP_DOES_ACD_CHECK"] == "1"
    # A project (or the host test profile) can turn ACD off.
    d = _defines(render_lwipopts(NetProfile(dhcp=True, dhcp_acd=False)))
    assert d["LWIP_DHCP_DOES_ACD_CHECK"] == "0"


def test_checksum_offload_turns_off_software_checksums() -> None:
    d = _defines(render_lwipopts(NetProfile(checksum_offload=True)))
    for name in ("CHECKSUM_GEN_IP", "CHECKSUM_GEN_TCP", "CHECKSUM_GEN_UDP",
                 "CHECKSUM_GEN_ICMP", "CHECKSUM_CHECK_IP", "CHECKSUM_CHECK_TCP",
                 "CHECKSUM_CHECK_UDP", "CHECKSUM_CHECK_ICMP"):
        assert d[name] == "0", name


def test_mtu_drives_tcp_mss() -> None:
    d = _defines(render_lwipopts(NetProfile(mtu=1400)))
    assert d["TCP_MSS"] == "1360"


def test_memory_budget_is_data_driven() -> None:
    d = _defines(render_lwipopts(NetProfile(mem_size_kb=32, tcp_pcb=16)))
    assert d["MEM_SIZE"] == "(32 * 1024)"
    assert d["MEMP_NUM_TCP_PCB"] == "16"


def test_generated_header_carries_no_long_hex() -> None:
    # The contract gate forbids >=8-digit hex in hand-written source; the emitted
    # header must stay clean too (it lands in the include path of real firmware).
    text = render_lwipopts(NetProfile())
    assert not re.search(r"0x[0-9A-Fa-f]{8,}", text)


# --- the profile builder: board/chip facts + [net] precedence --------------

_BOARD = {"roles": {"ethernet": {"peripheral": "gmac"}}}
_CHIP = {"peripherals": {"gmac": {}}}


def test_profile_defaults_when_data_is_silent() -> None:
    p = net_profile(_BOARD, _CHIP, {})
    assert p.host is False
    assert p.mtu == 1500
    assert p.checksum_offload is False
    assert p.tcp_mss == 1460


def test_board_eth_role_mtu_flows_into_profile() -> None:
    board = {"roles": {"ethernet": {"peripheral": "gmac", "mtu": 1400}}}
    p = net_profile(board, _CHIP, {})
    assert p.mtu == 1400
    assert p.tcp_mss == 1360


def test_chip_peripheral_declares_checksum_offload() -> None:
    chip = {"peripherals": {"gmac": {"checksum_offload": True}}}
    p = net_profile(_BOARD, chip, {})
    assert p.checksum_offload is True


def test_project_net_table_overrides_board_fact() -> None:
    board = {"roles": {"ethernet": {"peripheral": "gmac", "mtu": 1400}}}
    p = net_profile(board, _CHIP, {"mtu": 9000, "dhcp": True})
    assert p.mtu == 9000        # [net] wins over the board's 1400
    assert p.tcp_mss == 8960
    assert p.dhcp is True
