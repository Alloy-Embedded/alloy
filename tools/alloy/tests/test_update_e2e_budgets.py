"""The wall-clock budgets in scripts/update_e2e.py, driven against a device double.

WHY THIS EXISTS. `bootloader-e2e` was red for five days at one assertion — the
wait after a refused downgrade — with `last output: b''`, and the silence read
as a trap. It was not: that phase is DEFINED to emit nothing until the
bootloader's update window expires, and its window is the TRAFFIC-EXTENDED one
(3000 ms) while every other boot phase in the script waits out the quiet one
(500 ms). It cost 6x its neighbours and was handed their budget. Nothing in the
firmware changed; the runner got ~1.5x slower and the one phase with 1.34x of
headroom went over. See the budget comments in scripts/update_e2e.py.

WHAT IT TESTS AND WHAT IT DOES NOT. This drives the REAL harness — main(),
wait_for(), the real needle lists, the real budgets — against a DOUBLE of the
device: the same output strings, on a virtual clock, with per-phase costs that
are a RECORDING of the last green run of this job (2026-08-09, job
93191628703). The recording is scaled by a `slowdown` factor so a runner can be
simulated in milliseconds. So this test owns the BUDGETS and the phase
ORDERING. It cannot own the strings, the wire protocol or the firmware — the
emulation leg (ci.yml, "full-wire update lifecycle") owns those, and a double
that emitted the wrong banner would pass here and fail there.

THE WITNESS. `test_lifecycle_survives_the_runner_that_took_it_red` fails with
the exact CI message if step 3b is put back on PHASE_TIMEOUT_S.
"""

from __future__ import annotations

import importlib.util
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parents[3]
HARNESS_PY = REPO / "scripts" / "update_e2e.py"
BOOTLOADER_MAIN = REPO / "examples" / "bootloader_uart" / "src" / "main.cpp"


def _load_harness():
    """Import scripts/update_e2e.py — it is a script, not a package member."""
    spec = importlib.util.spec_from_file_location("update_e2e_under_test", HARNESS_PY)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


harness = _load_harness()


# --- the recording -----------------------------------------------------------
#
# Every number below was read off the last GREEN run of `bootloader-e2e`
# (2026-08-09, job 93191628703), from the harness's own `ok: <stage> [Ns of Bs]`
# lines. They are wall-clock seconds on that runner, and `slowdown` scales them.
#
#   trial boot + confirm ................  20 s   } a reset that waits out the
#   confirm persisted across cycle ......  20 s   } 500 ms QUIET window
#   trial 1/3 ...........................  20 s   }
#   still running ... after the downgrade  112 s  <- the 3000 ms TRAFFIC window
#   trial 2/3, trial 3/3, rollback ...... 168/164/166 s  <- IWDG (2 s virtual)
#
# The 1.55 factor is the same runs' arithmetic: the 20-22 s phases cost 30-34 s
# on 2026-08-15 (jobs 93420051016, 95022651597).
BOOT_S = 20.0        # reset -> boot, quiet window
REFUSED_BOOT_S = 112.0   # refusal -> boot, traffic-extended window
WATCHDOG_BOOT_S = 168.0  # trial jump -> app hangs -> IWDG reset -> boot
UPDATE_S = 10.0      # pushing an image over the wire (outside any wait_for)

GREEN_RUNNER = 1.0   # 2026-08-09, by construction
RED_RUNNER = 1.55    # 2026-08-15: 34/22 and 32/20 on the unchanged 500 ms phases

POLL_QUANTUM_S = 0.25  # one harness read() of the socket, in virtual seconds


class VirtualClock:
    """Wall time that only advances when the harness polls or sleeps."""

    def __init__(self) -> None:
        self.t = 1_000.0

    def monotonic(self) -> float:
        return self.t

    def sleep(self, seconds: float) -> None:
        self.t += seconds


@dataclass
class Image:
    version: int
    app: str  # "ota_app" (confirms itself) | "uart_echo" (never confirms)

    def encode(self) -> bytes:
        return f"IMG v={self.version} app={self.app}".encode()

    @staticmethod
    def decode(blob: bytes) -> Image:
        m = re.fullmatch(rb"IMG v=(\d+) app=(\w+)", blob)
        assert m, f"not one of this test's images: {blob!r}"
        return Image(int(m.group(1)), m.group(2).decode())


