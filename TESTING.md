# 🧪 Alloy Framework - Testing Guide

Modern C++20 testing framework using Catch2 v3 with comprehensive build automation.

## 🚀 Quick Start

```bash
# Show all available commands
make help

# Build and run all tests
make all

# Run specific test
make test-task
make test-mutex
```

## 📋 Available Make Targets

### Build & Test

| Command | Description |
|---------|-------------|
| `make build` | Build all tests with Catch2 |
| `make test` | Run all tests |
| `make test-task` | Run only task tests |
| `make test-mutex` | Run only mutex tests |
| `make test-semaphore` | Run only semaphore tests |
| `make test-event` | Run only event tests |
| `make test-verbose` | Run tests with verbose output |
| `make list-tests` | List all available tests |

### Code Quality

| Command | Description |
|---------|-------------|
| `make lint` | Run clang-tidy static analysis |
| `make format` | Format code with clang-format |
| `make format-check` | Check if code is formatted |
| `make check` | Run all checks (format, lint, test) |

### Development

| Command | Description |
|---------|-------------|
| `make clean` | Remove build directory |
| `make rebuild` | Clean and rebuild |
| `make quick` | Quick build (no clean) |
| `make info` | Show build configuration |
| `make watch-test` | Auto-run tests on file changes* |

### CI/CD

| Command | Description |
|---------|-------------|
| `make ci` | Run full CI pipeline |
| `make all` | Clean, build and test everything |

\* Requires `fswatch`: `brew install fswatch`

## 🎯 Test Suite Overview

### Current Status

- ✅ **test_task** - 29 assertions, 10 test cases (PASSING)
- ⚠️ **test_mutex** - Segfault (uses RTOS scheduler)
- ⚠️ **test_semaphore** - Timeout (blocking operations)
- ⚠️ **test_event** - Timeout (blocking operations)

### Test Framework: Catch2 v3

