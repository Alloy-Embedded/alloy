# CLI Development Prerequisite

**Status**: BLOCKING
**Priority**: CRITICAL
**Estimated Time**: 1 week (24 hours)

---

## Summary

**The Enhanced CLI (`enhance-cli-professional-tool`) CANNOT START until Phase 4 of this spec completes.**

This document explains exactly what needs to be done before CLI development can begin.

---

## What Blocks CLI?

The CLI spec requires a clean, organized codegen architecture to implement:
1. **YAML schemas** - needs `core/schema_validator.py` foundation
2. **Metadata commands** - needs `generators/` structure to locate peripheral generators
3. **ValidationService wrapper** - needs `core/validators/` with stable interfaces
4. **Project templates** - needs separation from peripheral templates

**Current state**: The codegen system has 91 Python files in a flat structure with unclear organization.

**Required state**: Clean `core/`, `generators/`, `vendors/`, `templates/` separation with plugin architecture.

---

## Exact Prerequisite: Phase 4 (Codegen Reorganization)

### What Phase 4 Delivers

**New Directory Structure**:
```
tools/codegen/
├── core/                      # Core functionality (CLI imports from here)
│   ├── svd_parser.py
│   ├── template_engine.py     # CLI uses this for project templates
│   ├── schema_validator.py    # CLI extends this for YAML schemas
│   ├── validators/            # CLI wraps these in ValidationService
│   │   ├── syntax_validator.py
│   │   ├── semantic_validator.py
│   │   ├── compile_validator.py
│   │   └── test_validator.py
│   └── file_utils.py
│
├── generators/                # Peripheral generators (CLI discovers these)
│   ├── gpio_generator.py      # CLI metadata commands need to locate these
│   ├── uart_generator.py
│   ├── spi_generator.py
│   └── ...
│
├── templates/                 # Template separation
│   ├── platform/              # Peripheral templates (owned by Library Quality)
│   │   ├── gpio.hpp.j2
│   │   ├── uart.hpp.j2
│   │   └── ...
│   └── board/
│       └── board.hpp.j2
│   # NOTE: CLI will add templates/projects/ for blinky, uart_logger, etc.
│
├── metadata/                  # Metadata structure (coordinated with CLI)
│   ├── schema/
│   │   ├── platform.schema.json
│   │   └── peripheral.schema.json
│   └── platforms/
│       └── stm32f4/
│
└── vendors/                   # Vendor-specific logic
    ├── st/
    ├── atmel/
    └── nordic/
```

### What Phase 4 Tasks Accomplish

| Task | Time | CLI Dependency |
|------|------|----------------|
| 4.1 Design New Structure | 2h | Defines `core/validators/` interface |
| 4.2 Create Migration Script | 4h | Creates `core/schema_validator.py` foundation |
| 4.3 Execute Migration | 4h | Provides template engine API to CLI |
| 4.4 Update Imports/Paths | 6h | Coordinates YAML metadata structure |
| 4.5 Test Generators | 4h | Validates `generators/` discoverable by CLI |
| 4.6 Update Documentation | 4h | Documents APIs CLI will consume |

**Total**: 24 hours (1 week)

---

## What CLI Can Do After Phase 4

Once Phase 4 completes, CLI can immediately start **Phase 0: YAML Migration**:

1. ✅ **Add PyYAML dependency** - can safely add to `requirements.txt`
2. ✅ **Create YAML schemas** - extends `core/schema_validator.py`
3. ✅ **Implement metadata commands** - discovers generators in `generators/`
4. ✅ **Add ValidationService** - wraps validators from `core/validators/`
5. ✅ **Create project templates** - adds to `templates/projects/` (won't conflict)

---

## Implementation Order

### Week 1: Library Quality Phase 4 (THIS WEEK)
**Owner**: Library Quality team
**Goal**: Reorganize codegen system
**Tasks**: See `tasks.md` Phase 4 (24 hours)
**Deliverable**: Clean plugin architecture ready for CLI

### Week 2: CLI Phase 0 Begins (NEXT WEEK)
**Owner**: CLI team
**Goal**: YAML Migration
**Prerequisites**: ✅ Phase 4 completed
**Tasks**: See `enhance-cli-professional-tool/tasks.md` Phase 0 (20 hours)

### Weeks 3-12: Parallel Development
Both specs continue in parallel with weekly sync meetings.

---

## Coordination Points

### Before Phase 4.2 Completes
- [ ] Library Quality team documents validator interfaces
- [ ] CLI team reviews interface design
- [ ] Agreement on validator API contract

### After Phase 4.3 Completes
- [ ] Library Quality team provides template engine API docs
- [ ] CLI team plans project template integration
- [ ] Agreement on template variable naming

### Before Phase 4.4 Completes
- [ ] Library Quality team defines metadata structure
- [ ] CLI team designs YAML schema format
- [ ] Agreement on metadata field names and types

### After Phase 4 Fully Completes
- [ ] Library Quality team confirms all tests pass
- [ ] CLI team verifies can import from new structure
- [ ] **CLI Phase 0 begins immediately**

---

## Success Criteria for Unblocking CLI

Phase 4 is considered complete when:

1. ✅ All 91 Python files reorganized into `core/`, `generators/`, `vendors/`
2. ✅ `core/validators/` directory exists with 4 validator modules
3. ✅ `core/schema_validator.py` exists and is documented
4. ✅ `core/template_engine.py` API is documented
5. ✅ `generators/` contains all peripheral generators
6. ✅ `templates/platform/` contains peripheral templates
7. ✅ All imports updated and working
8. ✅ All existing tests pass (no regressions)
9. ✅ Documentation updated with new structure
10. ✅ CLI team confirms they can import from new structure

**Validation**: Run `pytest tools/codegen/tests/ -v` - all tests must pass.

---

## Risk Mitigation

**Risk**: Phase 4 breaks existing build system
**Mitigation**: Migration script includes rollback capability

**Risk**: Phase 4 takes longer than 1 week
**Impact**: CLI start delayed by same amount
**Mitigation**: Daily progress updates, escalate blockers immediately

**Risk**: CLI team discovers missing API after Phase 4
**Mitigation**: Review checkpoints at 4.2, 4.3, 4.4 (see Coordination Points above)

---

## Next Steps

1. **Library Quality team**: Start Phase 4.1 (Design New Structure) immediately
2. **CLI team**: Review this document and Phase 4 tasks, provide feedback on validator/template APIs needed
3. **Both teams**: Schedule checkpoint meetings for 4.2, 4.3, 4.4 completion
4. **Both teams**: Agree on "Phase 4 complete" criteria before starting

---

## Questions?

- See `openspec/changes/INTEGRATION_LIBRARY_CLI.md` for full parallel execution plan
- See `library-quality-improvements/tasks.md` Phase 4 for detailed task breakdown
- See `library-quality-improvements/proposal.md` Phase 4 for technical design

**Once Phase 4 is done, CLI development can proceed at full speed!** 🚀
