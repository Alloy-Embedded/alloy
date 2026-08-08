"""`alloy sbom` — what is actually IN this firmware image.

The thing legal asks for in the first meeting, and the thing a hand-kept
inventory always gets wrong: an inventory is edited by whoever remembers, and
the day somebody vendors a new tree it silently stops being true. So nothing
here is a list. Every component is DERIVED:

  compiled  build.build_inputs() — the same seam the compiler is driven from.
            littlefs appears iff the board has an fs role; monocypher iff the
            project configured an OTA key; lwIP iff the sources include the
            facade AND the board has a MAC. Stop compiling one and it stops
            being reported, with no second edit.
  generated build.generated_sources() — board.cpp / vector_table.c / boot2.c,
            attributed to the device database that emitted them.
  linked    the linker's own map file. `-Wl,-Map` opens with "Archive member
            included to satisfy reference by file", which is the ground truth
            for the toolchain runtime: on a plain blink only libgcc.a
            contributes, even though libc/libm/libstdc++ were all searched.
            This is why the SBOM needs a completed build, not just codegen.

SCOPE, stated once and repeated in every output: this describes the FIRMWARE
IMAGE. The Python side of alloy (pyyaml, jsonschema, cryptography, …) is a
build-host dependency, ships in no device, and is not covered here — `uv export`
or `pip-audit` against tools/alloy is the right tool for that question.

A source file that belongs to no known package is NOT dropped. It is reported
as a component with an undeclared licence (with the nearest licence file found
by walking up the tree), and `--strict` turns that into a non-zero exit. That
is the forgetting-proof property: a newly vendored tree is loud, not absent.
"""

from __future__ import annotations

import hashlib
import json
import os
import re
import tomllib
from dataclasses import asdict, dataclass, field
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

from .build import build_inputs, generated_sources
from .emit.common import EmitError
from .project import Project

SCHEMA = "alloy.sbom.v1"

_LICENSE_FILE_NAMES = ("LICENSE", "LICENCE", "COPYING", "NOTICE")


@dataclass
class Component:
    """One thing that ends up in (or is compiled into) the image."""

    name: str
    kind: str                    # application|framework|vendored|device-data|toolchain-runtime|blob|library
    version: str | None = None
    version_evidence: str | None = None
    license: str | None = None   # SPDX expression where it is actually known
    license_evidence: str | None = None
    license_file: str | None = None
    origin: str | None = None
    evidence: str = ""           # why it is in THIS image
    files: list[str] = field(default_factory=list)

    @property
    def declared(self) -> bool:
        # The application is the user's OWN code: alloy has no business calling
        # its licence undeclared, and --strict is about third-party obligations.
        return self.license is not None or self.kind == "application"


# --------------------------------------------------------------------------
# Version readers. Each one reads the tree, so a vendor bump moves the SBOM
# without anyone editing it. Where a tree records no version, the reader
# returns None and the report says "unrecorded" rather than inventing one.
# --------------------------------------------------------------------------

def _lwip_version(root: Path) -> tuple[str | None, str | None]:
    header = root / "src" / "include" / "lwip" / "init.h"
    if not header.is_file():
        return None, None
    text = header.read_text(errors="ignore")
    parts = []
    for macro in ("LWIP_VERSION_MAJOR", "LWIP_VERSION_MINOR", "LWIP_VERSION_REVISION"):
        match = re.search(rf"^#define\s+{macro}\s+(\d+)", text, re.MULTILINE)
        if match is None:
            return None, None
        parts.append(match.group(1))
    return ".".join(parts), "src/include/lwip/init.h (LWIP_VERSION_*)"


def _littlefs_version(root: Path) -> tuple[str | None, str | None]:
    header = root / "lfs.h"
    if not header.is_file():
        return None, None
    match = re.search(r"^#define\s+LFS_VERSION\s+0x([0-9a-fA-F]{8})", header.read_text(errors="ignore"),
                      re.MULTILINE)
    if match is None:
        return None, None
    raw = int(match.group(1), 16)
    return f"{raw >> 16}.{raw & 0xFFFF}", "lfs.h (LFS_VERSION)"


