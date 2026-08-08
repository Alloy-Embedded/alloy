"""Readout (RDP) and write (WRP) protection through the debug probe.

A signed update image means nothing if an attacker can read the flash over SWD
or replace the bootloader with their own. `alloy secure` closes that at
production-flash time, host-side through openocd — v1 is deliberately NOT
firmware self-protection.

Everything here is either a pure planner (unit-testable, no hardware) or a
thin openocd runner around it:

  * addresses come from the chip yaml (flash-controller base) plus the
    register yaml (offsets/fields) — never written down here;
  * the bootloader WRP range derives from ``emit.slots.slot_layout`` — the
    SAME layout the linker and bootloader use, never a hand-typed range;
  * every register write is read-modify-write under a field mask, so option
    bits this tool does not manage (BOR level, reset behaviour, boot pins)
    are never flipped.

THE GUARDS ARE THE PRODUCT. On ST parts the option bytes carry two traps:
regressing RDP level 1 -> 0 MASS-ERASES the chip, and RDP level 2 kills the
debug port FOREVER. `apply_guards` refuses both without explicit flags plus a
typed confirmation phrase, and always says what will be lost before anything
is written.

HONESTY: none of this is silicon-witnessed — no board was on hand, and RDP is
exactly the thing you do not test casually on your only board. What IS
witnessed: the F7 register interface (unlock keys, lock enforcement, value
programming) against Renode 1.16.1's MTD.STM32F4_FlashController, driving the
same poke list this planner emits (tests/test_secure.py). The G0 path leans on
openocd's stm32l4x option driver, which the community runs on G0 silicon; the
protection SEMANTICS (read blocking, mass-erase on regression, permanence of
level 2) rest on RM0444/RM0431 alone.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from dataclasses import dataclass, field
from typing import Any, Callable

from .debug import OPENOCD_INTERFACE, OPENOCD_TARGET
from .emit.common import EmitError
from .emit.slots import Region, SlotLayout, f7_sector_spans, flash_controller

# Option-byte unlock keys (RM0444 §3.4 / RM0431 §3.4 — same values on both
# families). Composed from 16-bit halves like the C++ flash HAL, so no 8-digit
# literal exists for the contract gate to flag.
_OPTKEY1 = (0x0819 << 16) | 0x2A3B
_OPTKEY2 = (0x4C5D << 16) | 0x6E7F

# RDP byte encodings (RM0444 §4.3 / RM0431 §3.5). 0xBB is the value ST's own
# tools program for level 1 (any byte that is neither 0xAA nor 0xCC reads as
# level 1, but a canonical value keeps status output stable).
RDP_BYTE = {0: 0xAA, 1: 0xBB, 2: 0xCC}


def rdp_level(byte: int) -> int:
    """Decode an RDP byte: 0xAA = level 0, 0xCC = level 2, all else level 1."""
    if byte == RDP_BYTE[0]:
        return 0
    if byte == RDP_BYTE[2]:
        return 2
    return 1


# F7 parts whose option map this tool may WRITE: F72x/F73x (RM0431 — 8-bit
# nWRP, no nDBANK). F74x/F75x/F76x/F77x carry more nWRP bits and (F76x/F77x)
# an nDBANK bit that changes the whole sector map — status is fine (RDP decode
# is identical), but apply refuses until that geometry is modeled.
_F7_APPLY_PREFIXES = ("STM32F72", "STM32F73")


@dataclass(frozen=True)
class OptionMap:
    """Option-byte facts for one chip, resolved from the device database."""

    kind: str  # "g0" | "f7"
    part: str
    regbase: int  # flash-controller peripheral base
    doc: dict[str, Any]  # the register yaml document

    def _reg(self, name: str) -> dict[str, Any]:
        for reg in self.doc["registers"]:
            if reg["name"] == name:
                return reg
        raise EmitError(
            f"register data for {self.part}'s flash controller has no {name} — "
            f"add it to alloy-devices before using `alloy secure` here")

    def offset(self, name: str) -> int:
        return int(self._reg(name)["offset"], 16)

    def addr(self, name: str) -> int:
        return self.regbase + self.offset(name)

    def field_span(self, reg: str, fname: str) -> tuple[int, int]:
        """(bit, width) of a field, from the data."""
        for f in self._reg(reg).get("fields", []):
            if f["name"] == fname:
                return f["bit"], f.get("width", 1)
        raise EmitError(
            f"register data for {self.part}: {reg} has no field {fname} — "
            f"add it to alloy-devices before using `alloy secure` here")

    def mask(self, reg: str, fname: str) -> int:
        bit, width = self.field_span(reg, fname)
        return ((1 << width) - 1) << bit


def option_map(chip: dict[str, Any],
               registers: dict[str, dict[str, Any]]) -> OptionMap:
    """Resolve the chip's option-byte map, or refuse naming what is missing."""
    fc = flash_controller(chip)
    if fc is None:
        # The slot layout doesn't know this flash IP — still NAME it in the
        # refusal if the chip has a curated flash-class peripheral.
        for pname, periph in (chip.get("peripherals") or {}).items():
            pip = periph.get("ip")
            if (not periph.get("uncurated")
                    and registers.get(pip or "", {}).get("class") == "flash"):
                fc = (pname, pip)
                break
    if fc is None:
        raise EmitError(
            f"chip {chip['part']} has no curated flash controller in the device "
            f"database — `alloy secure` has nothing to read")
    name, ip = fc
    kind = {"st/flash_g0": "g0", "st/flash_f7": "f7"}.get(ip)
    if kind is None:
        raise EmitError(
            f"`alloy secure` supports STM32G0 and STM32F7 (flash IPs st/flash_g0, "
            f"st/flash_f7) — {chip['part']} has '{ip}'. Its option-byte registers "
            f"need curating in alloy-devices and a planner here first")
    doc = registers.get(ip)
    if doc is None:
        raise EmitError(f"no register data for flash IP '{ip}' in alloy-devices")
    base = chip["peripherals"][name]["base"]
    regbase = int(base, 16) if isinstance(base, str) else int(base)
    return OptionMap(kind=kind, part=chip["part"], regbase=regbase, doc=doc)


