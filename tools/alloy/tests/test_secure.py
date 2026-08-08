"""`alloy secure` — the planner, the decoder and, above all, the GUARDS.

A security feature with a casual UX is a liability, so the guard matrix is
pinned exhaustively: every destructive transition must refuse without its
explicit flag AND typed phrase, every warning must name what is lost, and the
WRP range must DERIVE from the slot layout (a hand-typed range passing these
tests would be a bug in the tests).

Nothing here touches hardware: the planner and decoder are pure, and the
openocd runner is exercised against a fake. The final class is the one
emulated witness — it drives the SAME poke list the planner emits into
Renode's F7 flash-controller model and is skipped when Renode is absent.
"""

from __future__ import annotations

import copy
import json
import os
import re
import subprocess
from pathlib import Path
from types import SimpleNamespace
from typing import Any

import pytest

from alloy_cli import secure
from alloy_cli.devices import load_chip, load_registers
from alloy_cli.emit.common import EmitError
from alloy_cli.emit.slots import f7_sector_spans, slot_layout

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _board(board_id: str) -> dict:
    return json.loads((ALLOY_ROOT / "boards" / board_id / "board.json").read_text())


def _ctx(chip_id: str) -> tuple[dict, secure.OptionMap, Any]:
    chip = load_chip(DEVICES_ROOT, chip_id)
    omap = secure.option_map(chip, load_registers(DEVICES_ROOT))
    return chip, omap, slot_layout(chip)


def _g0_words(omap: secure.OptionMap, rdp: int = 0xAA,
              wrp_a: tuple[int, int] = (0x7F, 0x00),
              wrp_b: tuple[int, int] = (0x7F, 0x00)) -> dict[int, int]:
    """Synthetic register words. (0x7F, 0x00) is STRT > END = area disabled,
    the reset state per RM0444."""
    return {
        omap.addr("OPTR"): rdp,
        omap.addr("WRP1AR"): (wrp_a[1] << 16) | wrp_a[0],
        omap.addr("WRP1BR"): (wrp_b[1] << 16) | wrp_b[0],
    }


def _f7_word(rdp: int = 0xAA, nwrp: int = 0xFF, extra: int = 0) -> int:
    return (nwrp << 16) | (rdp << 8) | extra


# ── the map comes from data, not from this module ───────────────────────────


@skip_no_devices
def test_addresses_are_chip_base_plus_register_offset() -> None:
    chip, omap, _ = _ctx("st/stm32g071rb")
    base = int(chip["peripherals"]["flash"]["base"], 16)
    assert omap.addr("OPTR") == base + omap.offset("OPTR")
    # Perturb the FACT: move the flash controller and the address must follow.
    moved = copy.deepcopy(chip)
    moved["peripherals"]["flash"]["base"] = "0x50000000"
    omap2 = secure.option_map(moved, load_registers(DEVICES_ROOT))
    assert omap2.addr("OPTR") == 0x50000000 + omap.offset("OPTR")


@skip_no_devices
def test_unsupported_flash_ip_is_refused_by_name() -> None:
    chip = load_chip(DEVICES_ROOT, "st/stm32g071rb")
    alien = copy.deepcopy(chip)
    alien["peripherals"]["flash"]["ip"] = "st/flash_l4"
    with pytest.raises(EmitError, match="st/flash_l4"):
        secure.option_map(alien, load_registers(DEVICES_ROOT))


def test_rdp_byte_decode() -> None:
    assert secure.rdp_level(0xAA) == 0
    assert secure.rdp_level(0xCC) == 2
    # ANY other byte is level 1 — not just the canonical 0xBB.
    for byte in (0xBB, 0x00, 0xFF, 0x55):
        assert secure.rdp_level(byte) == 1


# ── status decode ───────────────────────────────────────────────────────────


