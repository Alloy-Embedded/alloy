"""Interactive serial monitor (raw termios, no external deps).

Bidirectional: keystrokes go to the board, board output to the terminal.
Ctrl-] exits (the idf.py convention, so Ctrl-C can reach the target app).
"""

from __future__ import annotations

import os
import select
import sys
import termios
import tty
from typing import Any

from .emit.common import EmitError
from .flash import find_serial_port

_BAUD_CONSTANTS = {
    9600: termios.B9600,
    19200: termios.B19200,
    38400: termios.B38400,
    57600: termios.B57600,
    115200: termios.B115200,
    230400: termios.B230400,
}

_EXIT_KEY = b"\x1d"  # Ctrl-]


def monitor(board: dict[str, Any]) -> None:
    probe = board.get("probe", {})
    uart_role = board.get("roles", {}).get("debug_uart")
    if uart_role is None:
        raise EmitError(f"board {board['id']} declares no debug_uart role")
    baud = uart_role.get("baud", 115200)
    baud_const = _BAUD_CONSTANTS.get(baud)
    if baud_const is None:
        raise EmitError(f"unsupported monitor baud {baud}")

    port = find_serial_port(probe)
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CREAD | termios.CLOCAL | termios.CS8
    attrs[3] = 0
    attrs[4] = baud_const
    attrs[5] = baud_const
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

    print(f"monitor: {port} @ {baud} — Ctrl-] to quit")
    stdin_fd = sys.stdin.fileno()
    interactive = sys.stdin.isatty()
    saved = termios.tcgetattr(stdin_fd) if interactive else None
    try:
        if interactive:
            tty.setcbreak(stdin_fd)
        while True:
            watch = [fd, stdin_fd] if interactive else [fd]
            readable, _, _ = select.select(watch, [], [], 0.1)
            if fd in readable:
                try:
                    data = os.read(fd, 4096)
                except BlockingIOError:
                    data = b""
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
            if interactive and stdin_fd in readable:
                key = os.read(stdin_fd, 64)
                if _EXIT_KEY in key:
                    break
                os.write(fd, key)
    except KeyboardInterrupt:
        pass
    finally:
        if saved is not None:
            termios.tcsetattr(stdin_fd, termios.TCSANOW, saved)
        os.close(fd)
        print("\nmonitor: closed")
