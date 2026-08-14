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


def _open_port(board: dict[str, Any]) -> tuple[Any, str, int]:
    """(port, name, baud) — the setup both the interactive and the NDJSON
    monitor need, including the DTR/RTS handling that differs per probe."""
    probe = board.get("probe")
    if probe is None:
        raise EmitError(f"board {board['id']} declares no probe — no serial to monitor")
    if not board.get("roles", {}).get("debug_uart"):
        raise EmitError(f"board {board['id']} declares no debug_uart role")

    import serial  # noqa: PLC0415

    port_name = find_serial_port(probe)
    baud = _resolve_baud(board)
    port = serial.Serial(port_name, baud, timeout=0.1)
    if probe.get("kind") == "esptool":
        port.dtr = False
        port.rts = False
    return port, port_name, baud


def monitor(board: dict[str, Any], bus_model: dict[str, Any] | None = None) -> None:
    import serial  # noqa: PLC0415

    from .bus_decode import format_message, frame_scanner  # noqa: PLC0415

    port, port_name, baud = _open_port(board)
    probe = board.get("probe") or {}
    # When the project declares a wire contract, bus datagrams on this link
    # are decoded into readable lines instead of printing as binary confetti
    # between the log lines. Bytes that are not a complete, CRC-valid frame
    # pass through untouched, so no log text is ever eaten by a false frame.
    scanner = frame_scanner(bus_model) if bus_model is not None else None
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
            if not data:
                continue
            if scanner is not None:
                data, msgs = scanner.feed(data)
                # One stream, one write path: mixing sys.stdout with
                # sys.stdout.buffer here would interleave by flush order,
                # not by arrival, and scramble the log.
                for m in msgs:
                    sys.stdout.buffer.write(
                        (format_message(m) + "\r\n").encode("utf-8", "replace"))
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
        if scanner is not None:
            held = scanner.flush()  # a held partial frame was text after all
            if held:
                sys.stdout.buffer.write(held)
                sys.stdout.buffer.flush()
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


# ---------------------------------------------------------------- NDJSON mode

def monitor_ndjson(board: dict[str, Any],
                   bus_model: dict[str, Any] | None = None) -> None:
    """The same serial link, as one JSON object per line on stdout.

    The interactive monitor puts the terminal in cbreak mode to forward
    keystrokes, which needs a TTY it does not have when an editor spawns it. So
    this is a separate path rather than a flag threaded through that one: RX is
    line-buffered and stamped, TX arrives as whole lines on stdin.

    Timestamps are milliseconds since the link opened, not wall clock — what a
    reader wants from a device log is the interval between lines.

    With a bus model, decoded datagrams arrive on the SAME stream as
    {"t": …, "bus": {…}} — same timeline, so a panel can show a message
    beside the log line that preceded it. The decode happens here, never in
    the editor: the IDE renders what the CLI already understood.
    """
    import json  # noqa: PLC0415
    import time  # noqa: PLC0415

    import serial  # noqa: PLC0415

    from .bus_decode import frame_scanner  # noqa: PLC0415

    port, port_name, baud = _open_port(board)
    started = time.monotonic()
    scanner = frame_scanner(bus_model) if bus_model is not None else None

    def emit(obj: dict[str, Any]) -> None:
        sys.stdout.write(json.dumps(obj) + "\n")
        sys.stdout.flush()

    emit({"event": "open", "port": port_name, "baud": baud,
          "bus_decode": scanner is not None})
    stop = threading.Event()

    def writer() -> None:
        """Whole lines from stdin go to the device.

        End of stdin means "nothing more to send", NOT "close the link" — a
        caller that only wants to LISTEN passes no stdin at all, and stopping
        there would end the session before a single line was read.
        """
        for line in sys.stdin:
            if stop.is_set():
                return
            try:
                port.write(line.encode("utf-8", "replace"))
            except (OSError, serial.SerialException):
                return

    threading.Thread(target=writer, daemon=True).start()

    pending = bytearray()
    try:
        while not stop.is_set():
            try:
                data = port.read(4096)
            except (OSError, serial.SerialException) as exc:
                emit({"event": "error", "message": str(exc)})
                break
            if not data:
                continue
            if scanner is not None:
                data, msgs = scanner.feed(data)
                for m in msgs:
                    emit({"t": round((time.monotonic() - started) * 1000),
                          "bus": m})
            pending.extend(data)
            while b"\n" in pending:
                raw, _, rest = pending.partition(b"\n")
                pending = bytearray(rest)
                emit({"t": round((time.monotonic() - started) * 1000),
                      "line": raw.rstrip(b"\r").decode("utf-8", "replace")})
    finally:
        stop.set()
        if scanner is not None:
            # Bytes held as a possible frame that never completed are text
            # after all — releasing them is what keeps a trailing prompt from
            # disappearing into the scanner.
            pending.extend(scanner.flush())
        if pending:
            # Do not swallow a device that printed a prompt with no newline.
            emit({"t": round((time.monotonic() - started) * 1000),
                  "line": pending.decode("utf-8", "replace"), "partial": True})
        port.close()
        emit({"event": "closed"})
