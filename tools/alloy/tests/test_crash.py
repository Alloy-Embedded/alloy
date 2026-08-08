"""Unit tests for the crash decoder (`alloy crash`).

The parsing and the CFSR table are where this can silently lie — a wrong bit
name sends someone hunting the wrong fault — so both are tested against the
architectural definitions (ARMv7-M ARM, DDI 0403E.b §B3.2.15), not against the
code's own output. The addr2line integration runs only where the ARM toolchain
and a built crash_report ELF exist; everywhere else it is an honest skip, not
a fake pass.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest

from alloy_cli import crash

ALLOY_ROOT = Path(__file__).resolve().parents[3]

# --- report-line parsing ----------------------------------------------------

_REAL_LINE = ("RECOVERED FROM A FAULT  pc=0x080001f4 lr=0x080001e1 "
              "status=0x00000000 (2 in a row)")


def test_parse_the_exact_line_the_example_prints() -> None:
    got = crash.parse_report_line(_REAL_LINE)
    assert got == {"pc": 0x080001F4, "lr": 0x080001E1, "status": 0,
                   "consecutive": 2}


def test_parse_tolerates_extra_noise_and_any_token_order() -> None:
    line = "[12:03] dev1| status=0x02000000 pc=0x0800abcd garbage lr=0xfffffff9"
    got = crash.parse_report_line(line)
    assert got == {"pc": 0x0800ABCD, "lr": 0xFFFFFFF9, "status": 0x02000000}


def test_parse_ignores_unknown_tokens_and_bare_text() -> None:
    assert crash.parse_report_line("no crash on record — first clean boot") == {}
    # A name= token the record does not define must not be invented into one.
    assert crash.parse_report_line("foo=0x12345678") == {}


def test_parse_takes_sp_psr_address_when_a_future_firmware_prints_them() -> None:
    got = crash.parse_report_line("pc=0x08000100 sp=0x20008fe0 psr=0x21000000 "
                                  "address=0x20030000")
    assert got["sp"] == 0x20008FE0
    assert got["psr"] == 0x21000000
    assert got["address"] == 0x20030000


# --- CFSR decode ------------------------------------------------------------


def test_cfsr_zero_decodes_to_nothing() -> None:
    # The ARMv6-M answer (no CFSR exists) and the honest v7-M "no bits" answer.
    assert crash.decode_cfsr(0) == []
    assert crash.address_validity(0) is None


def test_cfsr_undefinstr_is_a_usage_fault_at_bit_16() -> None:
    got = crash.decode_cfsr(1 << 16)
    assert len(got) == 1
    assert got[0]["fault"] == "UsageFault"
    assert got[0]["name"] == "UNDEFINSTR"


def test_cfsr_invstate_names_the_corrupted_function_pointer() -> None:
    (got,) = crash.decode_cfsr(1 << 17)
    assert got["name"] == "INVSTATE"
    assert "Thumb" in got["meaning"]


def test_cfsr_precise_bus_error_with_bfarvalid() -> None:
    status = (1 << 9) | (1 << 15)  # PRECISERR | BFARVALID
    names = {f["name"] for f in crash.decode_cfsr(status)}
    # BFARVALID is validity metadata, not a fault of its own.
    assert names == {"PRECISERR"}
    assert crash.address_validity(status) == "bfar"


def test_cfsr_mmarvalid_wins_reading_mmfar() -> None:
    status = (1 << 1) | (1 << 7)  # DACCVIOL | MMARVALID
    assert crash.address_validity(status) == "mmfar"
    (got,) = crash.decode_cfsr(status)
    assert got["fault"] == "MemManage"


def test_cfsr_every_documented_bit_decodes_exactly_once() -> None:
    all_bits = 0
    for bit, _, _, _ in crash._CFSR_BITS:
        all_bits |= 1 << bit
    assert len(crash.decode_cfsr(all_bits)) == len(crash._CFSR_BITS)


# --- address handling -------------------------------------------------------


def test_exc_return_values_are_not_code_addresses() -> None:
    assert crash.is_exc_return(0xFFFFFFF9)
    assert crash.is_exc_return(0xFFFFFFFD)
    assert not crash.is_exc_return(0x080001F4)
    assert not crash.is_exc_return(0x20001000)


def test_lr_queries_step_back_into_the_call_instruction() -> None:
    # lr = return address with the Thumb bit set; the BL that produced it
    # starts 2 bytes earlier (mid-instruction is fine for addr2line).
    assert crash.query_address("lr", 0x080001E1) == 0x080001DE
    assert crash.query_address("pc", 0x080001F4) == 0x080001F4


# --- the envelope, without any toolchain ------------------------------------


def test_report_without_elf_still_decodes_status_and_says_why(tmp_path: Path) -> None:
    values = crash.parse_report_line(_REAL_LINE)
    report = crash.crash_report(values, tmp_path / "missing.elf", tool=None)
    assert report["schema"] == "alloy.crash.v1"
    assert report["symbolized"] is False
    assert "no ELF" in report["reason"]
    assert report["status"]["value"] == 0
    assert report["consecutive"] == 2


def test_report_without_addr2line_names_the_missing_tool(tmp_path: Path) -> None:
    elf = tmp_path / "app.elf"
    elf.write_bytes(b"\x7fELF")
    report = crash.crash_report({"pc": 0x08000100}, elf, tool=None)
    assert "addr2line" in report["reason"]
    assert "alloy setup" in report["reason"]


def test_status_only_report_needs_no_elf_and_no_tool() -> None:
    report = crash.crash_report({"status": (1 << 25)}, elf=None, tool=None)
    assert report["reason"] is None
    assert report["status"]["faults"][0]["name"] == "DIVBYZERO"
    # Address field absent AND no validity bit -> no invented faulting address.
    assert report["status"]["faulting_address"] is None


def test_exc_return_lr_is_labelled_not_symbolized() -> None:
    report = crash.crash_report({"lr": 0xFFFFFFF9}, elf=None, tool=None)
    assert report["reason"] is None  # nothing symbolizable was asked for
    (frame,) = report["frames"]
    assert frame["function"] is None
    assert "EXC_RETURN" in frame["note"]


# --- addr2line integration (real toolchain, real ELF) -----------------------

_ADDR2LINE = crash.locate_addr2line()


def _built_crash_report_elf() -> Path | None:
    """The crash_report example's last build, wherever it was built for."""
    example = ALLOY_ROOT / "examples" / "crash_report"
    for elf in sorted(example.glob(".alloy/build-tree/*/out/crash_report.elf")):
        return elf
    return None


