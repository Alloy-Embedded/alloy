"""Cross-platform serial-port discovery (pyserial list_ports).

Replaces the macOS-only /dev/cu.* globs: ports are matched by USB VID per
probe kind, which works identically on macOS (cu.*), Linux (ttyACM/ttyUSB)
and Windows (COMx). board.json probe.port remains an explicit override.
"""

from __future__ import annotations

from typing import Any

from .emit.common import EmitError

# USB vendor IDs per probe kind — tool-integration data, not silicon facts.
_KIND_VIDS: dict[str, set[int]] = {
    # CDC-ACM debug probes with a built-in VCP.
    "stlink": {0x0483},                  # STMicroelectronics
    "cmsis-dap": {0x03EB, 0x0D28},       # Atmel EDBG, ARM DAPLink
    # USB-UART bridges in front of a bare UART (ESP32 boards & friends).
    "esptool": {0x0403, 0x10C4, 0x1A86, 0x303A},  # FTDI, CP210x, CH34x, Espressif
}


def find_serial_port(probe: dict[str, Any]) -> str:
    if port := probe.get("port"):
        return str(port)
    kind = probe.get("kind", "")
    if kind == "bootsel":
        raise EmitError(
            "this board flashes over BOOTSEL mass-storage and exposes no "
            "serial port — wire a USB-serial adapter to use the UART"
        )

    from serial.tools import list_ports  # noqa: PLC0415

    vids = _KIND_VIDS.get(kind, set())
    all_ports = sorted(list_ports.comports(), key=lambda p: p.device)
    candidates = [p for p in all_ports if p.vid in vids] if vids else list(all_ports)

    if len(candidates) == 1:
        return candidates[0].device
    if len(candidates) == 2 and candidates[0].vid == candidates[1].vid \
            and candidates[0].pid == candidates[1].pid:
        # Dual-channel FTDI (WROVER-KIT FT2232H): channel A is JTAG,
        # channel B — the later-sorted device — is the UART.
        return candidates[1].device

    names = ", ".join(f"{p.device} ({p.description})" for p in candidates) or "(none)"
    raise EmitError(
        f"could not auto-pick a serial port for probe kind '{kind}' "
        f"(found: {names}) — set probe.port in board.json or plug exactly one board"
    )
