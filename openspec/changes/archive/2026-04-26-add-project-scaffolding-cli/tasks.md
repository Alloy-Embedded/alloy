# Tasks: Project Scaffolding CLI

Tasks are grouped by phase. Each phase is independently mergeable and leaves the repo in a
working state. Do not start a phase before the previous phase is reviewed and merged.

## 1. Package the CLI
- [x] 1.1 Create `tools/alloy-cli/` with `pyproject.toml`, `src/alloy_cli/__init__.py`,
      and entry point `alloy = alloy_cli.main:main`
- [x] 1.2 Re-export current `alloyctl.py` subcommands (`doctor`, `configure`, `build`,
      `flash`, `monitor`, `new`, `validate`, `info`, `compile-commands`) by delegating to
      the existing implementation; no behavior change
- [x] 1.3 Add `pipx`/`uv tool install` instructions to `docs/QUICKSTART.md`
- [x] 1.4 Add a smoke test that installs the package in a temp venv and runs `alloy --help`
- [x] 1.5 Add CI job that exercises `alloy --version` and `alloy info` against the repo

## 2. SDK manager
- [x] 2.1 Add `alloy sdk install <version>` that fetches the runtime and `alloy-devices`
      into `~/.alloy/sdk/<version>/{runtime,devices}` with pinned commit SHAs
- [x] 2.2 Add `alloy sdk list`, `alloy sdk use <version>`, `alloy sdk path` (prints active path)
- [x] 2.3 Persist the active version in `~/.alloy/config.toml`
- [x] 2.4 `--vendored` mode for `alloy new`: DEFERRED to follow-up
      `add-vendored-sdk-mode`. The `alloy sdk install` + `sdk use` path covers the
      shared-SDK case; vendoring runtime and devices into the project tree (with a
      project-local `alloy.lock`) is only meaningful from `alloy new --vendored`,
      which itself is a separate UX surface. No user has hit a wall without it
      yet — kept as a clean follow-up rather than a half-shipped flag.
- [x] 2.5 Tests: install a tagged version into a temp `ALLOY_HOME`, assert SHAs match

## 3. Toolchain manager
- [x] 3.1 Add `tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml` with curated pins for
      xPack `arm-none-eabi-gcc` and OpenOCD (sha256 placeholders pending validation;
      AVR-GCC pin deferred until needed by an active board)
- [x] 3.2 Add `alloy toolchain install <name>[@version]` that downloads the pinned release
      into `~/.alloy/toolchains/<name>/<version>` with checksum verification
- [x] 3.3 Add `alloy toolchain list`, `alloy toolchain which <name>`
      (use/select deferred: a single pinned default version per toolchain is sufficient
      until multi-version coexistence is needed)
- [x] 3.4 `alloy doctor --fix` auto-install: DEFERRED to follow-up
      `add-doctor-autofix`. The current `alloy doctor` reports missing toolchains
      with the exact `alloy toolchain install <name>` command for the user to run.
      Auto-install requires the pinned SHA256 set in `_toolchain_pins.toml` to be
      fully validated end-to-end (currently a mix of validated and placeholder
      hashes per task 3.1) before silent installs are safe. The "report and let
      the user run the install command" path is shipping in this change.
- [x] 3.5 Tests: stub the download URL, verify checksum failure aborts; verify generated
      paths land in `~/.alloy/toolchains/...`

## 4. Project scaffolding (`alloy new`)
- [x] 4.1 Replace the body of `alloy new` with a templated (Jinja2) generator that emits:
      `CMakeLists.txt`, `CMakePresets.json`, `src/main.cpp`, `.gitignore`, `README.md`
- [x] 4.2 The generated `CMakeLists.txt` consumes Alloy through
      `add_subdirectory(${ALLOY_ROOT})` with the active SDK path baked in
      (find_package migration deferred until pinned toolchain SHAs land; see design.md)
- [x] 4.3 The generated `CMakePresets.json` references the pinned toolchain bin dir on
      PATH when the toolchain is installed; otherwise defers to the user's environment
- [x] 4.4 Generate `.vscode/{settings.json,tasks.json,launch.json}`:
      clangd settings, build/flash/monitor tasks, OpenOCD launch when the board declares one
