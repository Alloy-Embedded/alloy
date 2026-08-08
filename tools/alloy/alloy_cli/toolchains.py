"""`alloy setup` — verify and install toolchains into ~/.alloy/tools.

PATH-first doctrine: a tool found on PATH (or already under ~/.alloy/tools)
is used as-is; downloads happen only for missing archive-kind tools, from
the pinned manifest (toolchains.json) with sha256 verification when a
digest is pinned. The computed digest is always printed so unpinned entries
can be pinned after their first verified download. `--json-progress`
streams NDJSON events for IDE integration.

AIR-GAPPED INSTALLS. A corporate build server usually has no route to
developer.arm.com. The archive an install needs is content-addressed by the
manifest's sha256, so WHERE the bytes come from is not a security property —
only the digest is. `--from <dir>` (or ALLOY_TOOLS_MIRROR) therefore takes the
same archives from a local directory and runs them through the same
verification; `--offline` additionally refuses to reach the network at all, so
a build server cannot silently "work" by phoning home. The mirror is populated
on a connected machine with `alloy setup --fetch <dir> [--platform <key>]`,
which downloads, verifies and lays the archives out under their manifest file
names — including for a platform other than the one doing the fetching.
"""

from __future__ import annotations

import hashlib
import json
import os
import platform
import shutil
import ssl
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path
from typing import Any

import certifi

from .emit.common import EmitError

TOOLS_DIR = Path.home() / ".alloy" / "tools"


def _manifest() -> dict[str, Any]:
    return json.loads((Path(__file__).parent / "toolchains.json").read_text())


def mirror_dir(explicit: str | None = None) -> Path | None:
    """The local archive mirror: --from wins, then ALLOY_TOOLS_MIRROR, else none.

    A missing directory is an error rather than a silent fallback to the
    network — an air-gapped operator who typo'd the path must hear about it
    here, not three minutes into a download that cannot happen.
    """
    raw = explicit or os.environ.get("ALLOY_TOOLS_MIRROR")
    if not raw:
        return None
    path = Path(raw).expanduser()
    if not path.is_dir():
        raise EmitError(f"toolchain mirror {path} is not a directory")
    return path


def mirror_name(url: str) -> str:
    """The file name an archive has inside a mirror: its manifest URL basename."""
    return url.rsplit("/", 1)[-1]


_OFFLINE_HELP = (
    "no toolchain mirror configured. On a machine with network access run\n"
    "  alloy setup --fetch /media/mirror --platform {plat}\n"
    "copy that directory to this host, then\n"
    "  alloy setup --from /media/mirror --offline\n"
    "(or export ALLOY_TOOLS_MIRROR=/media/mirror). The archives are verified\n"
    "against the same pinned sha256 either way."
)


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
        elif kind == "mirror":
            print(f"  {event['tool']}: from mirror {event['path']}", flush=True)
        elif kind == "sha256":
            print(f"  {event['tool']}: sha256 {event['digest']}"
                  + ("" if event["pinned"] else "  (UNPINNED — add to toolchains.json)"),
                  flush=True)


def _download(name: str, url: str, archive: Path, json_progress: bool) -> str:
    """Fetch `url` into `archive`, returning its sha256. Network required."""
    digest = hashlib.sha256()
    req = urllib.request.Request(url, headers={"User-Agent": "alloy-setup"})
    # Verify TLS against certifi's CA bundle, not the interpreter's default
    # store — a stock macOS Python has no system trust roots, so a plain
    # urlopen fails every HTTPS download with CERTIFICATE_VERIFY_FAILED.
    ctx = ssl.create_default_context(cafile=certifi.where())
    try:
        with urllib.request.urlopen(req, context=ctx) as resp, open(archive, "wb") as out:  # noqa: S310
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
    except OSError as exc:
        # No network is the NORMAL state on a build server, not an anomaly — so
        # the failure has to teach the way out rather than print a socket error.
        raise EmitError(
            f"{name}: cannot reach {url} ({exc}), and "
            + _OFFLINE_HELP.format(plat=_platform_key())
        ) from exc
    return digest.hexdigest()