@skip_no_devices
def test_g0_status_fresh_part() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(omap, chip, layout, _g0_words(omap))
    assert (st.rdp_level, st.rdp_byte) == (0, 0xAA)
    assert st.wrp == ()
    assert st.bootloader_protected is False


@skip_no_devices
def test_g0_status_protected_part() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(
        omap, chip, layout, _g0_words(omap, rdp=0xBB, wrp_a=(0, 15)))
    assert st.rdp_level == 1
    assert st.wrp == ("pages 0-15",)
    assert st.bootloader_protected is True


@skip_no_devices
def test_g0_partial_wrp_does_not_count_as_bootloader_protection() -> None:
    """Pages 0-7 leave half the bootloader writable — status must say NOT."""
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(
        omap, chip, layout, _g0_words(omap, wrp_a=(0, 7)))
    assert st.bootloader_protected is False


@skip_no_devices
def test_f7_status_decodes_active_low_nwrp() -> None:
    chip, omap, layout = _ctx("st/stm32f722")
    addr = omap.addr("OPTCR")
    st = secure.decode_status(omap, chip, layout, {addr: _f7_word()})
    assert (st.rdp_level, st.wrp, st.bootloader_protected) == (0, (), False)
    # Clearing bits 0 and 1 (active-low) protects sectors 0 and 1 = bootloader.
    st = secure.decode_status(omap, chip, layout,
                              {addr: _f7_word(rdp=0xBB, nwrp=0xFC)})
    assert st.rdp_level == 1
    assert st.wrp == ("sector 0", "sector 1")
    assert st.bootloader_protected is True
    # Only sector 0 protected: half a bootloader is not protection.
    st = secure.decode_status(omap, chip, layout, {addr: _f7_word(nwrp=0xFE)})
    assert st.bootloader_protected is False


# ── WRP derives from the slot layout ────────────────────────────────────────


@skip_no_devices
def test_g0_wrp_range_derives_from_the_layout() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(omap, chip, layout, _g0_words(omap))
    plan = secure.plan_apply(omap, chip, layout, st, None, True)
    # 32 KB bootloader / 2 KB pages = pages 0..15, from slot_layout — the same
    # numbers the linker script and bootloader build trust.
    pages = layout.bootloader.size // layout.page_size
    s_bit, _ = omap.field_span("WRP1AR", "WRP1A_STRT")
    e_bit, _ = omap.field_span("WRP1AR", "WRP1A_END")
    value = (0 << s_bit) | ((pages - 1) << e_bit)
    mask = omap.mask("WRP1AR", "WRP1A_STRT") | omap.mask("WRP1AR", "WRP1A_END")
    assert (f"stm32l4x option_write 0 {omap.offset('WRP1AR'):#x} "
            f"{value:#010x} {mask:#010x}") in plan.openocd
    assert pages == 16  # the current layout fact, stated so a change is loud


@skip_no_devices
def test_f7_sector_spans_match_the_slot_layout() -> None:
    """The sector walk `secure` uses and the layout the bootloader links into
    must be the same geometry — pin them to each other."""
    chip, _, layout = _ctx("st/stm32f722")
    flash = next(m for m in chip["memories"] if m["kind"] == "flash")
    total = int(flash["size"])
    spans = f7_sector_spans(total)
    assert sum(spans) == total
    assert len(spans) == 8  # 512 K parts: 4 small + 1 medium + 3 big
    assert layout.bootloader.size == spans[0] + spans[1]
    assert layout.slot_a.size == spans[5]
    assert secure.f7_bootloader_sectors(chip, layout) == [0, 1]


def test_f7_sector_spans_2m_has_twelve_sectors() -> None:
    # F767 (2 M): 4x32K + 128K + 7x256K = 12 sectors = 12 nWRP bits (RM0410).
    assert len(f7_sector_spans(2 * 1024 * 1024)) == 12


# ── the guard matrix, exhaustively ──────────────────────────────────────────

PART = "STM32G071RB"


