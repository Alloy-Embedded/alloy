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
- [ ] 2.1 Add `alloy sdk install <version>` that fetches the runtime and `alloy-devices`
      into `~/.alloy/sdk/<version>/{runtime,devices}` with pinned commit SHAs
- [ ] 2.2 Add `alloy sdk list`, `alloy sdk use <version>`, `alloy sdk path` (prints active path)
- [ ] 2.3 Persist the active version in `~/.alloy/config.toml`
- [ ] 2.4 Add `--vendored` mode: clone runtime and devices into the project tree and write
      a project-local `alloy.lock` with SHAs
- [ ] 2.5 Tests: install a tagged version into a temp `ALLOY_HOME`, assert SHAs match

## 3. Toolchain manager
- [ ] 3.1 Add `tools/alloy-cli/src/alloy_cli/toolchains.toml` with curated pins for xPack
      `arm-none-eabi-gcc`, OpenOCD, and `avr-gcc`
- [ ] 3.2 Add `alloy toolchain install <name>[@version]` that downloads the pinned release
      into `~/.alloy/toolchains/<name>/<version>` with checksum verification
- [ ] 3.3 Add `alloy toolchain list`, `alloy toolchain which <name>`, `alloy toolchain use`
- [ ] 3.4 Wire `alloy doctor --fix` to install missing toolchains after explicit consent
- [ ] 3.5 Tests: stub the download URL, verify checksum failure aborts; verify generated
      paths land in `~/.alloy/toolchains/...`

## 4. Project scaffolding (`alloy new`)
- [ ] 4.1 Replace the body of `alloy new` with a templated generator that emits:
      `CMakeLists.txt`, `CMakePresets.json`, `src/main.cpp`, `.gitignore`, `README.md`
- [ ] 4.2 The generated `CMakeLists.txt` consumes Alloy through `find_package(Alloy CONFIG)`
      using the active SDK path; no `-DALLOY_ROOT` hand-edit required
- [ ] 4.3 The generated `CMakePresets.json` references the pinned toolchain path from the
      toolchain manager
- [ ] 4.4 Generate `.vscode/{settings.json,tasks.json,launch.json}`:
      - clangd settings pointing at `build/<preset>/compile_commands.json`
      - tasks for `alloy build`, `alloy flash`, `alloy monitor`
      - launch config for OpenOCD (and J-Link where the board manifest declares it)
- [ ] 4.5 `alloy new` validates that the active SDK and required toolchain are present and
      offers to install them when missing
- [ ] 4.6 Tests: scaffold a project for at least three boards (one ST, one Microchip, one
      RPi), configure and build them in CI, run their host-test counterparts where available

## 5. Raw-MCU scaffolding
- [ ] 5.1 Extend `alloy new` to accept `--mcu <part-number>` instead of `--board`
- [ ] 5.2 Resolve the MCU against `alloy-devices`; fail with a clear message and pointer
      to the descriptor repo if the part is not yet supported
- [ ] 5.3 Generate a minimal board layer (startup + clock + linker script) when no manifest
      board exists; do not synthesize peripherals
- [ ] 5.4 Document the supported set in `docs/SUPPORT_MATRIX.md`
- [ ] 5.5 Tests: scaffold and build for at least one `--mcu` target that has no manifest entry

## 6. Documentation and migration
- [ ] 6.1 Rewrite `docs/QUICKSTART.md` to lead with `pipx install alloy-cli` and `alloy new`
- [ ] 6.2 Update `docs/CMAKE_CONSUMPTION.md` to describe the scaffolded project layout as
      the recommended downstream consumption path
- [ ] 6.3 Update `docs/BOARD_TOOLING.md` with the new flash/monitor entry points
- [ ] 6.4 Add `docs/CLI.md` describing the full subcommand surface
- [ ] 6.5 Mark `scripts/alloyctl.py` as deprecated alias; print a one-line notice on use

## 7. Installer and distribution
- [ ] 7.1 Publish `alloy-cli` to PyPI (resolve naming question first)
- [ ] 7.2 Add a one-line installer script (`get.alloy.dev` or hosted in-repo) that
      installs `pipx` if missing and runs `pipx install alloy-cli`
- [ ] 7.3 Add release automation: tagging the runtime publishes a matching CLI version
- [ ] 7.4 Add `alloy --version` reporting CLI, active SDK, and active toolchain versions
