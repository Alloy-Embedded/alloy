#!/usr/bin/env python3
"""Mass programming: what a factory runs, per board, in what order.

    python examples/factory/line.py --serials serials.csv --board nucleo_g071rb

THE ORDER IS THE PRODUCT. Every step here is reversible except the last one, and
the last one is reversible only by mass-erasing the chip. Getting the order
wrong does not fail loudly at the bench — it fails after you have built four
hundred boards. So this script enforces it rather than documenting it:

  1. FLASH THE BOOTLOADER          probe writes the bootloader region. The only
                                   step that needs a probe to load code — a blank
                                   board has nothing to take an update with.
  2. LOAD THE LINE-TEST IMAGE      `alloy update` puts examples/factory in slot A
                                   over the same UART the field uses.
  3. PROVISION THE IDENTITY        `alloy provision write` — serial, MAC, hw rev,
                                   batch. Verified by readback inside that verb.
  4. RUN THE LINE TEST             power-cycle, read the debug UART, require
                                   `LINE TEST: PASS`. This is the step that
                                   proves the DEVICE agrees with the WORK ORDER:
                                   the firmware found the identity, parsed it and
                                   printed it back.
  5. LOAD THE PRODUCT FIRMWARE     over the FIELD path (`alloy update`, the UART
                                   bootloader), not the probe — so every board
                                   that ships has had its update path exercised
                                   once, on the line, where a failure is cheap.
  6. LOCK THE PART                 `alloy secure apply --rdp 1 --wrp-bootloader`.
                                   DEAD LAST, and here is why:
                                     * RDP level 1 blocks the debug probe from
                                       flash, so steps 1-3 become impossible;
                                     * on uniform-page flash the identity page is
                                       the LAST PAGE OF THE BOOTLOADER REGION, so
                                       --wrp-bootloader freezes it. Provision
                                       before you secure or the serial number
                                       silently never lands (`alloy provision
                                       write`'s readback is what catches it).
                                   Returning to RDP 0 to fix a mistake MASS-ERASES
                                   the board — bootloader, both slots, identity.

SERIAL NUMBERS ARE CONSUMED, NEVER REUSED. A ledger file next to the serial list
records every serial that has been written to a board. Handing the same serial
to two devices is the one production mistake with no field workaround at all, so
a re-run after a crash resumes at the next unused serial instead of starting
over. `--dry-run` prints every command and consumes nothing.

HONESTY — READ THIS BEFORE YOU TRUST IT. No board was on hand: this script has
never driven a probe. What IS tested (tools/alloy/tests/test_factory_line.py) is
everything that is pure — the ledger's refusals, the work-order parser, the step
ORDER, and the exact argv of every command. What is NOT tested is that those
commands do what openocd's documentation says they do on real silicon. Treat the
first board through this line as the experiment it is.
"""

from __future__ import annotations

import argparse
import csv
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

#: The steps, in the only order that works. Named so an operator resuming a
#: half-finished board can say where they were, and so the tests can assert the
#: order without re-deriving it.
STEP_ORDER = (
    "flash-bootloader",
    "load-line-test",
    "provision",
    "line-test",
    "load-product",
    "secure",
)

#: The line test's verdict lines (examples/factory/src/main.cpp prints exactly
#: one of them). Matched as substrings of a UART transcript.
PASS_MARK = "LINE TEST: PASS"
FAIL_MARK = "LINE TEST: FAIL"


class LineError(RuntimeError):
    """A refusal. Always names the board and the step."""


@dataclass(frozen=True)
class WorkOrderRow:
    """One board's worth of the work order."""

    serial: str
    mac: str | None = None
    hw_rev: int = 0
    batch: int = 0


