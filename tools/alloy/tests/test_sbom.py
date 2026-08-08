"""Guards on `alloy sbom`.

The value of this command is one property: it CANNOT quietly omit something.
Every test below is aimed at that — the version readers must read the real
vendored trees (so a bump is reflected without an edit), the link parser must
report only what actually linked, and a source file belonging to no declared
package must surface as an undeclared component rather than disappear.
"""

from __future__ import annotations

import json
import tomllib
from pathlib import Path

import pytest

from alloy_cli import sbom
from alloy_cli.emit.common import EmitError
from alloy_cli.project import Project

REPO = Path(__file__).resolve().parents[3]


# ---------------------------------------------------------------------------
# Version readers, against the REAL vendored trees in this repo.
# ---------------------------------------------------------------------------

def test_lwip_version_is_read_from_the_vendored_header() -> None:
    version, evidence = sbom._lwip_version(REPO / "src/alloy/net/vendor/lwip")
    assert version is not None and version.count(".") == 2
    assert "init.h" in evidence


def test_littlefs_version_is_decoded_from_lfs_version() -> None:
    version, evidence = sbom._littlefs_version(REPO / "src/alloy/fs/vendor")
    assert version is not None
    major, minor = version.split(".")
    assert major == "2" and int(minor) >= 0
    assert "lfs.h" in evidence


def test_monocypher_version_comes_from_the_vendoring_note() -> None:
    version, evidence = sbom._monocypher_version(REPO / "third_party/monocypher")
    assert version is not None and version[0].isdigit()
    assert "README" in evidence


def test_version_readers_return_none_for_a_tree_that_is_not_there(tmp_path) -> None:
    """Absent is reported as absent — never as a fabricated version."""
    for reader in (sbom._lwip_version, sbom._littlefs_version, sbom._monocypher_version):
        assert reader(tmp_path) == (None, None)


def test_every_declared_vendored_tree_exists_with_its_licence_file() -> None:
    """The package table is only allowed to name things that are really here —
    a stale entry would advertise a licence for code that is not in the repo."""
    for spec in sbom._VENDORED:
        assert (REPO / spec.rel).is_dir(), f"{spec.name}: {spec.rel} is gone"
        assert (REPO / spec.license_file).is_file(), f"{spec.name}: {spec.license_file} is gone"


def test_pyproject_version_prefers_the_file_over_a_stale_module_constant() -> None:
    version, evidence = sbom._pyproject_version(REPO / "tools" / "alloy")
    declared = tomllib.loads((REPO / "tools/alloy/pyproject.toml").read_text())
    assert version == declared["project"]["version"]
    assert "pyproject.toml" in evidence


# ---------------------------------------------------------------------------
# The link map parser: "what linked", not "what was searched".
# ---------------------------------------------------------------------------

_MAP = """\
Archive member included to satisfy reference by file (symbol)

/tc/lib/libgcc.a(_udivsi3.o)
                              CMakeFiles/x.dir/main.cpp.obj (__aeabi_uidiv)
/tc/lib/libgcc.a(_clzsi2.o)
                              /tc/lib/libgcc.a(_udivsi3.o) (__clzsi2)
/tc/lib/libc_nano.a(memcpy.o)
                              CMakeFiles/x.dir/main.cpp.obj (memcpy)

Discarded input sections

 .group  0x0 0x8 CMakeFiles/x.dir/main.cpp.obj

Memory Configuration
LOAD /tc/lib/libm.a
LOAD /tc/lib/libstdc++_nano.a
"""


def test_linked_archives_reports_only_contributing_archives(tmp_path) -> None:
    map_file = tmp_path / "x.map"
    map_file.write_text(_MAP)
    found = sbom.linked_archives(map_file)
    assert set(found) == {"/tc/lib/libgcc.a", "/tc/lib/libc_nano.a"}
    assert found["/tc/lib/libgcc.a"] == ["_clzsi2.o", "_udivsi3.o"]
    # libm and libstdc++ were SEARCHED (they appear as LOAD lines further down)
    # but contributed nothing — an SBOM that listed them would be over-claiming.
    assert not any("libm" in a or "libstdc++" in a for a in found)


def test_linked_archives_of_a_missing_map_is_empty(tmp_path) -> None:
    assert sbom.linked_archives(tmp_path / "nope.map") == {}


def test_runtime_components_name_the_licence_of_libgcc(tmp_path) -> None:
    map_file = tmp_path / "x.map"
    map_file.write_text(_MAP)
    comps = sbom._runtime_components(map_file)
    by_name = {c.name: c for c in comps}
    libgcc = next(c for n, c in by_name.items() if "libgcc" in n)
    assert libgcc.license == "GPL-3.0-or-later WITH GCC-exception-3.1"
    assert libgcc.kind == "toolchain-runtime"
    assert "_udivsi3.o" in libgcc.evidence
    # newlib is a licence COLLECTION; alloy refuses to assert one for it.
    newlib = next(c for n, c in by_name.items() if "newlib" in n)
    assert newlib.license is None


