# Design: Project Scaffolding CLI

## Context
The runtime-tooling spec already commits to stable, board-oriented entry points for configure,
build, flash, monitor, and downstream CMake consumption. What it does not yet cover is how a
user *starts* a project and acquires the toolchain. Today that gap is filled by ad-hoc
documentation plus `scripts/alloyctl.py`, which is only reachable from inside a runtime
checkout. To make Alloy a credible multi-vendor product, the scaffolding and acquisition step
must be a first-class part of the tooling surface rather than an undocumented prerequisite.

This design picks the smallest set of decisions that lets us deliver scaffolding incrementally
without locking the project into choices we cannot revisit later.

## Goals / Non-Goals

Goals:
- One supported command path from "I have nothing installed" to "I am editing `main.cpp` in
  VS Code with working build, flash, and debug".
- The same CLI is the entry point for new and existing projects (`new`, `build`, `flash`,
  `monitor`, `doctor`, `sdk`, `toolchain`).
- SDK and toolchain acquisition are explicit, versioned, and inspectable, not hidden inside
  CMake or shell scripts.
- The scaffolded project consumes Alloy through the documented downstream CMake contract; it
  does not rely on internal targets or in-tree examples.
- Existing `alloyctl.py` users keep working until the new CLI is at parity.

Non-Goals:
- Replacing CMake, the toolchain files, or the board manifest.
- Generating vendor HAL code (that remains the job of `alloy-devices` / codegen).
- Shipping a VS Code extension or IDE plugin in this change.
- Supporting arbitrary user-supplied toolchains at install time; the toolchain manager pins a
  small, curated set.

## Decisions

### Decision: Implementation language is Python, distributed via `pipx`/`uv tool install`
- Rationale: `alloyctl.py` is already Python and contains most of the configure/build/flash
  logic we need. A Python CLI installable through `pipx` gives us a single-command install
  on macOS, Linux, and WSL, and avoids the maintenance cost of a Rust/Go rewrite during the
  formative phase.
- Alternatives considered:
  - Rust/Go single binary: better polish and startup time, but doubles the work and forces a
    rewrite of the alloyctl logic before any user-visible improvement ships.
  - Bash: insufficient for SDK/toolchain manifest handling and cross-platform support.
- Trade-off accepted: users must have Python 3.10+ available; the one-line installer
  bootstraps `pipx` if missing. A future migration to a single binary remains possible
  because the CLI surface is contractually stable.

### Decision: Shared SDK cache at `~/.alloy/sdk/<version>` with opt-in per-project vendoring
- Rationale: A shared cache makes `alloy new` fast and keeps disk usage bounded when users
  have many projects. CI and reproducibility-sensitive users can opt into vendored mode
  (`alloy new --vendored`), which clones the runtime and descriptors into the project tree
  and pins commit SHAs in a project-local lockfile.
- Alternatives considered: always vendor (slow, large repos), always shared (breaks CI
  reproducibility, forces global state).
- Trade-off accepted: two modes to support, but the vendored mode is a thin variant that
  reuses the same generated CMake.

### Decision: Toolchain manager pins curated releases, does not wrap a system package manager
- Rationale: Multi-vendor support means we cannot rely on system packages being current or
  even available (xPack is not in most distro repos). The CLI downloads pinned upstream
  releases (xPack ARM GCC, OpenOCD, AVR GCC) into `~/.alloy/toolchains/<name>/<version>`
  and points generated CMake presets at those paths. Users can override with
  `--system-toolchain` to opt out.
- Alternatives considered: delegating to Homebrew/apt (not portable, version drift), writing
  a generic toolchain protocol (premature; we have ~3 toolchains to manage).
- Trade-off accepted: we own the toolchain manifest and must update it when xPack/OpenOCD
  release. A small `toolchains.toml` in-repo keeps that update mechanical.

### Decision: VS Code integration is generated files, not an extension
- Rationale: Files (`.vscode/launch.json`, `settings.json`, `tasks.json`) deliver 90% of the
  value (one-click build/flash/debug, working clangd) at a fraction of the maintenance cost
  of a published extension.
- Trade-off: users do not get a board picker UI; they re-run `alloy new` or edit the preset.
  Acceptable for the first iteration.

### Decision: Raw-MCU scaffolding requires an `alloy-devices` descriptor; no synthetic boards
- Rationale: The runtime/device boundary in `project.md` is non-negotiable. `alloy new --mcu
  STM32G474RET6` resolves the MCU against `alloy-devices` and generates a minimal board
  layer (LED-less, UART-less, just startup + clock) when no full board manifest entry
  exists. If the descriptor is missing, the command fails with a clear pointer to
  `alloy-devices` rather than synthesizing one.
- Trade-off: coverage of `alloy new --mcu` is gated on descriptor coverage. This keeps the
  runtime honest and pushes vendor breadth into the right repo.

## Risks / Trade-offs

- **Two CLIs during transition.** `alloyctl.py` and the new `alloy` CLI must coexist until
  parity is reached. Mitigation: the new CLI delegates to `alloyctl.py` internally for
  flash/monitor in early phases; `alloyctl.py` becomes a thin shim once parity lands.
- **Toolchain pin staleness.** If `toolchains.toml` is not updated, users get old compilers.
  Mitigation: `alloy doctor` warns when the pinned toolchain is older than N months; CI runs
  weekly to refresh pins.
- **Network dependency at scaffolding time.** `alloy new` needs network access for SDK and
  toolchain fetch. Mitigation: `--offline` flag uses an existing cache; clear errors when
  cache is missing.
- **Windows support.** Initial scope is macOS + Linux + WSL. Native Windows is best-effort
  until the toolchain pins are validated there.

## Migration Plan
1. Land the CLI package (`tools/alloy-cli/`) as a thin wrapper that re-exports
   `alloyctl.py` behavior. Users can `pipx install` it from the repo.
2. Add SDK manager and toolchain manager incrementally, behind explicit subcommands. Old
   flows continue to work.
3. Replace the body of `alloyctl.py` with a call into the new CLI once parity lands; keep
   the script as a deprecated alias for one release.
4. Update `QUICKSTART.md` to lead with `alloy new`; keep the in-tree example flow as a
   secondary path for runtime contributors.

## Resolved Questions
- PyPI distribution name is `alloy-cli`. The console script is `alloy`.

## Open Questions
- Lockfile format for vendored mode: TOML, JSON, or reuse `CMakePresets.json` machinery?
- How aggressive should `alloy doctor --fix` be: install missing toolchains automatically,
  or print the exact `alloy toolchain install` command and require explicit consent?
