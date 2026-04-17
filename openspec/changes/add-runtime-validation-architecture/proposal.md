## Why

`alloy` is becoming descriptor-driven, but the current test layout does not prove the behaviors that
matter most for a bare-metal runtime:

- whether the selected device starts through the expected startup path
- whether board bring-up writes the right registers in the right order
- whether the descriptor-driven hot path stays zero-overhead
- whether the runtime can scale across many MCUs without requiring the physical board for every
  change

Today the repo has compile tests, unit tests, integration tests, regression tests, RTOS tests,
and separate assembly checks, but those suites are not organized as one validation architecture.
They also do not define a clear confidence ladder from "compiles" to "boots in emulation" to
"passes on hardware".

Because `alloy` owns runtime behavior, the validation architecture must live in `alloy`, not in
`alloy-devices`. The device repo should continue to publish facts; the runtime repo should prove
that it consumes those facts correctly.

## What Changes

- **NEW** define a first-class runtime validation architecture in `alloy`
- **NEW** keep validation ownership in `alloy`, with `alloy-devices` used as an input source for
  descriptors and generated artifacts
- **NEW** introduce a layered validation model:
  - compile-contract coverage
  - ELF/startup inspection
  - host MMIO validation
  - Renode emulation
  - hardware spot-checks
  - assembly / size verification
- **NEW** define a host MMIO backend that records register effects without introducing production
  runtime overhead
- **NEW** define a Renode harness for descriptor-driven bring-up, starting with `same70`
- **NEW** define the initial representative runtime-validation matrix explicitly:
  - `same70_xplained`
  - `nucleo_g071rb`
  - `nucleo_f401re`
- **NEW** require each emulation-covered foundational board to prove both boot progress and a
  deterministic debug-UART observable
- **NEW** reorganize the test tree around validation purpose instead of historical category names
- **NEW** define CMake/CTest profiles and labels for validation layers
- **NEW** operationalize representative host MMIO and Renode ladders in GitHub Actions CI
- **MODIFIED** strengthen zero-overhead verification so that "zero-overhead" is backed by explicit
  validation gates instead of design intent only

## Why Same70 First

`same70` is a good first emulation target because:

- it is already part of the descriptor-driven migration
- it exercises a non-STM32 bring-up path early
- it pressures the architecture to stay multi-vendor from the beginning
- once the harness works on `same70`, adding STM32 and other families is less likely to hide
  vendor assumptions

## Scope

This change defines the architecture, ownership, taxonomy, and rollout plan for runtime
validation. It does **not** require immediate implementation of every test layer in the same
change. The implementation is intentionally phased:

1. stabilize the test taxonomy and build profiles
2. implement host MMIO validation
3. add `Renode` for `same70`
4. migrate other families afterward
5. operationalize the representative validation ladders in CI

## Impact

- Affected specs:
  - `runtime-validation` (new)
  - `build-and-selection`
  - `zero-overhead-runtime`
- Affected code and layout:
  - `tests/**`
  - `tools/assembly_verification/**`
  - `cmake/**`
  - `.github/workflows/**`
  - `CMakePresets.json`
  - startup and bring-up smoke coverage
  - future `Renode` scripts and platform overlays
