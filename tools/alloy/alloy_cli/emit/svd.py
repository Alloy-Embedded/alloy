"""CMSIS-SVD emission — the register viewer, from data we already curate.

Every ARM debugger front end (Cortex-Debug included) reads a peripheral register
map from an .svd file. alloy already owns that information: `alloy-devices`
curates registers per IP VERSION, with offsets, access, bit positions and named
enumerated values, and each chip says which instances it has, at which base, on
which IRQ. Emitting SVD from it costs no new data and turns "inspect the
peripheral" from a missing feature into a generated one.

Two honesty rules shape the output:

* SVD is an ARM/CMSIS format. A non-ARM chip is refused, not faked.
* A peripheral the database admits but has not curated has no registers to show.
  It is skipped and counted, so the caller can say what is missing rather than
  present a half-empty device as complete.
"""

from __future__ import annotations

from typing import Any
from xml.sax.saxutils import escape

from .common import EmitError

# CMSIS cpu names. Anything not in here is not something SVD can describe.
_CPU = {
    "cm0": "CM0", "cm0plus": "CM0PLUS", "cm3": "CM3", "cm4": "CM4",
    "cm7": "CM7", "cm33": "CM33",
}
_ACCESS = {"rw": "read-write", "ro": "read-only", "wo": "write-only"}


def _text(value: Any) -> str:
    """One line of XML-safe text — SVD descriptions are attributes of a tree
    viewer, and a stray newline makes them unreadable."""
    return escape(" ".join(str(value).split()))


def _span(registers: list[dict[str, Any]]) -> int:
    """Bytes the curated registers occupy, rounded up to a word."""
    end = 0
    for reg in registers:
        offset = int(reg["offset"], 16)
        width = int(reg.get("size", 32)) // 8
        array = reg.get("array")
        if array:
            offset += int(array["stride"]) * (int(array["count"]) - 1)
        end = max(end, offset + width)
    return (end + 3) & ~3


def _field_xml(field: dict[str, Any], indent: str) -> list[str]:
    width = int(field.get("width", 1))
    out = [f"{indent}<field>",
           f"{indent}  <name>{_text(field['name'])}</name>"]
    if field.get("description"):
        out.append(f"{indent}  <description>{_text(field['description'])}</description>")
    out += [f"{indent}  <bitOffset>{int(field['bit'])}</bitOffset>",
            f"{indent}  <bitWidth>{width}</bitWidth>"]
    values = field.get("values") or {}
    if values:
        out.append(f"{indent}  <enumeratedValues>")
        for name, value in sorted(values.items(), key=lambda kv: kv[1]):
            out += [f"{indent}    <enumeratedValue>",
                    f"{indent}      <name>{_text(name)}</name>",
                    f"{indent}      <value>{int(value)}</value>",
                    f"{indent}    </enumeratedValue>"]
        out.append(f"{indent}  </enumeratedValues>")
    out.append(f"{indent}</field>")
    return out


def _register_xml(reg: dict[str, Any], indent: str) -> list[str]:
    array = reg.get("array")
    # SVD spells a repeated register as a dim/dimIncrement pair with %s in the
    # name; that is exactly what `array` describes.
    name = f"{reg['name']}[%s]" if array else reg["name"]
    out = [f"{indent}<register>"]
    if array:
        out += [f"{indent}  <dim>{int(array['count'])}</dim>",
                f"{indent}  <dimIncrement>{int(array['stride'])}</dimIncrement>"]
    out.append(f"{indent}  <name>{_text(name)}</name>")
    if reg.get("description"):
        out.append(f"{indent}  <description>{_text(reg['description'])}</description>")
    out += [f"{indent}  <addressOffset>{reg['offset']}</addressOffset>",
            f"{indent}  <size>{int(reg.get('size', 32))}</size>",
            f"{indent}  <access>{_ACCESS[reg['access']]}</access>"]
    if reg.get("reset"):
        out.append(f"{indent}  <resetValue>{reg['reset']}</resetValue>")
    fields = reg.get("fields") or []
    if fields:
        out.append(f"{indent}  <fields>")
        for field in fields:
            out += _field_xml(field, f"{indent}    ")
        out.append(f"{indent}  </fields>")
    out.append(f"{indent}</register>")
    return out


