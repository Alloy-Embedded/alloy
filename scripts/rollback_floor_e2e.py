#!/usr/bin/env python3
"""Does the anti-rollback floor SURVIVE an automatic rollback?

Written by the adversarial review of task #47, which found that the one line
keeping anti-rollback from being defeated had no test at all. Landed so it
cannot regress silently.

The bootloader raises the floor at HELLO by verifying the slot about to be
overwritten (examples/bootloader_uart/src/main.cpp arming_sink::begin). That is
the ONLY thing that keeps the floor from dropping when a failed trial moves the
device back to an older confirmed slot: running_version alone would say 1 while
the other slot still holds 5, and an attacker replaying v3 would be accepted.

  1. install v1 (ota_app, slot B), trial, CONFIRM        -> running = 1
  2. install v5 (uart_echo, slot A) and let it never confirm
  3. three watchdog trials -> AUTOMATIC ROLLBACK to slot B (v1)
     running_version is now 1 again, but slot A still holds v5
  4. push v3. floor = max(running 1, target-slot 5) = 5, so v3 MUST be refused.
     Without the target-slot raise the floor is 1 and v3 is accepted.
"""
import sys, socket, time
from pathlib import Path
sys.path.insert(0, "tools/alloy")
import serial
from alloy_cli.ota_host import update, UpdateError

good, bad5, mid3 = (Path(a).read_bytes() for a in sys.argv[1:4])
link = serial.serial_for_url("socket://localhost:3456", timeout=2)
s = socket.create_connection(("localhost", 12349), timeout=5); s.settimeout(0.3)
time.sleep(0.3)
def drain():
    try:
        while s.recv(4096): pass
    except Exception: pass
drain(); s.sendall(b'mach set "upd"\r\n'); time.sleep(0.4); drain()
def cycle():
    s.sendall(b"machine Reset\r\n"); time.sleep(0.6); drain()
    s.sendall(b"start\r\n"); time.sleep(0.6); drain()
def wait(needles, budget, stage):
    got=b""; end=time.monotonic()+budget; want=list(needles)
    while want and time.monotonic()<end:
        c=link.read(256)
        if c:
            got+=c
            while want and want[0] in got:
                got=got.split(want[0],1)[1]; want.pop(0)
    if want: raise SystemExit(f"FAIL [{stage}]: never saw {want[0]!r}; tail={got[-300:]!r}")
    print(f"  ok: {stage}", flush=True)

info = update(link, good, retries=10); print("  ok: v1 -> slot B", info, flush=True)
wait([b"trial boot slot B", b"ota_app confirmed"], 150, "v1 confirmed")
cycle()
info = update(link, bad5, retries=10); print("  ok: v5 -> slot A", info, flush=True)
wait([b"trial boot slot A"], 150, "v5 trial 1/3")
wait([b"trial boot slot A"], 600, "v5 trial 2/3")
wait([b"trial boot slot A"], 600, "v5 trial 3/3")
wait([b"reverted, boot slot B", b"alloy ota_app ready"], 600, "AUTOMATIC ROLLBACK to v1")
cycle()
try:
    info = update(link, mid3, retries=10)
    print(f"  FAIL: v3 ACCEPTED after the rollback ({info}) — the floor DROPPED "
          f"back to the running version; slot A's v5 was forgotten", flush=True)
    raise SystemExit(1)
except UpdateError as e:
    if "ota_error 11" not in str(e):
        raise SystemExit(f"FAIL: refused, but not as a rollback: {e}")
    print(f"  ok: v3 REFUSED -> {e}", flush=True)
print("PASS: the floor survived an automatic rollback", flush=True)
