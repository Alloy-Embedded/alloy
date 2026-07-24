"""`alloy lib` — discover and vendor ecosystem libraries.

A library is a self-contained directory (manifest + concept-templated header +
host test) that names no chip, only alloy's horizontal concepts. The registry
(libs/registry.toml in the framework) is a curated index; `add` copies the
library into the project's ./libs/<name> and records it in alloy.toml so the
build picks up its include dir. Seed sources are in-tree paths; external
libraries use git: sources with the identical manifest shape.
"""

from __future__ import annotations

import shutil
import sys
import tomllib
from pathlib import Path
from typing import Any

from .project import ProjectError, _find_alloy_root


def _registry(alloy_root: Path) -> dict[str, Any]:
    path = alloy_root / "libs" / "registry.toml"
    if not path.exists():
        raise ProjectError(f"no library registry at {path}")
    data = tomllib.loads(path.read_text())
    if data.get("schema") != "alloy.registry.v1":
        raise ProjectError(f"{path}: expected schema alloy.registry.v1")
    return {name: spec for name, spec in data.items() if name != "schema"}


def _entries(alloy_root: Path) -> list[tuple[str, dict[str, Any]]]:
    return sorted(_registry(alloy_root).items())


def cmd_lib_list(alloy_root: Path, *, category: str | None = None) -> int:
    rows = _entries(alloy_root)
    if category:
        rows = [(n, s) for n, s in rows if s.get("category") == category]
    if not rows:
        print("no libraries in the registry" + (f" for category '{category}'" if category else ""))
        return 0
    width = max(len(n) for n, _ in rows)
    for name, spec in rows:
        print(f"{name:{width}}  {spec.get('category', '?'):8}  {spec.get('summary', '')}")
    return 0


def cmd_lib_search(alloy_root: Path, term: str) -> int:
    t = term.lower()
    rows = [
        (n, s)
        for n, s in _entries(alloy_root)
        if t in n.lower()
        or t in s.get("summary", "").lower()
        or t in s.get("category", "").lower()
    ]
    if not rows:
        print(f"no library matches '{term}'")
        return 1
    width = max(len(n) for n, _ in rows)
    for name, spec in rows:
        print(f"{name:{width}}  {spec.get('category', '?'):8}  {spec.get('summary', '')}")
    return 0


def cmd_lib_info(alloy_root: Path, name: str) -> int:
    reg = _registry(alloy_root)
    if name not in reg:
        print(f"error: no library '{name}' in the registry (try `alloy lib list`)", file=sys.stderr)
        return 1
    spec = reg[name]
    print(f"{name}  {spec.get('version', '?')}")
    print(f"  category: {spec.get('category', '?')}")
    print(f"  summary:  {spec.get('summary', '')}")
    print(f"  concepts: {', '.join(spec.get('concepts', []))}")
    print(f"  source:   {spec.get('source', '?')}")
    return 0


def _resolve_source(alloy_root: Path, name: str, source: str) -> Path:
    """Return a local directory holding the library named `name`."""
    kind, _, ref = source.partition(":")
    if kind == "path":
        src = (alloy_root / ref).resolve()
        if not (src / "alloy.lib.toml").exists():
            raise ProjectError(f"library source {src} has no alloy.lib.toml")
        return src
    if kind == "git":
        raise ProjectError(
            f"git sources are not fetched yet (v1 seed): clone {ref} into ./libs/{name} manually"
        )
    raise ProjectError(f"unknown source kind '{kind}' for library '{name}'")


def _record_in_toml(toml_path: Path, name: str, version: str) -> None:
    """Add `name = "version"` under [libs] in alloy.toml (create the table if absent)."""
    text = toml_path.read_text()
    line = f'{name} = "{version}"'
    if line in text:
        return
    if "[libs]" in text:
        out = []
        for ln in text.splitlines():
            out.append(ln)
            if ln.strip() == "[libs]":
                out.append(line)
        toml_path.write_text("\n".join(out) + "\n")
    else:
        sep = "" if text.endswith("\n") else "\n"
        toml_path.write_text(f"{text}{sep}\n[libs]\n{line}\n")


def cmd_lib_add(project_dir: Path, name: str) -> int:
    root = project_dir.resolve()
    toml_path = root / "alloy.toml"
    if not toml_path.exists():
        print(f"error: {root} is not an alloy project (no alloy.toml)", file=sys.stderr)
        return 1
    alloy_root = _find_alloy_root(root)
    reg = _registry(alloy_root)
    if name not in reg:
        print(f"error: no library '{name}' in the registry (try `alloy lib list`)", file=sys.stderr)
        return 1
    spec = reg[name]
    src = _resolve_source(alloy_root, name, spec.get("source", ""))

    dest = root / "libs" / name
    if dest.exists():
        print(f"{name} already vendored at {dest} — updating")
        shutil.rmtree(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(src, dest)

    _record_in_toml(toml_path, name, str(spec.get("version", "0.0.0")))
    print(f"added {name} {spec.get('version', '')} -> {dest}")
    print(f"  include it: #include <{name}.hpp>   (namespace alloy::lib)")
    print("  the build now compiles ./libs/*/include onto the include path")
    return 0
