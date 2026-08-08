"""Guards on the chip-database pin.

The property under test is the one a company asks about: a project that has
declared which device database it was built against must REFUSE to build
against a different one. Everything here is aimed at that — including the
negative controls (an unpinned project keeps working; a pin that matches is
silent; a database that changed by one byte is caught).
"""

from __future__ import annotations

import json
import shutil
import textwrap
from pathlib import Path

import pytest

from alloy_cli import devicedb
from alloy_cli.devicedb import PinError
from alloy_cli.project import ProjectError, load_project

REPO = Path(__file__).resolve().parents[3]
DEVICES = REPO.parent / "alloy-devices"


# ---------------------------------------------------------------------------
# A small, real database on disk: three files across the three hashed trees.
# ---------------------------------------------------------------------------

def make_db(root: Path, *, version: str | None = "0.3.0",
            chip_text: str = "id: fake1\n") -> Path:
    (root / "schema").mkdir(parents=True)
    (root / "registers" / "acme").mkdir(parents=True)
    (root / "chips" / "acme").mkdir(parents=True)
    (root / "schema" / "chip.json").write_text('{"title": "alloy.chip.v1"}\n')
    (root / "registers" / "acme" / "uart.yaml").write_text("kind: registers\n")
    (root / "chips" / "acme" / "fake1.yaml").write_text(chip_text)
    if version is not None:
        (root / "pyproject.toml").write_text(
            f'[project]\nname = "alloy-devices"\nversion = "{version}"\n')
    return root


def make_project(root: Path, devices_table: str = "") -> Path:
    root.mkdir(parents=True, exist_ok=True)
    (root / "src").mkdir(exist_ok=True)
    (root / "src" / "main.cpp").write_text("int main() { return 0; }\n")
    (root / "alloy.toml").write_text(textwrap.dedent(f"""\
        [project]
        name = "{root.name}"

        [board]
        id = "nucleo_g071rb"

        [alloy]
        root = "{REPO}"
        """) + devices_table)
    return root


# ---------------------------------------------------------------------------
# The digest: what it covers, and what it must notice.
# ---------------------------------------------------------------------------

def test_digest_is_stable_across_two_identical_databases(tmp_path: Path) -> None:
    a = make_db(tmp_path / "a")
    b = make_db(tmp_path / "b")
    assert devicedb.content_digest(a) == devicedb.content_digest(b)
    assert devicedb.content_digest(a).startswith("sha256:")


