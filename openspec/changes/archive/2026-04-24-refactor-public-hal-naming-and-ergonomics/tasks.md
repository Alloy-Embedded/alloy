## 1. OpenSpec Baseline

- [x] 1.1 Add `public-hal-api`, `migration-cleanup`, and `runtime-tooling` deltas

## 2. Ergonomic Public Alias Layer

- [x] 2.1 Add `alloy::dev` aliases for selected peripherals, pins, and expert signals
- [x] 2.2 Add public role aliases like `hal::tx`, `hal::rx`, `hal::scl`, `hal::sda`, `hal::sck`, `hal::miso`, and `hal::mosi`
- [x] 2.3 Add per-peripheral public route aliases such as `uart::route`, `spi::route`, and `i2c::route`
- [x] 2.4 Add GPIO public aliases so normal usage no longer needs to teach `device::pin<>`

## 3. Ambiguity And Expert Escape Hatches

- [x] 3.1 Implement compile-time resolution from public role aliases to canonical signal ids where the semantic traits are unambiguous
- [x] 3.2 Ensure ambiguous cases fail with actionable alternatives
- [x] 3.3 Preserve an explicit expert path that can still target canonical signal detail

## 4. Docs And Examples

- [x] 4.1 Update cookbook and migration docs to teach the ergonomic surface first
- [x] 4.2 Update raw HAL examples to use the public aliases where appropriate
- [x] 4.3 Keep board-oriented examples on the existing `board::*` path

## 5. Tooling And Diagnostics

- [x] 5.1 Update diagnostics to print ergonomic spellings first
- [x] 5.2 Preserve canonical detail as secondary information for expert debugging
- [x] 5.3 Ensure `alloyctl explain/diff` examples match the renamed public surface

## 6. Compatibility And Validation

- [x] 6.1 Keep canonical forms compiling during migration
- [x] 6.2 Add compile coverage for both ergonomic and canonical forms
- [x] 6.3 Re-run zero-overhead and public HAL validation where the route layer changes
- [x] 6.4 Validate the change with `openspec validate refactor-public-hal-naming-and-ergonomics --strict`
