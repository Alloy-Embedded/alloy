# CoreZero Testing Guide

Comprehensive testing framework for CoreZero ESP-IDF integration.

## Table of Contents

- [Overview](#overview)
- [Test Types](#test-types)
- [Running Tests](#running-tests)
- [Unit Tests](#unit-tests)
- [Build Validation](#build-validation)
- [Writing Tests](#writing-tests)

## Overview

CoreZero uses a multi-layered testing approach:

1. **Unit Tests**: Test core functionality (Result<T>, Error handling)
2. **Build Validation**: Verify all examples compile successfully
3. **Integration Tests**: Validate complete system behavior (manual for now)

## Test Types

### Unit Tests

Tests for core framework components:
- `Result<T, E>` type and monadic operations
- Error code mappings
- Type traits and concepts
- Utility functions

**Location**: `tests/unit/`

### Build Validation

Automated compilation tests for all ESP32 examples:
- WiFi examples (Station, AP, Scanner)
- BLE examples (Scanner)
- HTTP Server example
- MQTT Client example
- Bare-metal examples (blink)

**Script**: `scripts/testing/validate_esp32_builds.sh`

### Integration Tests

Manual hardware tests (automated tests planned for future):
- WiFi connectivity
- BLE device discovery
- HTTP server requests
- MQTT pub/sub

**Location**: `tests/integration/` (planned)

## Running Tests

### Quick Start

Run all tests:
```bash
./scripts/testing/run_all_tests.sh
```

### Unit Tests Only

```bash
./scripts/testing/run_unit_tests.sh
```

Output:
```
==========================================
  CoreZero Unit Tests
==========================================

Configuring tests...
Building tests...
Build successful

Running tests...

Running test: result_ok_construction... PASS
Running test: result_error_construction... PASS
Running test: result_void_ok... PASS
...
==========================================
  Test Summary
==========================================
Total:  20
Passed: 20
Failed: 0
==========================================

All tests passed!
```

### Build Validation Only

```bash
# Requires ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

./scripts/testing/validate_esp32_builds.sh
```

Test specific example:
```bash
./scripts/testing/validate_esp32_builds.sh esp32_wifi_station
```

Output:
```
==========================================
  ESP32 Build Validation
==========================================

ESP-IDF Path: /home/user/esp/esp-idf
ESP-IDF Version: ESP-IDF v5.1

Testing all ESP32 examples...

[TEST] Building blink_esp32...
[PASS] blink_esp32 (142 KB)
[TEST] Building esp32_wifi_station...
[PASS] esp32_wifi_station (831 KB)
[TEST] Building esp32_mqtt_iot...
[PASS] esp32_mqtt_iot (750 KB)
...

==========================================
  Test Summary
==========================================
Total:   8
Passed:  8
Failed:  0
Skipped: 0
==========================================
```

## Unit Tests

### Structure

Unit tests use a simple assertion framework:

```cpp
TEST(test_name) {
    // Test code
    ASSERT(condition);
}
```

### Running Specific Tests

```bash
cd tests/unit
mkdir build && cd build
cmake ..
cmake --build .
./test_result  # Run Result<T> tests
```

### Adding New Tests

1. Create test file in `tests/unit/`
2. Add to `CMakeLists.txt`:
   ```cmake
   add_executable(test_my_feature test_my_feature.cpp)
   add_test(NAME MyFeatureTest COMMAND test_my_feature)
   ```
3. Run tests

Example test structure:

```cpp
#include "../../src/my_feature.hpp"
#include <cassert>

TEST(my_feature_works) {
    auto result = my_function();
    ASSERT(result.is_ok());
    ASSERT(result.value() == expected);
}

int main() {
    run_test_my_feature_works();
    // Print summary
    return (tests_passed == tests_run) ? 0 : 1;
}
```

## Build Validation

### What It Tests

For each ESP32 example:
1. ✅ CMakeLists.txt is valid
2. ✅ All includes are resolved
3. ✅ Code compiles without errors
4. ✅ Binary is generated
5. ✅ Binary size is reported

### Interpreting Results

**PASS**: Example built successfully
```
[PASS] esp32_wifi_station (831 KB)
```

**FAIL**: Compilation errors
```
[FAIL] esp32_mqtt_iot
  Last 20 lines of build output:
    error: 'mqtt_client' was not declared in this scope
    ...
```

**SKIP**: Not an ESP-IDF project
```
[SKIP] some_example (not an ESP-IDF project)
```

### Build Size Reference

Typical binary sizes:
- **Bare-metal blink**: ~140 KB
- **WiFi Station**: ~830 KB
- **BLE Scanner**: ~630 KB
- **HTTP Server**: ~730 KB
- **MQTT Client**: ~750 KB

Large increases may indicate:
- Unused components being linked
- Debug symbols enabled
- Missing optimization flags

## Writing Tests

### Best Practices

1. **Test One Thing**: Each test should verify a single behavior
2. **Clear Names**: Use descriptive test names (e.g., `test_result_map_chains_correctly`)
3. **Arrange-Act-Assert**: Structure tests clearly
4. **No Side Effects**: Tests should not depend on each other
5. **Fast**: Unit tests should run quickly

### Example Test Pattern

```cpp
TEST(feature_behavior_expected_outcome) {
    // Arrange
    auto input = setup_test_data();

    // Act
    auto result = function_under_test(input);

    // Assert
    ASSERT(result.is_ok());
    ASSERT(result.value() == expected_value);
}
```

### Error Cases

Always test error paths:

```cpp
TEST(function_returns_error_on_invalid_input) {
    auto result = function_under_test(invalid_input);
    ASSERT(result.is_error());
    ASSERT(result.error() == ErrorCode::InvalidParameter);
}
```

### Edge Cases

Test boundary conditions:

```cpp
TEST(function_handles_empty_input) {
    auto result = function_under_test("");
    ASSERT(result.is_ok());
}

TEST(function_handles_max_size_input) {
    std::string large_input(MAX_SIZE, 'x');
    auto result = function_under_test(large_input);
    ASSERT(result.is_ok());
}
```

## Manual Testing Checklist

For hardware validation (until automated tests are implemented):

### WiFi Station
- [ ] Connects to 2.4GHz network
- [ ] Obtains IP address via DHCP
- [ ] HTTP GET request succeeds
- [ ] Reconnects after disconnection

### WiFi AP
- [ ] Creates access point
- [ ] Client can connect
- [ ] DHCP assigns IPs to clients
- [ ] Station list updates correctly

### BLE Scanner
- [ ] Discovers nearby devices
- [ ] Reports RSSI correctly
- [ ] Handles device names properly
- [ ] Async scanning works

### HTTP Server
- [ ] Server starts on port 80
- [ ] GET requests return responses
- [ ] POST with body works
- [ ] JSON responses formatted correctly

### MQTT Client
- [ ] Connects to broker
- [ ] Publishes messages successfully
- [ ] Receives subscribed messages
- [ ] QoS levels work correctly
- [ ] TLS connection succeeds (if configured)

## Troubleshooting

### Unit Tests Fail to Build

**Problem**: Compiler errors
```
Solution: Ensure C++20 compiler is available
- GCC 10+ or Clang 12+
- Check CMake version (3.16+)
```

### Build Validation Fails

**Problem**: ESP-IDF not found
```
Solution: Source ESP-IDF environment
. $HOME/esp/esp-idf/export.sh
```

**Problem**: Example fails to build
```
Solution: Check error output in test log
- Verify all components are available
- Check for typos in CMakeLists.txt
- Ensure ESP-IDF version is 5.0+
```

### Tests Pass but Hardware Doesn't Work

This indicates integration issues:
1. Check WiFi credentials
2. Verify hardware connections
3. Monitor serial output for errors
4. Test with simpler examples first

## Future Improvements

Planned enhancements:
- [ ] Hardware-in-the-loop (HIL) automated testing
- [ ] Mock ESP-IDF for faster unit tests
- [ ] Integration test framework
- [ ] Performance benchmarks
- [ ] Coverage reports
- [ ] Continuous fuzzing

## Contributing

When adding features:
1. Write unit tests first (TDD)
2. Ensure all tests pass
3. Add integration test if applicable
4. Update this documentation

## See Also

- [ESP32 Integration Guide](ESP32_INTEGRATION_GUIDE.md)
- [Examples Documentation](../examples/)
- [OpenSpec Tasks](../openspec/changes/integrate-esp-idf-framework/tasks.md)
