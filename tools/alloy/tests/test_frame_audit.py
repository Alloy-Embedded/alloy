"""Unit tests for `alloy frame-audit` DWARF parsing."""

from __future__ import annotations

from alloy_cli import frame_audit

# A trimmed objdump --dwarf=info excerpt: two coroutine frame structs plus a
# non-frame struct and a member DIE that must not confuse the parser.
_DWARF = """\
 <1><100>: Abbrev Number: 5 (DW_TAG_base_type)
    <101>   DW_AT_byte_size   : 4
    <105>   DW_AT_name        : (indirect string, offset: 0x1): unsigned int
 <2><53a7>: Abbrev Number: 24 (DW_TAG_structure_type)
    <53a8>   DW_AT_name        : (indirect string, offset: 0x13aa8): _ZN12_GLOBAL__N_16uptimeERN5alloy5async12task_storageILj256EEE.Frame
    <53ac>   DW_AT_byte_size   : 64
 <3><53b0>: Abbrev Number: 9 (DW_TAG_member)
    <53b1>   DW_AT_name        : (indirect string, offset: 0x2): __resume_fn
    <53b5>   DW_AT_byte_size   : 4
 <2><548e>: Abbrev Number: 24 (DW_TAG_structure_type)
    <548f>   DW_AT_name        : (indirect string, offset: 0x1816d): _ZN12_GLOBAL__N_15blinkERN5alloy5async12task_storageILj128EEE.Frame
    <5493>   DW_AT_byte_size   : 72
 <2><5600>: Abbrev Number: 24 (DW_TAG_structure_type)
    <5601>   DW_AT_name        : (indirect string, offset: 0x9): coroutine_handle
    <5605>   DW_AT_byte_size   : 4
"""


def test_parses_only_frame_structs() -> None:
    frames = frame_audit._frames_from_dwarf(_DWARF)
    names = {n for n, _ in frames}
    sizes = dict(frames)
    assert len(frames) == 2  # the two .Frame structs, not the base type/member/handle
    uptime = next(n for n in names if "uptime" in n)
    blink = next(n for n in names if "blink" in n)
    assert sizes[uptime] == 64
    assert sizes[blink] == 72


def test_declared_n_extracted_from_mangled_name() -> None:
    m = frame_audit._DECL_N.search("_ZN...task_storageILj256EEE.Frame")
    assert m and int(m.group(1)) == 256


def test_suggest_n_gives_rounded_headroom() -> None:
    # 64 B -> at least a slot above, rounded to 16.
    n = frame_audit._suggest_n(64)
    assert n >= 64 and n % 16 == 0 and n > 64


def test_no_frames_is_empty_not_error() -> None:
    assert frame_audit._frames_from_dwarf("nothing here\n") == []
