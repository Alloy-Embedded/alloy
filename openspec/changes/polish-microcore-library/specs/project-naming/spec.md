# Spec: Project Naming Standardization

## ADDED Requirements

### Requirement: Consistent MicroCore branding in all documentation
All user-facing documentation MUST use "MicroCore" as the project name, with "ucore" as the CLI/namespace abbreviation.

#### Scenario: User reads README
```markdown
# MicroCore - Modern C++20 Embedded HAL

Zero-overhead hardware abstraction layer...

```bash
./ucore build nucleo_f401re blink
```

**Expected**: No references to "Alloy" anywhere in README
**Rationale**: Professional appearance, clear identity

#### Scenario: Developer searches codebase for old references
```bash
$ rg -i "alloy" --type md docs/
# (no results)
```

**Expected**: Zero matches in user-facing markdown files
**Rationale**: Complete migration from old name

### Requirement: Namespace uses ucore throughout
All C++ code MUST use `ucore::` namespace prefix, never `alloy::`.

#### Scenario: Compile any example
```cpp
#include "board.hpp"

int main() {
    using namespace ucore::board;  // OK
    // using namespace alloy::board;  // Compilation error
}
```

**Expected**: Code compiles, no alloy namespace exists
**Rationale**: Already implemented, validates completion

### Requirement: CLI binary named ucore
The main command-line tool MUST be named `ucore` not `alloy.py`.

#### Scenario: User follows quickstart guide
```bash
$ ./ucore list boards
Available boards:
  nucleo_f401re    STM32F401RE Cortex-M4 84MHz
  ...
```

**Expected**: Command works, no alloy.py referenced
**Rationale**: Consistent branding across all touchpoints

## MODIFIED Requirements

### Requirement: CMake project name reflects MicroCore
Root CMakeLists.txt project() declaration MUST use MicroCore.

#### Scenario: Configure CMake project
```cmake
cmake_minimum_required(VERSION 3.25)
project(MicroCore VERSION 1.0.0 LANGUAGES CXX)
```

**Expected**: Project name is "MicroCore" not "Alloy"
**Rationale**: Build system consistency

## REMOVED Requirements

None. This is additive standardization.
