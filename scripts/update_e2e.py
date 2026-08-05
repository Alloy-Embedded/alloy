#!/usr/bin/env python3
"""The full-wire firmware-update proof, against an emulated device.

Renode runs the REAL bootloader with its UART bridged to a TCP socket; this
script plays the field technician: it pushes a packed image through the REAL
`alloy update` client over that socket, then keeps listening and asserts the
device REBOOTS into the image it just received (bootloader banner -> "boot slot
A" -> the app's own banner). Every layer is the production one — host client,
wire protocol, transport receiver, updater, flash driver, verify, Cortex-M
reset+jump — with zero hardware.

Usage: update_e2e.py <image.img> [port] (default 3456; Renode side must have
  emulation CreateServerSocketTerminal <port> "term" false
  connector Connect sysbus.<uart> term
running before this script starts.)
"""

from __future__ import annotations

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools" / "alloy"))
import serial  # noqa: E402

from alloy_cli.ota_host import update  # noqa: E402


def wait_for(link, needles: list[bytes], deadline_s: float) -> None:
    """Read the UART stream until every needle has appeared, in order."""
    got = b""
    end = time.monotonic() + deadline_s
    want = list(needles)
    while want and time.monotonic() < end:
        chunk = link.read(256)
        if chunk:
            got += chunk
            while want and want[0] in got:
                got = got.split(want[0], 1)[1]
                want.pop(0)
    if want:
        raise SystemExit(
            f"FAIL: never saw {want[0]!r} after the update; last output: {got[-200:]!r}")


def main() -> None:
    image = Path(sys.argv[1]).read_bytes()
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 3456
    link = serial.serial_for_url(f"socket://localhost:{port}", timeout=2)

    # The bootloader may still be printing its startup lines; the HELLO retry
    # loop inside update() rides over them.
    info = update(link, image, retries=10)
    print(f"update accepted: {info}")

    # The device prints this, resets (SYSRESETREQ), and the fresh boot must pick
    # the image we just wrote — bootloader banner again, then the app's banner.
    wait_for(link, [b"update ok, rebooting",
                    b"alloy bootloader",
                    b"boot slot A",
                    b"alloy uart_echo ready"], deadline_s=30)
    print("PASS: device rebooted into the updated firmware")


if __name__ == "__main__":
    main()
