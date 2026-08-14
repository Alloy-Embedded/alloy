"""Every library in the registry must be built and instantiated by a test.

WHY THIS GATE EXISTS. A shared utility library I read had SEVEN headers with
outright syntax errors — a `void` function with a `return <expr>;`, a const
member calling a non-const one, a header with no include guard. They survived
for years, in a repository with CI, because a header-only module that nothing
instantiates is never compiled: the template is only parsed, never checked. Four
of its ten directories were not even listed in its own CMakeLists.

A `libs/` directory actively invites that failure. `tests/CMakeLists.txt` globs
`libs/*/tests/test_*.cpp`, so a library WITH a test is compiled on every run —
but a library with no test, or with a test that only includes the header without
constructing anything, passes silently forever.

So this checks three things per registry entry, and each one is a distinct way
the old library failed:

  1. the manifest and include directory exist where the registry says;
  2. there is at least one test file;
  3. that test actually NAMES the library's namespace — a test that includes a
     header and asserts nothing instantiates nothing.

It is a lint over the tree, not a compile: the compile happens in the host-test
job, and this is what guarantees there is something there for it to compile.
"""

from __future__ import annotations

import re
import tomllib
from pathlib import Path

import pytest

ALLOY_ROOT = Path(__file__).resolve().parents[3]
LIBS = ALLOY_ROOT / "libs"
REGISTRY = LIBS / "registry.toml"


def _registry() -> dict[str, dict]:
    data = tomllib.loads(REGISTRY.read_text())
    return {name: entry for name, entry in data.items() if isinstance(entry, dict)}


def _in_tree() -> list[tuple[str, Path]]:
    """Registry entries that live in this repo. A `git:` source is somebody
    else's repository and carries its own tests; only `path:` is ours to gate."""
    out = []
    for name, entry in _registry().items():
        source = entry.get("source", "")
        if source.startswith("path:"):
            out.append((name, ALLOY_ROOT / source.removeprefix("path:")))
    return out


def test_the_registry_is_not_empty() -> None:
    """A gate that silently iterates over nothing is not a gate."""
    assert len(_in_tree()) >= 8


@pytest.mark.parametrize("name,root", _in_tree(), ids=lambda v: v if isinstance(v, str) else "")
def test_library_has_a_manifest_and_includes(name: str, root: Path) -> None:
    assert root.is_dir(), f"registry points {name} at {root}, which does not exist"
    manifest = root / "alloy.lib.toml"
    assert manifest.is_file(), f"{name} has no alloy.lib.toml"
    declared = tomllib.loads(manifest.read_text())
    assert declared["lib"]["name"] == name, (
        f"{name}'s manifest calls itself {declared['lib']['name']} — the registry "
        f"is generated from the manifests, so the two disagreeing means one of "
        f"them was hand-edited")
    for include in declared.get("headers", {}).get("include", ["include"]):
        assert (root / include).is_dir(), f"{name} declares include dir {include}, absent"


@pytest.mark.parametrize("name,root", _in_tree(), ids=lambda v: v if isinstance(v, str) else "")
def test_library_has_a_test_that_instantiates_it(name: str, root: Path) -> None:
    """The whole point. A header nothing constructs is a header nothing checks."""
    tests = sorted((root / "tests").glob("test_*.cpp")) if (root / "tests").is_dir() else []
    assert tests, (
        f"{name} has no tests/test_*.cpp. A header-only library that no test "
        f"instantiates is never compiled — its templates are parsed and never "
        f"checked, which is how seven headers with syntax errors survived years "
        f"in the library this gate was written for.")

    namespace = tomllib.loads((root / "alloy.lib.toml").read_text()) \
        .get("headers", {}).get("namespace", "alloy::lib")
    expected = f"{namespace}::{name}"
    body = "\n".join(t.read_text() for t in tests)
    assert expected in body, (
        f"{name}'s tests never name {expected}. Including a header without "
        f"constructing anything from it compiles nothing: the test has to build "
        f"a value or call a function, not just #include.")


@pytest.mark.parametrize("name,root", _in_tree(), ids=lambda v: v if isinstance(v, str) else "")
def test_library_tests_actually_assert(name: str, root: Path) -> None:
    """A test file with no assertion is a compile check wearing a test's name.

    That is not worthless — it is exactly what catches a syntax error — but it
    must be a deliberate choice, and every library here has real behaviour to
    check.
    """
    tests = sorted((root / "tests").glob("test_*.cpp"))
    body = "\n".join(t.read_text() for t in tests)
    assertions = len(re.findall(r"\bALLOY_CHECK(_EQ)?\s*\(|\bstatic_assert\s*\(", body))
    assert assertions >= 3, (
        f"{name}'s tests carry {assertions} assertions across {len(tests)} file(s)")


def test_every_in_tree_library_directory_is_in_the_registry() -> None:
    """The other direction: a library on disk that the registry does not list is
    invisible to `alloy lib`, so nobody can install it and nobody notices it rot.
    """
    on_disk = {p.name for p in LIBS.iterdir()
               if p.is_dir() and (p / "alloy.lib.toml").is_file()}
    listed = {name for name, _ in _in_tree()}
    missing = sorted(on_disk - listed)
    assert not missing, f"libraries on disk but not in registry.toml: {missing}"
