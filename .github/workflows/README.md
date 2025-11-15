# Alloy Framework - CI/CD Workflows

This directory contains GitHub Actions workflows for continuous integration and deployment.

## Workflows Overview

### 1. Build and Test (`build.yml`)

**Trigger**: Push/PR to `main` or `develop`

**Jobs**:

#### Host Tests
- Runs on Ubuntu
- Executes all unit, integration, and regression tests
- Uses Catch2 test framework
- Test count: 100 tests (49 unit + 20 integration + 31 regression)

#### Embedded Builds
- Build matrix for all supported boards:
  - `nucleo_f401re` (STM32F4)
  - `nucleo_f722ze` (STM32F7)
  - `nucleo_g071rb` (STM32G0)
  - `nucleo_g0b1re` (STM32G0)
- Uses ARM GCC toolchain
- Builds blink example for each board
- Reports binary sizes
- Uploads artifacts (.elf, .bin, .hex)

#### Hardware Tests Build
- Builds hardware validation tests for each board
- Tests: `hw_gpio_led_test`, `hw_clock_validation`, `hw_board_validation`
- Uploads test binaries for manual hardware validation

#### Code Generation Validation
- Validates code generation workflow
- Runs dry-run of complete code generation
- Checks status of generated files

#### Build System Validation
- Runs CMake build system validation
- Checks for orphaned source files
- Validates platform isolation

**Artifacts**:
- Test results (host tests)
- Binary files for all boards
- Hardware test binaries

---

### 2. Code Quality (`code-quality.yml`)

**Trigger**: Push/PR to `main` or `develop`

**Jobs**:

#### Clang-Format Check
- Validates code formatting against `.clang-format`
- Checks all `.cpp` and `.hpp` files
- Fails if any file needs formatting

**Fix formatting locally**:
```bash
find src tests examples boards -name "*.cpp" -o -name "*.hpp" | \
  xargs clang-format -i
```

#### Clang-Tidy Static Analysis
- Runs modern C++ static analysis
- Checks: modernize, performance, readability, bugprone
- Analyzes core and HAL code (excluding generated files)

#### Cppcheck Static Analysis
- Runs cppcheck with all checks enabled
- Focuses on core and HAL directories
- Uploads analysis report

#### Include Guard Validation
- Verifies all headers have include guards or `#pragma once`
- Expected format: `ALLOY_<PATH>_<FILE>_HPP`

#### TODO/FIXME Report
- Generates report of all TODOs, FIXMEs, XXXs
- Uploads report as artifact

#### Documentation Coverage
- Checks required documentation files exist
- Estimates function documentation coverage
- Reports coverage percentage

**Critical Checks** (will fail workflow):
- Clang-format violations
- Cppcheck errors
- Missing include guards

---

## Using the Workflows

### Viewing Workflow Results

1. Go to **Actions** tab in GitHub repository
2. Select workflow run
3. View job results and logs

### Downloading Artifacts

**Binary artifacts**:
```
Actions → Workflow Run → Artifacts → binaries-<board>
```

**Test results**:
```
Actions → Workflow Run → Artifacts → test-results-host
```

**Reports**:
```
Actions → Workflow Run → Artifacts → cppcheck-report
Actions → Workflow Run → Artifacts → todo-report
```

### Manual Workflow Trigger

Both workflows support `workflow_dispatch` for manual triggering:

1. Go to **Actions** tab
2. Select workflow
3. Click **Run workflow**
4. Select branch
5. Click **Run workflow**

---

## Local Development

### Running Tests Locally

**Host tests**:
```bash
cmake -B build-host -DALLOY_PLATFORM=host -DALLOY_BUILD_TESTS=ON
cmake --build build-host
cd build-host && ctest --output-on-failure
```

**Embedded build**:
```bash
cmake -B build-nucleo \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DALLOY_BOARD=nucleo_f401re
cmake --build build-nucleo --target blink
```

### Code Formatting

**Check formatting**:
```bash
find src -name "*.cpp" -o -name "*.hpp" | \
  xargs clang-format --dry-run --Werror
```

**Fix formatting**:
```bash
find src -name "*.cpp" -o -name "*.hpp" | \
  xargs clang-format -i
```

### Static Analysis

**Clang-tidy**:
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/core/*.cpp -p build
```

**Cppcheck**:
```bash
cppcheck --enable=all --std=c++20 -I src src/core
```

### Build System Validation

```bash
cmake -B build -DALLOY_PLATFORM=host -DALLOY_BUILD_TESTS=ON
cmake --build build --target validate-build-system
```

---

## Workflow Configuration

### Build Matrix

To add a new board to the build matrix, edit `build.yml`:

```yaml
matrix:
  board:
    - nucleo_f401re
    - nucleo_f722ze
    - nucleo_g071rb
    - nucleo_g0b1re
    - your_new_board  # Add here
  include:
    - board: your_new_board
      platform: stm32xx
      mcu: STM32XXXXxx
```

### Code Quality Checks

To adjust clang-tidy checks, edit `code-quality.yml`:

```yaml
--checks='modernize-*,performance-*,readability-*,bugprone-*'
```

---

## Troubleshooting

### Build Failures

**ARM GCC not found**:
- Workflow installs `gcc-arm-none-eabi` automatically
- Locally: `sudo apt-get install gcc-arm-none-eabi`

**Test failures**:
- Check test logs in Actions → Job → Test step
- Run locally with `ctest --output-on-failure --verbose`

### Format Check Failures

**Files need formatting**:
```bash
# See which files need formatting
find src -name "*.cpp" -o -name "*.hpp" | \
  xargs clang-format --dry-run --Werror

# Fix all files
find src -name "*.cpp" -o -name "*.hpp" | \
  xargs clang-format -i
```

### Include Guard Failures

**Fix include guards**:
```cpp
// Option 1: Use #pragma once (recommended for modern compilers)
#pragma once

// Option 2: Use traditional include guard
#ifndef ALLOY_HAL_CORE_GPIO_HPP
#define ALLOY_HAL_CORE_GPIO_HPP
// ... code ...
#endif  // ALLOY_HAL_CORE_GPIO_HPP
```

---

## CI/CD Best Practices

### Before Pushing

1. **Run tests locally**:
   ```bash
   cmake -B build-host -DALLOY_BUILD_TESTS=ON
   cmake --build build-host && cd build-host && ctest
   ```

2. **Check formatting**:
   ```bash
   find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run
   ```

3. **Build all boards** (if possible):
   ```bash
   ./scripts/build_all_boards.sh
   ```

### Pull Request Guidelines

- **All tests must pass** before merging
- **No formatting violations**
- **No new cppcheck errors**
- Review artifact sizes (binary size regression check)

### Artifact Management

- Artifacts are kept for 90 days by default
- Download binaries for hardware validation
- Use test reports to track failures

---

## Workflow Status Badges

Add to README.md:

```markdown
![Build Status](https://github.com/your-org/corezero/actions/workflows/build.yml/badge.svg)
![Code Quality](https://github.com/your-org/corezero/actions/workflows/code-quality.yml/badge.svg)
```

---

## Future Enhancements

Planned workflow improvements:

- [ ] Release automation workflow
- [ ] Binary size regression tracking
- [ ] Performance benchmark CI
- [ ] Hardware-in-the-loop testing
- [ ] Code coverage reporting
- [ ] Automatic changelog generation
- [ ] Version tagging automation

---

## References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [ARM GCC Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain)
- [Clang-Format Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [Clang-Tidy Checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)

---

**Last Updated**: 2025-11-15
**Maintainer**: Alloy Framework Team