def _monocypher_version(root: Path) -> tuple[str | None, str | None]:
    # Monocypher's own headers say "version __git__" in a vendored copy, so the
    # only honest source is the vendoring note we wrote when we took it.
    readme = root / "README.md"
    if not readme.is_file():
        return None, None
    match = re.search(r"^\|\s*Version\s*\|\s*([^\s|]+)\s*\|", readme.read_text(errors="ignore"),
                      re.MULTILINE)
    if match is None:
        return None, None
    return match.group(1), "README.md (vendoring note — the upstream headers carry no version)"


def _pyproject_version(root: Path) -> tuple[str | None, str | None]:
    """The version of a checkout, from its pyproject.toml.

    NOT from a module's __version__ constant: alloy-devices ships
    `alloy_devices.__version__ = "0.1.0"` while its pyproject says 0.3.0, and an
    SBOM that quietly reports the stale one is worse than one that reports
    nothing. pyproject is what the release workflow asserts against the tag, so
    it is the number that means something.
    """
    pyproject = root / "pyproject.toml"
    if not pyproject.is_file():
        # A wheel install has no pyproject; fall back to the installed metadata.
        from importlib.metadata import PackageNotFoundError, version  # noqa: PLC0415
        try:
            return version("alloy-devices"), "installed alloy-devices distribution metadata"
        except PackageNotFoundError:
            return None, None
    data = tomllib.loads(pyproject.read_text()).get("project", {})
    found = data.get("version")
    return (found, f"{pyproject} ([project] version)") if found else (None, None)


@dataclass(frozen=True)
class _Vendored:
    """A third-party tree alloy carries in its own repository."""

    rel: str
    name: str
    license: str
    license_file: str
    origin: str
    version: Any = None  # callable(root) -> (version, evidence)


_VENDORED = (
    _Vendored("src/alloy/net/vendor/lwip", "lwIP", "BSD-3-Clause",
              "src/alloy/net/vendor/lwip/COPYING", "https://savannah.nongnu.org/projects/lwip/",
              _lwip_version),
    _Vendored("src/alloy/fs/vendor", "littlefs", "BSD-3-Clause",
              "src/alloy/fs/vendor/LICENSE.md", "https://github.com/littlefs-project/littlefs",
              _littlefs_version),
    _Vendored("third_party/monocypher", "Monocypher", "BSD-2-Clause OR CC0-1.0",
              "third_party/monocypher/LICENCE.md", "https://monocypher.org/",
              _monocypher_version),
)

# Archive basename -> (component name, SPDX licence, upstream). The link is the
# evidence that these are in the image; the licence is a property of the
# toolchain, so it is only asserted for the archives whose licence is a matter
# of public record for every GCC/newlib bare-metal toolchain.
_RUNTIME_ARCHIVES = {
    "libgcc": ("GCC low-level runtime library (libgcc)",
               "GPL-3.0-or-later WITH GCC-exception-3.1",
               "https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html"),
    "libstdc++": ("GNU libstdc++", "GPL-3.0-or-later WITH GCC-exception-3.1",
                  "https://gcc.gnu.org/onlinedocs/libstdc++/"),
    "libstdc++_nano": ("GNU libstdc++ (nano)", "GPL-3.0-or-later WITH GCC-exception-3.1",
                       "https://gcc.gnu.org/onlinedocs/libstdc++/"),
    "libc": ("newlib C library", None, "https://sourceware.org/newlib/"),
    "libc_nano": ("newlib C library (nano)", None, "https://sourceware.org/newlib/"),
    "libm": ("newlib math library", None, "https://sourceware.org/newlib/"),
    "libnosys": ("newlib syscall stubs (nosys)", None, "https://sourceware.org/newlib/"),
    "libg": ("newlib C library (debug)", None, "https://sourceware.org/newlib/"),
    "libg_nano": ("newlib C library (debug, nano)", None, "https://sourceware.org/newlib/"),
}


def _nearest_license_file(start: Path, stop: Path) -> Path | None:
    """Walk up from `start` to `stop` looking for a licence file.

    This is what catches a tree nobody declared: the file is still attributed
    to whatever directory carries its licence, so the SBOM names the licence
    FILE even when it cannot name the licence.
    """
    current = start if start.is_dir() else start.parent
    while True:
        for name in _LICENSE_FILE_NAMES:
            for candidate in sorted(current.glob(f"{name}*")):
                if candidate.is_file():
                    return candidate
        if current == stop or current.parent == current:
            return None
        current = current.parent


