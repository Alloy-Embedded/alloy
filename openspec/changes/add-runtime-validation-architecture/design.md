## Context

The runtime boundary is moving in the right direction:

- `alloy-devices` owns generated device facts
- `alloy` owns runtime behavior

Validation must follow the same split.

If runtime validation is scattered across ad hoc tests, hardware-only checks, and one-off scripts,
the repo will keep answering the wrong questions:

- "Does it compile?"
- instead of "Did the runtime consume the descriptor contract correctly?"

For a bare-metal runtime, the real questions are:

- did startup integrate the selected vectors and reset path correctly?
- did board bring-up configure clocks, resets, routes, and GPIO as intended?
- did the runtime stay zero-overhead on the hot path?
- can we verify those properties without owning every board?

This design creates one validation architecture that answers those questions in layers.

## Goals

- keep validation ownership in `alloy`
- validate descriptor consumption without needing hardware for every change
- separate what can be proven on the host from what requires emulation or silicon
- add `Renode` without blocking on a new codegen format first
- start with `same70` and then generalize to other foundational families
- make "zero-overhead" a measured property, not a slogan

## Non-Goals

- moving runtime behavior into `alloy-devices`
- rebuilding `alloy-codegen` before proving the first validation path
- requiring full peripheral simulation before enabling the first `Renode` target
- replacing all current tests in one commit
- claiming that host MMIO or emulation can replace all hardware validation

## Decision 1: Validation Lives In The Alloy Repo

Validation of runtime behavior SHALL live in `alloy`.

Reason:

- `alloy-devices` publishes facts
- `alloy` applies behavior to those facts
- only `alloy` can prove that startup, bring-up, routing, and driver logic behave correctly

Implication:

- test harnesses
- MMIO backends
- `Renode` scripts
- board bring-up smoke programs
- assembly/size gates

belong in `alloy`.

`alloy-devices` remains an input to validation, not the owner of runtime validation policy.

## Decision 2: Validation Is Layered By Confidence

The validation stack SHALL be organized as a confidence ladder.

| Layer | Primary question | Runs on | Expected speed | Coverage |
|---|---|---:|---:|---|
| Compile contract | Does the selected API/device contract compile? | host CI | fast | broad |
| ELF / startup inspection | Are vectors, symbols, sections, and startup artifacts present? | host CI with cross toolchain | fast | broad |
| Host MMIO | Did bring-up write the right registers in the right order? | host CI | fast | broad on foundational flows |
| Renode emulation | Does the selected firmware actually start and reach the expected boot milestone? | host CI / nightly | medium | representative families |
| Hardware spot-check | Does the representative board behave on real silicon? | lab / gated CI | slow | selective |
| Assembly / size | Did the hot path stay zero-overhead? | host CI | fast | focused |

No single layer replaces the others.

## Decision 3: Host MMIO Is The Canonical Runtime Bring-Up Harness

The first scalable validation backend SHALL be a host MMIO backend.

### What It Proves

The host MMIO backend proves:

- register writes and reads issued by the runtime
- ordering of side effects
- final register state
- descriptor-driven bring-up behavior for clocks, resets, GPIO, pinmux, and foundational UART

### What It Does Not Prove

The host MMIO backend does **not** prove:

- real CPU reset execution
- instruction-exact startup on Cortex-M
- flash wait states, PLL lock, analog behavior, or silicon errata

Those belong to ELF/startup inspection, `Renode`, or hardware.

## Decision 4: Host MMIO Must Not Pollute Production Runtime Paths

The test backend SHALL be selected at build time and SHALL NOT introduce runtime indirection into
production builds.

Target shape:

```text
production:
  direct_mmio_transport
    -> inline register access

host validation:
  recording_mmio_transport
    -> same register operation surface
    -> writes/reads captured in trace buffers
```

The production path remains:

- inline
- non-virtual
- non-allocating

The host path is test-only and may record trace information.

## Decision 5: Start By Reusing Existing Descriptor Artifacts

The first implementation SHALL reuse existing published artifacts from `alloy-devices` and the
current runtime contract.

It SHALL NOT require a new generator format before the first validation target works.

This avoids a common failure mode:

- inventing a large new validation schema
- blocking implementation on generator churn
- not learning whether the harness architecture is actually correct

Initial data sources may include:

- selected runtime headers
- generated startup source / vectors
- existing device metadata and reports

Only after `same70` works should the team decide whether a dedicated generated validation manifest
is justified.

## Decision 6: Reorganize Tests By Validation Purpose

The current `tests/` tree SHALL move toward a purpose-driven layout.

Target shape:

```text
tests/
  compile/
  elf/
  host_mmio/
    framework/
    scenarios/
    fixtures/
  emulation/
    renode/
      common/
      same70/
  hardware/
    common/
    same70/
  perf/
    assembly/
```

Migration note:

- `tests/unit`, `tests/integration`, `tests/regression`, and `tools/assembly_verification`
  may remain temporarily
