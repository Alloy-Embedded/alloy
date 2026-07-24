"""Bidirectional serial monitor — cross-platform (pyserial).

A reader thread streams device bytes to stdout; the main thread forwards
keystrokes to the device. Raw keystroke input is per-OS behavior: termios
cbreak on POSIX, msvcrt on Windows. Exit with Ctrl-] (the idf.py
convention, so Ctrl-C can reach the target app).
"""

from __future__ import annotations

import sys
import threading
from typing import Any

from .emit.common import EmitError
from .ports import find_serial_port

_EXIT_KEY = b"\x1d"  # Ctrl-]

_BAUD_WHITELIST = {9600, 19200, 38400, 57600, 115200, 230400}


def _resolve_baud(board: dict[str, Any]) -> int:
    baud = int(board.get("roles", {}).get("debug_uart", {}).get("baud", 115200))
    if baud not in _BAUD_WHITELIST:
        raise EmitError(f"unsupported baud {baud} (known: {sorted(_BAUD_WHITELIST)})")
    return baud


def monitor(board: dict[str, Any]) -> None:
    probe = board.get("probe")
    if probe is None:
        raise EmitError(f"board {board['id']} declares no probe — no serial to monitor")
    if not board.get("roles", {}).get("debug_uart"):
        raise EmitError(f"board {board['id']} declares no debug_uart role")

    import serial  # noqa: PLC0415

    port_name = find_serial_port(probe)
    baud = _resolve_baud(board)
    port = serial.Serial(port_name, baud, timeout=0.1)
    # pyserial asserts DTR/RTS on open; on FT2232H auto-download circuits
    # (ESP32 boards) an asserted RTS holds the chip in reset — release both.
    # But do this ONLY for those USB-UART bridges: on an EDBG/CMSIS-DAP CDC
    # (SAM boards) driving DTR/RTS low instead HOLDS the target in reset, so
    # the monitor would see nothing. Leave the lines untouched for debug probes.
    if probe.get("kind") == "esptool":
        port.dtr = False
        port.rts = False

    print(f"monitor: {port_name} @ {baud} (Ctrl-] to exit)")
    stop = threading.Event()

    def reader() -> None:
        while not stop.is_set():
            try:
                data = port.read(4096)
            except (OSError, serial.SerialException):
                stop.set()
                break
            if data:
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()

    t = threading.Thread(target=reader, daemon=True)
    t.start()
    try:
        _forward_keys(port, stop)
    finally:
        stop.set()
        t.join(timeout=1.0)
        port.close()
        print("\nmonitor: closed")


def _forward_keys(port: Any, stop: threading.Event) -> None:
    if sys.platform == "win32":
        import msvcrt  # noqa: PLC0415
        import time  # noqa: PLC0415

        while not stop.is_set():
            if msvcrt.kbhit():
                ch = msvcrt.getch()
                if ch == _EXIT_KEY:
                    return
                port.write(ch)
            else:
                time.sleep(0.02)
        return

    import select  # noqa: PLC0415
    import termios  # noqa: PLC0415
    import tty  # noqa: PLC0415

    fd = sys.stdin.fileno()
    saved = termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        while not stop.is_set():
            ready, _, _ = select.select([fd], [], [], 0.1)
            if not ready:
                continue
            ch = sys.stdin.buffer.read1(1)
            if ch == _EXIT_KEY:
                return
            port.write(ch)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, saved)