def test_no_flags_is_advice_never_action() -> None:
    g = secure.apply_guards(PART, 0, None, False)
    assert g.noop and "--rdp" in g.noop
    assert not g.refusal and not g.phrase and not g.confirm


def test_same_level_is_a_noop() -> None:
    g = secure.apply_guards(PART, 1, 1, False)
    assert g.noop == "already at RDP level 1 — nothing to do"


def test_level_2_current_refuses_everything() -> None:
    for target, wrp in ((0, False), (1, False), (2, False), (None, True)):
        g = secure.apply_guards(PART, 2, target, wrp,
                                accept_mass_erase=True, permanent=True)
        assert g.refusal and "PERMANENT" in g.refusal


def test_0_to_1_warns_about_the_regression_before_anything() -> None:
    g = secure.apply_guards(PART, 0, 1, False)
    assert not g.refusal and not g.phrase
    assert g.confirm  # y/N (or --yes) — routine, not phrase-grade
    assert any("MASS-ERASES" in w for w in g.warnings)
    assert any("blocks debugger access" in w for w in g.warnings)


def test_1_to_0_refuses_without_the_flag() -> None:
    g = secure.apply_guards(PART, 1, 0, False)
    assert g.refusal and "--accept-mass-erase" in g.refusal
    # The warning about what is lost prints BEFORE the refusal.
    assert any("MASS-ERASES ALL FLASH" in w for w in g.warnings)
    assert any("bootloader" in w for w in g.warnings)


def test_1_to_0_with_the_flag_still_demands_the_typed_phrase() -> None:
    g = secure.apply_guards(PART, 1, 0, False, accept_mass_erase=True)
    assert not g.refusal
    assert g.phrase == f"erase {PART}"


def test_rdp2_refuses_without_permanent() -> None:
    for current in (0, 1):
        g = secure.apply_guards(PART, current, 2, False)
        assert g.refusal and "--permanent" in g.refusal
        assert any("IRREVERSIBLE" in w for w in g.warnings)
        assert any("manufacturing decision" in w for w in g.warnings)


def test_rdp2_with_permanent_demands_the_typed_phrase() -> None:
    g = secure.apply_guards(PART, 0, 2, False, permanent=True)
    assert not g.refusal
    assert g.phrase == f"permanently disable debug on {PART}"


def test_accept_mass_erase_does_not_leak_into_rdp2() -> None:
    """The flags are NOT interchangeable: consenting to a mass erase is not
    consenting to permanence."""
    g = secure.apply_guards(PART, 0, 2, False, accept_mass_erase=True)
    assert g.refusal and "--permanent" in g.refusal


def test_wrp_alone_is_routine_confirm() -> None:
    g = secure.apply_guards(PART, 0, None, True)
    assert not g.refusal and not g.phrase and g.confirm
    assert any("bootloader" in w for w in g.warnings)


def test_same_level_plus_wrp_keeps_only_the_wrp_warning() -> None:
    g = secure.apply_guards(PART, 1, 1, True)
    assert not g.refusal and not g.noop
    assert not any("MASS-ERASES" in w for w in g.warnings)


def test_bogus_level_is_refused() -> None:
    g = secure.apply_guards(PART, 0, 3, False)
    assert g.refusal


# ── apply plans ─────────────────────────────────────────────────────────────


@skip_no_devices
def test_g0_plan_shape_and_reset_expectation() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(omap, chip, layout, _g0_words(omap))
    plan = secure.plan_apply(omap, chip, layout, st, 1, True)
    cmds = list(plan.openocd)
    assert cmds[0] == "init" and cmds[-1] == "shutdown"
    rdp_mask = omap.mask("OPTR", "RDP")
    assert (f"stm32l4x option_write 0 {omap.offset('OPTR'):#x} "
            f"{0xBB:#010x} {rdp_mask:#010x}") in cmds
    # The staged-marker precedes option_load, so the runner can tell a reset
    # (expected) from a failure before anything was written.
    assert cmds.index(f"echo {secure.WRITES_STAGED_MARK}") \
        < cmds.index("stm32l4x option_load 0")
    assert plan.resets_chip and plan.pokes == ()


