"""`alloy new --with-tests` and the project-suite half of `alloy test`.

The end-to-end proof (scaffold, compile, run, watch five tests pass) is the
`scaffold` CI job — it needs a C++ toolchain and cmake. What is tested here is
everything that can go wrong WITHOUT a compiler: the templates rendering
correctly, `alloy test` choosing the right suite, and the coverage path failing
with an actionable message instead of a traceback.
"""

from __future__ import annotations

from pathlib import Path

import pytest

from alloy_cli import apptests

REPO = Path(__file__).resolve().parents[3]


def _cli(monkeypatch, *argv: str) -> int:
    """Drive the real CLI in-process: argv through argparse through the command,
    so the test exercises the actual parser wiring and not a hand-built
    Namespace that could drift from it."""
    import sys  # noqa: PLC0415

    from alloy_cli import cli  # noqa: PLC0415

    monkeypatch.setattr(sys, "argv", ["alloy", *argv])
    try:
        cli.main()
    except SystemExit as exit_:
        return int(exit_.code or 0)
    return 0


def _new(tmp_path: Path, *args: str, monkeypatch) -> int:
    monkeypatch.setenv("ALLOY_ROOT", str(REPO))
    monkeypatch.chdir(tmp_path)
    return _cli(monkeypatch, "new", *args)


# --- templates -------------------------------------------------------------


def test_scaffold_writes_the_four_files(tmp_path: Path) -> None:
    (tmp_path / "src").mkdir()
    written = apptests.scaffold(tmp_path, "widget")
    names = {p.relative_to(tmp_path).as_posix() for p in written}
    assert names == {"src/app.hpp", "src/main.cpp",
                     "tests/test_app.cpp", "tests/CMakeLists.txt"}
    for path in written:
        assert path.exists()


def test_scaffold_without_main_leaves_main_alone(tmp_path: Path) -> None:
    """The --chip route makes a board with no roles; a main.cpp that opens
    board::debug_uart would not compile there."""
    (tmp_path / "src").mkdir()
    (tmp_path / "src" / "main.cpp").write_text("// the clean-board template\n")
    apptests.scaffold(tmp_path, "widget", with_main=False)
    assert (tmp_path / "src" / "main.cpp").read_text() == "// the clean-board template\n"
    assert (tmp_path / "src" / "app.hpp").exists()
    assert (tmp_path / "tests" / "CMakeLists.txt").exists()


def test_project_name_is_substituted_everywhere(tmp_path: Path) -> None:
    (tmp_path / "src").mkdir()
    apptests.scaffold(tmp_path, "widget")
    cmake = (tmp_path / "tests" / "CMakeLists.txt").read_text()
    assert "project(widget_tests CXX)" in cmake
    assert "add_test(NAME widget_tests COMMAND widget_tests)" in cmake
    assert "widget up" in (tmp_path / "src" / "main.cpp").read_text()


def test_no_placeholder_survives_rendering(tmp_path: Path) -> None:
    """A missed @NAME@ is a broken scaffold that still 'succeeds'. The templates
    are C++ and CMake, both full of braces, so they are rendered by placeholder
    substitution rather than str.format — this pins that nothing is left over."""
    (tmp_path / "src").mkdir()
    for path in apptests.scaffold(tmp_path, "widget"):
        assert "@NAME@" not in path.read_text(), path


def test_cmake_template_kept_its_variable_expansions(tmp_path: Path) -> None:
    """The other half of that trade: `${ALLOY_ROOT}` must survive intact. An
    over-eager de-escaping once ate the closing braces of C++ initialisers."""
    (tmp_path / "src").mkdir()
    apptests.scaffold(tmp_path, "widget")
    cmake = (tmp_path / "tests" / "CMakeLists.txt").read_text()
    assert "${ALLOY_ROOT}/src" in cmake
    assert "${ALLOY_ROOT}/tests" in cmake
    assert "${ALLOY_ROOT}/libs" in cmake
    assert "${APP_TESTS}" in cmake
    # and the C++ brace-init that the same bug corrupted
    body = (tmp_path / "tests" / "test_app.cpp").read_text()
    assert "app::thermostat t{r.bus, r.heater, {}};" in body


def test_tests_cmake_refuses_a_bare_cmake(tmp_path: Path) -> None:
    (tmp_path / "src").mkdir()
    apptests.scaffold(tmp_path, "widget")
    assert "ALLOY_ROOT not set" in (tmp_path / "tests" / "CMakeLists.txt").read_text()


def test_scaffolded_tests_use_both_fake_sources(tmp_path: Path) -> None:
    """The whole point of the scaffold is discoverability: it must actually
    demonstrate tests/doubles.hpp AND libs/testkit, not just one."""
    (tmp_path / "src").mkdir()
    apptests.scaffold(tmp_path, "widget")
    body = (tmp_path / "tests" / "test_app.cpp").read_text()
    assert "doubles.hpp" in body
    assert "testkit/mock_bus.hpp" in body
    assert "alloy::test::fake_pin" in body
    assert "alloy::testkit::mock_i2c" in body


