## 1. Core Types

- [x] 1.1 Create `src/core/types.hpp` with basic type aliases
- [x] 1.2 Create `src/core/concepts.hpp` with utility concepts
- [x] 1.3 Add `ALLOY_CORE_TYPES_HPP` include guard

## 2. Error Handling

- [x] 2.1 Create `src/core/error.hpp` with ErrorCode enum
- [x] 2.2 Implement `Result<T, ErrorCode>` template class
- [x] 2.3 Add `is_ok()` and `is_error()` methods
- [x] 2.4 Add `value()` and `error()` accessors
- [x] 2.5 Implement static factory methods (`ok()`, `error()`)
- [x] 2.6 Add proper copy/move semantics

## 3. Testing

- [x] 3.1 Create unit tests for Result<T, ErrorCode>
- [x] 3.2 Test success case handling (18 tests total)
- [x] 3.3 Test error case handling
- [x] 3.4 Test copy and move operations