def test_digest_notices_a_one_byte_change_in_a_chip(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    before = devicedb.content_digest(db)
    (db / "chips" / "acme" / "fake1.yaml").write_text("id: fake2\n")
    assert devicedb.content_digest(db) != before


def test_digest_notices_a_rename_even_though_no_byte_changed(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    before = devicedb.content_digest(db)
    (db / "chips" / "acme" / "fake1.yaml").rename(db / "chips" / "acme" / "fake9.yaml")
    assert devicedb.content_digest(db) != before


def test_digest_ignores_prose_and_editor_noise(tmp_path: Path) -> None:
    """A README bump must not invalidate every shipped product's pin."""
    db = make_db(tmp_path / "db")
    before = devicedb.content_digest(db)
    (db / "README.md").write_text("# hello\n")
    (db / "chips" / "acme" / ".DS_Store").write_bytes(b"\x00\x01")
    (db / "CHANGELOG.md").write_text("## 0.4.0\n")
    assert devicedb.content_digest(db) == before


def test_digest_covers_all_three_trees(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    seen = {p.relative_to(db).parts[0] for p in devicedb.digest_files(db)}
    assert seen == {"schema", "registers", "chips"}


def test_digest_of_the_real_database_is_reproducible() -> None:
    if not (DEVICES / "chips").is_dir():
        pytest.skip("alloy-devices checkout not present")
    assert devicedb.content_digest(DEVICES) == devicedb.content_digest(DEVICES)


# ---------------------------------------------------------------------------
# The declared version.
# ---------------------------------------------------------------------------

def test_version_comes_from_the_databases_pyproject(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db", version="1.2.3")
    assert devicedb.declared_version(db) == "1.2.3"


def test_a_foreign_pyproject_is_not_mistaken_for_the_database(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db", version=None)
    (db / "pyproject.toml").write_text(
        '[project]\nname = "something-else"\nversion = "9.9.9"\n')
    # Falls through to installed metadata, which is never "9.9.9".
    assert devicedb.declared_version(db) != "9.9.9"


def test_real_database_declares_a_version() -> None:
    if not (DEVICES / "chips").is_dir():
        pytest.skip("alloy-devices checkout not present")
    version = devicedb.declared_version(DEVICES)
    assert version is not None and version[0].isdigit()


@pytest.mark.parametrize(("spec", "actual", "ok"), [
    ("0.3.0", "0.3.0", True),
    ("0.3.0", "0.3.1", False),
    ("==0.3.0", "0.3.0", True),
    ("0.3", "0.3.0", True),      # a two-part pin is not defeated by a three-part release
    ("0.3", "0.3.1", False),
    (">=0.3.0", "0.3.0", True),
    (">=0.3.0", "0.4.0", True),
    (">=0.3.0", "0.2.9", False),
    (">= 0.3.0", "1.0.0", True),
])
def test_version_matching(spec: str, actual: str, ok: bool) -> None:
    assert devicedb.version_matches(spec, actual) is ok


def test_a_pep440_range_is_refused_rather_than_guessed() -> None:
    with pytest.raises(PinError, match="dotted numeric"):
        devicedb.version_matches("~=0.3", "0.3.0")


# ---------------------------------------------------------------------------
# The pin table itself.
# ---------------------------------------------------------------------------

def test_unknown_keys_in_the_devices_table_are_refused(tmp_path: Path) -> None:
    project = make_project(tmp_path / "p", '\n[devices]\nrevision = "abc"\n')
    with pytest.raises(PinError, match="not a known key"):
        devicedb.read_pin(project)


def test_a_non_string_pin_is_refused(tmp_path: Path) -> None:
    project = make_project(tmp_path / "p", "\n[devices]\nversion = 3\n")
    with pytest.raises(PinError, match="must be a string"):
        devicedb.read_pin(project)


def test_no_devices_table_is_no_pin(tmp_path: Path) -> None:
    project = make_project(tmp_path / "p")
    assert devicedb.read_pin(project) == {}
    devicedb.check_pin(project, make_db(tmp_path / "db"))  # no raise


# ---------------------------------------------------------------------------
# check_pin: the assertion that makes the pin load-bearing.
# ---------------------------------------------------------------------------

def test_matching_version_and_digest_are_silent(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    project = make_project(tmp_path / "p", "\n[devices]\n"
                           f'version = "0.3.0"\ndigest = "{devicedb.content_digest(db)}"\n')
    devicedb.check_pin(project, db)


def test_a_moved_version_is_caught(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db", version="0.4.0")
    project = make_project(tmp_path / "p", '\n[devices]\nversion = "0.3.0"\n')
    with pytest.raises(PinError, match="version mismatch"):
        devicedb.check_pin(project, db)


def test_a_database_that_moved_WITHOUT_a_version_bump_is_caught_by_the_digest(
        tmp_path: Path) -> None:
    """The case alloy-devices' own main branch is in: same version, new facts."""
    db = make_db(tmp_path / "db", version="0.3.0")
    project = make_project(tmp_path / "p", "\n[devices]\n"
                           f'version = "0.3.0"\ndigest = "{devicedb.content_digest(db)}"\n')
    devicedb.check_pin(project, db)                     # control: passes now
    (db / "chips" / "acme" / "fake1.yaml").write_text("id: fake1\nuart_base: 0x4000\n")
    assert devicedb.declared_version(db) == "0.3.0"     # version says nothing changed
    with pytest.raises(PinError, match="CONTENT mismatch"):
        devicedb.check_pin(project, db)


def test_a_version_pin_against_a_silent_database_says_so(tmp_path: Path,
                                                         monkeypatch) -> None:
    db = make_db(tmp_path / "db", version=None)
    monkeypatch.setattr(devicedb, "declared_version", lambda _root: None)
    project = make_project(tmp_path / "p", '\n[devices]\nversion = "0.3.0"\n')
    with pytest.raises(PinError, match="declares no version"):
        devicedb.check_pin(project, db)


# ---------------------------------------------------------------------------
# Resolution: [devices] path, and what beats what.
# ---------------------------------------------------------------------------

def test_devices_path_wins_over_the_environment(tmp_path: Path, monkeypatch) -> None:
    pinned = make_db(tmp_path / "pinned")
    other = make_db(tmp_path / "other")
    monkeypatch.setenv("ALLOY_DEVICES_ROOT", str(other))
    project = make_project(tmp_path / "p", f'\n[devices]\npath = "{pinned}"\n')
    assert load_project(project).devices_root == pinned


def test_a_relative_devices_path_resolves_against_the_project(tmp_path: Path,
                                                              monkeypatch) -> None:
    make_db(tmp_path / "db")
    monkeypatch.delenv("ALLOY_DEVICES_ROOT", raising=False)
    project = make_project(tmp_path / "p", '\n[devices]\npath = "../db"\n')
    assert load_project(project).devices_root == (tmp_path / "db").resolve()


def test_a_devices_path_that_is_not_a_database_is_refused(tmp_path: Path) -> None:
    (tmp_path / "empty").mkdir()
    project = make_project(tmp_path / "p", f'\n[devices]\npath = "{tmp_path / "empty"}"\n')
    with pytest.raises(PinError, match="not a chip database"):
        load_project(project)


def test_the_pin_catches_an_env_var_that_redirects_the_database(
        tmp_path: Path, monkeypatch) -> None:
    """The point of checking the assertion against WHATEVER resolved."""
    pinned = make_db(tmp_path / "pinned")
    project = make_project(tmp_path / "p", "\n[devices]\n"
                           f'digest = "{devicedb.content_digest(pinned)}"\n')
    monkeypatch.setenv("ALLOY_DEVICES_ROOT", str(pinned))
    load_project(project)                               # control: the right one passes
    monkeypatch.setenv("ALLOY_DEVICES_ROOT",
                       str(make_db(tmp_path / "swapped", chip_text="id: other\n")))
    with pytest.raises(PinError, match="CONTENT mismatch"):
        load_project(project)


def test_every_verb_is_covered_because_load_project_checks(tmp_path: Path,
                                                           monkeypatch) -> None:
    """No route may reach codegen with an unpinned-away database."""
    db = make_db(tmp_path / "db", version="0.9.9")
    monkeypatch.setenv("ALLOY_DEVICES_ROOT", str(db))
    project = make_project(tmp_path / "p", '\n[devices]\nversion = "0.3.0"\n')
    with pytest.raises(PinError, match="version mismatch"):
        load_project(project)


def test_a_pin_failure_is_not_a_ProjectError_subclass() -> None:
    """The load-bearing detail behind the test below.

    cmd_boards and _roots catch ProjectError to mean "not in a project, use
    the framework defaults". If PinError were a ProjectError, those two would
    swallow a mismatch and carry on against the refused database.
    """
    assert not issubclass(PinError, ProjectError)


def test_the_verbs_that_tolerate_no_project_do_not_swallow_a_pin_failure(
        tmp_path: Path, monkeypatch) -> None:
    from alloy_cli.cli import _roots, cmd_boards  # noqa: PLC0415
    import argparse  # noqa: PLC0415

    db = make_db(tmp_path / "db", version="0.9.9")
    monkeypatch.setenv("ALLOY_DEVICES_ROOT", str(db))
    project = make_project(tmp_path / "p", '\n[devices]\nversion = "0.3.0"\n')
    args = argparse.Namespace(project=str(project), json=False)
    with pytest.raises(PinError, match="version mismatch"):
        _roots(args)
    with pytest.raises(PinError, match="version mismatch"):
        cmd_boards(args)


# ---------------------------------------------------------------------------
# write_pin / `alloy devices --pin`.
# ---------------------------------------------------------------------------

def test_write_pin_appends_a_section_that_then_verifies(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    project = make_project(tmp_path / "p")
    devicedb.write_pin(project, db)
    pin = devicedb.read_pin(project)
    assert pin["version"] == "0.3.0" and pin["digest"].startswith("sha256:")
    devicedb.check_pin(project, db)


def test_write_pin_rewrites_in_place_and_keeps_the_path(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    project = make_project(
        tmp_path / "p",
        f'\n[devices]\npath = "{db}"\nversion = "0.0.1"\ndigest = "sha256:dead"\n')
    devicedb.write_pin(project, db)
    text = (project / "alloy.toml").read_text()
    assert text.count("[devices]") == 1
    pin = devicedb.read_pin(project)
    assert pin["path"] == str(db)
    assert pin["version"] == "0.3.0" and pin["digest"] != "sha256:dead"
    assert "[board]" in text and "[alloy]" in text   # nothing else was eaten


def test_write_pin_does_not_disturb_a_following_section(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    project = make_project(tmp_path / "p",
                           '\n[devices]\nversion = "0.0.1"\n\n[libs]\nmodbus = "0.4.0"\n')
    devicedb.write_pin(project, db)
    import tomllib  # noqa: PLC0415
    data = tomllib.loads((project / "alloy.toml").read_text())
    assert data["libs"] == {"modbus": "0.4.0"}
    assert data["devices"]["version"] == "0.3.0"


def test_pin_only_version_or_only_digest(tmp_path: Path) -> None:
    db = make_db(tmp_path / "db")
    project = make_project(tmp_path / "p")
    devicedb.write_pin(project, db, digest=False)
    assert set(devicedb.read_pin(project)) == {"version"}
    devicedb.write_pin(project, db, version=False)
    assert set(devicedb.read_pin(project)) == {"digest"}


def test_cli_devices_json_reports_root_version_and_digest(tmp_path: Path,
                                                          monkeypatch, capsys) -> None:
    from alloy_cli.cli import cmd_devices  # noqa: PLC0415
    import argparse  # noqa: PLC0415

    db = make_db(tmp_path / "db")
    monkeypatch.setenv("ALLOY_DEVICES_ROOT", str(db))
    project = make_project(tmp_path / "p")
    args = argparse.Namespace(project=str(project), json=True, pin=False,
                              version_only=False, digest_only=False)
    assert cmd_devices(args) == 0
    out = json.loads(capsys.readouterr().out)
    assert out["schema"] == "alloy.devices.v1"
    assert out["root"] == str(db)
    assert out["version"] == "0.3.0"
    assert out["digest"] == devicedb.content_digest(db)
    assert out["files"] == 3 and out["pin"] is None


def test_cli_devices_pin_then_report_round_trips(tmp_path: Path,
                                                 monkeypatch, capsys) -> None:
    from alloy_cli.cli import cmd_devices  # noqa: PLC0415
    import argparse  # noqa: PLC0415

    db = make_db(tmp_path / "db")
    monkeypatch.setenv("ALLOY_DEVICES_ROOT", str(db))
    project = make_project(tmp_path / "p")
    common = {"project": str(project), "version_only": False, "digest_only": False}
    assert cmd_devices(argparse.Namespace(json=False, pin=True, **common)) == 0
    capsys.readouterr()
    assert cmd_devices(argparse.Namespace(json=True, pin=False, **common)) == 0
    out = json.loads(capsys.readouterr().out)
    assert out["pin"]["digest"] == out["digest"]

    # …and now the report itself refuses a swapped database, like a build would.
    shutil.rmtree(db / "chips")
    (db / "chips" / "acme").mkdir(parents=True)
    (db / "chips" / "acme" / "fake1.yaml").write_text("id: changed\n")
    with pytest.raises(PinError, match="CONTENT mismatch"):
        cmd_devices(argparse.Namespace(json=True, pin=False, **common))