@skip_no_devices
def test_plan_drops_what_already_matches() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(
        omap, chip, layout, _g0_words(omap, rdp=0xBB, wrp_a=(0, 15)))
    with pytest.raises(EmitError, match="already matches"):
        secure.plan_apply(omap, chip, layout, st, 1, True)


@skip_no_devices
def test_f7_plan_is_masked_rmw_over_the_read_word() -> None:
    chip, omap, layout = _ctx("st/stm32f722")
    # A realistic OPTCR with unrelated user bits set (BOR level, WDG choices
    # live in the low byte) — the plan must not disturb ONE of them.
    cur = _f7_word(rdp=0xAA, nwrp=0xFF, extra=0xED)
    st = secure.decode_status(omap, chip, layout, {omap.addr("OPTCR"): cur})
    plan = secure.plan_apply(omap, chip, layout, st, 1, True)
    keys, staged_write, start_write = plan.pokes[:2], plan.pokes[2], plan.pokes[3]
    assert [v for _, v in keys] == [(0x0819 << 16) | 0x2A3B, (0x4C5D << 16) | 0x6E7F]
    staged = staged_write[1]
    rdp_mask = omap.mask("OPTCR", "RDP")
    nwrp01 = 0b11 << omap.field_span("OPTCR", "NWRP")[0]
    optlock = omap.mask("OPTCR", "OPTLOCK")
    optstrt = omap.mask("OPTCR", "OPTSTRT")
    # Everything outside {RDP, the two bootloader nWRP bits, the two control
    # bits} is untouched — the whole point of read-modify-write under masks.
    assert (staged ^ cur) & ~(rdp_mask | nwrp01 | optlock | optstrt) == 0
    assert staged & optlock == 0 and staged & optstrt == 0
    assert start_write[1] == staged | optstrt
    # nWRP is ACTIVE-LOW: protecting = clearing.
    assert staged & nwrp01 == 0
    assert plan.verify_mask == rdp_mask | nwrp01
    assert not plan.resets_chip


@skip_no_devices
def test_f767_apply_is_gated_but_status_is_not() -> None:
    chip, omap, layout = _ctx("st/stm32f767")
    word = {omap.addr("OPTCR"): _f7_word()}
    st = secure.decode_status(omap, chip, layout, word)  # status: fine
    assert st.rdp_level == 0
    with pytest.raises(EmitError, match="nDBANK"):
        secure.plan_apply(omap, chip, layout, st, 1, False)


@skip_no_devices
def test_wrp_without_slot_layout_is_an_honest_refusal() -> None:
    chip, omap, _ = _ctx("st/stm32g071rb")
    st = secure.decode_status(omap, chip, None, _g0_words(omap))
    with pytest.raises(EmitError, match="slot layout"):
        secure.plan_apply(omap, chip, None, st, None, True)


# ── the openocd runner, against a fake ──────────────────────────────────────


def _fake_runner(returncode: int, output: str):
    calls: list[list[str]] = []

    def run(argv: list[str]) -> Any:
        calls.append(argv)
        return SimpleNamespace(returncode=returncode, stdout=output, stderr="")

    run.calls = calls  # type: ignore[attr-defined]
    return run


@skip_no_devices
def test_secure_ignores_the_boards_declared_runner() -> None:
    """nucleo_g071rb declares runner=probe-rs, but secure is openocd-only in
    v1 — the invocation must be openocd regardless."""
    chip, omap, _ = _ctx("st/stm32g071rb")
    argv = secure.openocd_argv(_board("nucleo_g071rb"), chip, ["init"])
    assert argv[0] == "openocd"
    assert "interface/stlink.cfg" in argv and "target/stm32g0x.cfg" in argv