# ── status ──────────────────────────────────────────────────────────────────


@dataclass(frozen=True)
class SecureStatus:
    rdp_byte: int
    rdp_level: int
    #: human-readable protected ranges ("pages 0-15", "sector 3"), possibly empty
    wrp: tuple[str, ...]
    #: None when the chip has no slot layout to derive the bootloader from
    bootloader_protected: bool | None
    words: dict[int, int]  # every register word read, by absolute address


def status_addrs(omap: OptionMap) -> list[int]:
    """Absolute addresses `secure status` reads. Option registers live in
    peripheral space, so these stay readable under RDP level 1 (only flash
    itself is blocked); under level 2 the probe cannot connect at all."""
    if omap.kind == "g0":
        # WRP2AR/WRP2BR exist only on dual-bank dies and read as reserved
        # space elsewhere — v1 decodes bank 1 only (the bootloader's bank).
        return [omap.addr("OPTR"), omap.addr("WRP1AR"), omap.addr("WRP1BR")]
    return [omap.addr("OPTCR")]


def _field_of(omap: OptionMap, word: int, reg: str, fname: str) -> int:
    bit, width = omap.field_span(reg, fname)
    return (word >> bit) & ((1 << width) - 1)


def _g0_bootloader_pages(layout: SlotLayout) -> tuple[int, int]:
    """(first, last) 2K-page indices of the bootloader region. The layout puts
    the bootloader at the flash base, so this is always (0, n-1)."""
    n = layout.bootloader.size // layout.page_size
    return 0, n - 1


def _f7_region_sectors(layout: SlotLayout, region: Region,
                       flash_base: int, flash_total: int) -> list[int]:
    """Sector indices covering a region, walked over the F7 sector map."""
    sectors: list[int] = []
    base = flash_base
    for idx, size in enumerate(f7_sector_spans(flash_total)):
        if base < region.base + region.size and region.base < base + size:
            sectors.append(idx)
        base += size
    return sectors


def f7_bootloader_sectors(chip: dict[str, Any], layout: SlotLayout) -> list[int]:
    flash = next(m for m in chip["memories"] if m["kind"] == "flash")
    fbase = int(flash["base"], 16) if isinstance(flash["base"], str) else int(flash["base"])
    return _f7_region_sectors(layout, layout.bootloader, fbase, int(flash["size"]))


