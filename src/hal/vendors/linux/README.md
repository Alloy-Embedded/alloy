# Linux/Host Platform

This is a **mock/simulation platform** for unit testing HAL code on the development machine without requiring embedded hardware.

## Purpose

- **Unit Testing**: Test HAL APIs without hardware
- **CI/CD**: Automated testing in continuous integration
- **Development**: Rapid iteration without flashing firmware

## Implementation Status

Currently MINIMAL - only exists to satisfy build system requirements for test builds.

For actual Linux peripheral access (serial ports, GPIO via sysfs, etc.), see future platform implementations.

## Test Build

```bash
cmake -B build-tests -DALLOY_BOARD=host -DALLOY_BUILD_TESTS=ON
cmake --build build-tests
ctest --test-dir build-tests
```
