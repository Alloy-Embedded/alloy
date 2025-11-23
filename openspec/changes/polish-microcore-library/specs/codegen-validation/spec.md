# Spec: Code Generation Validation

## ADDED Requirements

### Requirement: Generated code must compile without warnings
All generated register definitions MUST compile cleanly with strict warnings enabled.

#### Scenario: Validate STM32F4 generated code
```bash
$ cd tools/codegen/tests
$ cmake -B build-stm32f4 -DTARGET=stm32f401
$ cmake --build build-stm32f4

# Compiler flags include:
# -Wall -Wextra -Wpedantic -Werror
# -Wconversion -Wsign-conversion
```

**Expected**:
- Zero compiler warnings
- Zero errors
- Generated types match SVD specifications

**Rationale**: Prevents subtle bugs from generated code

#### Scenario: Check generated code formatting
```bash
$ clang-format --dry-run --Werror src/generated/stm32f401/*.hpp
# (no output = correctly formatted)
```

**Expected**: Generated code follows project style guide
**Rationale**: Consistency and readability

### Requirement: Generated code validated at build time
CMake build MUST fail if generated code is stale or invalid.

#### Scenario: SVD file modified but code not regenerated
```bash
$ touch tools/svd/stm32f401.svd
$ cmake --build build
CMake Error: Generated code out of date!
  Run: ./ucore generate stm32f401
  Or: make regenerate-all
```

**Expected**: Clear error message with remediation steps
**Rationale**: Prevents using outdated generated code

#### Scenario: Regenerate and validate
```bash
$ ./ucore generate stm32f401
Parsing SVD: tools/svd/stm32f401.svd
Generating: src/generated/stm32f401/
Validating: Compiling test file...
✓ Generated code valid

$ cmake --build build
# Continues normally
```

**Expected**: Automatic validation after generation
**Rationale**: Fast feedback loop

### Requirement: Generated types match SVD specifications
Register types MUST exactly match SVD field definitions.

#### Scenario: Verify GPIO register structure
```cpp
// Generated from SVD
namespace ucore::generated::stm32f401 {
    struct GPIOA_Registers {
        volatile uint32_t MODER;     // Offset 0x00
        volatile uint32_t OTYPER;    // Offset 0x04
        volatile uint32_t OSPEEDR;   // Offset 0x08
        // ... matches SVD exactly
    };

    static_assert(sizeof(GPIOA_Registers) == 0x400);
    static_assert(offsetof(GPIOA_Registers, MODER) == 0x00);
}
```

**Expected**: Static assertions verify layout matches SVD
**Rationale**: Type safety and correctness

## MODIFIED Requirements

### Requirement: Code generation integrated into build system
Extend existing codegen-system MUST include validation.

#### Scenario: Add validation to existing generate command
```bash
$ ./ucore generate stm32f401 --validate
Parsing SVD...
Generating code...
Compiling validation test...
Running clang-tidy...
✓ All checks passed
```

**Expected**: Single command generates and validates
**Rationale**: Streamlined workflow

## REMOVED Requirements

None. This adds validation to existing generation.
