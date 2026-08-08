"""Pinning the chip database a project was built against.

The facts a build is made of — register offsets, bit fields, memory maps, IRQ
numbers — do not live in this repo. They live in ``alloy-devices``, resolved at
build time from an env var, a sibling checkout or an installed wheel. That is
convenient for development and unacceptable for a shipped product: the same
``alloy build``, a year later, on a machine whose sibling checkout has moved,
can compile different facts into the same firmware without saying a word.

A project closes that hole by declaring what it expects:

    [devices]
    path = "/opt/alloy-devices"        # optional: WHERE (wins over discovery)
    version = "0.3.0"                  # optional: WHAT it calls itself
    digest = "sha256:…"                # optional: WHAT IT ACTUALLY IS

``path`` only redirects discovery. ``version`` and ``digest`` are assertions
checked against whatever was resolved — by any route, including
``ALLOY_DEVICES_ROOT`` — so an operator who redirects the database around a
project's back still trips the pin instead of silently shipping other facts.

``version`` is the cheap pin (a string the database declares about itself); it
cannot detect a checkout that moved *after* the release it claims to be, and
alloy-devices' own main branch does exactly that. ``digest`` is the honest one:
a content hash over every schema/register/chip file. Use it for anything you
have to be able to rebuild.
"""

from __future__ import annotations

import hashlib
import re
import tomllib
from pathlib import Path

# The trees whose bytes decide what gets compiled. Deliberately not the whole
# repo: README/CHANGELOG/tests churn without changing a single register offset,
# and a digest that moved for those would be ignored within a week.
DIGEST_TREES = ("schema", "registers", "chips")
DIGEST_SUFFIXES = (".yaml", ".yml", ".json")

_OPS = ("==", ">=")


class PinError(RuntimeError):
    """A project's [devices] pin does not match the resolved database."""


def read_pin(project_root: Path) -> dict[str, str]:
    """The ``[devices]`` table of a project's alloy.toml, validated."""
    toml_path = project_root / "alloy.toml"
    if not toml_path.exists():
        return {}
    table = tomllib.loads(toml_path.read_text()).get("devices") or {}
    if not isinstance(table, dict):
        raise PinError(f"{toml_path}: [devices] must be a table")
    known = {"path", "version", "digest"}
    for key, value in table.items():
        if key not in known:
            raise PinError(
                f"{toml_path}: [devices] {key} is not a known key "
                f"(known: {', '.join(sorted(known))})"
            )
        if not isinstance(value, str):
            raise PinError(f"{toml_path}: [devices] {key} must be a string")
    return dict(table)


def declared_version(devices_root: Path) -> str | None:
    """The version the database declares about itself, or None if it cannot say.

    A checkout answers from its pyproject.toml; an installed wheel answers from
    its distribution metadata. Deliberately NOT ``alloy_devices.__version__``:
    that constant was maintained by hand and said 0.1.0 through the 0.3.0
    release — the bug this reader found and alloy-devices has since fixed by
    deriving it. Reading pyproject/metadata keeps the pin honest against an
    older database whose constant is still wrong, and against any future one
    that drifts again. A pin must read the number the release process asserts.
    """
    pyproject = devices_root / "pyproject.toml"
    if pyproject.exists():
        try:
            data = tomllib.loads(pyproject.read_text())
        except (OSError, tomllib.TOMLDecodeError):
            data = {}
        project = data.get("project") or {}
        if project.get("name") == "alloy-devices" and isinstance(
                project.get("version"), str):
            return project["version"]
    try:
        from importlib.metadata import PackageNotFoundError, version  # noqa: PLC0415

        return version("alloy-devices")
    except (ImportError, PackageNotFoundError):
        return None


def digest_files(devices_root: Path) -> list[Path]:
    """Every file the digest covers, in the order it hashes them."""
    found: list[Path] = []
    for tree in DIGEST_TREES:
        base = devices_root / tree
        if not base.is_dir():
            continue
        found += sorted(
            p for p in base.rglob("*")
            if p.is_file() and p.suffix in DIGEST_SUFFIXES
        )
    return found


