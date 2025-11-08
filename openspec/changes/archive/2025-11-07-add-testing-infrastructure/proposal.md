## Why

Testability is a core requirement. Need Google Test framework configured for unit testing HAL implementations and application logic.

## What Changes

- Configure Google Test via CMake FetchContent
- Create test structure in tests/unit/
- Add example unit tests for core error handling
- Set up CTest integration
- Document testing practices

## Impact

- Affected specs: testing-infrastructure (new capability)
- Affected code: tests/unit/, CMakeLists.txt
- Enables TDD and validation of all implementations