def linked_archives(map_file: Path) -> dict[str, list[str]]:
    """Archives that actually contributed members, from the linker map.

    The map opens with `Archive member included to satisfy reference by file`
    and each entry is `<archive>(<member>)` at column 0. An archive merely
    SEARCHED (libc, libm, libstdc++ on most alloy images) never appears here —
    which is the whole point: this is what linked, not what was offered.
    """
    if not map_file.is_file():
        return {}
    found: dict[str, list[str]] = {}
    started = False
    for line in map_file.read_text(errors="ignore").splitlines():
        if not started:
            started = line.startswith("Archive member included")
            continue
        if not line.strip():
            continue
        if line[0].isspace():
            continue  # the "referenced by" continuation line
        match = re.match(r"^(?P<archive>\S+\.a)\((?P<member>[^)]+)\)\s*$", line)
        if match is None:
            break  # first non-entry at column 0 ends the section
        found.setdefault(match.group("archive"), []).append(match.group("member"))
    return {a: sorted(set(m)) for a, m in sorted(found.items())}


def _toolchain_license_file(archive: Path, package: str) -> str | None:
    """Where this toolchain keeps the package's licence, if it ships one.

    Toolchain layouts differ (xPack keeps distro-info/licenses/<pkg>-<ver>, Arm's
    own release ships share/doc). Look in both and report the path found, or
    nothing — never a guess.
    """
    # The map spells archives through the compiler's relative lookup path
    # (…/bin/../lib/gcc/…), so normalise before walking up or the parents are
    # nonsense.
    root = Path(os.path.normpath(archive))
    for _ in range(12):
        root = root.parent
        if root.parent == root:
            return None
        for pattern in (f"distro-info/licenses/{package}-*", f"share/doc/{package}*",
                        f"distro-info/licenses/{package}"):
            for hit in sorted(root.glob(pattern)):
                return str(hit)
    return None


def _runtime_components(map_file: Path) -> list[Component]:
    out: list[Component] = []
    for archive, members in linked_archives(map_file).items():
        path = Path(archive)
        stem = path.name[: -len(".a")]
        known = _RUNTIME_ARCHIVES.get(stem)
        name, spdx, origin = known if known else (
            f"toolchain archive {path.name}", None, None)
        package = "gcc" if stem.startswith(("libgcc", "libstdc++")) else "newlib"
        version, version_evidence = None, None
        license_file = _toolchain_license_file(path, package)
        if license_file:
            match = re.search(rf"{package}-([0-9][^/]*)$", license_file)
            if match:
                version, version_evidence = match.group(1), f"toolchain layout ({license_file})"
        out.append(Component(
            name=name,
            kind="toolchain-runtime",
            version=version,
            version_evidence=version_evidence,
            license=spdx,
            license_evidence=("well-known licence of this component in every GCC "
                              "bare-metal toolchain") if spdx else None,
            license_file=license_file,
            origin=origin,
            evidence=f"linker pulled {len(members)} member(s) from {path.name}: "
                     + ", ".join(members[:6]) + (", …" if len(members) > 6 else ""),
        ))
    return out


def _library_components(project: Project) -> list[Component]:
    """Ecosystem libraries from [libs] in alloy.toml.

    Header-only, so they never appear in the compiled source list — but their
    code is inlined into the image all the same, and their manifests already
    declare a licence and a version. Reported from the manifest of the copy the
    build actually put on the include path.
    """
    toml_path = project.root / "alloy.toml"
    if not toml_path.exists():
        return []
    names = tomllib.loads(toml_path.read_text()).get("libs", {})
    out: list[Component] = []
    for name in sorted(names):
        for base in (project.root / "libs" / name, project.alloy_root / "libs" / name):
            manifest = base / "alloy.lib.toml"
            if not manifest.exists():
                continue
            lib = tomllib.loads(manifest.read_text()).get("lib", {})
            out.append(Component(
                name=lib.get("name", name),
                kind="library",
                version=lib.get("version"),
                version_evidence=f"{manifest.name} ([lib] version)" if lib.get("version") else None,
                license=lib.get("license"),
                license_evidence=f"{manifest.name} ([lib] license)" if lib.get("license") else None,
                license_file=str(manifest),
                origin=lib.get("homepage"),
                evidence=f"[libs] in alloy.toml — headers from {base} on the include path",
            ))
            break
    return out