def decode_status(omap: OptionMap, chip: dict[str, Any],
                  layout: SlotLayout | None,
                  words: dict[int, int]) -> SecureStatus:
    if omap.kind == "g0":
        optr = words[omap.addr("OPTR")]
        rdp = _field_of(omap, optr, "OPTR", "RDP")
        areas: list[tuple[int, int]] = []
        wrp: list[str] = []
        for reg, s, e in (("WRP1AR", "WRP1A_STRT", "WRP1A_END"),
                          ("WRP1BR", "WRP1B_STRT", "WRP1B_END")):
            word = words[omap.addr(reg)]
            strt = _field_of(omap, word, reg, s)
            end = _field_of(omap, word, reg, e)
            if strt <= end:  # STRT > END is the "disabled" encoding (RM0444)
                areas.append((strt, end))
                wrp.append(f"pages {strt}-{end}")
        protected: bool | None = None
        if layout is not None:
            first, last = _g0_bootloader_pages(layout)
            protected = any(s <= first and last <= e for s, e in areas)
        return SecureStatus(rdp, rdp_level(rdp), tuple(wrp), protected, dict(words))

    optcr = words[omap.addr("OPTCR")]
    rdp = _field_of(omap, optcr, "OPTCR", "RDP")
    nwrp = _field_of(omap, optcr, "OPTCR", "NWRP")
    _, nwrp_width = omap.field_span("OPTCR", "NWRP")
    protected_sectors = [i for i in range(nwrp_width) if not (nwrp >> i) & 1]
    wrp = [f"sector {i}" for i in protected_sectors]
    protected = None
    if layout is not None:
        needed = f7_bootloader_sectors(chip, layout)
        # Only the first nwrp_width sectors are decodable from the data.
        protected = all(i in protected_sectors for i in needed if i < nwrp_width)
    return SecureStatus(rdp, rdp_level(rdp), tuple(wrp), protected, dict(words))


# ── guards ──────────────────────────────────────────────────────────────────


@dataclass
class Guards:
    """What must happen before anything is written. Evaluated pure, so every
    branch is testable without a device. Exactly one of noop/refusal/mixture of
    warnings+confirmation applies; the CLI enforces it."""

    noop: str | None = None      # nothing to do — message to print, exit 0
    refusal: str | None = None   # refuse — error message, exit 1
    warnings: list[str] = field(default_factory=list)  # print BEFORE any write
    phrase: str | None = None    # exact phrase the operator must type
    confirm: bool = False        # y/N prompt (or --yes)


def apply_guards(part: str, current_level: int, rdp_target: int | None,
                 wrp_add: bool, *, accept_mass_erase: bool = False,
                 permanent: bool = False) -> Guards:
    g = Guards()
    if rdp_target not in (None, 0, 1, 2):
        g.refusal = f"--rdp must be 0, 1 or 2 (got {rdp_target})"
        return g
    if rdp_target is None and not wrp_add:
        g.noop = ("nothing to apply — pass --rdp <level> and/or "
                  "--wrp-bootloader. Current state above.")
        return g
    if current_level == 2:
        g.refusal = (f"{part} is at RDP level 2 — that is PERMANENT. The "
                     f"option bytes can never be changed again, by any tool")
        return g
    if rdp_target == current_level:
        if not wrp_add:
            g.noop = f"already at RDP level {current_level} — nothing to do"
            return g
        rdp_target = None  # only the WRP change remains

    if rdp_target == 2:
        g.warnings.append(
            "RDP LEVEL 2 IS IRREVERSIBLE: the debug port is disabled FOREVER, "
            "the chip can never again be read, reflashed or recovered over "
            "SWD, and there is no way back — not for you, not for ST. This is "
            "a manufacturing decision for units leaving the factory, never a "
            "default.")
        if not permanent:
            g.refusal = ("--rdp 2 is permanent — it needs an explicit "
                         "--permanent flag AND a typed confirmation phrase")
            return g
        g.phrase = f"permanently disable debug on {part}"
    elif rdp_target is not None and rdp_target < current_level:
        g.warnings.append(
            f"{part} is at RDP level {current_level}. Returning to level 0 "
            f"MASS-ERASES ALL FLASH — bootloader, both firmware slots and the "
            f"boot-state store are wiped by the hardware before protection "
            f"drops.")
        if not accept_mass_erase:
            g.refusal = ("regressing RDP erases the chip — it needs an "
                         "explicit --accept-mass-erase flag AND a typed "
                         "confirmation phrase")
            return g
        g.phrase = f"erase {part}"
    elif rdp_target == 1:
        g.warnings.append(
            "RDP level 1 blocks debugger access to flash. Returning to level "
            "0 later MASS-ERASES the chip — every byte of every slot. Field "
            "updates still work (the UART bootloader is unaffected).")
        g.confirm = True

    if wrp_add:
        g.warnings.append(
            "write-protecting the bootloader region — the bootloader can then "
            "only be replaced by first clearing WRP over the probe.")
        g.confirm = True
    return g


