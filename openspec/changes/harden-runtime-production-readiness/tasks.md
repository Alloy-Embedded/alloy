## 1. OpenSpec Baseline

- [ ] 1.1 Add the `runtime-release-discipline` capability
- [ ] 1.2 Add runtime-device-boundary and zero-overhead deltas

## 2. Support Tiers

- [ ] 2.1 Define support tiers for boards
- [ ] 2.2 Define support tiers for peripheral classes
- [ ] 2.3 Publish the support status in active docs

## 3. Runtime/Devices Compatibility

- [ ] 3.1 Define how an `alloy` release declares compatible `alloy-devices` versions or contract ranges
- [ ] 3.2 Add checks that fail when runtime and published contract expectations drift

## 4. Release Gates

- [ ] 4.1 Define the minimum foundational validation gate for release
- [ ] 4.2 Define required example/build coverage for release
- [ ] 4.3 Define zero-overhead and size-regression gates for release claims

## 5. Migration Discipline

- [ ] 5.1 Add a breaking-change checklist for public HAL and build model changes
- [ ] 5.2 Require migration notes and updated examples for breaking changes

## 6. Docs And Automation

- [ ] 6.1 Add a release checklist document
- [ ] 6.2 Add support matrix maintenance rules
- [ ] 6.3 Add CI automation for the declared release gates where feasible

## 7. Validation

- [ ] 7.1 Verify the support matrix and compatibility declarations are testable
- [ ] 7.2 Validate the change with `openspec validate harden-runtime-production-readiness --strict`
