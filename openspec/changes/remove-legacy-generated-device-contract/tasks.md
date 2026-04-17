## 1. OpenSpec

- [ ] 1.1 Add `runtime-device-boundary` deltas requiring a runtime-only import boundary
- [ ] 1.2 Add `startup-runtime` deltas requiring startup consumption through the runtime contract
- [ ] 1.3 Add `migration-cleanup` deltas removing legacy generated header usage

## 2. Selected Device Import Layer

- [ ] 2.1 Update `cmake/templates/selected_config.hpp.in` to include runtime startup contract
- [ ] 2.2 Update `cmake/alloy_devices.cmake` to treat runtime startup as the selected descriptor
      boundary
- [ ] 2.3 Remove legacy generated startup descriptor include paths from the supported selected
      import layer

## 3. Startup Runtime Consumption

- [ ] 3.1 Update `src/device/startup.hpp` to alias the typed runtime startup contract
- [ ] 3.2 Keep startup implementation units linkable without relying on legacy startup descriptor
      headers
- [ ] 3.3 Update board/build integration if needed so generated startup sources still compile

## 4. Test And Tooling Cleanup

- [ ] 4.1 Update host MMIO selected-config overrides to include runtime startup contract
- [ ] 4.2 Update startup/runtime compile tests to stop including legacy generated startup headers
- [ ] 4.3 Remove any remaining direct dependency on legacy generated C++ headers from validation
      harnesses in `alloy`

## 5. Naming Cleanup

- [ ] 5.1 Rename selected import macros and internal helper naming from `runtime-lite` toward
      `runtime` where the rename is mechanical and safe
- [ ] 5.2 Update docs to describe `generated/runtime/**` as the default and only supported device
      contract

## 6. Validation

- [ ] 6.1 Validate the selected import layer against published runtime-only device trees
- [ ] 6.2 Validate with relevant startup/bring-up tests
- [ ] 6.3 Validate with `openspec validate remove-legacy-generated-device-contract --strict`
