# Code Generator Test Suite

Comprehensive test suite for the Alloy HAL code generators.

## Quick Start

```bash
# Run all unit tests (49 tests in 0.03s)
pytest tests/test_*.py -v

# Run specific test file
pytest tests/test_register_generation.py -v  # 13 tests
pytest tests/test_enum_generation.py -v      # 21 tests
pytest tests/test_pin_generation.py -v       # 15 tests

# Run with coverage
pytest tests/test_*.py --cov=cli/generators --cov-report=html
open htmlcov/index.html
```

## Test Files

### `test_register_generation.py`
Unit tests for register structure generation.

**Test Classes**:
- `TestRegisterStructGeneration` - Basic functionality
- `TestPIOARegression` - PIOA RESERVED field bug regression tests
- `TestRegisterArrays` - Register array handling
- `TestNamespaceGeneration` - Namespace creation
- `TestStaticAssertions` - Static assertion generation
- `TestAccessorFunction` - Peripheral accessor functions

**Coverage**: 13 tests, 30% generator coverage, 99% test code coverage

### `test_enum_generation.py`
Unit tests for enumeration generation.

**Test Classes**:
- `TestEnumHelperFunctions` - Helper function tests
- `TestEnumGeneration` - Enum code generation
- `TestEnumEdgeCases` - Edge cases and special scenarios

**Coverage**: 21 tests, 58% generator coverage, 99% test code coverage

### `test_pin_generation.py`
Unit tests for pin function generation.

**Test Classes**:
- `TestPinFunctionDataStructures` - Data structure tests
- `TestPinFunctionGeneration` - Pin function header generation
- `TestPinGlobalNumbering` - Global pin numbering (PC8 bug regression)
- `TestPinEdgeCases` - Edge cases and special scenarios

**Coverage**: 15 tests, 42% generator coverage, 99% test code coverage

### `test_helpers.py`
Reusable test fixtures and helper functions.

**Key Functions**:
- `create_test_register()` - Create Register objects for testing
- `create_test_peripheral()` - Create Peripheral objects
- `create_test_device()` - Create SVDDevice objects
- `create_pioa_test_peripheral()` - PIOA with ABCDSR[2] bug scenario
- `create_same70_test_device()` - SAME70 test device

**Assertion Helpers**:
- `AssertHelpers.assert_compiles()` - Compile C++ code
- `AssertHelpers.assert_contains_all()` - Check multiple patterns
- `AssertHelpers.assert_not_contains_any()` - Check patterns absent

## Writing Tests

### Example: Simple Register Test
```python
from tests.test_helpers import create_test_register, create_test_peripheral, create_test_device
from cli.generators.generate_registers import generate_register_struct

def test_simple_register():
    """Test basic register generation"""
    peripheral = create_test_peripheral(
        name="TEST",
        base_address=0x40000000,
        registers=[
            create_test_register("REG1", 0x0000, description="Test Register"),
        ]
    )
    device = create_test_device()

    output = generate_register_struct(peripheral, device)

    assert "struct TEST_Registers" in output
    assert "volatile uint32_t REG1" in output
    assert "Test Register" in output
```

### Example: Array Register Test
```python
def test_register_array():
    """Test register array generation"""
    peripheral = create_test_peripheral(
        name="TEST",
        base_address=0x40000000,
        registers=[
            create_test_register("ARRAY", 0x0000, size=32, dim=4),
        ]
    )
    device = create_test_device()

    output = generate_register_struct(peripheral, device)

    assert "volatile uint32_t ARRAY[4]" in output
```

### Example: Compilation Test
```python
def test_code_compiles():
    """Test that generated code actually compiles"""
    peripheral = create_test_peripheral(...)
    device = create_test_device()

    output = generate_register_struct(peripheral, device)

    # This will compile the code and raise AssertionError if it fails
    AssertHelpers.assert_compiles(output)
```

## Regression Tests

### PIOA RESERVED Field Bug
**Problem**: RESERVED_0074[12] should be RESERVED_0078[8]
**Root Cause**: ABCDSR[2] array dimension not accounted for
**Test**: `test_pioa_reserved_field_size()`

### Pin Numbering Bug
**Problem**: PC8 = 8 should be PC8 = 72
**Root Cause**: Port-relative instead of global numbering
**Formula**: GlobalPin = (Port * 32) + Pin
**Test**: `test_pin_numbering_global()`

## Test Coverage

Current coverage for `cli/generators/generate_registers.py`:
- **30% line coverage** (54/180 lines)
- **100% of tests passing**

### Covered:
- ✅ Basic register generation
- ✅ Register arrays
- ✅ RESERVED field calculation
- ✅ Different register sizes
- ✅ Namespace generation
- ✅ Static assertions
- ✅ Accessor functions

### Not Covered Yet:
- ⏸️ Bitfield generation
- ⏸️ Enum generation
- ⏸️ Error handling
- ⏸️ Edge cases (empty peripherals, invalid offsets, etc.)

## Best Practices

### 1. Use Test Helpers
Don't create objects manually - use the helper functions:
```python
# Good ✅
peripheral = create_test_peripheral("PIOA", 0x400E0E00)

# Bad ❌
peripheral = Peripheral(name="PIOA", base_address=0x400E0E00, ...)
```

### 2. Test One Thing
Each test should verify one specific behavior:
```python
# Good ✅
def test_array_syntax():
    """Test that arrays use bracket syntax"""
    output = generate_register_struct(...)
    assert "ARRAY[4]" in output

# Bad ❌
def test_everything():
    """Test arrays, RESERVED, namespaces, etc."""
    # Too much in one test!
```

### 3. Name Tests Descriptively
```python
# Good ✅
def test_pioa_reserved_field_size():
    """Test that PIOA RESERVED field has correct size"""

# Bad ❌
def test_pioa():
    """Test PIOA"""
```

### 4. Document Regression Tests
Always explain the bug being prevented:
```python
def test_array_offset_regression():
    """
    REGRESSION: Array offsets were calculated incorrectly

    Bug: next_offset = current_offset + size
    Fix: next_offset = current_offset + (size * dim)
    """
```

### 5. Use Assertion Helpers
```python
# Good ✅
AssertHelpers.assert_contains_all(
    output,
    "expected pattern 1",
    "expected pattern 2"
)

# Less clear ❌
assert "expected pattern 1" in output
assert "expected pattern 2" in output
```

## CI Integration (TODO)

```yaml
# .github/workflows/test.yml
name: Tests
on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
      - run: pip install pytest pytest-cov
      - run: pytest tests/ --cov=. --cov-report=xml
      - uses: codecov/codecov-action@v2
```

## Next Steps

1. **Add bitfield generator tests** - Test bit field generation
2. **Add enum generator tests** - Test enumeration generation
3. **Add compilation tests** - Actually compile generated code
4. **Increase coverage to 95%** - Test edge cases
5. **Setup CI** - Run tests on every commit

## References

- [TESTING.md](../TESTING.md) - Testing strategy
- [REFACTORING_PLAN.md](../REFACTORING_PLAN.md) - Refactoring plan
- [TEST_RESULTS.md](../TEST_RESULTS.md) - Current test results

---
Last updated: 2025-11-07
