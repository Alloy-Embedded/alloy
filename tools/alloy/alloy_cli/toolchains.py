"""`alloy setup` — verify and install toolchains into ~/.alloy/tools.

PATH-first doctrine: a tool found on PATH (or already under ~/.alloy/tools)
is used as-is; downloads happen only for missing archive-kind tools, from
the pinned manifest (toolchains.json) with sha256 verification when a
digest is pinned. The computed digest is always printed so unpinned entries
can be pinned after their first verified download. `--json-progress`
streams NDJSON events for IDE integration.
"""

from __future__ import annotations

import hashlib
import json
import platform
import shutil
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path
from typing import Any

from .emit.common import EmitError

TOOLS_DIR = Path.home() / ".alloy" / "tools"


def _manifest() -> dict[str, Any]:
    return json.loads((Path(__file__).parent / "toolchains.json").read_text())


def _platform_key() -> str:
    os_name = {"darwin": "darwin", "linux": "linux", "win32": "windows"}.get(sys.platform)
    machine = platform.machine().lower()
    arch = {"arm64": "arm64", "aarch64": "aarch64" if os_name == "linux" else "arm64",
            "x86_64": "x86_64", "amd64": "x86_64"}.get(machine, machine)
    return f"{os_name}-{arch}"


def _installed_bin(tool: str, spec: dict[str, Any]) -> Path | None:
    check = spec["check"]
    exe = check + (".exe" if sys.platform == "win32" else "")
    local = TOOLS_DIR / tool / spec.get("bin_hint", "bin") / exe
    if local.exists():
        return local
    # Historic layout (xtensa was installed as ~/.alloy/tools/<strip_prefix>/bin).
    for candidate in TOOLS_DIR.glob(f"*/bin/{exe}"):
        return candidate
    return None


def check_status(families: set[str] | None = None) -> list[dict[str, Any]]:
    manifest = _manifest()
    plat = _platform_key()
    rows: list[dict[str, Any]] = []
    for name, spec in manifest["tools"].items():
        fams = set(spec["families"])
        if families and "*" not in fams and not (fams & families):
            continue
        on_path = shutil.which(spec["check"])
        local = None if on_path else _installed_bin(name, spec)
        row: dict[str, Any] = {
            "tool": name,
            "check": spec["check"],
            "kind": spec["kind"],
            "families": sorted(fams),
            "status": "path" if on_path else "installed" if local else "missing",
            "path": on_path or (str(local) if local else None),
        }
        if row["status"] == "missing":
            if spec["kind"] == "system":
                os_key = {"darwin": "darwin", "linux": "linux", "win32": "windows"}[sys.platform]
                row["remedy"] = spec["remedy"].get(os_key, "install manually")
            else:
                plat_spec = spec.get("platforms", {}).get(plat)
                row["installable"] = plat_spec is not None
                if plat_spec is None:
                    row["remedy"] = f"no {plat} archive in the manifest — install manually"
        rows.append(row)
    return rows


def _emit(event: dict[str, Any], json_progress: bool) -> None:
    if json_progress:
        print(json.dumps(event), flush=True)
    else:
        kind = event["event"]
        if kind == "download":
            pct = event.get("pct")
            end = "\n" if pct == 100 else "\r"
            print(f"  {event['tool']}: downloading {pct or 0:3d}%", end=end, flush=True)
        elif kind == "extract":
            print(f"  {event['tool']}: extracting…", flush=True)
        elif kind == "done":
            print(f"  {event['tool']}: installed -> {event['path']}", flush=True)
        elif kind == "sha256":
            print(f"  {event['tool']}: sha256 {event['digest']}"
                  + ("" if event["pinned"] else "  (UNPINNED — add to toolchains.json)"),
                  flush=True)


def install_tool(name: str, json_progress: bool = False) -> Path:
    manifest = _manifest()
    spec = manifest["tools"].get(name)
    if spec is None:
        raise EmitError(f"unknown tool '{name}' (manifest: {', '.join(manifest['tools'])})")
    if spec["kind"] == "system":
        raise EmitError(f"{name} is system-managed — {spec['remedy']}")
    plat = _platform_key()
    plat_spec = spec.get("platforms", {}).get(plat)
    if plat_spec is None:
        raise EmitError(f"{name}: no archive for {plat} in the manifest")

    url = plat_spec["url"]
    dest = TOOLS_DIR / name
    dest.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        archive = Path(tmp) / url.rsplit("/", 1)[-1]
        digest = hashlib.sha256()
        req = urllib.request.Request(url, headers={"User-Agent": "alloy-setup"})
        with urllib.request.urlopen(req) as resp, open(archive, "wb") as out:
            total = int(resp.headers.get("Content-Length") or 0)
            got = 0
            last_pct = -1
            while chunk := resp.read(1 << 20):
                out.write(chunk)
                digest.update(chunk)
                got += len(chunk)
                pct = int(got * 100 / total) if total else None
                if pct is not None and pct != last_pct:
                    last_pct = pct
                    _emit({"event": "download", "tool": name, "pct": pct}, json_progress)

        hexdigest = digest.hexdigest()
        pinned = plat_spec.get("sha256")
        _emit({"event": "sha256", "tool": name, "digest": hexdigest,
               "pinned": pinned is not None}, json_progress)
        if pinned is not None and pinned != hexdigest:
            raise EmitError(
                f"{name}: sha256 mismatch (expected {pinned}, got {hexdigest}) — refusing"
            )

        _emit({"event": "extract", "tool": name}, json_progress)
        extract_dir = Path(tmp) / "extract"
        extract_dir.mkdir()
        if archive.name.endswith(".zip"):
            with zipfile.ZipFile(archive) as zf:
                zf.extractall(extract_dir)
        else:
            with tarfile.open(archive) as tf:
                tf.extractall(extract_dir, filter="tar")

        strip = plat_spec.get("strip_prefix")
        src_root = extract_dir / strip if strip and (extract_dir / strip).is_dir() \
            else next(p for p in extract_dir.iterdir() if p.is_dir())
        if dest.exists():
            shutil.rmtree(dest)
        shutil.move(str(src_root), str(dest))

    _emit({"event": "done", "tool": name, "path": str(dest)}, json_progress)
    return dest


def setup(families: set[str] | None, check_only: bool,
          json_out: bool, json_progress: bool) -> int:
    rows = check_status(families)
    if check_only or all(r["status"] != "missing" for r in rows):
        if json_out:
            print(json.dumps({"schema": "alloy.setup.v1", "platform": _platform_key(),
                              "tools": rows}, indent=2))
        else:
            for r in rows:
                mark = {"path": "ok (PATH)", "installed": "ok (~/.alloy/tools)",
                        "missing": "MISSING"}[r["status"]]
                extra = f" — {r.get('remedy')}" if r.get("remedy") else ""
                print(f"{r['tool']:20} {mark:24} {r.get('path') or ''}{extra}")
        return 0 if all(r["status"] != "missing" for r in rows) else 1

    failures = 0
    for r in rows:
        if r["status"] != "missing":
            continue
        if r["kind"] == "system":
            print(f"{r['tool']}: system-managed — {r['remedy']}", file=sys.stderr)
            failures += 1
            continue
        if not r.get("installable"):
            print(f"{r['tool']}: {r['remedy']}", file=sys.stderr)
            failures += 1
            continue
        install_tool(r["tool"], json_progress=json_progress)
    return 1 if failures else 0