@skip_no_devices
def test_unknown_probe_kind_is_refused_by_name() -> None:
    chip, omap, _ = _ctx("st/stm32g071rb")
    board = {"id": "x", "probe": {"kind": "jlink-something"}}
    with pytest.raises(EmitError, match="jlink-something"):
        secure.openocd_argv(board, chip, ["init"])
    with pytest.raises(EmitError, match="needs a debug probe"):
        secure.openocd_argv({"id": "x"}, chip, ["init"])


def test_parse_mdw() -> None:
    out = "Info : blah\n0x40022020: fffffeaa \n0x4002202c: 0000007f\n"
    assert secure.parse_mdw(out) == {0x40022020: 0xFFFFFEAA,
                                     0x4002202C: 0x0000007F}


@skip_no_devices
def test_read_status_decodes_a_transcript() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    words = _g0_words(omap, rdp=0xBB)
    out = "".join(f"{a:#010x}: {v:08x}\n" for a, v in words.items())
    st = secure.read_status(_board("nucleo_g071rb"), chip, omap, layout,
                            runner=_fake_runner(0, out))
    assert st.rdp_level == 1


@skip_no_devices
def test_connect_failure_names_rdp2_as_a_cause() -> None:
    chip, omap, layout = _ctx("st/stm32g071rb")
    with pytest.raises(EmitError, match="RDP level 2"):
        secure.read_status(_board("nucleo_g071rb"), chip, omap, layout,
                           runner=_fake_runner(1, "Error: init mode failed"))


@skip_no_devices
def test_g0_apply_treats_reset_after_staging_as_success() -> None:
    """OBL_LAUNCH resets the chip mid-session; openocd exits non-zero. That is
    the DOCUMENTED success path — but only after the staged marker printed."""
    chip, omap, layout = _ctx("st/stm32g071rb")
    st = secure.decode_status(omap, chip, layout, _g0_words(omap))
    plan = secure.plan_apply(omap, chip, layout, st, 1, False)
    board = _board("nucleo_g071rb")
    verified, msg = secure.run_apply(
        board, chip, plan,
        runner=_fake_runner(1, f"{secure.WRITES_STAGED_MARK}\nconn dropped"))
    assert not verified and "RESET itself" in msg
    # Without the marker the same exit code is a real failure.
    with pytest.raises(EmitError, match="nothing should have changed"):
        secure.run_apply(board, chip, plan,
                         runner=_fake_runner(1, "Error: init mode failed"))


@skip_no_devices
def test_f7_apply_verifies_the_readback() -> None:
    chip, omap, layout = _ctx("st/stm32f722")
    cur = _f7_word()
    st = secure.decode_status(omap, chip, layout, {omap.addr("OPTCR"): cur})
    plan = secure.plan_apply(omap, chip, layout, st, 1, True)
    board = _board("nucleo_f722ze")
    good = f"{secure.WRITES_STAGED_MARK}\n" \
           f"{plan.verify_addr:#010x}: {plan.pokes[3][1]:08x}\n"
    verified, msg = secure.run_apply(board, chip, plan,
                                     runner=_fake_runner(0, good))
    assert verified and "verified" in msg
    bad = f"{secure.WRITES_STAGED_MARK}\n{plan.verify_addr:#010x}: {cur:08x}\n"
    with pytest.raises(EmitError, match="mismatch"):
        secure.run_apply(board, chip, plan, runner=_fake_runner(0, bad))


# ── the CLI glue: the typed phrase is enforced where the user meets it ──────


def _cli_project(tmp_path: Path) -> Path:
    root = tmp_path / "proj"
    (root / "src").mkdir(parents=True)
    (root / "alloy.toml").write_text(
        f'[project]\nname = "proj"\n\n[board]\nid = "nucleo_g071rb"\n\n'
        f'[alloy]\nroot = "{ALLOY_ROOT}"\n')
    return root


