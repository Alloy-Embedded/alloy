# Code Generation Tests

Tests for validating generated register definitions from SVD files.

## Purpose

These tests ensure that:
1. Generated code compiles correctly
2. Register structures have expected properties
3. Namespaces are organized correctly
4. All expected peripherals are available
5. Code generation produces valid C++20 code

## Test Status

### ✅ Core Tests (Passing)
- `test_result` - Result<T, E> error handling (20 tests)
- `test_types` - Type aliases (16 tests)
- `test_memory` - Memory utilities (13 tests)
- `test_error_code` - ErrorCode enum (13 tests)
- `test_concepts` - C++20 concepts (26 tests)
- `test_units` - BaudRate units (30 tests)

**Total: 118 tests passing**

### ⚠️ Code Generation Tests (Issues Found)

#### SAME70 Family (Atmel/Microchip)
**Status**: ❌ Disabled - Has codegen issues

**Problems Found**:
1. **pin_functions.hpp** - Template specialization errors
   - Using value-based template parameters instead of types
   - Example: `PinFunction<pins::PA8, PeripheralFunction::C>`
   - `pins::PA8` should be a type, not a value
   - Need to fix code generator to use proper template parameters

2. **usbhs_bitfields.hpp** - Syntax errors
   - Multiple "expected unqualified-id" errors
   - Lines 1214-1233 have issues
   - Likely macro or field name conflicts

**Files**:
- `test_same70_codegen.cpp` - Created but disabled
- MCU: ATSAME70Q21
- Vendor: Atmel/Microchip
- Family: SAME70

#### STM32F1 Family (STMicroelectronics)
**Status**: ❌ Disabled - Has codegen issues

**Problems Found**:
1. **adc1_registers.hpp** - Syntax errors
   - Unknown type name 'x' (lines 52, 59, 66, 73)
   - Duplicate member 'uint32_t' (lines 63, 70, 77)
   - Expected ';' at end of declaration list
   - Likely issue with reserved register names or array syntax

**Files**:
- `test_stm32f1_codegen.cpp` - Created but disabled
- MCU: STM32F103
- Vendor: STMicroelectronics
- Family: STM32F1

#### STM32F4 Family (STMicroelectronics)
**Status**: ❌ Disabled - Has codegen issues

**Problems Found**:
1. Similar issues to STM32F1 (not fully investigated yet)
   - Likely same register generation problems

**Files**:
- `test_stm32f4_codegen.cpp` - Created but disabled
- MCU: STM32F407
- Vendor: STMicroelectronics
- Family: STM32F4

## Action Items

### High Priority
1. **Fix pin_functions.hpp generation**
   - Change from value-based to type-based template parameters
   - Or use constexpr functions instead of template specialization
   - Generator file: `tools/codegen/cli/generators/generate_pin_functions.py`

2. **Fix register array generation**
   - Handle reserved/unnamed fields properly
   - Avoid using 'x' as field name (reserved/conflicts)
   - Handle `uint32_t` field name collisions
   - Generator file: `tools/codegen/cli/generators/generate_registers.py`

3. **Fix USB bitfields generation**
   - Identify and fix syntax errors in usbhs_bitfields.hpp
   - Check for macro conflicts or reserved keywords
   - Generator file: `tools/codegen/cli/generators/generate_bitfields.py`

### Medium Priority
4. **Add more MCU family tests**
   - STM32F0 (already has generated code)
   - SAMV71 (already has generated code)
   - STM32F7 (if exists)

5. **Add bitfield value tests**
   - Test that bitfield constants have correct values
   - Test bitfield manipulation functions

### Low Priority
6. **Add integration tests**
   - Test cross-peripheral interactions
   - Test typical usage patterns
   - Test compile-time vs runtime behavior

## How to Enable Tests

Once code generation issues are fixed:

1. **Edit** `tests/unit/CMakeLists.txt`
2. **Uncomment** the appropriate test:
   ```cmake
   add_executable(test_same70_codegen codegen/test_same70_codegen.cpp)
   add_test(NAME SAME70CodegenTest COMMAND test_same70_codegen)
   ```
3. **Add** to `run_tests` dependencies
4. **Rebuild**: `cd build_tests && cmake .. && make`
5. **Test**: `make test`

## Testing Methodology

Each test file:
1. Includes the `register_map.hpp` for the target MCU
2. Verifies compilation (if it compiles, basic codegen works)
3. Checks register struct sizes and properties
4. Verifies namespace hierarchy
5. Tests that expected peripherals exist
6. Validates type traits (trivial, standard layout, etc.)

## Success Criteria

A family test passes when:
- ✅ All code compiles without errors
- ✅ All namespaces are accessible
- ✅ Register structures have correct properties
- ✅ Expected peripherals are present
- ✅ Type safety is maintained

## Current Results

```
Test project /Users/lgili/Documents/01 - Codes/01 - Github/corezero/build_tests
    ✅ Core Tests: 6/6 suites passing (118 tests)
    ⚠️  Codegen Tests: 0/3 families passing (issues found)

Code generation needs fixes before MCU-specific tests can pass.
```

## Benefits When Working

Once these tests pass:
1. ✅ Confidence in code generator correctness
2. ✅ Regression detection when updating generator
3. ✅ Validation of new MCU family additions
4. ✅ Documentation of supported families
5. ✅ CI/CD integration for quality assurance

## Notes

- These tests are **compile-time focused** - they verify code generation quality
- They do **not** test hardware behavior (no hardware required)
- They catch **code generation bugs early** in the development cycle
- They provide **fast feedback** on generator changes (<1 second per family)
