"""Unit tests for `alloy ci-init`.

A generated workflow that does not parse, or that installs a toolchain nobody
needs, is worse than none — the user finds out from a red CI run they did not
write. So: it must be valid YAML, it must only ask for what the chosen boards
need, and it must not silently overwrite a file the user has since edited.
"""

from __future__ import annotations

from pathlib import Path

import pytest
import yaml

from alloy_cli import ci_init
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]


class _FakeProject:
    def __init__(self, root: Path, board: str = "nucleo_g071rb") -> None:
        self.root = root
        self.name = "widget"
        self.board_id = board


def _project(tmp_path: Path, board: str = "nucleo_g071rb",
             local: list[str] | None = None) -> _FakeProject:
    root = tmp_path / "proj"
    root.mkdir(parents=True, exist_ok=True)
    for name in local or []:
        (root / "boards" / name).mkdir(parents=True)
        (root / "boards" / name / "board.json").write_text("{}")
    return _FakeProject(root, board)


def _parse(path: Path) -> dict:
    return yaml.safe_load(path.read_text())


# ------------------------------------------------------------- the workflow

def test_the_generated_workflow_is_valid_yaml(tmp_path: Path) -> None:
    path, _ = ci_init.write_workflow(_project(tmp_path), ALLOY_ROOT)
    parsed = _parse(path)
    assert parsed["name"] == "alloy"
    steps = parsed["jobs"]["build"]["steps"]
    assert any("checkout" in str(s.get("uses", "")) for s in steps)
    assert steps[-1]["run"].startswith("alloy matrix --boards")


def test_yaml_survives_the_on_key(tmp_path: Path) -> None:
    """`on:` is YAML's boolean True unless quoted by the parser — a workflow
    generator that gets this wrong produces a file GitHub silently ignores."""
    path, _ = ci_init.write_workflow(_project(tmp_path), ALLOY_ROOT)
    parsed = _parse(path)
    triggers = parsed.get("on", parsed.get(True))
    assert triggers is not None and "push" in triggers


# ------------------------------------------------------------- board choice

def test_it_targets_what_the_project_targets(tmp_path: Path) -> None:
    """Not every supported board: a single-board project should not download an
    Xtensa toolchain on every push to prove portability it never claimed."""
    _, details = ci_init.write_workflow(_project(tmp_path), ALLOY_ROOT)
    assert details["boards"] == ["nucleo_g071rb"]
    assert details["needs_xtensa"] is False


def test_local_boards_are_built_and_validated(tmp_path: Path) -> None:
    path, details = ci_init.write_workflow(
        _project(tmp_path, local=["my_widget"]), ALLOY_ROOT)
    assert set(details["boards"]) == {"nucleo_g071rb", "my_widget"}
    runs = [s.get("run", "") for s in _parse(path)["jobs"]["build"]["steps"]]
    assert "alloy board-validate my_widget" in runs
    # A framework board is not validated — it is not the user's to fix.
    assert "alloy board-validate nucleo_g071rb" not in runs


def test_all_widens_to_every_supported_board(tmp_path: Path) -> None:
    _, details = ci_init.write_workflow(
        _project(tmp_path), ALLOY_ROOT, every_board=True)
    assert len(details["boards"]) > 5
    assert "same70_xplained" in details["boards"]


def test_an_esp_board_brings_its_toolchain(tmp_path: Path) -> None:
    path, details = ci_init.write_workflow(
        _project(tmp_path, board="esp32_devkit"), ALLOY_ROOT)
    assert details["needs_xtensa"] is True
    text = path.read_text()
    assert "xtensa-esp-elf" in text and "actions/cache@v4" in text
    # The cache key must be the archive, or a toolchain bump reuses a stale one.
    assert ci_init.XTENSA_ARCHIVE in text
    assert _parse(path)["jobs"]["build"]["steps"], "still valid YAML with the extra steps"


def test_an_arm_only_project_gets_no_xtensa_steps(tmp_path: Path) -> None:
    path, _ = ci_init.write_workflow(_project(tmp_path), ALLOY_ROOT)
    assert "xtensa" not in path.read_text()


def test_explicit_boards_win(tmp_path: Path) -> None:
    _, details = ci_init.write_workflow(
        _project(tmp_path), ALLOY_ROOT, boards=["nucleo_f722ze", "rp2040_zero"])
    assert details["boards"] == ["nucleo_f722ze", "rp2040_zero"]


# ------------------------------------------------------------- overwriting

def test_it_refuses_to_clobber_an_edited_workflow(tmp_path: Path) -> None:
    project = _project(tmp_path)
    path, _ = ci_init.write_workflow(project, ALLOY_ROOT)
    path.write_text("# hand-edited\n")
    with pytest.raises(EmitError, match="--force"):
        ci_init.write_workflow(project, ALLOY_ROOT)
    assert path.read_text() == "# hand-edited\n", "the user's edit must survive"

    ci_init.write_workflow(project, ALLOY_ROOT, force=True)
    assert "GENERATED" in path.read_text()


def test_the_toolchain_pins_match_the_frameworks_own_ci() -> None:
    """A workflow pinned to a toolchain alloy is not tested against fails for
    reasons the user did not cause."""
    ci = (ALLOY_ROOT / ".github" / "workflows" / "ci.yml").read_text()
    assert ci_init.ARM_RELEASE in ci
    assert ci_init.XTENSA_ARCHIVE in ci