- but the target architecture is the layout above

The new layout answers "what is this proving?" at a glance.

## Decision 7: CMake And CTest Must Expose Validation Profiles

Validation layers SHALL be first-class in build selection.

Target concepts:

- `compile`
- `elf`
- `host-mmio`
- `renode`
- `hardware`
- `assembly`

Expected outcomes:

- test selection by CTest label
- clean CI matrix definition
- straightforward local execution for one device or one layer

Representative commands may look like:

```text
ctest -L host-mmio
ctest -L renode
ctest -L assembly
```

The exact target names can change, but the validation layers must be explicit and stable.

## Decision 8: Same70 Is The First Renode Target

The first emulation target SHALL be `same70`.

### Acceptance For The First Target

The initial `same70` `Renode` harness should prove at least:

- the ELF loads successfully
- the CPU starts from the selected startup path
- execution reaches a boot milestone:
  - `main`
  - or a known UART banner
  - or a known symbol/hook point
- foundational bring-up does not hard-fault
- expected startup / bring-up register activity can be observed

### Peripheral Scope For The First Target

The first target does not need full SoC fidelity.

It is acceptable to combine:

- built-in `Renode` models where they exist
- Python peripherals / hooks for missing but simple blocks
- watchpoints and symbol hooks for startup proof points

The goal is:

- prove the validation architecture
- not emulate the entire chip perfectly in phase 1

## Decision 9: Renode Assets Should Be Thin Overlays, Not A Second Device Database

`Renode` assets in `alloy` SHALL remain thin and runtime-focused.

They may contain:

- platform overlays
- monitor scripts
- UART hooks
- Python stubs for missing peripherals
- boot assertions

They SHALL NOT become a parallel hand-maintained copy of the full device database.

Practical rule:

- addresses, names, and startup artifacts should come from published descriptor sources whenever
  possible
- only emulation-specific glue should be handwritten in `alloy`

## Decision 10: Zero-Overhead Validation Is A Separate Gate

Host MMIO and `Renode` prove correctness of behavior.

They do not prove zero-overhead.

Zero-overhead SHALL remain a dedicated gate using:

- assembly comparison
- size checks
- compile-time structure checks

Host MMIO complements zero-overhead checks by validating side effects with the same descriptor path
used in production.

## Decision 11: Hardware Validation Uses Representative Boards, Not Exhaustive Coverage

`alloy` does not need every physical board to maintain confidence.

Hardware validation SHALL use representative boards by family/vendor.

Initial intent after the first emulation target:

- one representative Microchip board
- one representative STM32 G0 board
- one representative STM32 F4 board

The validation model scales by:

- broad compile coverage
- broad host MMIO coverage
- representative emulation
- representative hardware spot-checks

not by exhaustive physical ownership.

## Decision 12: STM32F4 Is The Second Renode Family After SAME70

After `same70`, the second Renode family SHALL be `stm32f4`.

Reason:

- the currently validated Renode distribution already ships a CPU-level `stm32f4` platform and a
  board-level `stm32f4_discovery` platform
- the same distribution ships a CPU-level `stm32g0` platform, but not an equivalent board-level
  starting point for the current Alloy G0 board target
- `stm32f4` therefore reduces handwritten overlay risk while still moving the validation ladder
  into the ST family

Initial target:

- `nucleo_f401re` with a thin overlay derived from Renode's existing STM32F4 assets

Practical consequence:

- `stm32g0` expands cross-vendor validation first in `host_mmio`
- `stm32f4` is the next emulation-family implementation target

## Decision 13: Initial Hardware Spot-Checks Are Board-Focused Runbooks

The first hardware layer SHALL be documented as explicit board runbooks instead of pretending to be
portable automation.

Initial matrix:

| Family | Board | Mandatory smoke | Extended smoke |
|---|---|---|---|
| Microchip SAME70 | `same70_xplained` | `blink`, `uart_logger` | `dma_probe` |
| STM32G0 | `nucleo_g071rb` | `blink`, `uart_logger` | — |
| STM32F4 | `nucleo_f401re` | `blink`, `uart_logger` | `dma_probe` |

Required observables:

- `blink`
  - visible LED activity proving boot to `main()` and board bring-up
- `uart_logger`
  - deterministic UART banner plus heartbeat at `115200 8N1`
- `dma_probe`
  - deterministic DMA binding output on boards that already expose typed DMA helpers

Practical consequence:

- hardware coverage stays small and intentional
- runbooks use real build targets already published by the repo
- flash/probe steps stay lab-owned until Alloy grows a deliberate hardware automation layer

## Decision 14: Representative Emulation Coverage Must Be Explicit And UART-Backed

`alloy` SHALL describe the current emulation-covered board set explicitly instead of implying that
all published MCUs boot in emulation today.

Current representative board set:

| Family | Board | Required emulation proof |
|---|---|---|
| Microchip SAME70 | `same70_xplained` | boot marker, boot banner, deterministic `USART0` byte count |
| STM32G0 | `nucleo_g071rb` | boot marker, boot banner, deterministic `USART2` byte count |
| STM32F4 | `nucleo_f401re` | boot marker, boot banner, deterministic `USART2` byte count |

Practical consequence:

- the runtime-validation ladder is representative, not exhaustive
- adding a new emulation-covered board requires documenting its proof obligations
- UART proof is part of the bring-up contract for the covered foundational boards

## Decision 15: Representative Validation Ladders Must Run In CI

The representative runtime-validation ladders SHALL be exercised in GitHub Actions CI.

Required CI shape:

- host CI runs the representative `host_mmio` ladder from a clean preset flow
- embedded CI builds descriptor-contract smoke targets for the current foundational boards
- emulation CI runs the representative `Renode` ladders for the current foundational boards

Practical consequence:

- runtime-validation is not "local-only"
- regressions in host bring-up paths and representative emulation paths fail CI directly
- the repo keeps a stable answer for what is currently validated automatically

## Proposed Test Responsibilities

| Validation layer | Owner repo | Artifact examples |
|---|---|---|
| Compile contract | `alloy` | compile test sources, CMake targets |
| ELF / startup inspection | `alloy` | symbol/section checks, vector assertions |
| Host MMIO | `alloy` | recording backend, trace assertions |
| Renode | `alloy` | `.repl`, `.resc`, hooks, Robot tests |
| Hardware spot-check | `alloy` | board smoke tests, flashing scripts |
| Descriptor publication quality | `alloy-devices` | generator reports, contract validation |

## Proposed Host MMIO Architecture

### Framework

```text
tests/host_mmio/framework/
  mmio_space.hpp
  mmio_trace.hpp
  register_expect.hpp
  descriptor_bindings.hpp
  boot_probe.hpp
```

### Test Pattern

1. select one board / device profile
2. run runtime bring-up entry point on host backend
3. collect trace of register operations
4. assert:
   - accessed registers
   - value transitions
   - ordering constraints
   - final state

### Assertion Style

Prefer assertions by descriptor identity when possible:

- `RegisterId`
- `FieldId`
- `ClockGateId`
- `ResetLineId`
- route / instance identifiers

Use raw addresses only when the descriptor contract does not yet expose enough structure.

## Proposed Renode Architecture

### Layout

```text
tests/emulation/renode/
  common/
    boot_assertions.robot
    hooks/
    include/
  same70/
    same70.repl
    same70_boot.resc
    same70_boot.robot
```

### Initial Boot Assertions

- vector table is loaded
- reset symbol is reached
- execution reaches `main` or boot banner
- no fault banner or lockup is observed
- expected UART line appears, if available

### Expected Same70 Supporting Pieces

- memory map and CPU definition
- debug UART path
- enough clock/reset/peripheral modeling for foundational boot
- Python stubs for unsupported but required peripherals

## Proposed Migration Of Existing Tests

| Current location | Target location |
|---|---|
| `tests/compile_tests/*` | `tests/compile/*` |
| startup / bring-up integration tests | `tests/host_mmio/scenarios/*` |
| `tools/assembly_verification/*` | `tests/perf/assembly/*` or a thin wrapper there |
| future Renode assets | `tests/emulation/renode/*` |

`tests/unit`, `tests/integration`, and `tests/regression` can coexist temporarily, but new
descriptor-driven bring-up tests should land in the target structure.

## Rollout Plan

### Phase 1: Validation skeleton

- create new validation capability in OpenSpec
- add test taxonomy and CMake labels
- document ownership and confidence ladder

### Phase 2: Host MMIO

- implement recording MMIO backend
- migrate foundational bring-up tests to host MMIO
- add trace assertions for clock/reset/GPIO/UART

### Phase 3: SAME70 Renode

- add `same70` `Renode` platform overlay and scripts
- prove boot to `main` or boot banner
- keep the overlay thin and descriptor-fed where possible

### Phase 4: Hardening

- wire CI profiles
- connect zero-overhead gates to the validation stack
- migrate more tests into the new layout

### Phase 5: Expansion

- add STM32 G0
- add STM32 F4
- add more representative hardware spot-checks

### Phase 6: CI Operationalization

- close the clean host preset flow
- run representative host MMIO ladders in CI
- run representative `Renode` ladders in CI
- keep the documented coverage matrix aligned with what CI actually exercises

## Risks

### Risk: Host MMIO gives false confidence

Mitigation:

- clearly separate what host MMIO can and cannot prove
- require ELF/startup inspection and `Renode` for startup sign-off

### Risk: Renode overlay duplicates device facts

Mitigation:

- keep overlay thin
- reuse published artifacts first
- postpone generator changes until the first target works

### Risk: Test reorganization churns too much at once

Mitigation:

- keep migration phased
- allow legacy layout to coexist temporarily
- move new descriptor-driven tests first

## Open Questions

- after the first two emulation targets, is a generated validation manifest justified or still
  premature?