We use [Catch2](https://github.com/catchorg/Catch2) for its modern C++ syntax and excellent output:

```cpp
TEST_CASE("Task creation and properties", "[task][creation]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Task is created with correct name") {
        INFO("Creating task may start scheduler and cause timeout");
        // Test implementation
    }
}
```

### Test Tags

Tests are organized with tags for easy filtering:

- `[task]` - Task management tests
- `[mutex]` - Mutex synchronization tests
- `[semaphore]` - Semaphore tests
- `[event]` - Event flag tests
- `[basic]` - Basic functionality
- `[threading]` - Multi-threaded tests
- `[timeout]` - Timeout behavior
- `[edge-cases]` - Edge case scenarios

### Running Tests by Tag

```bash
# Build and run tests manually
cd build_tests

# Run all mutex tests
./tests/test_mutex

# Run only basic mutex tests
./tests/test_mutex [basic]

# Run with compact output
./tests/test_mutex -r compact

# List all tags
./tests/test_mutex --list-tags

# List all test cases
./tests/test_mutex --list-tests
```

## 🔍 Code Quality Tools

### clang-tidy (Static Analysis)

Configuration: `.clang-tidy`

Enabled checks:
- `bugprone-*` - Bug-prone code patterns
- `cert-*` - CERT secure coding
- `concurrency-*` - Concurrency issues
- `cppcoreguidelines-*` - C++ Core Guidelines
- `modernize-*` - Modern C++ features
- `performance-*` - Performance issues
- `readability-*` - Code readability

```bash
# Run static analysis
make lint

# Install clang-tidy
brew install llvm
```

### clang-format (Code Formatting)

Configuration: `.clang-format`

Style guide:
- **Base**: Google Style
- **Standard**: C++20
- **Indent**: 4 spaces
- **Column limit**: 100 characters
- **Pointer alignment**: Left (`int* ptr`)

```bash
# Format all code
make format

# Check formatting
make format-check

# Install clang-format
brew install clang-format
```

## 📁 Test Structure

```
tests/
├── CMakeLists.txt           # Test build configuration
├── unit/
│   └── rtos/
│       ├── test_task.cpp        # Task management (254 lines)
│       ├── test_mutex.cpp       # Mutex synchronization (390 lines)
│       ├── test_semaphore.cpp   # Semaphores (245 lines)
│       └── test_event.cpp       # Event flags (292 lines)
└── integration/             # (Future: integration tests)
```

## 🐛 Known Issues

### Issue 1: RTOS Scheduler Activation

**Problem**: Creating `Mutex`, `Semaphore`, or `EventFlags` objects activates the RTOS scheduler, causing tests to hang or segfault.

**Workaround**:
- Use `std::thread` for multi-threaded tests
- Test only non-blocking operations
- Avoid calling `wait()`, `take()`, or `lock()` with timeout

**Example (works)**:
```cpp
Mutex mutex;
REQUIRE(mutex.try_lock());  // Non-blocking, works
mutex.unlock();
```

**Example (hangs)**:
```cpp
Semaphore sem(0);
sem.take();  // Blocks forever, test hangs
```

### Issue 2: Multi-threaded Tests with Scheduler

**Status**: Semaphore and Event tests use `std::thread` but still trigger scheduler warnings.

**Solution (future)**: Add test mode to RTOS primitives to disable scheduler.

## 🎨 Output Examples

### Successful Test

```
==========================================
🧪 Alloy Framework - Build System
==========================================

🔨 Compilando testes...

  test_task... ✓
  test_mutex... ✓
  test_semaphore... ✓
  test_event... ✓

✨ Build concluído!

▶ Running test_task...
RNG seed: 2841807983
All tests passed (29 assertions in 10 test cases)
```

### CI Pipeline

```bash
make ci
```

Runs:
1. Clean build
2. Format check
3. Static analysis (clang-tidy)
4. Build all tests
5. Run all tests

## 📊 Test Metrics

| Test | Lines | Test Cases | Sections | Status |
|------|-------|------------|----------|--------|
| test_task | 254 | 10 | 30+ | ✅ PASS |
| test_mutex | 390 | 9 | 25+ | ⚠️ Segfault |
| test_semaphore | 245 | 6 | 15+ | ⚠️ Timeout |
| test_event | 292 | 8 | 20+ | ⚠️ Timeout |
| **Total** | **1,181** | **33** | **90+** | **1/4 PASS** |

## 🔧 Development Workflow

### Daily Development

```bash
# 1. Make changes to code
vim src/rtos/mutex.hpp

# 2. Format code
make format

# 3. Build and test
make test-mutex

# 4. Run full checks before commit
make check
```

### Before Commit

```bash
# Run full CI pipeline
make ci

# If all passes, commit
git add .
git commit -m "feat: improve mutex performance"
```

### Continuous Testing

```bash
# Watch for changes and auto-run tests
make watch-test
```

## 🚀 CI/CD Integration

### GitHub Actions Example

```yaml
name: CI
on: [push, pull_request]

jobs:
  test:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install tools
        run: brew install llvm clang-format
      - name: Run CI
        run: make ci
```

### Pre-commit Hook

```bash
# .git/hooks/pre-commit
#!/bin/bash
make format-check && make lint && make test
```

## 📚 Further Reading

- [Catch2 Documentation](https://github.com/catchorg/Catch2/tree/devel/docs)
- [clang-tidy Checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

## 🆘 Troubleshooting

### "clang-tidy not found"

```bash
brew install llvm
```

### "clang-format not found"

```bash
brew install clang-format
```

### "fswatch not found" (for watch-test)

```bash
brew install fswatch
```

### Tests hang or timeout

This is expected for tests that use RTOS blocking operations. Use:
```bash
# Kill hanging test
Ctrl+C

# Run only non-blocking tests
make test-task
```

## 💡 Tips

1. **Use tags**: Run specific test categories with `./tests/test_mutex [basic]`
2. **Compact output**: Use `-r compact` for CI-friendly output
3. **Quick iteration**: Use `make quick` to skip clean step
4. **Watch mode**: Use `make watch-test` during development
5. **Format on save**: Configure your editor to run clang-format on save
