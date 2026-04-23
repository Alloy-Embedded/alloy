## 1. OpenSpec Baseline

- [x] 1.1 Add the `runtime-validation` capability
- [x] 1.2 Add build-and-selection deltas for validation profiles
- [x] 1.3 Add zero-overhead-runtime deltas for explicit validation gates
- [x] 1.4 Validate the change with `openspec validate add-runtime-validation-architecture --strict`

## 2. Test Taxonomy And Build Plumbing

- [x] 2.1 Define the target `tests/` layout for compile, ELF, host MMIO, Renode, hardware, and assembly validation
- [x] 2.2 Add CMake / CTest labels or equivalent build profiles for each validation layer
- [x] 2.3 Keep current legacy test directories buildable during migration

## 3. Host MMIO Foundation

- [x] 3.1 Introduce a test-only MMIO recording backend that does not change production hot paths
- [x] 3.2 Add trace/assertion helpers for register writes, ordering, and final register state
- [x] 3.3 Migrate foundational bring-up tests to the host MMIO path
- [x] 3.4 Cover startup-adjacent bring-up flows such as clock/reset/GPIO/UART initialization

## 4. ELF / Startup Inspection

- [x] 4.1 Add cross-toolchain checks for vector table presence, reset symbol, and relevant sections
- [x] 4.2 Define the minimum startup proof required before a board is considered emulation-ready

## 5. Same70 Renode Bring-Up

- [x] 5.1 Add `Renode` common scripts, hooks, and boot assertions
- [x] 5.2 Add the first `same70` platform overlay and launch script
- [x] 5.3 Prove boot to `main` or a deterministic boot milestone for the first `same70` scenario
- [x] 5.4 Observe key bring-up behavior in emulation without creating a second device database

## 6. Zero-Overhead Hardening

- [x] 6.1 Connect assembly / size verification to the new validation architecture
- [x] 6.2 Keep zero-overhead checks separate from behavioral correctness checks

## 7. Expansion After Same70

- [x] 7.1 Decide the second representative `Renode` family after `same70`
- [x] 7.2 Migrate more descriptor-driven tests into the new taxonomy
- [x] 7.3 Add representative hardware spot-check plans for foundational boards

## 8. CI Operationalization And Coverage Closure

- [x] 8.1 Close the clean host validation preset flow so `host_mmio` can run from a fresh checkout
- [x] 8.2 Make representative emulation coverage explicit and keep each covered board tied to a deterministic UART observable
- [x] 8.3 Run representative host MMIO and `Renode` validation in GitHub Actions CI