@dataclass
class DeviceDouble:
    """The bootloader and its two apps, as the UART sees them.

    Event-driven on the virtual clock: nothing happens until the harness reads
    or sleeps, and every boot lands `cost * slowdown` seconds after whatever
    triggered it. The state machine is the real one in miniature — target slot
    alternates, the anti-rollback floor is max(running, target slot), a trial of
    a non-confirming app is reset by the watchdog, three of those exhaust the
    attempts and the device reverts.
    """

    clock: VirtualClock
    slowdown: float = 1.0
    brick_after_refusal: bool = False

    slot_version: dict[int, int] = field(default_factory=lambda: {0: 0, 1: 0})
    slot_app: dict[int, str] = field(default_factory=dict)
    running_slot: int | None = None
    running_version: int = 0
    confirmed_slot: int | None = None
    pending_trial: int | None = None
    trials_left: int = 0
    refused_once: bool = False
    window_open: bool = True  # blank board: waiting for an update, forever
    events: list = field(default_factory=list)  # (due, callable) -> bytes

    # --- scheduling ---
    def _at(self, delay: float, fn) -> None:
        self.events.append((self.clock.t + delay * self.slowdown, fn))
        self.events.sort(key=lambda e: e[0])

    def pump(self) -> bytes:
        out = b""
        while self.events and self.events[0][0] <= self.clock.t:
            _, fn = self.events.pop(0)
            out += fn()
        return out

    # --- what the harness does to it ---
    def install(self, blob: bytes) -> dict:
        """`alloy update` over the wire. Raises exactly like the real client."""
        assert self.window_open, "the device had already left its update window"
        # EVERY received byte re-arms the window, so whatever boot was scheduled
        # for the end of the 500 ms quiet one does not happen (main.cpp: the
        # deadline is pushed to now + 3000 ms on each byte).
        self.events.clear()
        self.clock.sleep(UPDATE_S * self.slowdown)
        target = 0 if self.running_slot == 1 else 1
        floor = max(self.running_version, self.slot_version[target])
        if Image.decode(blob).version <= floor:
            # NAKed. The last frame re-armed the bootloader's window to 3000 ms,
            # and nobody speaks again — this is the phase the whole bug is about.
            self.refused_once = True
            self._at(REFUSED_BOOT_S, self._boot)
            raise harness.UpdateError(
                "device rejected update (ota_error 11: rollback — the device already "
                "holds a version at or above this image's, and refuses the downgrade)")
        img = Image.decode(blob)
        self.slot_version[target] = img.version
        self.slot_app[target] = img.app
        self.pending_trial = target
        self.trials_left = 3
        info = {"target_slot": target, "running_version": self.running_version}
        # rx.finished() -> print and SYSRESETREQ on the next instruction: this
        # boot pays NO window at all, which is why it costs the same 20 s as a
        # plain power cycle rather than the 112 s of a refusal.
        self._at(0.0, lambda: b"update ok, rebooting\r\n")
        self._at(BOOT_S, self._boot)
        return info

    def power_cycle(self) -> None:
        self.events.clear()
        self.window_open = True
        self._at(BOOT_S, self._boot)

    # --- what it does by itself ---
    def _boot(self) -> bytes:
        self.window_open = False  # the window expired; we are in the app now
        if self.brick_after_refusal and self.refused_once:
            return b""  # a device that really is stuck: the hypothesis this refutes
        out = b"alloy bootloader\r\n"
        if self.pending_trial is not None and self.trials_left > 0:
            slot = self.pending_trial
            self.trials_left -= 1
            out += b"trial boot slot A\r\n" if slot == 0 else b"trial boot slot B\r\n"
            app = self.slot_app[slot]
            out += f"alloy {app} ready\r\n".encode()
            if app == "ota_app":
                out += b"ota_app confirmed\r\n"
                self.pending_trial = None
                self.confirmed_slot = slot
                self.running_slot = slot
                self.running_version = self.slot_version[slot]
            else:
                # never confirms, never feeds: the watchdog the bootloader armed
                # before the jump resets it, with nobody pressing anything.
                self._at(WATCHDOG_BOOT_S, self._boot)
            return out
        if self.pending_trial is not None:  # attempts exhausted -> rollback
            self.pending_trial = None
            slot = self.confirmed_slot
            assert slot is not None
            self.running_slot = slot
            out += b"reverted, boot slot A\r\n" if slot == 0 else b"reverted, boot slot B\r\n"
            out += f"alloy {self.slot_app[slot]} ready\r\n".encode()
            return out
        slot = self.running_slot
        assert slot is not None, "the harness booted a device with no valid image"
        out += b"boot slot A\r\n" if slot == 0 else b"boot slot B\r\n"
        out += f"alloy {self.slot_app[slot]} ready\r\n".encode()
        return out


