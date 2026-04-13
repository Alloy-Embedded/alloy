## 1. OpenSpec

- [ ] 1.1 Add boundary deltas stating that hot-path runtime code targets runtime-lite only
- [ ] 1.2 Add public API deltas for zero-overhead descriptor consumption
- [ ] 1.3 Add cleanup deltas for removing reflection-table dependency from foundational paths

## 2. Device Import Layer

- [x] 2.1 Split `src/device` imports into runtime-lite and reflection views
- [x] 2.2 Stop exporting family-wide reflection tables as the default driver dependency

## 3. Runtime Backend

- [ ] 3.1 Replace lookup-style helpers in `runtime_backend` with thin execution helpers over
      generated compile-time refs
- [ ] 3.2 Remove normal-use dependence on `std::string_view` / table scanning in the hot path

## 4. Driver Migration

- [ ] 4.1 Rebuild GPIO on runtime-lite only
- [ ] 4.2 Rebuild UART on runtime-lite only
- [ ] 4.3 Rebuild SPI on runtime-lite only
- [ ] 4.4 Rebuild I2C on runtime-lite only

## 5. Boards And Examples

- [ ] 5.1 Ensure foundational board defaults consume runtime-lite only
- [ ] 5.2 Ensure foundational examples do not pull reflection tables into the normal path

## 6. Cleanup And Gates

- [x] 6.1 Remove or isolate reflection-only helpers from foundational hot-path code
- [x] 6.2 Add compile coverage proving foundational paths build against runtime-lite only
- [x] 6.3 Validate with `openspec validate consume-runtime-lite-device-contract --strict`