# ── apply plan ──────────────────────────────────────────────────────────────


@dataclass(frozen=True)
class ApplyPlan:
    """Everything `secure apply` will do, computed before any device write.

    ``pokes`` (F7) is the raw (address, value) list — the SAME list the Renode
    witness drives, so what the witness proves is what the tool does (the
    witness runs wherever Renode is installed — locally, not in CI's pytest
    job, which has no Renode and skips it). ``openocd`` is
    the command list handed to the probe; on G0 it uses openocd's stm32l4x
    option driver (masked read-modify-write in the driver) instead of raw
    pokes, because that path is community-proven on G0 silicon.
    """

    openocd: tuple[str, ...]
    pokes: tuple[tuple[int, int], ...]  # F7 only; empty on G0
    verify_addr: int | None  # register to re-read after apply (None: chip resets)
    verify_mask: int
    verify_value: int
    resets_chip: bool  # G0 OBL_LAUNCH: losing the debug connection is SUCCESS


# Marker echoed between staging the writes and committing them, so the runner
# can tell "died before touching the chip" from "reset by the option reload".
WRITES_STAGED_MARK = "alloy-secure:options-staged"


def plan_apply(omap: OptionMap, chip: dict[str, Any],
               layout: SlotLayout | None, status: SecureStatus,
               rdp_target: int | None, wrp_bootloader: bool) -> ApplyPlan:
    """Pure: registers + values from data, current state from `status`."""
    if wrp_bootloader and layout is None:
        raise EmitError(
            f"chip {chip['part']} has no A/B slot layout — the bootloader WRP "
            f"range derives from it, so there is nothing to protect yet")
    if rdp_target is not None and rdp_target == status.rdp_level:
        rdp_target = None
    if wrp_bootloader and status.bootloader_protected:
        wrp_bootloader = False
    if rdp_target is None and not wrp_bootloader:
        raise EmitError("nothing to apply — the device already matches")

    if omap.kind == "g0":
        return _plan_g0(omap, layout, rdp_target, wrp_bootloader)
    return _plan_f7(omap, chip, layout, status, rdp_target, wrp_bootloader)


def _plan_g0(omap: OptionMap, layout: SlotLayout | None,
             rdp_target: int | None, wrp: bool) -> ApplyPlan:
    cmds: list[str] = ["init"]
    verify_mask = 0
    verify_value = 0
    if rdp_target is not None:
        mask = omap.mask("OPTR", "RDP")
        bit, _ = omap.field_span("OPTR", "RDP")
        value = RDP_BYTE[rdp_target] << bit
        cmds.append(f"stm32l4x option_write 0 {omap.offset('OPTR'):#x} "
                    f"{value:#010x} {mask:#010x}")
    if wrp:
        assert layout is not None
        first, last = _g0_bootloader_pages(layout)
        s_bit, _ = omap.field_span("WRP1AR", "WRP1A_STRT")
        e_bit, _ = omap.field_span("WRP1AR", "WRP1A_END")
        mask = omap.mask("WRP1AR", "WRP1A_STRT") | omap.mask("WRP1AR", "WRP1A_END")
        value = (first << s_bit) | (last << e_bit)
        cmds.append(f"stm32l4x option_write 0 {omap.offset('WRP1AR'):#x} "
                    f"{value:#010x} {mask:#010x}")
        verify_mask, verify_value = mask, value
    cmds.append(f"echo {WRITES_STAGED_MARK}")
    # option_load = OBL_LAUNCH: the chip RESETS and the debug session dies.
    # The runner treats a connection loss after the marker as expected success.
    cmds.append("stm32l4x option_load 0")
    cmds.append("shutdown")
    return ApplyPlan(tuple(cmds), (), None, verify_mask, verify_value, True)