class FakeLink:
    """serial.Serial, as `wait_for` uses it: one read = one poll = one quantum."""

    def __init__(self, device: DeviceDouble, clock: VirtualClock) -> None:
        self.device = device
        self.clock = clock

    def read(self, _n: int) -> bytes:
        self.clock.sleep(POLL_QUANTUM_S)
        return self.device.pump()


class FakeMonitorSocket:
    """Renode's monitor socket, as Monitor drives it."""

    def __init__(self, device: DeviceDouble) -> None:
        self.device = device

    def settimeout(self, _s: float) -> None:
        pass

    def recv(self, _n: int) -> bytes:
        raise TimeoutError

    def sendall(self, payload: bytes) -> None:
        if b"machine Reset" in payload:
            self.device.power_cycle()


def run_lifecycle(monkeypatch, tmp_path: Path, slowdown: float, *,
                  brick_after_refusal: bool = False) -> DeviceDouble:
    """Drive the REAL scripts/update_e2e.py main() against the double."""
    clock = VirtualClock()
    device = DeviceDouble(clock, slowdown, brick_after_refusal)

    good = tmp_path / "good.img"
    bad = tmp_path / "bad.img"
    old = tmp_path / "old.img"
    good.write_bytes(Image(1, "ota_app").encode())
    bad.write_bytes(Image(2, "uart_echo").encode())
    old.write_bytes(Image(0, "ota_app").encode())  # the replay: same firmware, v0

    monkeypatch.setattr(harness, "time", clock)
    monkeypatch.setattr(harness, "serial",
                        type("S", (), {"serial_for_url": staticmethod(
                            lambda url, timeout: FakeLink(device, clock))}))
    monkeypatch.setattr(harness, "socket",
                        type("K", (), {"create_connection": staticmethod(
                            lambda addr, timeout: FakeMonitorSocket(device))}))
    monkeypatch.setattr(harness, "update",
                        lambda link, blob, retries=1: device.install(blob))
    monkeypatch.setattr(sys, "argv", ["update_e2e.py", str(good), str(bad),
                                      "3456", "12349", f"--replay={old}"])
    harness.main()
    return device


def phase_costs(captured: str) -> dict[str, tuple[float, float]]:
    """The harness's own `ok: <stage> [Ns of Bs]` lines -> {stage: (took, budget)}."""
    out = {}
    for line in captured.splitlines():
        m = re.match(r"\s*ok: (.+) \[(\d+)s of (\d+)s\]$", line)
        if m:
            out[m.group(1)] = (float(m.group(2)), float(m.group(3)))
    return out


# --- the tests ---------------------------------------------------------------

def test_lifecycle_survives_the_runner_that_took_it_red(monkeypatch, tmp_path, capsys):
    """THE WITNESS. At the measured 2026-08-15 runner speed the whole lifecycle
    must fit its budgets. It does not if the wait after a refused downgrade is
    budgeted as an ordinary boot phase: 112 s x 1.55 = 174 s > PHASE_TIMEOUT_S,
    and the failure is the CI one, byte for byte —

        FAIL [still running its own firmware after refusing the downgrade]:
             never saw b'boot slot B' in 150s (budget 150s); last output: b''
    """
    run_lifecycle(monkeypatch, tmp_path, RED_RUNNER)
    out = capsys.readouterr().out
    assert "PASS: install -> trial -> confirm -> DOWNGRADE REFUSED" in out
    costs = phase_costs(out)
    refusal = "still running its own firmware after refusing the downgrade"
    assert refusal in costs, out
    took, budget = costs[refusal]
    assert took == pytest.approx(REFUSED_BOOT_S * RED_RUNNER, abs=2)
    assert budget == harness.EMULATION_BOUND_TIMEOUT_S, (
        "the wait after a refused downgrade is emulation-bound like the watchdog "
        "legs, not firmware-bound like an ordinary boot")


