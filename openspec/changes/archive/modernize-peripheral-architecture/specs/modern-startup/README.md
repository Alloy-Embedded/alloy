# Modern ARM Startup & Initialization - OpenSpec

**Status**: 📝 PROPOSAL
**Date**: 2025-11-11
**Priority**: HIGH
**Complexity**: MEDIUM

---

## 🎯 Executive Summary

This specification defines the modernization of ARM Cortex-M startup code to C++23 standards, making it more flexible, maintainable, and auto-generated.

### What This Achieves

1. **Modern C++23 Startup** - Replace legacy startup code with modern C++ idioms
2. **Per-MCU Auto-Generation** - Generate startup code from metadata (like hardware policies)
3. **Cleanup Legacy Code** - Remove old interrupt/systick classes from hal/startup

### Key Benefits

- ✅ **90% Less Boilerplate** - Auto-generated startup code
- ✅ **Type Safety** - Compile-time vector table validation
- ✅ **Flexibility** - Early initialization hooks for application setup
- ✅ **Zero Overhead** - All abstractions inline/constexpr
- ✅ **Easy Porting** - One metadata file per MCU

---

## 📋 Three-Part Plan

### Part 1: Modern ARM Startup (C++23)

**Goal**: Replace legacy startup with modern C++23 implementation

**Files Affected**:
- `src/hal/vendors/arm/cortex_m7/startup.hpp` (modernize)
- `src/hal/vendors/arm/cortex_m7/startup.cpp` (modernize)
- `src/hal/vendors/arm/cortex_m7/vector_table.hpp` (new)

**Key Changes**:
- Use `constexpr` and `consteval` for vector table generation
- Template-based initialization hooks
- Modern exception handling setup
- C++23 standard library features

**Impact**: ~500 lines refactored

---

### Part 2: Per-MCU Startup Generation

**Goal**: Auto-generate startup code from MCU metadata (like hardware policies)

**Files Created**:
- `metadata/platform/same70_startup.json` (metadata)
- `tools/codegen/cli/generators/templates/startup.cpp.j2` (template)
- `tools/codegen/cli/generators/startup_generator.py` (generator)

**Key Features**:
- Declarative metadata (vector count, memory layout, reset defaults)
- Jinja2 templates for startup code
- Integration with existing codegen CLI
- Per-MCU customization support

**Impact**: +800 lines new infrastructure

---

### Part 3: Cleanup Legacy Code

**Goal**: Remove deprecated interrupt/systick classes from hal/startup

**Files Removed/Modified**:
- `src/hal/vendors/arm/same70/startup/` (cleanup)
- Old interrupt manager classes (identify & remove)
- Old systick classes (identify & remove)

**Migration Strategy**:
- Audit all references to old code
- Migrate to new hardware policies
- Remove deprecated files
- Update documentation

**Impact**: ~1000 lines removed

---

## 📊 Success Criteria

| Criterion | Target | How to Verify |
|-----------|--------|---------------|
| Startup code modernized | C++23 features used | Code review |
| Auto-generation working | Generates for SAME70 | Run `./generate_startup.sh same70` |
| Legacy code removed | 0 deprecated files | File audit |
| Examples updated | Uses new startup | Build & run examples |
| Zero overhead maintained | Same binary size | Size comparison |
| Documentation complete | All parts documented | Review docs |

---

## 🔗 Related Documents

- [SPEC.md](./SPEC.md) - Complete technical specification
- [EXAMPLES.md](./EXAMPLES.md) - Code examples for each part
- [MIGRATION.md](./MIGRATION.md) - Migration guide from old to new
- [TESTING.md](./TESTING.md) - Testing strategy

---

## 📅 Implementation Phases

### Phase 1: Analysis (1-2 hours)
- Audit existing startup code
- Identify all deprecated classes
- Map dependencies

### Phase 2: Modern Startup (4-6 hours)
- Implement C++23 startup.hpp/cpp
- Create constexpr vector table
- Add initialization hooks
- Test with LED blink example

### Phase 3: Auto-Generation (6-8 hours)
- Create startup metadata format
- Implement Jinja2 templates
- Create Python generator
- Integrate with codegen CLI
- Test generation for SAME70

### Phase 4: Cleanup (2-3 hours)
- Remove deprecated files
- Update all references
- Update documentation
- Final testing

**Total Estimated Time**: 13-19 hours

---

## 🎓 Technical Approach

### Principle 1: Zero Overhead
All abstractions must inline to direct code execution with no runtime cost.

### Principle 2: Compile-Time Safety
Use templates and constexpr to catch errors at compile time.

### Principle 3: Metadata-Driven
Startup code generated from declarative JSON metadata.

### Principle 4: Backward Compatible
Existing examples should work with minimal changes.

---

## 🚀 Quick Start

After implementation, generating startup code will be:

```bash
# Generate startup for SAME70
./tools/codegen/cli/generators/generate_startup.sh same70

# Output:
# src/hal/vendors/arm/same70/startup_same70.cpp
# src/hal/vendors/arm/same70/vector_table_same70.hpp
```

Application code becomes:

```cpp
#include "boards/same70_xplained/board.hpp"

// Optional: Early initialization hook
extern "C" void early_init() {
    // Configure PLL, flash wait states, etc.
}

int main() {
    board::init();  // Uses generated startup
    // Application logic...
}
```

---

**Next**: See [SPEC.md](./SPEC.md) for complete technical specification
