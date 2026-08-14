"""Unit tests for `alloy symbols` and the [budget] gate.

The parsers are where this can silently lie, and one of them already did: the
first version decided "this section is copied from flash at startup" from
`vma != lma` alone. objdump reports an LMA for NOLOAD sections too — it is
wherever the previous loaded section ended — so `.bss` and the stack region
came out labelled as copied. The flags line is what distinguishes them, and
every test below that mentions LOAD/CONTENTS exists because of that bug.

The whole module exists to answer one question that `size` cannot: did the
code land where the linker script said? A parser that answers it wrongly is
worse than not having it.
"""

from __future__ import annotations

from alloy_cli import symbols

# Real `arm-none-eabi-objdump -h` output from examples/fastcode on a G0B1RE.
_OBJDUMP = """\
fastcode.elf:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .isr_vector   000000bc  08000000  08000000  00001000  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  1 .text         00000ec0  080000c0  080000c0  000010c0  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .fastcode     00000014  20000000  08000f88  00002000  2**1
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  3 .bss          00000480  20000018  08000f9c  00002018  2**3
                  ALLOC
  4 ._heap_stack  00000804  200004bc  08000f9c  00002018  2**3
                  ALLOC
  5 .debug_info   0001bc56  00000000  00000000  00002018  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
"""

# Real `arm-none-eabi-nm --numeric-sort --print-size --demangle` output.
_NM = """\
08000000 000000bc D g_vector_table
080000c0 00000090 t frame_dummy
08000140 00000154 T main
08000860 00000090 T Reset_Handler
20000000 00000012 T hot_sum(unsigned long)
20000018 00000400 b some_buffer
         U __libc_init_array
20000418 B _sbss
"""


def test_parse_sections_reads_flags() -> None:
    got = {s["name"]: s for s in symbols.parse_sections(_OBJDUMP)}
    assert got[".text"]["size"] == 0xEC0
    assert got[".text"]["vma"] == 0x080000C0
    assert got[".fastcode"]["lma"] == 0x08000F88
    assert got[".fastcode"]["flags"] == ["CONTENTS", "ALLOC", "LOAD", "READONLY", "CODE"]


def test_only_loaded_sections_with_a_split_address_need_a_copy() -> None:
    got = {s["name"]: s for s in symbols.parse_sections(_OBJDUMP)}
    # .fastcode runs in RAM and is stored in flash — startup must move it.
    assert got[".fastcode"]["needs_copy"] is True
    # .text runs where it is stored.
    assert got[".text"]["needs_copy"] is False
    # THE REGRESSION: .bss and ._heap_stack carry an LMA but no CONTENTS/LOAD,
    # so nothing copies them. Deciding from vma != lma alone marks both copied.
    assert got[".bss"]["needs_copy"] is False
    assert got["._heap_stack"]["needs_copy"] is False
    assert got[".bss"]["loaded"] is False


def test_debug_sections_are_not_allocated() -> None:
    got = {s["name"]: s for s in symbols.parse_sections(_OBJDUMP)}
    assert got[".debug_info"]["alloc"] is False
    assert got[".text"]["alloc"] is True


def test_parse_nm_keeps_addresses_sizes_and_demangled_names() -> None:
    got = symbols.parse_nm(_NM)
    by_name = {s["name"]: s for s in got}
    assert by_name["hot_sum(unsigned long)"]["addr"] == 0x20000000
    assert by_name["hot_sum(unsigned long)"]["size"] == 0x12
    assert by_name["main"]["kind"] == "T"
    # An undefined symbol has no address column at all and must not become one.
    assert "__libc_init_array" not in by_name
    # A sizeless symbol is kept with size 0 — section markers are exactly what
    # you want to see in an address-sorted dump.
    assert by_name["_sbss"]["size"] == 0


def test_parse_nm_sorts_by_address() -> None:
    got = symbols.parse_nm(_NM)
    assert [s["addr"] for s in got] == sorted(s["addr"] for s in got)


def test_attribute_places_symbols_in_their_section() -> None:
    sections = symbols.parse_sections(_OBJDUMP)
    syms = symbols.parse_nm(_NM)
    symbols.attribute(syms, sections)
    by_name = {s["name"]: s for s in syms}
    assert by_name["hot_sum(unsigned long)"]["section"] == ".fastcode"
    assert by_name["main"]["section"] == ".text"
    assert by_name["some_buffer"]["section"] == ".bss"


def test_section_totals_count_only_occupying_symbols() -> None:
    sections = symbols.parse_sections(_OBJDUMP)
    syms = symbols.parse_nm(_NM)
    symbols.attribute(syms, sections)
    totals = symbols.section_totals(syms)
    assert totals[".fastcode"] == 0x12
    # _sbss is size 0 and 'B'; some_buffer is 0x400 and 'b'. Both occupy, one is
    # empty, so the total is the buffer alone.
    assert totals[".bss"] == 0x400


def test_budget_passes_under_the_ceiling() -> None:
    sections = symbols.parse_sections(_OBJDUMP)
    rows = symbols.check_budget(sections, {".fastcode": 64})
    assert rows == [{"section": ".fastcode", "limit": 64, "used": 0x14,
                     "ok": True, "reason": None}]


def test_budget_fails_over_the_ceiling_and_says_by_how_much() -> None:
    sections = symbols.parse_sections(_OBJDUMP)
    rows = symbols.check_budget(sections, {".fastcode": 8})
    assert rows[0]["ok"] is False
    assert "20 B over a 8 B budget" in rows[0]["reason"]


def test_a_budgeted_section_that_is_absent_FAILS() -> None:
    """The LUGpe case, and the reason this gate is worth having.

    That firmware placed sixteen hot functions in an `.itcm_functions` section
    that no startup ever copied. Every total looked right. A budget on a
    section that does not exist has to be a failure, or the one state you most
    want to catch is the one that passes silently.
    """
    sections = symbols.parse_sections(_OBJDUMP)
    rows = symbols.check_budget(sections, {".itcm_functions": 4096})
    assert rows[0]["ok"] is False
    assert rows[0]["used"] is None
    assert "not in the image" in rows[0]["reason"]


def test_a_budgeted_section_that_is_EMPTY_also_fails() -> None:
    """The subtler half of the same bug, and the one that got past me.

    Removing ALLOY_FASTCODE from a function does not remove `.fastcode` from
    the image — it leaves a zero-byte section behind. Checking only for absence
    reports that as "0 B / 64 B ok", which passes the exact state the budget
    exists to catch. Found by deleting the attribute from examples/fastcode and
    watching the build stay green.
    """
    empty = [{"name": ".fastcode", "size": 0, "vma": 0x20000000,
              "lma": 0x08000F88, "flags": [], "alloc": True, "loaded": False,
              "needs_copy": False}]
    row = symbols.check_budget(empty, {".fastcode": 64})[0]
    assert row["ok"] is False
    assert row["used"] == 0
    assert "EMPTY" in row["reason"]


def test_budget_is_measured_against_the_section_not_the_symbol_total() -> None:
    """Padding and literal pools are bytes the device really pays for.

    .fastcode is 0x14 by section and 0x12 by symbol; a budget of 0x13 must
    fail, or a budget could be overrun by alignment without anyone noticing.
    """
    sections = symbols.parse_sections(_OBJDUMP)
    assert symbols.check_budget(sections, {".fastcode": 0x13})[0]["ok"] is False
    assert symbols.check_budget(sections, {".fastcode": 0x14})[0]["ok"] is True