def _apply_args(project: Path, **kw: Any) -> Any:
    import argparse  # noqa: PLC0415

    defaults = dict(project=str(project), board=None, product=None, rdp=None,
                    wrp_bootloader=False, yes=False, accept_mass_erase=False,
                    permanent=False, dry_run=False)
    defaults.update(kw)
    return argparse.Namespace(**defaults)


@pytest.fixture
def cli_apply(tmp_path, monkeypatch):
    """cmd_secure_apply against a fake level-1 device; records run_apply."""
    from alloy_cli.cli import cmd_secure_apply  # noqa: PLC0415

    project = _cli_project(tmp_path)
    applied: list[Any] = []

    def fake_read_status(board, chip, omap, layout, runner=None):
        return secure.decode_status(omap, chip, layout,
                                    _g0_words(omap, rdp=0xBB))

    def fake_run_apply(board, chip, plan, runner=None):
        applied.append(plan)
        return True, "option bytes programmed and verified by readback"

    monkeypatch.setattr(secure, "read_status", fake_read_status)
    monkeypatch.setattr(secure, "run_apply", fake_run_apply)

    def run(argv_kw: dict[str, Any], typed: str | None = None) -> int:
        if typed is not None:
            monkeypatch.setattr("builtins.input", lambda _p: typed)
        return cmd_secure_apply(_apply_args(project, **argv_kw))

    return SimpleNamespace(run=run, applied=applied)


@skip_no_devices
def test_cli_wrong_phrase_writes_nothing(cli_apply, capsys) -> None:
    rc = cli_apply.run({"rdp": 0, "accept_mass_erase": True}, typed="erase it")
    assert rc == 1 and cli_apply.applied == []
    captured = capsys.readouterr()
    assert "nothing was written" in captured.err
    assert "MASS-ERASES ALL FLASH" in captured.out  # warned BEFORE the prompt


@skip_no_devices
def test_cli_exact_phrase_proceeds(cli_apply) -> None:
    rc = cli_apply.run({"rdp": 0, "accept_mass_erase": True},
                       typed="erase STM32G071RB")
    assert rc == 0 and len(cli_apply.applied) == 1


@skip_no_devices
def test_cli_rdp2_refused_without_permanent_even_with_yes(cli_apply, capsys) -> None:
    rc = cli_apply.run({"rdp": 2, "yes": True})
    assert rc == 1 and cli_apply.applied == []
    assert "--permanent" in capsys.readouterr().err


@skip_no_devices
def test_cli_no_flags_is_status_plus_advice(cli_apply, capsys) -> None:
    rc = cli_apply.run({})
    assert rc == 0 and cli_apply.applied == []
    out = capsys.readouterr().out
    assert "RDP: level 1" in out and "nothing to apply" in out


@skip_no_devices
def test_cli_dry_run_prints_the_commands_and_stops(cli_apply, capsys) -> None:
    rc = cli_apply.run({"wrp_bootloader": True, "dry_run": True})
    assert rc == 0 and cli_apply.applied == []
    out = capsys.readouterr().out
    assert "stm32l4x option_write" in out and "option_load" in out


# ── the Renode witness (F7 only; G0 has no flash-controller model) ──────────
#
# WHAT THIS PROVES: Renode 1.16.1's MTD.STM32F4_FlashController enforces the
# OPTKEYR unlock (locked writes ignored, key sequence unlocks, relock holds)
# and stores programmed OPTCR values — so the poke list the planner emits is
# witnessed to have the right ADDRESSES, ORDER and VALUES against a model of
# the register interface.
# WHAT IT CANNOT PROVE: actual readout blocking, the 1 -> 0 mass erase, RDP2
# permanence, WRP write refusal, or persistence across reset — the model has
# none of that behaviour. Those claims rest on RM0431 alone and remain
# hardware-unwitnessed (docs/guide/security.md says the same).