def read_work_order(path: Path) -> list[WorkOrderRow]:
    """Parse the work order (CSV with a `serial` column; `mac`, `hw_rev` and
    `batch` optional).

    Refuses a DUPLICATE SERIAL inside the file itself. That check is worth more
    than it looks: the usual way a factory gets duplicates is a spreadsheet
    copy-paste, and the duplicate is discovered when two devices report the same
    identity to a fleet server, months later, from different continents."""
    rows: list[WorkOrderRow] = []
    seen: dict[str, int] = {}
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None or "serial" not in reader.fieldnames:
            raise LineError(f"{path}: needs a CSV header with at least a "
                            f"`serial` column (got {reader.fieldnames})")
        for lineno, raw in enumerate(reader, start=2):
            serial = (raw.get("serial") or "").strip()
            if not serial:
                raise LineError(f"{path}:{lineno}: empty serial")
            if serial in seen:
                raise LineError(
                    f"{path}:{lineno}: serial {serial!r} already appears on line "
                    f"{seen[serial]}. Two boards with one serial is the "
                    f"production mistake with no field workaround — fix the work "
                    f"order before running the line")
            seen[serial] = lineno
            mac = (raw.get("mac") or "").strip() or None
            rows.append(WorkOrderRow(
                serial=serial, mac=mac,
                hw_rev=int(raw.get("hw_rev") or 0),
                batch=int(raw.get("batch") or 0)))
    if not rows:
        raise LineError(f"{path}: no rows")
    return rows