def components(project: Project, chip: dict[str, Any]) -> list[Component]:
    """Every component of this configuration's firmware, derived from the build."""
    alloy_root = project.alloy_root
    inputs = build_inputs(project, chip)

    def rel(path: Path) -> str:
        for base, label in ((project.root, "<project>"), (alloy_root, "<alloy>")):
            try:
                return f"{label}/{path.relative_to(base)}"
            except ValueError:
                continue
        return str(path)

    buckets: dict[str, list[Path]] = {}
    vendored_roots = [(alloy_root / spec.rel, spec) for spec in _VENDORED]

    unknown: dict[Path, list[Path]] = {}
    framework: list[Path] = []
    for path in sorted(inputs.compiled()):
        for root, spec in sorted(vendored_roots, key=lambda pair: len(str(pair[0])), reverse=True):
            if path.is_relative_to(root):
                buckets.setdefault(spec.name, []).append(path)
                break
        else:
            if path.is_relative_to(project.root / "src"):
                continue  # the application, reported separately
            if path.is_relative_to(alloy_root / "src"):
                framework.append(path)
            else:
                # Compiled, but from no tree alloy knows about. Attribute it to
                # the nearest directory that carries a licence file, so a newly
                # vendored package cannot be silently absent.
                anchor = _nearest_license_file(path, alloy_root)
                unknown.setdefault(anchor.parent if anchor else path.parent, []).append(path)

    # A project may declare its own licence in alloy.toml; alloy never guesses one.
    app_toml = project.root / "alloy.toml"
    app_license = None
    if app_toml.exists():
        app_license = tomllib.loads(app_toml.read_text()).get("project", {}).get("license")
    out: list[Component] = [Component(
        name=project.name,
        kind="application",
        license=app_license,
        license_evidence="alloy.toml ([project] license)" if app_license else None,
        evidence=f"the project's own sources under {rel(project.root / 'src')}",
        files=[rel(p) for p in sorted(inputs.app)],
    )]

    from . import __version__  # noqa: PLC0415

    out.append(Component(
        name="alloy",
        kind="framework",
        version=__version__,
        version_evidence="alloy_cli.__version__",
        license="MIT",
        license_evidence="repository LICENSE",
        license_file=str(alloy_root / "LICENSE") if (alloy_root / "LICENSE").is_file() else None,
        origin="https://github.com/Alloy-Embedded/alloy",
        evidence="the framework runtime + arch backend compiled into every image",
        files=[rel(p) for p in framework],
    ))

    for spec in _VENDORED:
        files = buckets.get(spec.name)
        if not files:
            continue  # this configuration does not compile it — so it is not in the image
        root = alloy_root / spec.rel
        version, version_evidence = spec.version(root) if spec.version else (None, None)
        out.append(Component(
            name=spec.name,
            kind="vendored",
            version=version,
            version_evidence=version_evidence,
            license=spec.license,
            license_evidence=spec.license_file,
            license_file=str(alloy_root / spec.license_file)
            if (alloy_root / spec.license_file).is_file() else None,
            origin=spec.origin,
            evidence=f"{len(files)} vendored source file(s) on this build's compile line",
            files=[rel(p) for p in files],
        ))

    for anchor, files in sorted(unknown.items()):
        licence = _nearest_license_file(anchor, alloy_root)
        out.append(Component(
            name=anchor.name,
            kind="vendored",
            license=None,
            license_file=str(licence) if licence else None,
            evidence="compiled into the image but declared in no package definition "
                     "— add it to _VENDORED in sbom.py",
            files=[rel(p) for p in files],
        ))

    out += _generated_components(project, chip, rel)
    out += _library_components(project)
    out += _runtime_components(project.build_dir / "out" / f"{project.name}.map")
    return out


