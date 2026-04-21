## 1. OpenSpec Baseline

- [ ] 1.1 Add the `runtime-tooling` capability
- [ ] 1.2 Add build-and-selection and migration-cleanup deltas

## 2. Stable Entry Points

- [ ] 2.1 Define supported board-oriented configure/build flows
- [ ] 2.2 Define supported flash/debug flows where the repo can guarantee them
- [ ] 2.3 Align presets, targets, and docs around those flows
- [ ] 2.4 Add and document a first-class downstream CMake consumption path, such as `find_package(Alloy)`, if the repo claims package-style integration

## 3. User-Facing Diagnostics

- [ ] 3.1 Surface connector/resource conflict diagnostics through user-facing messages
- [ ] 3.2 Surface clock/profile selection failures through user-facing messages
- [ ] 3.3 Add tests for representative diagnostic output
- [ ] 3.4 Add an explain path that can show why a connector/clock/resource fact exists when the generated contract publishes enough provenance
- [ ] 3.5 Add a diff path that can summarize meaningful target-to-target capability or routing differences for migration work

## 4. Examples And Cookbook

- [ ] 4.1 Ensure each primary public HAL class has at least one canonical example
- [ ] 4.2 Add short cookbook docs for common tasks
- [ ] 4.3 Keep examples aligned with the official runtime path only
- [ ] 4.4 Make the top-level quickstart honest and fast enough to reach `blink` or `hello world` on a foundational board without repo archeology

## 5. Support Matrix And Board Pages

- [ ] 5.1 Publish a board support matrix
- [ ] 5.2 Publish a peripheral support matrix
- [ ] 5.3 Publish short board pages or equivalent board-oriented docs

## 6. Migration Guides

- [ ] 6.1 Add migration guidance from STM32Cube HAL/LL concepts
- [ ] 6.2 Add migration guidance from `modm`
- [ ] 6.3 Add migration guidance from register-level libraries such as `libopencm3`

## 7. Validation

- [ ] 7.1 Verify documented commands and presets on the foundational set
- [ ] 7.2 Keep docs and support matrices honest with automated checks where possible
- [ ] 7.3 Validate diagnostics and downstream package entry points if claimed
- [ ] 7.4 Validate the change with `openspec validate build-runtime-tooling-and-user-experience --strict`
