# Naming Convention

## Decision: "Alloy" is the Canonical Name

**Date**: 2025-11-15
**Status**: Adopted
**Authors**: CoreZero Team

## Rationale

The project has been renamed from "CoreZero" to "Alloy" for the following reasons:

### 1. **Brevity**
- "Alloy" is 5 characters vs "CoreZero" at 8 characters
- Shorter names improve code readability and reduce typing
- Fits better in log output, documentation headers, and comments

### 2. **Uniqueness**
- GitHub search for "CoreZero" yields many results (generic term)
- "Alloy" is more distinctive in embedded systems context
- Easier to search for project-specific issues and documentation

### 3. **Metaphorical Fit**
- An **alloy** is a mixture of metals combined to create superior properties
- The framework combines multiple **hardware abstraction layers** (HALs) into a unified system
- Different platforms (STM32, SAME70, ESP32, etc.) are "alloyed" together
- Policy-based design mixes compile-time and runtime properties

### 4. **Consistency**
- CMake project already uses `project(alloy ...)`
- All namespaces already use `namespace alloy::`
- Target names already use `alloy-hal`, `alloy-core`
- Only documentation needed updating

## Implementation

### What Changed
- ✅ Project README title
- ✅ Build system help text (Makefile, scripts)
- ✅ CMake targets (already using "alloy")
- ✅ Namespaces (already using `alloy::`)

### What Did NOT Change
- ❌ Repository name (still "corezero" - GitHub URL unchanged)
- ❌ Git history (preserved all references for archaeology)
- ❌ OpenSpec documentation (historical context preserved)

## Usage Guidelines

### In Code
```cpp
// ✅ Correct
namespace alloy::hal::gpio { ... }
#ifndef ALLOY_HAL_GPIO_HPP
#define ALLOY_VERSION_MAJOR 0

// ❌ Avoid (deprecated)
namespace corezero::hal::gpio { ... }
#define COREZERO_HAL_GPIO_HPP
```

### In Documentation
```markdown
✅ "Alloy Framework"
✅ "The Alloy HAL"
✅ "Alloy's policy-based design"

❌ "CoreZero Framework" (deprecated)
❌ "The CoreZero HAL" (deprecated)
```

### In Build System
```cmake
# ✅ Correct (already in use)
project(alloy VERSION 0.1.0 LANGUAGES CXX C ASM)
add_library(alloy-hal ...)

# ❌ Avoid
project(corezero ...)
add_library(corezero-hal ...)
```

## Migration Notes

For users of the previous "CoreZero" naming:

1. **Source Code**: No changes required - namespaces already used `alloy::`
2. **Build Targets**: Already using `alloy-hal` - no changes needed
3. **Documentation**: Update references to "CoreZero" → "Alloy" in custom docs
4. **CMake Integration**: Project already exports `alloy::` targets

## See Also

- [OpenSpec: Consolidate Project Architecture](openspec/changes/consolidate-project-architecture/)
- [Design Document](openspec/changes/consolidate-project-architecture/design.md)
- [Implementation Tasks](openspec/changes/consolidate-project-architecture/tasks.md)