def _generated_components(project: Project, chip: dict[str, Any],
                          rel: Any) -> list[Component]:
    """Codegen output, attributed to the data it came from.

    board.cpp / vector_table.c / irq_data.c are alloy's own emission of
    alloy-devices facts (MIT + the database's licence). boot2.c is different in
    kind: it embeds a precompiled third-party BLOB, and the chip record carries
    that blob's licence — so it is reported as its own component, sourced from
    the database rather than from anything in this repo.
    """
    gen = generated_sources(project)
    out: list[Component] = []
    devices_version, devices_evidence = _pyproject_version(project.devices_root)
    out.append(Component(
        name="alloy-devices",
        kind="device-data",
        version=devices_version,
        version_evidence=devices_evidence,
        license="MIT",
        license_evidence="alloy-devices repository LICENSE",
        license_file=str(project.devices_root / "LICENSE")
        if (project.devices_root / "LICENSE").is_file() else None,
        origin="https://github.com/Alloy-Embedded/alloy-devices",
        evidence="register/pin/clock facts emitted into the generated sources",
        files=[rel(p) for p in gen],
    ))

    boot = chip.get("boot") or {}
    if any(p.name == "boot2.c" for p in gen) and boot.get("kind"):
        out.append(Component(
            name=f"{chip.get('part', chip.get('family', 'chip'))} {boot['kind']}",
            kind="blob",
            license=boot.get("license"),
            license_evidence=f"chips/{chip.get('vendor')}/{chip.get('family')}.yaml "
                             "(boot.license)" if boot.get("license") else None,
            evidence="precompiled boot stage embedded verbatim in the image "
                     "(generated boot2.c)",
            files=[rel(p) for p in gen if p.name == "boot2.c"],
        ))
    return out


# --------------------------------------------------------------------------
# Renderers. All deterministic: components come out sorted, and the one
# unavoidable timestamp honours SOURCE_DATE_EPOCH.
# --------------------------------------------------------------------------

def _created() -> tuple[str, bool]:
    epoch = os.environ.get("SOURCE_DATE_EPOCH")
    if epoch and epoch.isdigit():
        return datetime.fromtimestamp(int(epoch), UTC).strftime("%Y-%m-%dT%H:%M:%SZ"), True
    return datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ"), False


_SCOPE = ("Firmware image only. alloy's own Python build tooling and its "
          "dependencies ship in no device and are out of scope.")


def _ordered(items: list[Component]) -> list[Component]:
    order = {"application": 0, "framework": 1, "device-data": 2, "vendored": 3,
             "library": 4, "blob": 5, "toolchain-runtime": 6}
    return sorted(items, key=lambda c: (order.get(c.kind, 9), c.name.lower()))


def render_notice(project: Project, board_id: str, items: list[Component],
                  image: tuple[str, str] | None = None) -> str:
    lines = [
        f"NOTICE — third-party software in {project.name} ({board_id})",
        "=" * 72,
        "",
        "Generated by `alloy sbom`. Every entry below was derived from what this",
        "build actually compiles and links, not from a maintained list.",
        _SCOPE,
        "",
    ]
    if image:
        lines += [f"Image: {image[0]}  sha256 {image[1]}", ""]
    for comp in _ordered(items):
        head = comp.name + (f" {comp.version}" if comp.version else "")
        lines.append(head)
        lines.append("-" * len(head))
        lines.append(f"  kind      {comp.kind}")
        lines.append(f"  licence   {comp.license or 'NOT DECLARED — see licence file'}")
        if comp.license_file:
            lines.append(f"  licence text  {comp.license_file}")
        if comp.origin:
            lines.append(f"  upstream  {comp.origin}")
        if comp.version_evidence:
            lines.append(f"  version from  {comp.version_evidence}")
        lines.append(f"  in this image because  {comp.evidence}")
        lines.append("")
    undeclared = [c.name for c in items if not c.declared]
    if undeclared:
        lines.append("UNDECLARED LICENCES: " + ", ".join(sorted(undeclared)))
        lines.append("These components are in the image; their licence is not asserted "
                     "here. Read the licence file listed above before shipping.")
        lines.append("")
    return "\n".join(lines)


def _spdx_id(name: str, index: int) -> str:
    slug = re.sub(r"[^A-Za-z0-9.-]", "-", name)
    return f"SPDXRef-{index}-{slug}"