def test_an_unrecognised_archive_is_still_reported(tmp_path) -> None:
    """A toolchain that links something alloy has never heard of must produce a
    component, not silence."""
    map_file = tmp_path / "x.map"
    map_file.write_text(
        "Archive member included to satisfy reference by file (symbol)\n\n"
        "/tc/lib/libweird.a(thing.o)\n"
        "                              CMakeFiles/x.dir/main.cpp.obj (thing)\n\n"
    )
    comps = sbom._runtime_components(map_file)
    assert len(comps) == 1
    assert "libweird.a" in comps[0].name
    assert comps[0].license is None
    assert not comps[0].declared


# ---------------------------------------------------------------------------
# The forgetting-proof property, and the renderers.
# ---------------------------------------------------------------------------

def _fake_project(tmp_path: Path) -> Project:
    root = tmp_path / "proj"
    (root / "src").mkdir(parents=True)
    (root / "src" / "main.cpp").write_text("int main(){}\n")
    (root / "alloy.toml").write_text('[project]\nname = "proj"\n\n[board]\nid = "b"\n')
    return Project(root=root, name="proj", board_id="b",
                   alloy_root=REPO, devices_root=REPO.parent / "alloy-devices")


def test_a_source_from_no_declared_package_becomes_an_undeclared_component(
    tmp_path, monkeypatch
) -> None:
    """The whole point of deriving from the build: vendor a new tree and forget
    to declare it, and the SBOM shouts instead of staying silent."""
    project = _fake_project(tmp_path)
    newly_vendored = tmp_path / "somewhere" / "libmystery"
    newly_vendored.mkdir(parents=True)
    (newly_vendored / "LICENSE.txt").write_text("Mystery Public License\n")
    stray = newly_vendored / "mystery.c"
    stray.write_text("int f(void){return 0;}\n")

    monkeypatch.setattr(
        sbom, "build_inputs",
        lambda *_a: _Inputs([project.root / "src" / "main.cpp"], [stray]))
    monkeypatch.setattr(sbom, "generated_sources", lambda _p: [])

    items = sbom.components(project, {"boot": {}})
    mystery = next(c for c in items if c.name == "libmystery")
    assert mystery.license is None
    assert not mystery.declared
    assert mystery.license_file.endswith("LICENSE.txt")
    assert "declared in no package definition" in mystery.evidence


class _Inputs:
    """Stand-in for build.BuildInputs with an arbitrary compiled set."""

    def __init__(self, app: list[Path], extra: list[Path]) -> None:
        self.app = app
        self.runtime: list[Path] = []
        self.vendor = extra
        self.lfs: list[Path] = []
        self.lwip = {"c": [], "glue": [], "inc": []}

    def compiled(self) -> list[Path]:
        return [*self.app, *self.vendor]


def _items() -> list[sbom.Component]:
    return [
        sbom.Component(name="proj", kind="application", evidence="own sources"),
        sbom.Component(name="alloy", kind="framework", version="0.3.0", license="MIT",
                       origin="https://example.invalid/alloy", evidence="always"),
        sbom.Component(name="littlefs", kind="vendored", version="2.11",
                       license="BSD-3-Clause", license_file="/x/LICENSE.md",
                       evidence="2 vendored source file(s)"),
        sbom.Component(name="newlib", kind="toolchain-runtime",
                       license_file="/tc/COPYING.NEWLIB", evidence="linked"),
    ]


def test_notice_names_every_component_and_flags_the_undeclared(tmp_path) -> None:
    project = _fake_project(tmp_path)
    text = sbom.render_notice(project, "b", _items())
    for name in ("proj", "alloy", "littlefs", "newlib"):
        assert name in text
    assert "BSD-3-Clause" in text
    assert "UNDECLARED LICENCES: newlib" in text
    assert "Firmware image only" in text
    # The application's own licence is not a third-party obligation.
    assert "UNDECLARED LICENCES: newlib, proj" not in text


def test_spdx_is_valid_json_with_one_package_per_component(tmp_path) -> None:
    project = _fake_project(tmp_path)
    doc = json.loads(sbom.render_spdx(project, "b", _items()))
    assert doc["spdxVersion"] == "SPDX-2.3"
    assert len(doc["packages"]) == 4
    ids = {p["SPDXID"] for p in doc["packages"]}
    assert len(ids) == 4, "SPDXIDs must be unique"
    littlefs = next(p for p in doc["packages"] if p["name"] == "littlefs")
    assert littlefs["licenseDeclared"] == "BSD-3-Clause"
    assert littlefs["versionInfo"] == "2.11"
    newlib = next(p for p in doc["packages"] if p["name"] == "newlib")
    assert newlib["licenseDeclared"] == "NOASSERTION"
    # Every non-root package hangs off the root document element.
    assert len(doc["relationships"]) == 3