def test_the_fakes_the_scaffold_names_really_exist() -> None:
    """A template referring to a double that was since renamed compiles nowhere
    and would only be caught by the CI scaffold job."""
    doubles = (REPO / "tests" / "doubles.hpp").read_text()
    mock_bus = (REPO / "libs" / "testkit" / "mock_bus.hpp").read_text()
    assert "struct fake_pin" in doubles
    assert "struct mock_i2c" in mock_bus
    for member in ("queue_read", "last_addr", "last_write_len", "last_write", "fail",
                   "void reset()"):
        assert member in mock_bus, member


# --- suite selection -------------------------------------------------------


def test_project_suite_needs_an_alloy_toml(tmp_path: Path) -> None:
    (tmp_path / "tests").mkdir()
    (tmp_path / "tests" / "CMakeLists.txt").write_text("")
    assert apptests.project_suite(tmp_path) is None


def test_project_suite_needs_a_tests_cmakelists(tmp_path: Path) -> None:
    (tmp_path / "alloy.toml").write_text("[project]\nname='x'\n")
    assert apptests.project_suite(tmp_path) is None
    (tmp_path / "tests").mkdir()
    assert apptests.project_suite(tmp_path) is None


def test_project_suite_found_when_both_present(tmp_path: Path) -> None:
    (tmp_path / "alloy.toml").write_text("[project]\nname='x'\n")
    (tmp_path / "tests").mkdir()
    (tmp_path / "tests" / "CMakeLists.txt").write_text("")
    assert apptests.project_suite(tmp_path) == (tmp_path / "tests").resolve()


def test_framework_root_has_no_project_suite() -> None:
    """The regression that matters most: `alloy test` at the repo root must go
    on running the FRAMEWORK suite, which is what CI's host-tests job does. The
    repo has a tests/CMakeLists.txt and deliberately no alloy.toml."""
    assert (REPO / "tests" / "CMakeLists.txt").exists()
    assert not (REPO / "alloy.toml").exists()
    assert apptests.project_suite(REPO) is None


def test_an_example_without_tests_still_gets_the_framework_suite() -> None:
    """Examples have an alloy.toml and no tests/ — `alloy test` from inside one
    must not start claiming there is nothing to run."""
    assert apptests.project_suite(REPO / "examples" / "blink") is None


# --- coverage --------------------------------------------------------------


def test_coverage_without_gcovr_is_actionable(tmp_path: Path, monkeypatch,
                                              capsys) -> None:
    monkeypatch.setattr(apptests.shutil, "which", lambda _name: None)
    rc = apptests.coverage_report(tmp_path, tmp_path)
    assert rc == 1
    err = capsys.readouterr().err
    assert "gcovr" in err
    assert "pip install gcovr" in err
    # and it must say the profile is not lost
    assert str(tmp_path) in err


def test_gcov_tool_reads_the_compiler_cmake_used(tmp_path: Path) -> None:
    (tmp_path / "CMakeCache.txt").write_text(
        "CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++\nOTHER:BOOL=ON\n")
    assert apptests._gcov_tool(tmp_path) == []


def test_gcov_tool_asks_for_llvm_cov_under_clang(tmp_path: Path, monkeypatch) -> None:
    """Clang's .gcda are llvm's; plain gcov cannot parse them, so gcovr has to be
    told. Getting this wrong produces an empty report, not an error."""
    (tmp_path / "CMakeCache.txt").write_text(
        "CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++\n")
    monkeypatch.setattr(apptests.shutil, "which", lambda _n: "/usr/bin/llvm-cov")
    assert apptests._gcov_tool(tmp_path) == ["/usr/bin/llvm-cov gcov"]


def test_gcov_tool_reports_missing_llvm_cov(tmp_path: Path, monkeypatch) -> None:
    (tmp_path / "CMakeCache.txt").write_text(
        "CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++\n")
    monkeypatch.setattr(apptests.shutil, "which", lambda _n: None)
    assert apptests._gcov_tool(tmp_path) is None


# --- the CLI surface -------------------------------------------------------


def test_new_with_tests_scaffolds(tmp_path: Path, monkeypatch, capsys) -> None:
    assert _new(tmp_path, "proj", "--board", "nucleo_g071rb", "--with-tests",
                monkeypatch=monkeypatch) == 0
    assert (tmp_path / "proj" / "tests" / "test_app.cpp").exists()
    assert (tmp_path / "proj" / "src" / "app.hpp").exists()
    assert "alloy test" in capsys.readouterr().out


def test_new_without_the_flag_is_unchanged(tmp_path: Path, monkeypatch) -> None:
    assert _new(tmp_path, "proj", "--board", "nucleo_g071rb",
                monkeypatch=monkeypatch) == 0
    assert not (tmp_path / "proj" / "tests").exists()
    assert "app.hpp" not in (tmp_path / "proj" / "src" / "main.cpp").read_text()


@pytest.mark.parametrize(("verb", "flag"), [
    ("new", "--with-tests"), ("test", "--framework"), ("test", "--coverage"),
])
def test_the_new_flags_are_in_the_help(verb: str, flag: str, monkeypatch,
                                       capsys) -> None:
    assert _cli(monkeypatch, verb, "--help") == 0
    assert flag in capsys.readouterr().out
