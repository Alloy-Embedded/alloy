## Why

`alloy` is now much cleaner internally than many embedded libraries, but it still loses to the
best ecosystems on user experience.

The main gaps are:

- too much repo knowledge is still required to build, flash, debug, and validate boards
- diagnostics for connector/clock/resource conflicts are not yet a polished user-facing story
- examples and docs do not yet cover the public HAL surface like a product
- support status is not yet obvious from one public matrix

This is the main reason stronger internal architecture does not yet translate into a clearly
superior developer experience.

## What Changes

- define stable board-oriented build, flash, debug, and validation entry points
- surface connector/clock/resource diagnostics as a user-facing runtime/build experience
- add first-class package/integration entry points for downstream CMake consumers
- add user-facing explain/diff tooling on top of generated diagnostics and provenance
- build a complete official example and cookbook set for the main public HAL surface
- publish explicit board and peripheral support matrices
- add migration guides from vendor HALs, `modm`, and lower-level libraries

## Outcome

After this change, users should be able to discover:

- what is supported
- how to build it
- how to debug it
- why a configuration failed
- why a generated fact exists and how two targets differ
- where to find the canonical example

without reading internal implementation files first.

## Impact

- Affected specs:
  - `runtime-tooling` (new)
  - `build-and-selection`
  - `migration-cleanup`
- Affected code and docs:
  - `cmake/**`
  - `CMakePresets.json`
  - `examples/**`
  - `docs/**`
  - `scripts/**`