def test_cyclonedx_is_valid_json_and_drops_the_application_itself(tmp_path) -> None:
    project = _fake_project(tmp_path)
    doc = json.loads(sbom.render_cyclonedx(project, "b", _items()))
    assert doc["bomFormat"] == "CycloneDX"
    names = [c["name"] for c in doc["components"]]
    assert "proj" not in names  # the firmware IS the application; it is metadata
    assert {"alloy", "littlefs", "newlib"} <= set(names)
    littlefs = next(c for c in doc["components"] if c["name"] == "littlefs")
    assert littlefs["licenses"] == [{"expression": "BSD-3-Clause"}]
    assert "licenses" not in next(c for c in doc["components"] if c["name"] == "newlib")


def test_source_date_epoch_makes_the_documents_reproducible(tmp_path, monkeypatch) -> None:
    project = _fake_project(tmp_path)
    monkeypatch.setenv("SOURCE_DATE_EPOCH", "1700000000")
    first = sbom.render_spdx(project, "b", _items())
    second = sbom.render_spdx(project, "b", _items())
    assert first == second
    assert "2023-11-14T" in first
    assert json.loads(sbom.render_cyclonedx(project, "b", _items()))[
        "metadata"]["timestamp"].startswith("2023-11-14T")


def test_render_order_is_stable_regardless_of_input_order(tmp_path) -> None:
    project = _fake_project(tmp_path)
    forward = sbom.render_notice(project, "b", _items())
    backward = sbom.render_notice(project, "b", list(reversed(_items())))
    assert forward == backward


def test_sbom_refuses_without_a_link_map(tmp_path) -> None:
    project = _fake_project(tmp_path)
    with pytest.raises(EmitError, match="alloy build"):
        sbom.require_built(project)


def test_every_renderer_is_reachable_from_the_cli_choices() -> None:
    assert set(sbom.RENDERERS) == {"notice", "spdx", "cyclonedx", "json"}


# ---------------------------------------------------------------------------
# The repository-level NOTICE. `alloy sbom` answers "what is in THIS image";
# NOTICE answers "what is in this source distribution". They must not disagree,
# and the second one is a hand-written file — so it gets a rot gate.
# ---------------------------------------------------------------------------

def test_notice_file_covers_every_vendored_tree() -> None:
    notice = (REPO / "NOTICE").read_text()
    for spec in sbom._VENDORED:
        assert spec.name in notice, f"NOTICE does not mention {spec.name}"
        assert spec.license_file in notice, \
            f"NOTICE does not point at {spec.license_file}"


def test_notice_file_records_the_current_vendored_versions() -> None:
    """A vendor bump that leaves NOTICE claiming the old version is exactly the
    kind of quiet inaccuracy legal gets burned by."""
    notice = (REPO / "NOTICE").read_text()
    for spec in sbom._VENDORED:
        if spec.version is None:
            continue
        version, _evidence = spec.version(REPO / spec.rel)
        if version is None:
            continue
        assert version in notice, \
            f"NOTICE does not record {spec.name} {version} (read from the tree)"


def test_image_digest_prefers_the_binary_and_is_none_without_a_build(tmp_path) -> None:
    project = _fake_project(tmp_path)
    assert sbom.image_digest(project) is None
    out = project.build_dir / "out"
    out.mkdir(parents=True)
    (out / "proj.elf").write_bytes(b"elf")
    assert sbom.image_digest(project)[0] == "proj.elf"
    (out / "proj.bin").write_bytes(b"bin")
    name, digest = sbom.image_digest(project)
    assert name == "proj.bin"
    assert digest == __import__("hashlib").sha256(b"bin").hexdigest()


def test_the_image_digest_reaches_every_output_format(tmp_path) -> None:
    project = _fake_project(tmp_path)
    image = ("proj.bin", "de" * 32)
    assert image[1] in sbom.render_notice(project, "b", _items(), image)
    spdx = json.loads(sbom.render_spdx(project, "b", _items(), image))
    assert spdx["documentNamespace"].endswith(image[1])
    app = next(p for p in spdx["packages"] if p["name"] == "proj")
    assert app["checksums"] == [{"algorithm": "SHA256", "checksumValue": image[1]}]
    cdx = json.loads(sbom.render_cyclonedx(project, "b", _items(), image))
    assert cdx["metadata"]["component"]["hashes"][0]["content"] == image[1]
    assert json.loads(sbom.render_json(project, "b", _items(), image))["image"]["sha256"] \
        == image[1]