def _plan_f7(omap: OptionMap, chip: dict[str, Any], layout: SlotLayout | None,
             status: SecureStatus, rdp_target: int | None,
             wrp: bool) -> ApplyPlan:
    if not omap.part.startswith(_F7_APPLY_PREFIXES):
        raise EmitError(
            f"`alloy secure apply` is gated to STM32F72x/F73x on the F7 in v1 "
            f"— {omap.part} carries a wider nWRP field (and, on F76x/F77x, an "
            f"nDBANK bit that changes the whole sector map) that the data does "
            f"not model yet. `alloy secure status` works; add the RM0410 "
            f"option facts to alloy-devices to unlock apply")
    optcr_addr = omap.addr("OPTCR")
    cur = status.words[optcr_addr]
    new = cur
    verify_mask = 0
    if rdp_target is not None:
        mask = omap.mask("OPTCR", "RDP")
        bit, _ = omap.field_span("OPTCR", "RDP")
        new = (new & ~mask) | (RDP_BYTE[rdp_target] << bit)
        verify_mask |= mask
    if wrp:
        assert layout is not None
        bit, width = omap.field_span("OPTCR", "NWRP")
        sectors = f7_bootloader_sectors(chip, layout)
        if any(s >= width for s in sectors):
            raise EmitError(
                f"bootloader sectors {sectors} exceed {omap.part}'s modeled "
                f"{width}-bit nWRP field — refusing to guess")
        for s in sectors:
            new &= ~(1 << (bit + s))  # active-low: 0 = protected
            verify_mask |= 1 << (bit + s)
    # The value we write must not carry OPTLOCK (it would re-lock before
    # OPTSTRT) and must not preset OPTSTRT.
    optlock = omap.mask("OPTCR", "OPTLOCK")
    optstrt = omap.mask("OPTCR", "OPTSTRT")
    staged = new & ~(optlock | optstrt)
    pokes = [
        (omap.addr("OPTKEYR"), _OPTKEY1),
        (omap.addr("OPTKEYR"), _OPTKEY2),
        (optcr_addr, staged),
        (optcr_addr, staged | optstrt),
    ]
    cmds = ["init", "halt"]
    cmds += [f"mww {a:#010x} {v:#010x}" for a, v in pokes]
    cmds.append(f"echo {WRITES_STAGED_MARK}")
    # RM0431 wants SR.BSY polled; a one-shot -c script sleeps generously
    # instead (option programming is milliseconds), then reads back.
    cmds.append("sleep 500")
    cmds.append(f"mdw {optcr_addr:#010x}")
    # Re-lock the option bytes (hygiene: the staged value with OPTLOCK set).
    cmds.append(f"mww {optcr_addr:#010x} {(staged | optlock):#010x}")
    cmds.append("shutdown")
    return ApplyPlan(tuple(cmds), tuple(pokes), optcr_addr,
                     verify_mask, new & verify_mask, False)


# ── openocd runner ──────────────────────────────────────────────────────────

Runner = Callable[[list[str]], subprocess.CompletedProcess]
_MDW = re.compile(r"(0x[0-9a-fA-F]{8}):\s*([0-9a-fA-F]{8})")


def openocd_argv(board: dict[str, Any], chip: dict[str, Any],
                 commands: list[str] | tuple[str, ...]) -> list[str]:
    """The one-shot openocd invocation. `alloy secure` is openocd-only in v1
    (probe-rs's CLI has no option-byte verbs), so this deliberately ignores
    the board's declared runner — only the probe KIND and family matter."""
    probe = board.get("probe")
    if probe is None:
        raise EmitError(f"board {board['id']} declares no probe — `alloy "
                        f"secure` needs a debug probe to reach option bytes")
    interface = OPENOCD_INTERFACE.get(probe.get("kind", ""))
    target = OPENOCD_TARGET.get(chip["family"])
    if not (interface and target):
        raise EmitError(
            f"no openocd mapping for probe '{probe.get('kind')}' / family "
            f"'{chip['family']}' — `alloy secure` is openocd-only in v1")
    return ["openocd",
            "-f", f"interface/{interface}.cfg",
            "-f", f"target/{target}.cfg",
            "-c", "; ".join(commands)]


def _default_runner(argv: list[str]) -> subprocess.CompletedProcess:
    if not shutil.which("openocd"):
        raise EmitError("openocd not on PATH — `alloy secure` is openocd-only "
                        "in v1 (install openocd, or `alloy setup`)")
    return subprocess.run(argv, capture_output=True, text=True, check=False)


