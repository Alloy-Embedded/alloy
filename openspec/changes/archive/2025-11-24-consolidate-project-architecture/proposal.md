# Consolidate Project Architecture

## Problem

The CoreZero/Alloy framework has evolved rapidly, resulting in architectural inconsistencies that reduce maintainability and create confusion:

1. **Dual HAL Structure**: Both `/src/hal/vendors/` and `/src/hal/platform/` contain platform-specific code with unclear boundaries
2. **Naming Inconsistency**: Mixed use of "CoreZero" and "Alloy" throughout codebase, documentation, and build system
3. **Incomplete Board Abstraction**: Board layer uses `#ifdef` ladders instead of proper abstraction, breaking portability promise
4. **Documentation Mismatch**: README describes old structure that no longer exists
5. **Code Generation Sprawl**: 10+ separate generator scripts with duplicated logic
6. **CMake GLOB Anti-pattern**: Using `file(GLOB ...)` instead of explicit source lists
7. **Inconsistent APIs**: Clock, GPIO, and other peripheral APIs differ across MCU families
8. **Missing Type Safety**: No C++20 concepts to validate hardware policies

## Solution

Implement a **phased architectural consolidation** that:

1. **Unifies Directory Structure**: Merge `vendors/` and `platform/` into single coherent hierarchy
2. **Standardizes Naming**: Choose one name (Alloy) and apply consistently
3. **Fixes Board Abstraction**: Remove `#ifdef` ladders, use policy-based design throughout
4. **Updates Documentation**: Align README, guides, and examples with current reality
5. **Consolidates Code Generation**: Merge generators into unified system
6. **Modernizes Build System**: Replace GLOB with explicit lists, add validation
7. **Standardizes APIs**: Create common interfaces across all platforms
8. **Adds Type Safety**: Implement C++20 concepts for compile-time validation

## Impact

### Benefits
- **Maintainability**: Clear structure with single place for each concern
- **Onboarding**: New developers understand architecture immediately
- **Portability**: True board abstraction without platform-specific code in examples
- **Type Safety**: Compile-time validation catches errors before deployment
- **Documentation**: Accurate guides that match implementation

### Risk Mitigation
- **Incremental Changes**: Each phase is independently testable
- **Backward Compatibility**: Migration path for existing code
- **Validation at Each Step**: All examples must build and run after each phase
- **Automated Testing**: CI validates changes don't break existing functionality

### Affected Components
- `/src/hal/platform/` - Will be merged into `/src/hal/vendors/`
- `/src/hal/vendors/` - Will become canonical location with `/generated/` subdirectories
- `/boards/` - Board abstraction will be strengthened
- `/cmake/` - Build system will be modernized
- `/tools/codegen/` - Generators will be consolidated
- `/examples/` - Will be updated to use new structure
- `/README.md` - Will be rewritten to match reality
- All references to "CoreZero" will become "Alloy"

## Timeline

**Phase 1 (Consolidation)**: 1-2 weeks
- Merge directory structures
- Standardize naming
- Fix board abstraction
- Update documentation

**Phase 2 (Type Safety)**: 1 week
- Add C++20 concepts
- Implement policy validation
- Add compile-time checks

**Phase 3 (Quality)**: 2-3 weeks
- Consolidate code generation
- Fix CMake issues
- Standardize APIs
- Add comprehensive tests

**Phase 4 (Validation)**: 1 week
- Full regression testing
- Hardware validation on all boards
- Performance benchmarking
- Documentation review

## Success Criteria

- ✅ Single source of truth for platform-specific code
- ✅ Consistent naming throughout codebase
- ✅ No `#ifdef` ladders in board layer
- ✅ Documentation matches implementation 100%
- ✅ All existing examples build and run
- ✅ No performance regression
- ✅ Code generation consolidated to <5 scripts
- ✅ Explicit CMake source lists (no GLOB)
- ✅ Concepts validate all hardware policies
- ✅ Consistent APIs across all platforms