- [x] 4.5 `alloy new` validates that the active SDK is present and reports missing
      toolchain with the exact `alloy toolchain install` command (auto-install gated until
      sha256 pins are validated)
- [x] 4.6 CI scaffold-and-configure cross-board: DEFERRED to follow-up
      `add-cli-scaffold-ci-coverage`. Requires pinned-toolchain SHAs to be fully
      validated (same blocker as 3.4) so CI can install them without human-in-the-
      loop hash review. Today the scaffold is exercised by 65 unit tests against
      a synthetic SDK; cross-board configure-and-build in CI lands when the pin
      validation finishes.

## 5. Custom-board scaffolding (depends on add-custom-board-bringup)
- [x] 5.1 Switch every scaffold to the custom-board path: `alloy new` always generates a
      `board/` directory in the user's project and emits a CMakeLists that sets
      `ALLOY_BOARD=custom` plus the runtime's documented cache variables
- [x] 5.2 `--board <name>`: copy `boards/<name>/` from the active SDK into
      `<project>/board/` so the user starts from a known-good board and can extend it
- [x] 5.3 `--mcu <part-number>`: resolve the MCU against `alloy-devices`. If a board
      already exists in the catalog for that MCU, copy it (same as 5.2). Otherwise,
      generate a skeleton board (board.hpp, board_config.hpp, board.cpp, syscalls.cpp,
      linker script) deriving FLASH/RAM from `capabilities.json` when present and
      falling back to clearly-marked TODOs with a one-line warning otherwise
- [x] 5.4 Validate `<vendor>/<family>/<device>` against `alloy-devices/<sdk>/devices`
      before scaffolding; fail fast with a pointer to `alloy-devices` if the descriptor
      is missing
- [x] 5.5 Document the supported set and the custom-board recipe in
      `docs/SUPPORT_MATRIX.md` and reference `docs/CUSTOM_BOARDS.md`
- [x] 5.6 Tests: scaffold for `--board nucleo_g071rb`, `--mcu STM32G071RBT6` (catalog
      alias), `--mcu STM32G474RET6` (descriptor-only with full memory data), and an
      `--mcu` whose descriptor lacks memory regions (warning path); 56 tests total

## 6. Documentation and migration
- [x] 6.1 Rewrite `docs/QUICKSTART.md` to lead with `pipx install alloy-cli` and `alloy new`
- [x] 6.2 Update `docs/CMAKE_CONSUMPTION.md` to describe the scaffolded project layout as
      the recommended downstream consumption path
- [x] 6.3 Update `docs/BOARD_TOOLING.md` with the new flash/monitor entry points
- [x] 6.4 Add `docs/CLI.md` describing the full subcommand surface
- [x] 6.5 Mark `scripts/alloyctl.py` as deprecated alias; print a one-line notice on use
      (the notice is suppressed when alloy-cli delegates internally)

## 7. Installer and distribution
- [x] 7.1 `alloy-cli` published to PyPI under name `alloy-cli`. The release workflow
      `.github/workflows/release.yml` (job `publish-pypi`) uses
      `pypa/gh-action-pypi-publish` with OIDC trusted publishing (no secrets);
      hatch-vcs derives the wheel version from the git tag and the workflow
      verifies tag-vs-built parity before publish.
- [x] 7.2 One-line installer script: DEFERRED to follow-up
      `add-cli-installer-script`. A `get.alloy.dev`-style installer requires
      hosted infrastructure (DNS + static host) outside this repo. The shipped
      path is `pipx install alloy-cli` documented in `docs/QUICKSTART.md` and
      `docs/CLI.md` — that's the one command users type. Adding a wrapper
      script is convenience over substance and can land independently.
- [x] 7.3 Release automation tag → CLI version: `release.yml` triggers on
      `push.tags`, hatch-vcs derives the package version from the tag, the
      `verify-version` step rejects mismatches, and `publish-pypi` ships the
      wheel automatically. Single source of truth: the git tag.
- [x] 7.4 `alloy --version` reports CLI version, active SDK (with present /
      missing flag), and every installed toolchain (one line each). Output is
      grep-stable: tooling can match `^alloy `, `^sdk `, `^toolchain ` without
      parsing prose. Missing entries print `(none)` explicitly.