def render_spdx(project: Project, board_id: str, items: list[Component],
                image: tuple[str, str] | None = None) -> str:
    created, pinned = _created()
    ordered = _ordered(items)
    packages = []
    relationships = []
    root_id = _spdx_id(project.name, 0)
    for index, comp in enumerate(ordered):
        spdx_id = _spdx_id(comp.name, index + 1)
        packages.append({
            "SPDXID": spdx_id,
            "name": comp.name,
            "versionInfo": comp.version or "NOASSERTION",
            "downloadLocation": comp.origin or "NOASSERTION",
            "filesAnalyzed": False,
            "licenseConcluded": comp.license or "NOASSERTION",
            "licenseDeclared": comp.license or "NOASSERTION",
            "copyrightText": "NOASSERTION",
            "comment": comp.evidence,
            **({"attributionTexts": [f"licence text: {comp.license_file}"]}
               if comp.license_file else {}),
            # The application package carries the digest of the artefact this
            # document is ABOUT, so an SBOM cannot drift from its binary.
            **({"checksums": [{"algorithm": "SHA256", "checksumValue": image[1]}],
                "packageFileName": image[0]}
               if image and comp.kind == "application" else {}),
        })
        if index:
            relationships.append({
                "spdxElementId": root_id,
                "relatedSpdxElement": spdx_id,
                "relationshipType": "CONTAINS",
            })
    doc = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"{project.name}-{board_id}",
        # SPDX wants a namespace unique to THIS document. The image digest is
        # exactly that, and it is reproducible, so re-running the tool on the
        # same source yields the same namespace rather than a fresh UUID.
        "documentNamespace":
            f"https://alloy.invalid/sbom/{project.name}/{board_id}/"
            + (image[1] if image else "no-image"),
        "creationInfo": {
            "created": created,
            "creators": ["Tool: alloy-sbom"],
            "comment": _SCOPE + ("" if pinned else
                                 "  (created timestamp is wall-clock; set "
                                 "SOURCE_DATE_EPOCH for a reproducible document)"),
        },
        "packages": packages,
        "relationships": relationships,
    }
    return json.dumps(doc, indent=2, sort_keys=False) + "\n"


def render_cyclonedx(project: Project, board_id: str, items: list[Component],
                     image: tuple[str, str] | None = None) -> str:
    created, _pinned = _created()
    doc = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        "version": 1,
        "metadata": {
            "timestamp": created,
            "tools": [{"vendor": "alloy", "name": "alloy sbom"}],
            "component": {
                "type": "firmware", "name": project.name,
                "description": f"{project.name} for {board_id}. {_SCOPE}",
                **({"hashes": [{"alg": "SHA-256", "content": image[1]}]} if image else {}),
            },
        },
        "components": [
            {
                "type": "library",
                "name": comp.name,
                **({"version": comp.version} if comp.version else {}),
                **({"licenses": [{"expression": comp.license}]} if comp.license else {}),
                **({"externalReferences": [{"type": "website", "url": comp.origin}]}
                   if comp.origin else {}),
                "description": comp.evidence,
            }
            for comp in _ordered(items) if comp.kind != "application"
        ],
    }
    return json.dumps(doc, indent=2) + "\n"


def render_json(project: Project, board_id: str, items: list[Component],
                image: tuple[str, str] | None = None) -> str:
    return json.dumps({
        "schema": SCHEMA,
        "project": project.name,
        "board": board_id,
        "scope": _SCOPE,
        "image": {"file": image[0], "sha256": image[1]} if image else None,
        "components": [asdict(c) for c in _ordered(items)],
    }, indent=2) + "\n"


RENDERERS = {
    "notice": render_notice,
    "spdx": render_spdx,
    "cyclonedx": render_cyclonedx,
    "json": render_json,
}


def image_digest(project: Project) -> tuple[str, str] | None:
    """(artefact name, sha256) of the image this SBOM describes.

    An SBOM that does not say WHICH binary it describes is an essay. Builds are
    reproducible (see docs/guide/supply-chain.md), so this digest is a
    stable identity a customer can recompute — which is also what makes it a
    usable SPDX document namespace.
    """
    out = project.build_dir / "out"
    for candidate in (out / f"{project.name}.bin", out / f"{project.name}.elf"):
        if candidate.is_file():
            digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
            return candidate.name, digest
    return None


def require_built(project: Project) -> Path:
    """The map file the link produced, or a message that says how to get one."""
    map_file = project.build_dir / "out" / f"{project.name}.map"
    if not map_file.is_file():
        raise EmitError(
            f"no link map at {map_file} — `alloy sbom` reports what a build "
            "actually linked, so run `alloy build` for this board/product first"
        )
    return map_file
