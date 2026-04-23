## 1. OpenSpec Baseline

- [ ] 1.1 Add runtime-tooling and runtime-release-discipline deltas

## 2. Diagnostics

- [ ] 2.1 Enrich connector diagnostics with better alternatives
- [ ] 2.2 Enrich clock/profile diagnostics with board-oriented language
- [ ] 2.3 Add representative tests for failure output

## 3. Explain And Diff

- [ ] 3.1 Expand `alloyctl explain` with richer board/peripheral detail
- [ ] 3.2 Expand `alloyctl diff` with migration-focused capability differences
- [ ] 3.3 Ensure documented examples match the supported data source

## 4. Recovery And Debug

- [ ] 4.1 Add supported recovery flows to `alloyctl` where possible
- [ ] 4.2 Document board-aware recovery/debug flows
- [ ] 4.3 Exercise those flows in runtime tooling checks where feasible

## 5. Validation

- [ ] 5.1 Re-run `scripts/check_runtime_tooling.py`
- [ ] 5.2 Validate the change with `openspec validate harden-runtime-diagnostics-and-debug --strict`
