## ADDED Requirements

### Requirement: Error Code Enumeration

The system SHALL provide an ErrorCode enumeration for all common error conditions.

#### Scenario: ErrorCode enum defined
- **WHEN** including `core/error.hpp`
- **THEN** `ErrorCode` enum class SHALL be available
- **AND** it SHALL include at least: Ok, InvalidParameter, Timeout, Busy, NotSupported, HardwareError

### Requirement: Result Type

The system SHALL provide a Result<T, ErrorCode> template for type-safe error handling without exceptions.

#### Scenario: Result success case
- **WHEN** creating a Result with `Result<int>::ok(42)`
- **THEN** `is_ok()` SHALL return true
- **AND** `value()` SHALL return 42
- **AND** `is_error()` SHALL return false

#### Scenario: Result error case
- **WHEN** creating a Result with `Result<int>::error(ErrorCode::Timeout)`
- **THEN** `is_error()` SHALL return true
- **AND** `error()` SHALL return ErrorCode::Timeout
- **AND** `is_ok()` SHALL return false

#### Scenario: Result move semantics
- **WHEN** moving a Result object
- **THEN** it SHALL support move construction and assignment
- **AND** no dynamic allocation SHALL occur

### Requirement: Type Safety

The Result type SHALL enforce compile-time type safety and prevent misuse.

#### Scenario: Cannot access value on error
- **WHEN** Result contains an error
- **THEN** accessing `value()` SHALL be undefined behavior (caller must check first)
- **AND** `is_ok()` MUST be checked before calling `value()`

#### Scenario: Cannot access error on success
- **WHEN** Result contains a success value
- **THEN** accessing `error()` SHALL be undefined behavior
- **AND** `is_error()` MUST be checked before calling `error()`
