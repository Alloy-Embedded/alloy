## 1. Core Types

- [ ] 1.1 Create `src/core/types.hpp` with basic type aliases
- [ ] 1.2 Create `src/core/concepts.hpp` with utility concepts
- [ ] 1.3 Add `ALLOY_CORE_TYPES_HPP` include guard

## 2. Error Handling

- [ ] 2.1 Create `src/core/error.hpp` with ErrorCode enum
- [ ] 2.2 Implement `Result<T, ErrorCode>` template class
- [ ] 2.3 Add `is_ok()` and `is_error()` methods
- [ ] 2.4 Add `value()` and `error()` accessors
- [ ] 2.5 Implement static factory methods (`ok()`, `error()`)
- [ ] 2.6 Add proper copy/move semantics

## 3. Testing

- [ ] 3.1 Create unit tests for Result<T, ErrorCode>
- [ ] 3.2 Test success case handling
- [ ] 3.3 Test error case handling
- [ ] 3.4 Test copy and move operations