def _from_mirror(name: str, url: str, archive: Path, mirror: Path,
                 json_progress: bool) -> str:
    """Copy the archive out of a local mirror, returning its sha256."""
    src = mirror / mirror_name(url)
    if not src.is_file():
        raise EmitError(
            f"{name}: {src} is not in the mirror. Populate it on a connected "
            f"machine with `alloy setup --fetch {mirror} --platform {_platform_key()}`."
        )
    _emit({"event": "mirror", "tool": name, "path": str(src)}, json_progress)
    digest = hashlib.sha256()
    with open(src, "rb") as fh, open(archive, "wb") as out:
        while chunk := fh.read(1 << 20):
            out.write(chunk)
            digest.update(chunk)
    return digest.hexdigest()


def install_tool(name: str, json_progress: bool = False,
                 mirror: Path | None = None, offline: bool = False) -> Path:
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
        archive = Path(tmp) / mirror_name(url)
        if mirror is not None:
            hexdigest = _from_mirror(name, url, archive, mirror, json_progress)
        elif offline:
            raise EmitError(
                f"{name}: --offline was given but " + _OFFLINE_HELP.format(plat=plat)
            )
        else:
            hexdigest = _download(name, url, archive, json_progress)
        pinned = plat_spec.get("sha256")
        _emit({"event": "sha256", "tool": name, "digest": hexdigest,
               "pinned": pinned is not None}, json_progress)
        if pinned is None and os.environ.get("ALLOY_ALLOW_UNPINNED") != "1":
            raise EmitError(
                f"{name}: sha256 not pinned (computed {hexdigest}). Refusing to install an "
                f"unverified toolchain — pin this digest in toolchains.json, or set "
                f"ALLOY_ALLOW_UNPINNED=1 to override for a one-off."
            )
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


def platform_keys() -> list[str]:
    """Every platform key the manifest carries an archive for."""
    keys: set[str] = set()
    for spec in _manifest()["tools"].values():
        keys |= set(spec.get("platforms", {}))
    return sorted(keys)


def fetch(dest: Path, platforms: list[str], families: set[str] | None,
          json_progress: bool) -> int:
    """Populate a mirror: download every archive-kind tool for `platforms`,
    verify it against the pinned digest, and lay it out under its manifest file
    name so `--from <dest>` finds it.

    Run on a machine WITH network, for the platform of a machine without —
    hence the explicit --platform: the air-gapped build server is usually
    linux-x86_64 while the laptop populating the USB stick is not. An archive
    already present with the right digest is left alone, so a mirror can be
    topped up over a slow link without re-downloading gigabytes.
    """
    known = platform_keys()
    for plat in platforms:
        if plat not in known:
            raise EmitError(f"unknown platform '{plat}' — manifest has {', '.join(known)}")
    dest.mkdir(parents=True, exist_ok=True)
    manifest = _manifest()
    wanted: list[tuple[str, str, dict[str, Any]]] = []
    for name, spec in manifest["tools"].items():
        if spec.get("kind") != "archive":
            continue
        fams = set(spec["families"])
        if families and "*" not in fams and not (fams & families):
            continue
        for plat in platforms:
            plat_spec = spec.get("platforms", {}).get(plat)
            if plat_spec is not None:
                wanted.append((name, plat, plat_spec))

    if not wanted:
        raise EmitError("nothing to fetch — no archive-kind tool matched")

    failures = 0
    for name, plat, plat_spec in wanted:
        url = plat_spec["url"]
        pinned = plat_spec.get("sha256")
        target = dest / mirror_name(url)
        label = f"{name}/{plat}"
        if target.is_file() and pinned is not None:
            have = hashlib.sha256()
            with open(target, "rb") as fh:
                while chunk := fh.read(1 << 20):
                    have.update(chunk)
            if have.hexdigest() == pinned:
                print(f"  {label}: already mirrored -> {target.name}", flush=True)
                continue
            print(f"  {label}: mirrored copy has the wrong digest — refetching",
                  file=sys.stderr, flush=True)
        with tempfile.TemporaryDirectory() as tmp:
            staged = Path(tmp) / target.name
            try:
                hexdigest = _download(label, url, staged, json_progress)
            except EmitError as exc:
                print(f"  {label}: {exc}", file=sys.stderr, flush=True)
                failures += 1
                continue
            if pinned is None and os.environ.get("ALLOY_ALLOW_UNPINNED") != "1":
                print(f"  {label}: sha256 not pinned (computed {hexdigest}) — not mirrored",
                      file=sys.stderr, flush=True)
                failures += 1
                continue
            if pinned is not None and pinned != hexdigest:
                print(f"  {label}: sha256 mismatch (expected {pinned}, got {hexdigest})",
                      file=sys.stderr, flush=True)
                failures += 1
                continue
            shutil.move(str(staged), str(target))
        print(f"  {label}: mirrored -> {target.name}  sha256 {hexdigest}", flush=True)

    print(f"mirror at {dest} — use it with `alloy setup --from {dest} --offline`")
    return 1 if failures else 0