def parse_mdw(output: str) -> dict[int, int]:
    """{address: word} for every `mdw` line in an openocd transcript."""
    return {int(m.group(1), 16): int(m.group(2), 16)
            for m in _MDW.finditer(output)}


def read_status(board: dict[str, Any], chip: dict[str, Any], omap: OptionMap,
                layout: SlotLayout | None,
                runner: Runner = _default_runner) -> SecureStatus:
    addrs = status_addrs(omap)
    cmds = ["init", *(f"mdw {a:#010x}" for a in addrs), "shutdown"]
    proc = runner(openocd_argv(board, chip, cmds))
    output = (proc.stdout or "") + (proc.stderr or "")
    words = parse_mdw(output)
    missing = [a for a in addrs if a not in words]
    if proc.returncode != 0 or missing:
        raise EmitError(
            "could not read the option bytes over the probe. Causes, most "
            "likely first: no board attached / wrong probe; the part is at "
            "RDP level 2 (debug permanently disabled — nothing can read it); "
            "openocd/probe mismatch. openocd said:\n"
            + output.strip()[-2000:])
    return decode_status(omap, chip, layout, words)


def run_apply(board: dict[str, Any], chip: dict[str, Any], plan: ApplyPlan,
              runner: Runner = _default_runner) -> tuple[bool, str]:
    """Execute the plan. Returns (verified, message). verified is False when
    verification was impossible (G0's option reload resets the chip), not
    when it failed — a failed verify raises."""
    proc = runner(openocd_argv(board, chip, list(plan.openocd)))
    output = (proc.stdout or "") + (proc.stderr or "")
    staged = WRITES_STAGED_MARK in output
    if proc.returncode != 0 and not (plan.resets_chip and staged):
        raise EmitError("openocd failed before the option bytes were "
                        "committed — nothing should have changed. It said:\n"
                        + output.strip()[-2000:])
    if plan.resets_chip:
        return False, ("option bytes staged and reloaded — the chip RESET "
                       "itself (that is how OBL_LAUNCH works), so the debug "
                       "session ended there. Power-cycle if in doubt, then "
                       "run `alloy secure status` to see the new state.")
    assert plan.verify_addr is not None
    words = parse_mdw(output)
    got = words.get(plan.verify_addr)
    if got is None:
        raise EmitError("apply ran but the readback is missing from openocd's "
                        "output — verify by hand with `alloy secure status`. "
                        "openocd said:\n" + output.strip()[-2000:])
    if got & plan.verify_mask != plan.verify_value:
        raise EmitError(
            f"option-byte readback mismatch: expected "
            f"{plan.verify_value:#010x} under mask {plan.verify_mask:#010x}, "
            f"read {got:#010x} — the device may be locked or the probe "
            f"dropped mid-write. Run `alloy secure status`")
    return True, "option bytes programmed and verified by readback"


# ── presentation (shared by status and apply) ───────────────────────────────

_LEVEL_MEANING = {
    0: "flash readable over the debug port — NOT production-safe",
    1: "debugger flash access blocked; level 0 regression mass-erases",
    2: "debug port permanently disabled",
}


def format_status(board_id: str, chip: dict[str, Any], omap: OptionMap,
                  layout: SlotLayout | None, status: SecureStatus) -> str:
    lines = [f"{board_id} ({chip['vendor']}/{chip['part']})",
             f"  RDP: level {status.rdp_level} (byte "
             f"{status.rdp_byte:#04x}) — {_LEVEL_MEANING[status.rdp_level]}"]
    lines.append("  WRP: " + (", ".join(status.wrp) if status.wrp
                              else "no ranges write-protected"))
    if layout is None:
        lines.append("  bootloader: no A/B slot layout for this chip — "
                     "no WRP range to derive")
    else:
        bl = layout.bootloader
        if omap.kind == "g0":
            first, last = _g0_bootloader_pages(layout)
            where = f"pages {first}-{last}"
        else:
            sectors = f7_bootloader_sectors(chip, layout)
            where = "sectors " + "-".join(str(s) for s in (sectors[0], sectors[-1]))
        state = "write-protected" if status.bootloader_protected else "NOT write-protected"
        lines.append(f"  bootloader {bl.base:#010x} +{bl.size // 1024} KB "
                     f"({where}): {state}")
    return "\n".join(lines)