_ELF = _built_crash_report_elf()

needs_toolchain = pytest.mark.skipif(
    _ADDR2LINE is None, reason="arm-none-eabi-addr2line not on PATH or in ~/.alloy/tools")
needs_elf = pytest.mark.skipif(
    _ELF is None, reason="crash_report example not built (no ELF to symbolize)")


@needs_toolchain
@needs_elf
def test_symbolize_names_main_in_the_crash_report_elf() -> None:
    # Ask nm (same toolchain directory) where main really is, then require the
    # decoder to name it back — a self-consistent witness, not a golden value.
    nm = str(Path(_ADDR2LINE).parent / Path(_ADDR2LINE).name.replace(
        "addr2line", "nm"))
    out = subprocess.run([nm, str(_ELF)], capture_output=True, text=True,
                         check=True).stdout
    main_addr = next(int(line.split()[0], 16) for line in out.splitlines()
                     if line.strip().endswith(" T main"))
    report = crash.crash_report({"pc": main_addr}, _ELF, _ADDR2LINE)
    assert report["symbolized"] is True
    (frame,) = report["frames"]
    assert frame["function"] == "main"
    assert "main.cpp" in frame["location"]


@needs_toolchain
@needs_elf
def test_symbolize_out_of_image_address_answers_unknown_not_wrong() -> None:
    report = crash.crash_report({"pc": 0x1}, _ELF, _ADDR2LINE)
    (frame,) = report["frames"]
    assert frame["function"] in ("??", "$d", "$t") or frame["location"].startswith("??")


@needs_toolchain
def test_locate_addr2line_finds_an_executable() -> None:
    assert _ADDR2LINE is not None
    assert shutil.which(_ADDR2LINE) or Path(_ADDR2LINE).exists()
