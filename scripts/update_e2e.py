#!/usr/bin/env python3
"""The full trial/confirm/rollback firmware-update proof, against an emulated device.

Renode runs the REAL bootloader (blank flash) with its UART bridged to a TCP
socket and its monitor on a second port (the harness's "power button"). This
script then walks the whole product lifecycle through the REAL `alloy update`
client and asserts every transition on the UART:

  1. install GOOD app (ota_app, links slot B on a blank board)  -> update ok
  2. reboot: TRIAL boot slot B -> app health-checks -> "ota_app confirmed"
  3. power-cycle: normal "boot slot B" (the confirm stuck across reset)
  4. update BAD app (uart_echo v2, links slot A, never confirms) -> update ok
  5. trial boot slot A x3 (each consumes one attempt; app never confirms)
  6. next boot: "reverted, boot slot B" -> the GOOD app is back

Every layer is production: host client, wire protocol, transport receiver,
updater, flash driver, verify, the power-atomic boot_store, the trial/confirm
machine, and the Cortex-M reset/jump. Zero hardware.

Usage: update_e2e.py <good_slot_b.img> <bad_slot_a.img> [uart_port] [monitor_port]
Renode side (before this script):
  emulation CreateServerSocketTerminal <uart_port> "term" false
  connector Connect sysbus.<uart> term
plus a `macro reset` that re-LoadELF's the bootloader, and -P <monitor_port>.
"""

from __future__ import annotations

import os
import socket
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools" / "alloy"))
import serial  # noqa: E402

from alloy_cli.ota_host import update  # noqa: E402


# Wall-clock budget for a phase that includes an emulated slot ERASE plus a
# boot. On the SAME70 that erase is ~1 MB driven page-by-page through the Python
# EFC model (~128k bus writes), and a shared CI runner measures ~4x slower than
# a dev machine — the SAME70 boot legs take 44 s there versus 11 s here. 30 s was
# tuned on local timings and expired mid-boot in CI on a device that was working
# perfectly. A genuinely stuck device still fails; it just fails later.
PHASE_TIMEOUT_S = 150

#: The watchdog legs get their own budget, and it is the largest in this file.
#:
#: They are the only phases whose wall time is set by EMULATION SPEED rather
#: than by the firmware. The bootloader arms the IWDG for 2 s and jumps into an
#: app that hangs in a read poll; Renode's IWDG counts VIRTUAL time, so the
#: reset costs ~2 s of emulated execution — on the order of 10^8 instructions of
#: an idle loop — and a slow runner turns that into three or four minutes of
#: real time. At 240 s a GitHub runner having a bad day landed the reset at
#: t+235.5 s and the run went red with the firmware working perfectly.
#:
#: Raise it rather than shrink the test: the leg proves the one thing no unit
#: test can — that a hung trial recovers with nobody pressing anything.
WATCHDOG_TIMEOUT_S = int(os.environ.get("ALLOY_E2E_WATCHDOG_TIMEOUT_S", "600"))


def wait_for(link, needles: list[bytes], deadline_s: float, stage: str) -> None:
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
    took = deadline_s - (end - time.monotonic())
    if want:
        raise SystemExit(f"FAIL [{stage}]: never saw {want[0]!r} in {took:.0f}s "
                         f"(budget {deadline_s:.0f}s); last output: {got[-200:]!r}")
    # The elapsed time is the whole diagnosis when this goes red. A leg that
    # took 235s of a 240s budget is a budget problem; one that dies at 3s is a
    # firmware problem, and the message alone cannot tell them apart. flush,
    # because CI pipes stdout and a buffered log arrives with every line
    # carrying the same timestamp — which is exactly what made one of these
    # failures take hours to read.
    print(f"  ok: {stage} [{took:.0f}s of {deadline_s:.0f}s]", flush=True)


class Monitor:
    """The harness's power button: drives Renode's monitor socket."""

    def __init__(self, port: int):
        self.sock = socket.create_connection(("localhost", port), timeout=5)
        self.sock.settimeout(0.2)
        time.sleep(0.3)
        self._drain()
        self.send('mach set "upd"')

    def _drain(self) -> None:
        try:
            while self.sock.recv(4096):
                pass
        except (TimeoutError, OSError):
            pass

    def send(self, cmd: str) -> None:
        self.sock.sendall(cmd.encode() + b"\r\n")
        time.sleep(0.4)
        self._drain()

    def power_cycle(self) -> None:
        self.send("machine Reset")
        self.send("start")  # harmless if already running


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    good = Path(args[0]).read_bytes()
    bad = Path(args[1]).read_bytes()
    uart_port = int(args[2]) if len(args) > 2 else 3456
    mon_port = int(args[3]) if len(args) > 3 else 12349
    link = serial.serial_for_url(f"socket://localhost:{uart_port}", timeout=2)
    mon = Monitor(mon_port)

    # 1. blank board -> install the good app (bootloader targets slot B: 1-active(0))
    info = update(link, good, retries=10)
    assert info["target_slot"] == 1, f"expected target B on a blank board, got {info}"
    print(f"  ok: good image accepted -> slot B {info}", flush=True)

    # 2. reboot -> trial boot -> the app confirms itself
    wait_for(link, [b"update ok, rebooting", b"alloy bootloader",
                    b"trial boot slot B", b"alloy ota_app ready",
                    b"ota_app confirmed"], PHASE_TIMEOUT_S, "trial boot + confirm")

    # 3. power-cycle -> a NORMAL boot of the confirmed slot (state persisted)
    mon.power_cycle()
    wait_for(link, [b"alloy bootloader", b"boot slot B", b"alloy ota_app ready"],
             PHASE_TIMEOUT_S, "confirm persisted across power cycle")

    if "--good-only" in sys.argv:
        # Families whose watchdog isn't modeled in Renode yet stop here: the
        # install/trial/confirm/persist phases still prove the whole flash path
        # (erase, program, FLUSH DURABILITY across reset) end to end.
        print("PASS (good-only): install -> trial -> confirm -> persisted", flush=True)
        return

    # 4. update with the bad app (never confirms); device now targets slot A
    mon.power_cycle()  # reopen the bootloader's update window
    info = update(link, bad, retries=10)
    assert info["target_slot"] == 0, f"expected target A, got {info}"
    print(f"  ok: bad image accepted -> slot A {info}", flush=True)

    # 5. three trial boots with NO help from the harness: the bad app (uart_echo)
    # never confirms and never feeds, so the WATCHDOG the bootloader armed before
    # each trial jump resets the device by itself — the "hung trial" answer.
    # Only trial 1's reboot comes from the update's own reset.
    wait_for(link, [b"update ok, rebooting", b"alloy bootloader",
                    b"trial boot slot A", b"alloy uart_echo ready"], PHASE_TIMEOUT_S, "trial 1/3")
    for n in (2, 3):
        wait_for(link, [b"alloy bootloader", b"trial boot slot A",
                        b"alloy uart_echo ready"], WATCHDOG_TIMEOUT_S,
                 f"trial {n}/3 (watchdog self-reset, no power button)")

    # 6. attempts exhausted -> automatic rollback, again via the watchdog alone
    wait_for(link, [b"alloy bootloader", b"reverted, boot slot B",
                    b"alloy ota_app ready"], WATCHDOG_TIMEOUT_S,
             "AUTOMATIC ROLLBACK (autonomous)")

    print("PASS: install -> trial -> confirm -> bad update -> "
          "3 watchdog-reset trials -> autonomous rollback", flush=True)


if __name__ == "__main__":
    main()
