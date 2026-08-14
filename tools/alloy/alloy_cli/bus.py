"""Bus message registry — bus.toml is the wire CONTRACT between boards.

libs/bus draws one line: local topics are zero-config (the C++ type is the
topic), but a message that CROSSES A WIRE needs a declared, stable identity.
bus.toml is that declaration, and the two ends of a link compile from the
same file — copied verbatim between projects and reviewed like the contract
it is.

The id doctrine is products.py's nvm-key doctrine, verbatim, because the
failure mode is identical ("reordering the table can never silently renumber
what is already deployed in the field"):

  - ids are EXPLICIT u16, never auto-assigned. 0x0000 is refused as a
    sentinel; 0xFF00.. is reserved for framework frames.
  - an id is retired, NEVER deleted or reused (emit/ota_key.py's ring rule:
    deleting an entry silently re-points everything that names the next one).
    A retired entry is a tombstone: `retired = true` and nothing else — the
    layout is gone, the id is reserved forever.
  - same id => same layout, forever. A layout change is a NEW id; `version`
    is a runtime guard against a stale peer, not a license to change bytes.

One oracle, two reporting shapes: message_issues() is the single
implementation of every rule; bus_validate.py re-shapes it as located
issues for forms/CI, resolve() turns the first-error-free registry into the
model emit/bus.py renders. test_bus_validate pins the pair:

    P1  validate finds no errors  =>  resolve + emission succeed
    P2  resolve/emission fails    =>  validate found an error

Unknown keys are refused rather than ignored, top to bottom — a typo in a
wire id is the last thing that should fail open (the [devices] rule).
"""

from __future__ import annotations

import re
import tomllib
from pathlib import Path
from typing import Any

from .emit.common import EmitError

BUS_SCHEMA = "alloy.bus.v1"

#: Field types the v1 wire admits, with their encoded size. All little-endian
#: on the wire via alloy::byteorder; f32 crosses as its IEEE-754 bits.
FIELD_TYPES: dict[str, int] = {
    "u8": 1, "i8": 1, "u16": 2, "i16": 2, "u32": 4, "i32": 4,
    "f32": 4, "bool": 1,
}

#: Body cap — bounds the encode a bridge performs under the publish mask and
#: the RAM a receiver buffers. Mirrors wire_max_body in libs/bus/bus/wire.hpp.
MAX_BODY = 128

#: Reserved id space: 0x0000 is a sentinel, 0xFF00.. belongs to framework
#: frames (a future hello / manifest-hash exchange).
RESERVED_FLOOR = 0xFF00

_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_TOP_KEYS = frozenset({"schema", "messages"})
_MSG_KEYS = frozenset({"id", "version", "fields", "retired"})
_FIELD_KEYS = frozenset({"name", "type"})


def bus_toml_path(project_root: Path) -> Path:
    return project_root / "bus.toml"


def load_bus(project_root: Path) -> dict[str, Any]:
    """Parse bus.toml. Raises EmitError when the file is absent, unparseable,
    or carries the wrong schema line — everything else is message_issues()'s
    business, so validate can list ALL of it."""
    path = bus_toml_path(project_root)
    if not path.is_file():
        raise EmitError(f"{path}: no bus.toml here — a project without wire "
                        "messages simply has none")
    try:
        data = tomllib.loads(path.read_text())
    except tomllib.TOMLDecodeError as exc:
        raise EmitError(f"{path}: {exc}") from exc
    if data.get("schema") != BUS_SCHEMA:
        raise EmitError(f"{path}: schema must be \"{BUS_SCHEMA}\" "
                        f"(found {data.get('schema')!r})")
    return data