def content_digest(devices_root: Path) -> str:
    """``sha256:…`` over the database's schema/register/chip files.

    Path-sensitive as well as content-sensitive: renaming a chip changes the
    digest even though every byte survives. The wheel and the checkout lay the
    three trees out identically, so a digest taken from one matches the other.
    """
    outer = hashlib.sha256()
    for path in digest_files(devices_root):
        rel = path.relative_to(devices_root).as_posix()
        outer.update(rel.encode())
        outer.update(b"\0")
        outer.update(hashlib.sha256(path.read_bytes()).digest())
    return "sha256:" + outer.hexdigest()


def _parse_version(text: str) -> tuple[int, ...]:
    if not re.fullmatch(r"\d+(\.\d+)*", text):
        raise PinError(
            f"[devices] version: '{text}' is not a dotted numeric version — "
            "the pin compares release numbers (0.3.0), not PEP 440 ranges"
        )
    return tuple(int(part) for part in text.split("."))


def version_matches(spec: str, actual: str) -> bool:
    """Does ``actual`` satisfy the pin ``spec``?

    Two forms only: an exact version (``0.3.0``, or ``==0.3.0``) and a floor
    (``>=0.3.0``). Not PEP 440 — no ranges, no wildcards, no epochs. A pin that
    needs more than this wants a digest, not a cleverer version grammar.
    """
    spec = spec.strip()
    op = "=="
    for candidate in _OPS:
        if spec.startswith(candidate):
            op, spec = candidate, spec[len(candidate):].strip()
            break
    want = _parse_version(spec)
    have = _parse_version(actual)
    if op == "==":
        # 0.3 == 0.3.0: compare on the shorter tuple's length, so a two-part
        # pin is not defeated by a three-part release number.
        width = max(len(want), len(have))
        pad = lambda t: t + (0,) * (width - len(t))  # noqa: E731
        return pad(want) == pad(have)
    return have >= want


def check_pin(project_root: Path, devices_root: Path) -> None:
    """Enforce a project's ``[devices]`` version/digest pin. No pin, no work."""
    pin = read_pin(project_root)
    want_version = pin.get("version")
    want_digest = pin.get("digest")
    if want_version:
        actual = declared_version(devices_root)
        if actual is None:
            raise PinError(
                f"[devices] version = \"{want_version}\" cannot be checked: the "
                f"database at {devices_root} declares no version (no "
                "pyproject.toml, and alloy-devices is not installed)"
            )
        if not version_matches(want_version, actual):
            raise PinError(
                f"chip database version mismatch: alloy.toml pins "
                f"[devices] version = \"{want_version}\", but {devices_root} "
                f"is {actual}"
            )
    if want_digest:
        actual_digest = content_digest(devices_root)
        if actual_digest != want_digest.strip():
            raise PinError(
                f"chip database CONTENT mismatch: alloy.toml pins\n"
                f"  [devices] digest = \"{want_digest.strip()}\"\n"
                f"but {devices_root} hashes to\n"
                f"  {actual_digest}\n"
                "The facts this project would compile are not the facts it was "
                "pinned to. Check out the pinned database, or re-pin with "
                "`alloy devices --pin` if the move was intended."
            )


_SECTION_RE = re.compile(r"^\[devices\]\s*\n(?:(?!\[).*\n?)*", re.MULTILINE)


def write_pin(project_root: Path, devices_root: Path,
              *, version: bool = True, digest: bool = True) -> str:
    """Freeze the resolved database into alloy.toml's ``[devices]`` table.

    Rewrites an existing ``[devices]`` section in place (keeping any ``path``)
    and appends one otherwise. Returns the section as written.
    """
    toml_path = project_root / "alloy.toml"
    if not toml_path.exists():
        raise PinError(f"{project_root} is not an alloy project (no alloy.toml)")
    existing = read_pin(project_root)
    lines = ["[devices]"]
    if path := existing.get("path"):
        lines.append(f'path = "{path}"')
    if version:
        declared = declared_version(devices_root)
        if declared is None:
            raise PinError(
                f"cannot pin a version: {devices_root} declares none")
        lines.append(f'version = "{declared}"')
    if digest:
        lines.append(f'digest = "{content_digest(devices_root)}"')
    section = "\n".join(lines) + "\n"

    text = toml_path.read_text()
    if _SECTION_RE.search(text):
        new_text = _SECTION_RE.sub(lambda _: section, text, count=1)
    else:
        new_text = text.rstrip("\n") + "\n\n" + section
    toml_path.write_text(new_text)
    return section