class Ledger:
    """Which serials have already been burned into a board.

    Append-only, flushed after EVERY board, because the failure this exists for
    is the line PC losing power mid-batch. A ledger that is only written at the
    end is a ledger that tells you nothing on the day you need it."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.consumed: list[str] = []
        if path.exists():
            self.consumed = [ln.strip() for ln in path.read_text().splitlines()
                             if ln.strip()]

    def next_row(self, rows: list[WorkOrderRow]) -> WorkOrderRow:
        done = set(self.consumed)
        for row in rows:
            if row.serial not in done:
                return row
        raise LineError(
            f"every serial in the work order has been consumed "
            f"({len(self.consumed)} boards). Issue a new work order — this "
            f"script will not reuse a serial")

    def consume(self, serial: str) -> None:
        if serial in set(self.consumed):
            raise LineError(f"serial {serial!r} is already on a board "
                            f"(ledger {self.path})")
        self.consumed.append(serial)
        with self.path.open("a") as handle:
            handle.write(serial + "\n")
            handle.flush()


# ── the commands, as data ───────────────────────────────────────────────────
#
# Every step is a list of argv lists. Built as DATA so --dry-run prints exactly
# what would run and the tests assert on argv rather than on side effects.


def _alloy(*args: str) -> list[str]:
    return ["uv", "run", "--project", "tools/alloy", "alloy", *args]


def step_commands(step: str, *, board: str, row: WorkOrderRow,
                  bootloader_project: Path, linetest_project: Path,
                  product_image: Path, linetest_image: Path,
                  port: str) -> list[list[str]]:
    """The argv of one step. Unknown steps raise — there is no default."""
    if step == "flash-bootloader":
        # The only step that MUST use the probe: a blank board has nothing to
        # take an update with.
        return [_alloy("flash", "--slot", "bl", "--board", board,
                       "--project", str(bootloader_project))]
    if step == "load-line-test":
        # Over the FIELD path, not the probe. The bootloader is already on the
        # board and sitting in its update window, and using `alloy update` here
        # means every unit's update path is exercised before it ships.
        #
        # Pack the line-test image at image_version 0 (`alloy image --set-version
        # 0`): the anti-rollback floor is raised by whatever is already in the
        # slot, so a line-test image with a HIGH version would refuse the product
        # firmware that follows it at step 5.
        return [_alloy("update", str(linetest_image), "--port", port)]
    if step == "provision":
        cmd = _alloy("provision", "write", "--serial", row.serial,
                     "--board", board, "--project", str(linetest_project))
        if row.mac:
            cmd += ["--mac", row.mac]
        if row.hw_rev:
            cmd += ["--hw-rev", str(row.hw_rev)]
        if row.batch:
            cmd += ["--batch", str(row.batch)]
        return [cmd]
    if step == "line-test":
        return []  # not a subprocess: read the UART (see run_line_test)
    if step == "load-product":
        return [_alloy("update", str(product_image), "--port", port)]
    if step == "secure":
        return [_alloy("secure", "apply", "--rdp", "1", "--wrp-bootloader",
                       "--yes", "--board", board,
                       "--project", str(linetest_project))]
    raise LineError(f"unknown step {step!r} — the order is {STEP_ORDER}")


def run_line_test(port: str, baud: int, timeout: float) -> str:
    """Power-cycle happens by hand (or by the fixture); read the UART until the
    firmware states its verdict. Returns the transcript; raises on FAIL or on
    silence. Silence is a FAILURE, never a pass — a board that says nothing has
    not been tested, it has been ignored."""
    import serial  # noqa: PLC0415  (only the real line needs pyserial)

    transcript = ""
    with serial.Serial(port, baud, timeout=0.2) as handle:
        import time  # noqa: PLC0415

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            transcript += handle.read(256).decode("ascii", "replace")
            if PASS_MARK in transcript or FAIL_MARK in transcript:
                break
    if FAIL_MARK in transcript:
        line = next(ln for ln in transcript.splitlines() if FAIL_MARK in ln)
        raise LineError(f"line test FAILED: {line.strip()}")
    if PASS_MARK not in transcript:
        raise LineError(
            f"no verdict from the board in {timeout:.0f} s on {port}. It is not "
            f"a pass: reset the board with the UART connected, and check that "
            f"the line-test image is the one in slot A")
    return transcript


def program_one(row: WorkOrderRow, args: argparse.Namespace) -> None:
    """One board, all six steps, in order, stopping at the first failure.

    Stopping matters: a board that failed at step 3 must NOT reach step 6, or an
    unprovisioned board gets locked at RDP 1 and becomes scrap."""
    for step in STEP_ORDER:
        print(f"  [{step}]")
        if step == "line-test":
            if args.dry_run:
                print(f"    (would read {args.port} for `{PASS_MARK}`)")
                continue
            input("    power-cycle the board, then press Enter…")
            print("    " + run_line_test(args.port, args.baud,
                                         args.line_test_timeout).strip()
                  .replace("\n", "\n    "))
            continue
        if step == "secure" and args.no_secure:
            print("    skipped (--no-secure): this board is NOT production-locked")
            continue
        for argv in step_commands(
                step, board=args.board, row=row,
                bootloader_project=Path(args.bootloader_project),
                linetest_project=Path(args.linetest_project),
                product_image=Path(args.product_image),
                linetest_image=Path(args.linetest_image),
                port=args.port):
            print("    $ " + " ".join(argv))
            if args.dry_run:
                continue
            result = subprocess.run(argv, cwd=args.repo_root, check=False)
            if result.returncode != 0:
                raise LineError(
                    f"step `{step}` failed for serial {row.serial!r} (exit "
                    f"{result.returncode}). This board is NOT finished and must "
                    f"not be locked — set it aside")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--serials", required=True, type=Path,
                        help="work-order CSV (serial[,mac,hw_rev,batch])")
    parser.add_argument("--ledger", type=Path,
                        help="consumed-serial ledger (default: <serials>.done)")
    parser.add_argument("--board", required=True)
    parser.add_argument("--port", default="/dev/ttyACM0",
                        help="debug UART for the line test and `alloy update`")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--line-test-timeout", type=float, default=15.0)
    parser.add_argument("--bootloader-project", default="examples/bootloader_uart")
    parser.add_argument("--linetest-project", default="examples/factory")
    parser.add_argument("--linetest-image", default="/tmp/linetest.img")
    parser.add_argument("--product-image", required=True,
                        help="the shipping firmware, packed by `alloy image`")
    parser.add_argument("--count", type=int, default=1,
                        help="how many boards to program in this run")
    parser.add_argument("--no-secure", action="store_true",
                        help="skip the FINAL locking step — for engineering "
                             "samples that must stay debuggable. Such boards are "
                             "not production units")
    parser.add_argument("--dry-run", action="store_true",
                        help="print every command, consume no serials, touch "
                             "no board")
    parser.add_argument("--repo-root", default=str(repo_root))
    args = parser.parse_args()

    rows = read_work_order(args.serials)
    ledger = Ledger(args.ledger or args.serials.with_suffix(".done"))
    for n in range(args.count):
        row = ledger.next_row(rows)
        print(f"\n=== board {n + 1}/{args.count}: {row.serial} ===")
        program_one(row, args)
        if args.dry_run:
            print("  (dry run — serial NOT consumed)")
        else:
            ledger.consume(row.serial)
            print(f"  done: {row.serial}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except LineError as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(130)