def message_issues(data: dict[str, Any]) -> list[tuple[str, str]]:
    """THE oracle: every rule, expressed once, as (field, problem) pairs.
    Empty list == the registry is emittable."""
    issues: list[tuple[str, str]] = []
    for key in sorted(set(data) - _TOP_KEYS):
        issues.append((key, f"unknown key '{key}' (a typo here must not fail open)"))
    messages = data.get("messages", {})
    if not isinstance(messages, dict):
        issues.append(("messages", "[messages.<name>] tables expected"))
        return issues

    ids_seen: dict[int, str] = {}
    for name, decl in messages.items():
        where = f"messages.{name}"
        if not _IDENT.match(name):
            issues.append((where, f"'{name}' is not a valid C++ identifier"))
        if not isinstance(decl, dict):
            issues.append((where, "a message is a table (id = ..., fields = [...])"))
            continue
        for key in sorted(set(decl) - _MSG_KEYS):
            issues.append((f"{where}.{key}", f"unknown key '{key}'"))

        msg_id = decl.get("id")
        if not isinstance(msg_id, int) or isinstance(msg_id, bool):
            issues.append((f"{where}.id", "an explicit u16 id is required "
                           "(design decision: ids are never auto-assigned)"))
        else:
            if msg_id == 0:
                issues.append((f"{where}.id", "0x0000 is the reserved sentinel — pick another"))
            elif msg_id > 0xFFFF or msg_id < 0:
                issues.append((f"{where}.id", "an id is a u16"))
            elif msg_id >= RESERVED_FLOOR:
                issues.append((f"{where}.id",
                               f"0x{RESERVED_FLOOR:04X}..0xFFFF is reserved for framework frames"))
            if msg_id in ids_seen:
                issues.append((f"{where}.id",
                               f"duplicate id 0x{msg_id:04X} (also '{ids_seen[msg_id]}') — "
                               "an id names ONE layout forever"))
            else:
                ids_seen[msg_id] = name

        retired = decl.get("retired", False)
        if not isinstance(retired, bool):
            issues.append((f"{where}.retired", "retired is true or false"))
            retired = False
        if retired:
            extra = sorted(set(decl) & {"fields", "version"})
            if extra:
                issues.append((f"{where}",
                               "a retired id is a tombstone — the layout is gone, the id is "
                               f"reserved forever; drop {', '.join(extra)}"))
            continue

        version = decl.get("version", 1)
        if not isinstance(version, int) or isinstance(version, bool) \
                or not 1 <= version <= 255:
            issues.append((f"{where}.version", "version is a u8 >= 1"))

        fields = decl.get("fields")
        if not isinstance(fields, list) or not fields:
            issues.append((f"{where}.fields",
                           "an active message declares its fields "
                           "(fields = [{ name = ..., type = ... }])"))
            continue
        names_seen: set[str] = set()
        body = 0
        for i, f in enumerate(fields):
            fwhere = f"{where}.fields[{i}]"
            if not isinstance(f, dict):
                issues.append((fwhere, "a field is a table with name and type"))
                continue
            for key in sorted(set(f) - _FIELD_KEYS):
                issues.append((fwhere, f"unknown key '{key}'"))
            fname = f.get("name")
            if not isinstance(fname, str) or not _IDENT.match(fname):
                issues.append((fwhere, "field name must be a C++ identifier"))
            elif fname in names_seen:
                issues.append((fwhere, f"duplicate field name '{fname}'"))
            else:
                names_seen.add(fname)
            ftype = f.get("type")
            if ftype not in FIELD_TYPES:
                issues.append((fwhere,
                               f"unknown type '{ftype}' (one of: "
                               f"{', '.join(sorted(FIELD_TYPES))})"))
            else:
                body += FIELD_TYPES[ftype]
        if body > MAX_BODY:
            issues.append((f"{where}.fields",
                           f"body is {body} B; the slow-plane cap is {MAX_BODY} B — "
                           "split the message"))
    return issues


def resolve(data: dict[str, Any]) -> dict[str, Any]:
    """Registry -> the model emit/bus.py renders. Validate-before-emit: every
    problem is listed at once (alloy bus validate shows the same list)."""
    issues = message_issues(data)
    if issues:
        details = "\n".join(f"  {field}: {msg}" for field, msg in issues)
        raise EmitError("bus.toml failed validation "
                        "(alloy bus validate shows the same list):\n" + details)
    model: list[dict[str, Any]] = []
    for name, decl in data.get("messages", {}).items():
        if decl.get("retired", False):
            model.append({"name": name, "id": decl["id"], "retired": True})
            continue
        offset = 0
        fields: list[dict[str, Any]] = []
        for f in decl["fields"]:
            size = FIELD_TYPES[f["type"]]
            fields.append({"name": f["name"], "type": f["type"],
                           "size": size, "offset": offset})
            offset += size
        model.append({"name": name, "id": decl["id"],
                      "version": decl.get("version", 1), "retired": False,
                      "size": offset, "fields": fields})
    # Deterministic output: sorted by id (the wire's own order), like every
    # emitter here — no dict-insertion accidents in a generated header.
    model.sort(key=lambda m: m["id"])
    return {"messages": model}
