# Design: Custom Board Bring-Up Path

## Context
The runtime/device boundary in `project.md` is non-negotiable: behaviour lives in the
runtime, hardware facts live in `alloy-devices`, and boards are thin shims. Today, the
board layer also has to live *inside the runtime tree* because `alloy_resolve_board_manifest`
is a closed `if/elseif` chain. That keeps in-tree boards tidy but is incompatible with the
larger product story, where most boards belong to user projects and not to the alloy repo.

This change adds the smallest possible escape hatch to make user-owned boards first-class
without weakening the existing in-tree boards or bypassing the descriptor pipeline.

## Goals / Non-Goals

Goals:
- A user project can declare its own board outside the runtime tree and consume Alloy
  through the same `alloy::alloy` target the in-tree boards use.
- The descriptor-driven runtime path (`alloy_devices.cmake` → `selected.hpp`) is not
  altered: the custom board feeds it the same vendor/family/device tuple an in-tree
  board would feed it.
- The contract between user CMakeLists and the runtime is small, named, and documented.
- The in-tree foundational boards keep working without modification.
- A failure to set required cache variables produces a clear error, not a surprise build
  failure deep inside descriptor resolution.

Non-Goals:
- Generating linker scripts, startup glue, or board sources from `alloy-devices` data.
  That is the scaffolder's job and may not be deterministic for every MCU.
- Letting the user override the runtime with arbitrary code injection. The custom board
  branch only reads header/linker/device-tuple inputs; it does not accept code hooks.
- Supporting multiple custom boards selected at the same time. `ALLOY_BOARD=custom` is a
  singleton; multi-board projects are out of scope.

## Decisions

### Decision: Use a sentinel board name (`custom`) rather than auto-detecting
- Rationale: explicit selection avoids accidental mode switches when a user mistypes a
  board name. It also keeps the existing in-tree boards' resolution path untouched.
- Alternatives considered: auto-fall-through when no in-tree branch matches (rejected:
  silently switches modes), a build option `ALLOY_USE_CUSTOM_BOARD=ON` (rejected: more
  state to reason about than a single `ALLOY_BOARD` value).

### Decision: Custom-board inputs are CMake cache variables, not files
- Rationale: cache variables compose with `CMakePresets.json` cleanly, are visible in
  `cmake-gui`, and surface in `cmake -L`. They keep the contract first-class instead of
  hiding it inside an opt-in include.
- Alternatives considered: a `custom_board.cmake` file that the user is required to
  provide at a fixed path (rejected: invisible to tooling, harder to template), a
  JSON/TOML schema parsed by CMake (rejected: extra parser surface for no clear win).

### Decision: Required vs optional split is minimal and explicit
- Required: `ALLOY_CUSTOM_BOARD_HEADER`, `ALLOY_CUSTOM_LINKER_SCRIPT`,
  `ALLOY_DEVICE_VENDOR`, `ALLOY_DEVICE_FAMILY`, `ALLOY_DEVICE_NAME`, `ALLOY_DEVICE_ARCH`.
- Optional: `ALLOY_DEVICE_MCU` (cosmetic; used in info output), `ALLOY_FLASH_SIZE_BYTES`
  (used to size flash regions when descriptor lacks the data; defaults to 0 = unknown).
- Rationale: vendor/family/device are the keys the descriptor pipeline already needs; the
  header and linker script are the only two file inputs the runtime cannot derive.
- Trade-off: `ALLOY_DEVICE_ARCH` is required and duplicates information that *could* be
  inferred from descriptor data, but the manifest function returns it today, and other
  call sites depend on it. Inferring is a separate refactor.

### Decision: Validate descriptor existence at configure time
- The custom branch resolves `${ALLOY_DEVICES_ROOT}/<vendor>/<family>/generated/runtime/devices/<device>` and fails fast if the directory does not exist.
- Rationale: if the descriptor is missing, every downstream include in
  `alloy_devices.cmake` fails with cryptic file-not-found errors. Failing here keeps the
  diagnostic close to the cause.

### Decision: Add one focused configure test, not a full hardware test
- A synthetic external board folder under `tests/custom_board/` declares a known device
  (we will reuse the `nucleo_g071rb` descriptor) and configures alloy as a subproject.
- Rationale: the goal is to lock the contract, not to revalidate the full descriptor
  pipeline. The existing board-bringup tests already cover descriptor consumption.
- Trade-off: the synthetic test does not flash hardware. That is intentional; hardware
  validation continues to live in `tests/hardware/`.

## Risks / Trade-offs

- **Two branches in `alloy_resolve_board_manifest`.** Adds maintenance surface. Mitigation:
  the custom branch is small (cache reads + descriptor existence check) and shares the
  same outputs, so cross-cutting refactors stay easy.
- **Users skipping the CLI.** A user copying the contract by hand may forget a required
  variable. Mitigation: required-variable validation produces a one-line error naming
  the missing variable.
- **Drift between in-tree and external boards.** If we change what the manifest emits,
  external boards must follow. Mitigation: the custom-board doc lives next to the
  manifest source so the contract changes together with the function.
- **Board header search path.** The runtime today resolves board headers against
  `${CMAKE_SOURCE_DIR}` (the runtime tree). For a custom board, the header lives in the
  consuming project. The custom branch will resolve absolute paths only and document
  this; relative paths in cache variables will be rejected.

## Migration Plan
This change is additive. In-tree boards continue to use their dedicated branches. A
follow-up CLI change (`add-project-scaffolding-cli` task 5b) will switch *new* projects
to the custom path; the in-tree examples are not migrated.

## Open Questions
- Should the runtime emit a one-line `INFO` line at configure time when the custom
  branch is taken, naming vendor/family/device? Useful for debugging, but adds noise.
  Lean: yes, behind an existing `ALLOY_VERBOSE` flag if one exists.
- `ALLOY_DEVICE_ARCH` mapping: the manifest currently distinguishes `cortex-m0plus`,
  `cortex-m4`, `cortex-m7`, `riscv`, `avr`, `native`. Should we validate the user-supplied
  value against an enum, or accept any non-empty string and let downstream layers fail?
  Lean: validate, with a clear error listing accepted values.