def setup(families: set[str] | None, check_only: bool,
          json_out: bool, json_progress: bool,
          mirror: Path | None = None, offline: bool = False) -> int:
    rows = check_status(families)
    if mirror is not None:
        # An air-gapped operator has to be able to VERIFY the mirror before the
        # build server needs it, so say per missing tool whether the archive is
        # actually there — not just that a directory was named.
        manifest = _manifest()
        plat = _platform_key()
        for row in rows:
            if row["status"] != "missing" or not row.get("installable"):
                continue
            spec = manifest["tools"][row["tool"]].get("platforms", {}).get(plat, {})
            row["mirrored"] = (mirror / mirror_name(spec["url"])).is_file()
            if not row["mirrored"]:
                row["remedy"] = f"not in mirror {mirror} ({mirror_name(spec['url'])})"
    if check_only or all(r["status"] != "missing" for r in rows):
        if json_out:
            print(json.dumps({"schema": "alloy.setup.v1", "platform": _platform_key(),
                              "tools": rows}, indent=2))
        else:
            for r in rows:
                mark = {"path": "ok (PATH)", "installed": "ok (~/.alloy/tools)",
                        "missing": "MISSING"}[r["status"]]
                extra = f" — {r.get('remedy')}" if r.get("remedy") else ""
                if not extra and r.get("mirrored"):
                    # An air-gapped operator is checking the mirror, not the
                    # host: silence would read as "not checked".
                    extra = " — archive present in the mirror"
                print(f"{r['tool']:20} {mark:24} {r.get('path') or ''}{extra}")
        return 0 if all(r["status"] != "missing" for r in rows) else 1

    failures = 0
    for r in rows:
        if r["status"] != "missing":
            continue
        if r["kind"] == "system":
            # cmake and ninja are the one thing alloy cannot mirror: they are
            # whatever the OS image provides. Say so rather than printing a
            # `brew install` an air-gapped host can never run.
            note = " (alloy cannot mirror a system package — bake it into the " \
                "build image)" if offline or mirror is not None else ""
            print(f"{r['tool']}: system-managed — {r['remedy']}{note}", file=sys.stderr)
            failures += 1
            continue
        if not r.get("installable"):
            print(f"{r['tool']}: {r['remedy']}", file=sys.stderr)
            failures += 1
            continue
        # One unreachable archive must not hide the next: an air-gapped
        # operator wants the FULL list of what their mirror is missing, in one
        # run, not one name per trip to the sneakernet.
        try:
            install_tool(r["tool"], json_progress=json_progress,
                         mirror=mirror, offline=offline)
        except EmitError as exc:
            print(str(exc), file=sys.stderr)  # already names the tool
            failures += 1
    return 1 if failures else 0
