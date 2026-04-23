## 1. OpenSpec Baseline

- [x] 1.1 Add the `runtime-tooling` capability
- [x] 1.2 Add build-and-selection and migration-cleanup deltas

## 2. Stable Entry Points

- [x] 2.1 Define supported board-oriented configure/build flows
- [x] 2.2 Define supported flash/debug flows where the repo can guarantee them
- [x] 2.3 Align presets, targets, and docs around those flows
- [x] 2.4 Add and document a first-class downstream CMake consumption path, such as `find_package(Alloy)`, if the repo claims package-style integration

## 3. User-Facing Diagnostics

- [x] 3.1 Surface connector/resource conflict diagnostics through user-facing messages
- [x] 3.2 Surface clock/profile selection failures through user-facing messages
- [x] 3.3 Add tests for representative diagnostic output
- [x] 3.4 Add an explain path that can show why a connector/clock/resource fact exists when the generated contract publishes enough provenance
- [x] 3.5 Add a diff path that can summarize meaningful target-to-target capability or routing differences for migration work

## 4. Examples And Cookbook

- [x] 4.1 Ensure each primary public HAL class has at least one canonical example
- [x] 4.2 Add short cookbook docs for common tasks
- [x] 4.3 Keep examples aligned with the official runtime path only
- [x] 4.4 Make the top-level quickstart honest and fast enough to reach `blink` or `hello world` on a foundational board without repo archeology

## 5. Support Matrix And Board Pages

- [x] 5.1 Publish a board support matrix
- [x] 5.2 Publish a peripheral support matrix
- [x] 5.3 Publish short board pages or equivalent board-oriented docs

## 6. Migration Guides

- [x] 6.1 Add migration guidance from STM32Cube HAL/LL concepts
- [x] 6.2 Add migration guidance from `modm`
- [x] 6.3 Add migration guidance from register-level libraries such as `libopencm3`

## 7. Validation

- [x] 7.1 Verify documented commands and presets on the foundational set
- [x] 7.2 Keep docs and support matrices honest with automated checks where possible
- [x] 7.3 Validate diagnostics and downstream package entry points if claimed
- [x] 7.4 Validate the change with `openspec validate build-runtime-tooling-and-user-experience --strict`
