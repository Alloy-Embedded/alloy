## 1. Google Test Integration

- [ ] 1.1 Add FetchContent for Google Test in root CMakeLists.txt
- [ ] 1.2 Create `tests/unit/CMakeLists.txt`
- [ ] 1.3 Enable CTest with `enable_testing()`
- [ ] 1.4 Link test executable with gtest_main

## 2. Example Tests

- [ ] 2.1 Create `tests/unit/test_error_handling.cpp`
- [ ] 2.2 Write tests for Result<T, ErrorCode> success case
- [ ] 2.3 Write tests for Result<T, ErrorCode> error case
- [ ] 2.4 Create `tests/unit/test_gpio_host.cpp`
- [ ] 2.5 Write tests for host GPIO state management

## 3. CI Configuration

- [ ] 3.1 Create `.github/workflows/ci.yml` (basic)
- [ ] 3.2 Configure build and test steps
- [ ] 3.3 Document how to run tests locally

## 4. Documentation

- [ ] 4.1 Create `tests/README.md` with testing guide
- [ ] 4.2 Document test naming conventions
- [ ] 4.3 Add testing section to CONTRIBUTING.md