def test_every_phase_keeps_half_its_budget_in_reserve(monkeypatch, tmp_path, capsys):
    """The guard that would have caught this while the job was still GREEN.

    On 2026-08-09 the refusal phase already ate 112 s of 150 s — 75% — while its
    neighbours ate 13%. Nothing reported that, so the first news was a red job
    five days later. A phase over half its budget is a phase about to fail.
    """
    run_lifecycle(monkeypatch, tmp_path, RED_RUNNER)
    costs = phase_costs(capsys.readouterr().out)
    assert costs, "the harness printed no phase timings at all"
    hot = {k: (t, b) for k, (t, b) in costs.items() if t > 0.5 * b}
    assert not hot, f"phases with under 2x headroom on the slowest measured runner: {hot}"


def test_the_double_reproduces_the_last_green_run(monkeypatch, tmp_path, capsys):
    """Calibration: at slowdown 1.0 the double must reproduce job 93191628703's
    own numbers. If it does not, the red at 1.55 says nothing about CI."""
    run_lifecycle(monkeypatch, tmp_path, GREEN_RUNNER)
    costs = phase_costs(capsys.readouterr().out)
    recorded = {
        "trial boot + confirm": 20,
        "confirm persisted across power cycle": 20,
        "still running its own firmware after refusing the downgrade": 112,
        "trial 1/3": 20,
        "trial 2/3 (watchdog self-reset, no power button)": 168,
        "trial 3/3 (watchdog self-reset, no power button)": 168,
        "AUTOMATIC ROLLBACK (autonomous)": 168,
    }
    assert set(costs) == set(recorded)
    for stage, ci_seconds in recorded.items():
        assert costs[stage][0] == pytest.approx(ci_seconds, abs=2), stage


def test_a_device_that_stays_silent_after_the_refusal_still_fails(monkeypatch, tmp_path):
    """Raising the budget must not defang the assertion. A device that really is
    bricked by the refusal — the trap hypothesis this bug was mistaken for —
    still fails, just later."""
    with pytest.raises(SystemExit) as exc:
        run_lifecycle(monkeypatch, tmp_path, GREEN_RUNNER, brick_after_refusal=True)
    msg = str(exc.value)
    assert "still running its own firmware after refusing the downgrade" in msg
    assert f"budget {harness.EMULATION_BOUND_TIMEOUT_S}s" in msg


def test_the_recording_tracks_the_firmware_windows():
    """The double is a recording, and the thing it recorded is the bootloader's
    own update window. Tie the two together so a firmware change to either
    window re-opens the budget question instead of silently invalidating this
    file: cost is linear in the window, so the refusal phase should cost about
    (3000/500) = 6x an ordinary boot phase, and it costs 112/20 = 5.6x."""
    src = BOOTLOADER_MAIN.read_text(encoding="utf-8")
    quiet = int(re.search(
        r"deadline = alloy::uptime_ms\(\) \+ (\d+)u;\s*\n\s*for \(;;\)", src).group(1))
    rearm = int(re.search(
        r"rx\.on_byte\(byte\);\s*\n\s*deadline = alloy::uptime_ms\(\) \+ (\d+)u;",
        src).group(1))
    assert (quiet, rearm) == (500, 3000), (
        f"the bootloader's update window moved to {quiet} ms quiet / {rearm} ms "
        "re-armed; re-measure the phase costs in this file and the budgets in "
        "scripts/update_e2e.py before changing these numbers")
    assert REFUSED_BOOT_S / BOOT_S == pytest.approx(rearm / quiet, rel=0.25)