def _find_renode() -> str | None:
    from alloy_cli.cli import _find_renode as cli_find  # noqa: PLC0415

    found = cli_find()
    if found:
        return found
    local = Path.home() / "renode/Renode.app/Contents/MacOS/renode"
    return str(local) if os.access(local, os.X_OK) else None


RENODE = _find_renode()
_HEXLINE = re.compile(r"^0x([0-9A-Fa-f]+)\s*$", re.M)


def _renode_session(tmp: Path, repl: str, commands: list[str]) -> list[int]:
    """Run monitor commands headless; return every bare-hex line (the results
    of `sysbus ReadDoubleWord`) in order. A hang is a spin, never a pass —
    hence the hard timeout."""
    tmp.mkdir(parents=True, exist_ok=True)
    (tmp / "opt.repl").write_text(repl)
    script = "mach create \"opt\"\n" \
             "machine LoadPlatformDescription @opt.repl\n" \
             + "\n".join(commands) + "\nquit\n"
    (tmp / "opt.resc").write_text(script)
    proc = subprocess.run(
        [RENODE, "--disable-xwt", "--console", "opt.resc"],
        cwd=tmp, capture_output=True, text=True, timeout=180)
    values = [int(m.group(1), 16) for m in _HEXLINE.finditer(proc.stdout)]
    assert values, f"no read results in Renode output:\n{proc.stdout[-2000:]}"
    return values


@pytest.mark.skipif(RENODE is None, reason="Renode not installed")
@skip_no_devices
def test_renode_witnesses_the_f7_poke_list(tmp_path: Path) -> None:
    chip, omap, layout = _ctx("st/stm32f722")
    flash = next(m for m in chip["memories"] if m["kind"] == "flash")
    repl = (
        f"flash: Memory.MappedMemory @ sysbus {int(flash['base'], 16):#x}\n"
        f"    size: {int(flash['size']):#x}\n\n"
        f"flash_ctrl: MTD.STM32F4_FlashController @ sysbus {omap.regbase:#x}\n"
        f"    flash: flash\n")
    optcr = omap.addr("OPTCR")
    read = f"sysbus ReadDoubleWord {optcr:#x}"

    # Session 1: the model's reset OPTCR — the "current word" a real status
    # read would give (0x0FFFAAED, the RM0431 reset value).
    cur = _renode_session(tmp_path / "s1", repl, [read])[0]
    assert secure.rdp_level((cur >> 8) & 0xFF) == 0

    st = secure.decode_status(omap, chip, layout, {optcr: cur})
    plan = secure.plan_apply(omap, chip, layout, st, 1, True)
    staged = plan.pokes[2][1]
    optlock = omap.mask("OPTCR", "OPTLOCK")

    # Session 2 (fresh machine = same reset state): a locked write must be
    # IGNORED — the negative control that proves the unlock in the poke list
    # is load-bearing; then the planner's own pokes; then relock and retry.
    (tmp_path / "s2").mkdir()
    values = _renode_session(tmp_path / "s2", repl, [
        f"sysbus WriteDoubleWord {optcr:#x} {staged:#x}",   # still locked
        read,                                                # -> unchanged
        *(f"sysbus WriteDoubleWord {a:#x} {v:#x}" for a, v in plan.pokes),
        read,                                                # -> programmed
        f"sysbus WriteDoubleWord {optcr:#x} {(staged | optlock):#x}",  # relock
        f"sysbus WriteDoubleWord {optcr:#x} {cur:#x}",       # ignored again
        read,
    ])
    locked_read, programmed, after_relock = values[-3], values[-2], values[-1]
    assert locked_read == cur, "locked OPTCR write was not ignored"
    assert programmed & plan.verify_mask == plan.verify_value
    assert after_relock & plan.verify_mask == plan.verify_value, \
        "a write after relock changed the option bytes"