def emit_svd(chip: dict[str, Any],
             registers: dict[str, dict[str, Any]]) -> tuple[str, list[str]]:
    """(SVD document, names of peripherals left out because they are uncurated)."""
    cores = chip.get("cores") or []
    core = (cores[0] if cores else {}).get("name")
    if core not in _CPU:
        raise EmitError(
            f"SVD describes ARM Cortex-M devices; {chip.get('part', '?')} is "
            f"'{core or 'unknown'}'. Nothing to emit — the register viewer is an "
            f"ARM debugger feature.")

    irq_numbers = {i["name"]: i["number"] for i in (chip.get("interrupts") or [])}
    peripherals = chip.get("peripherals") or {}

    skipped: list[str] = []
    body: list[str] = []
    # One instance per IP carries the registers; the rest derive from it. That
    # is standard SVD and keeps a 400-register chip a readable file.
    emitted_for_ip: dict[str, str] = {}

    for name in sorted(peripherals):
        spec = peripherals[name]
        ip = spec.get("ip")
        regs = (registers.get(ip or "") or {}).get("registers")
        if not ip or not regs:
            skipped.append(name)
            continue
        upper = name.upper()
        origin = emitted_for_ip.get(ip)
        attr = f' derivedFrom="{origin}"' if origin else ""
        body.append(f"    <peripheral{attr}>")
        body.append(f"      <name>{_text(upper)}</name>")
        if not origin:
            description = (registers.get(ip) or {}).get("description")
            if description:
                body.append(f"      <description>{_text(description)}</description>")
        body.append(f"      <baseAddress>{spec['base']}</baseAddress>")
        if not origin:
            body += ["      <addressBlock>",
                     "        <offset>0</offset>",
                     f"        <size>{_span(regs):#x}</size>",
                     "        <usage>registers</usage>",
                     "      </addressBlock>"]
        irq = spec.get("irq")
        if irq and irq in irq_numbers:
            body += ["      <interrupt>",
                     f"        <name>{_text(irq)}</name>",
                     f"        <value>{irq_numbers[irq]}</value>",
                     "      </interrupt>"]
        if not origin:
            body.append("      <registers>")
            for reg in regs:
                body += _register_xml(reg, "        ")
            body.append("      </registers>")
            emitted_for_ip[ip] = upper
        body.append("    </peripheral>")

    if not body:
        raise EmitError(
            f"{chip.get('part', '?')}: no curated peripheral to describe — an "
            f"empty SVD would be worse than none")

    cpu = _CPU[core]
    fpu = "true" if (cores[0].get("fpu")) else "false"
    prio_bits = int(cores[0].get("nvic_prio_bits", 2))
    part = chip.get("part", "device")

    header = [
        '<?xml version="1.0" encoding="utf-8"?>',
        "<!-- GENERATED by `alloy svd` from alloy-devices curated register maps.",
        "     DO NOT EDIT: fix the data and regenerate. Peripherals with no",
        "     curated register file are absent, not empty. -->",
        '<device schemaVersion="1.1" xmlns:xs="http://www.w3.org/2001/XMLSchema-instance"',
        '        xs:noNamespaceSchemaLocation="CMSIS-SVD.xsd">',
        f"  <vendor>{_text(chip.get('vendor', ''))}</vendor>",
        f"  <name>{_text(part)}</name>",
        "  <version>1.0</version>",
        f"  <description>{_text(chip.get('family', part))} — curated by "
        f"alloy-devices</description>",
        "  <cpu>",
        f"    <name>{cpu}</name>",
        "    <revision>r0p0</revision>",
        "    <endian>little</endian>",
        "    <mpuPresent>false</mpuPresent>",
        f"    <fpuPresent>{fpu}</fpuPresent>",
        f"    <nvicPrioBits>{prio_bits}</nvicPrioBits>",
        "    <vendorSystickConfig>false</vendorSystickConfig>",
        "  </cpu>",
        "  <addressUnitBits>8</addressUnitBits>",
        "  <width>32</width>",
        "  <size>32</size>",
        "  <access>read-write</access>",
        "  <resetValue>0x0</resetValue>",
        "  <resetMask>0xFFFFFFFF</resetMask>",
        "  <peripherals>",
    ]
    return "\n".join([*header, *body, "  </peripherals>", "</device>", ""]), skipped
