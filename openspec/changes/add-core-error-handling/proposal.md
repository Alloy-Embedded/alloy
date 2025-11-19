## Why

Embedded systems need deterministic error handling without exceptions. We need a Result<T, Error> type similar to Rust for type-safe error handling with zero overhead.

## What Changes

- Create core types in `src/core/` (types.hpp, concepts.hpp, error.hpp)
- Implement `Result<T, ErrorCode>` template class
- Define `ErrorCode` enum with common error cases
- Add compile-time utilities using constexpr/consteval
- Document error handling patterns

## Impact

- Affected specs: core-error-handling (new capability)
- Affected code: src/core/error.hpp, src/core/types.hpp
- Foundation for all HAL APIs that can fail
