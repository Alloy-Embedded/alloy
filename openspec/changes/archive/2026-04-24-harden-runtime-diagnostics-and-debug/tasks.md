## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-tooling and runtime-release-discipline deltas

## 2. Diagnostics

- [x] 2.1 Enrich connector diagnostics with better alternatives
- [x] 2.2 Enrich clock/profile diagnostics with board-oriented language
- [x] 2.3 Add representative tests for failure output

## 3. Explain And Diff

- [x] 3.1 Expand `alloyctl explain` with richer board/peripheral detail
- [x] 3.2 Expand `alloyctl diff` with migration-focused capability differences
- [x] 3.3 Ensure documented examples match the supported data source

## 4. Recovery And Debug

- [x] 4.1 Add supported recovery flows to `alloyctl` where possible
- [x] 4.2 Document board-aware recovery/debug flows
- [x] 4.3 Exercise those flows in runtime tooling checks where feasible

## 5. Validation

- [x] 5.1 Re-run `scripts/check_runtime_tooling.py`
- [x] 5.2 Validate the change with `openspec validate harden-runtime-diagnostics-and-debug --strict`
