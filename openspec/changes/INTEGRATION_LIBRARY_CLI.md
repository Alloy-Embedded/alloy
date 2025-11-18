# Integration Plan: Library Quality + Enhanced CLI

**Document**: OpenSpec Integration Coordination
**Date**: 2025-01-17
**Specs**: `library-quality-improvements` + `enhance-cli-professional-tool`
**Status**: Planning

---

## Executive Summary

This document coordinates two major OpenSpec proposals to avoid duplication, ensure seamless integration, and optimize timeline through parallelization.

### Key Integration Points
- **YAML Metadata Format**: Shared across both specs
- **Validation Infrastructure**: Library Quality owns core, CLI wraps in service layer
- **Code Generation**: Coordinated restructuring
- **Timeline**: 12 weeks parallel (vs 21.5 weeks sequential) = **40% faster**

---

## 1. Ownership Matrix

| Component | Owner | Consumer | Notes |
|-----------|-------|----------|-------|
| **YAML Schemas** | CLI | Library Quality | CLI defines, LQ uses |
| **Metadata Validation Commands** | CLI | Library Quality | `alloy metadata validate` |
| **Core Validators** | Library Quality | CLI | Syntax, Semantic, Compile, Test |
| **ValidationService** | CLI | - | Wraps LQ validators |
| **Peripheral Templates** | Library Quality | CLI | GPIO, UART, SPI, I2C, etc. |
| **Project Templates** | CLI | - | blinky, uart_logger, rtos |
| **Codegen Structure** | Library Quality | CLI | LQ reorganizes, CLI builds on top |
| **Configuration System** | CLI | Both | `.alloy.yaml` |
| **Preview/Diff** | CLI | Library Quality | Uses for validation |
| **Incremental Generation** | CLI | Library Quality | Speeds development |

---

## 2. Shared Components

### 2.1 Metadata Database

**Location**: `tools/codegen/database/`

**Structure**:
```
database/
├── mcus/
│   ├── stm32f4.yaml             # CLI owns (discovery layer)
│   ├── same70.yaml
│   └── index.yaml
├── boards/
│   ├── nucleo_f401re.yaml       # CLI owns (board configs)
│   ├── same70_xplained.yaml
│   └── index.yaml
├── peripherals/
│   ├── gpio/
│   │   ├── stm32f4_gpio.yaml   # Library Quality owns (impl details)
│   │   └── same70_gpio.yaml
│   ├── uart/
│   │   ├── stm32f4_uart.yaml
│   │   └── same70_uart.yaml
│   └── index.yaml
└── templates/
    ├── peripheral/               # Library Quality owns
    │   ├── gpio.hpp.j2
    │   ├── uart.hpp.j2
    │   ├── spi.hpp.j2
    │   └── ...
    └── project/                  # CLI owns
        ├── blinky/
        ├── uart_logger/
        └── rtos_tasks/
```

**Format**: YAML (defined by CLI Phase 0)

**Schema Validation**:
- CLI provides: `alloy metadata validate [name] [--strict]`
- Library Quality uses for CI/CD validation

**Access**:
```python
# Both specs use same loader
from cli.loaders import YAMLDatabaseLoader

loader = YAMLDatabaseLoader('database/')
mcu = loader.load('mcus', 'stm32f4')
gpio_metadata = loader.load('peripherals/gpio', 'stm32f4_gpio')
```

### 2.2 Validation Pipeline

**Core Validators** (Library Quality owns):
```python
# tools/codegen/validators/
├── syntax_validator.py          # Clang AST parser
├── semantic_validator.py        # SVD cross-reference
├── compile_validator.py         # ARM GCC compilation
└── test_validator.py            # Auto-generate Catch2 tests
```

**Service Layer** (CLI owns):
```python
# tools/codegen/cli/services/validation_service.py
from validators import SyntaxValidator, SemanticValidator, CompileValidator

class ValidationService:
    """User-facing orchestration of validators"""

    @staticmethod
    def validate_all(file_path: Path) -> ValidationResult:
        result = ValidationResult()

        # Stage 1: Syntax (uses Library Quality validator)
        result.add_check("syntax", SyntaxValidator.validate(file_path))

        # Stage 2: Semantics (uses Library Quality validator)
        result.add_check("semantics", SemanticValidator.validate(file_path))

        # Stage 3: Compilation (uses Library Quality validator)
        result.add_check("compilation", CompileValidator.validate(file_path))

        return result
```

**Benefits**:
- ✅ **No duplication**: Single validation logic
- ✅ **Clear separation**: Library Quality = core, CLI = orchestration
- ✅ **Testable**: Library Quality tests validators, CLI tests service layer

### 2.3 Template System

**Peripheral Templates** (Library Quality owns):
- Location: `database/templates/peripheral/`
- Scope: GPIO, UART, SPI, I2C, ADC, Timer, DMA, startup
- Format: Jinja2 with YAML metadata
- Purpose: Generate low-level HAL code

**Project Templates** (CLI owns):
- Location: `database/templates/project/`
- Scope: blinky, uart_logger, rtos_tasks
- Format: Directory structure + CMakeLists.txt
- Purpose: Initialize new projects

**Relationship**:
```yaml
# CLI's project template references Library Quality's peripheral templates
# database/templates/project/uart_logger/config.yaml
project:
  name: uart_logger
  peripherals:
    - uart  # Uses Library Quality's uart.hpp.j2
    - gpio  # Uses Library Quality's gpio.hpp.j2
```

**Shared Infrastructure**:
```python
# tools/codegen/template_engine.py (shared by both)
class TemplateEngine:
    def render(self, template: str, context: dict) -> str:
        """Render Jinja2 template with context"""
        # Used by both peripheral and project templates
```

---

## 3. Timeline Coordination

### 3.1 Sequential Approach (DON'T DO THIS)

**Total**: 21.5 weeks

```
Weeks 1-10:  Library Quality (10 weeks)
Weeks 11-21.5: CLI (11.5 weeks)
```

**Problems**:
- ❌ Long time to value
- ❌ CLI can't leverage Library Quality improvements
- ❌ Risk of incompatible changes

### 3.2 Parallel Approach (RECOMMENDED)

**Total**: 12 weeks (40% faster!)

```
Phase 0: Foundation (Weeks 1-2)
├─ Library Quality Phase 4: Codegen Reorganization (1 week)
└─ CLI Phase 0: YAML Migration (1 week) [starts week 2]

Phase 1: Parallel Development (Weeks 3-5)
├─ Library Quality Phase 1: API Refactoring (3 weeks)
└─ CLI Phase 1: Foundation & Discovery (3 weeks) [parallel]

Phase 2: Integration (Weeks 6-9)
├─ Library Quality Phase 2: Template System (2 weeks)
├─ Library Quality Phase 3: Test Coverage (2 weeks) [weeks 8-9]
└─ CLI Phase 2: Validation Pipeline (2 weeks) [weeks 8-9, parallel]

Phase 3: Polish (Weeks 10-12)
├─ CLI Phase 3: Interactive Init (2 weeks)
├─ Library Quality Phase 5: Documentation (1 week) [parallel]
├─ Library Quality Phase 6: Startup Optimization (2 weeks) [parallel]
└─ CLI Phase 4-5: Build & Docs (2 weeks) [parallel]
```

**Critical Path**:
1. Library Quality Phase 4 (Codegen Reorg) - Week 1
2. CLI Phase 0 (YAML Migration) - Week 2
3. Parallel development - Weeks 3-12

**Dependencies**:
- CLI Phase 0 depends on: Nothing (can start after LQ Phase 4)
- Library Quality Phase 1 depends on: CLI Phase 0 (uses YAML)
- CLI Phase 2 depends on: Library Quality Phase 3 (validation infra)
- Library Quality Phase 2 depends on: CLI Phase 0 (YAML), LQ Phase 1

### 3.3 Week-by-Week Schedule

| Week | Library Quality | Enhanced CLI | Coordination |
|------|-----------------|--------------|--------------|
| **1** | Phase 4: Codegen Reorg (24h) | - | LQ team only |
| **2** | - | Phase 0: YAML Migration (20h) | Handoff meeting |
| **3** | Phase 1: API Refactoring (16h) | Phase 1: Foundation (17h) | Weekly sync starts |
| **4** | Phase 1 continues (16h) | Phase 1 continues (17h) | Metadata schema review |
| **5** | Phase 1 complete (16h) | Phase 1 complete (18h) | Integration checkpoint |
| **6** | Phase 2: Template System (24h) | - | CLI team uses LQ templates |
| **7** | Phase 2 continues (24h) | - | Template testing |
| **8** | Phase 3: Test Coverage (24h) | Phase 2: Validation (20h) | Validator integration |
| **9** | Phase 3 continues (24h) | Phase 2 continues (20h) | Test coordination |
| **10** | Phase 5: Documentation (24h) | Phase 3: Interactive Init (24h) | Parallel work |
| **11** | Phase 6: Startup Opt (24h) | Phase 4-5: Build/Docs (12h) | Parallel work |
| **12** | Phase 6 continues (24h) | Phase 4-5 continues (12h) | Final integration |

**Total Effort**:
- Library Quality: 232 hours (unchanged)
- CLI: 230 hours (unchanged)
- **Total Calendar Time**: 12 weeks (vs 21.5 sequential)

---

## 4. Communication Plan

### 4.1 Sync Meetings

**Weekly Sync** (Tuesday 2pm, 30 minutes):
- Review week's progress
- Identify blocking issues
- Coordinate upcoming work
- Update integration plan

**Critical Checkpoints**:
- **Week 1 End**: Codegen reorganization complete (LQ Phase 4)
- **Week 2 End**: YAML migration complete (CLI Phase 0)
- **Week 5 End**: Foundation phases complete (both specs)
- **Week 9 End**: Validation integration complete
- **Week 12 End**: Final integration testing

### 4.2 Shared Channels

**Slack**:
- `#openspec-library-quality` - Library Quality discussions
- `#openspec-cli` - CLI discussions
- `#openspec-integration` - Coordination (new)

**GitHub**:
- PRs with `[LQ]` or `[CLI]` prefix
- Cross-team review required for integration points
- Tag both teams: `@alloy/library-quality-team` `@alloy/cli-team`

### 4.3 Decision Log

**Location**: `openspec/changes/INTEGRATION_DECISIONS.md`

**Format**:
```markdown
## Decision: Use YAML for all metadata

**Date**: 2025-01-17
**Participants**: CLI team, Library Quality team
**Decision**: Adopt YAML format for all metadata (MCUs, boards, peripherals)
**Rationale**: 25-30% size reduction, inline comments, cleaner code snippets
**Impact**:
- CLI Phase 0 implements YAML loader
- Library Quality uses YAML in Phase 2+
- Backward compatibility for 2 months
**Action Items**:
- [ ] CLI: Implement YAMLDatabaseLoader (week 2)
- [ ] Library Quality: Update all examples to YAML (week 3)
- [ ] Both: Validate generated code identical (week 4)
```

---

## 5. Integration Points Detail

### 5.1 YAML Migration

**Owner**: CLI (Phase 0)

**Deliverables for Library Quality**:
1. `YAMLDatabaseLoader` class
2. Schema validation: `alloy metadata validate`
3. Migration script: `alloy migrate json-to-yaml`
4. Documentation: `YAML_METADATA_GUIDE.md`

**Library Quality Usage**:
```bash
# Week 3: Start using YAML
$ alloy migrate json-to-yaml metadata/stm32f4/gpio.json
✓ Converted to metadata/stm32f4/gpio.yaml

# Add comments for quirks
$ vim metadata/stm32f4/gpio.yaml
# ... add inline comments ...

# Validate
$ alloy metadata validate peripherals/gpio/stm32f4_gpio.yaml
✓ Valid YAML
✓ Schema: OK
✓ Linting: OK
```

**Acceptance Criteria**:
- [ ] YAMLDatabaseLoader loads all existing metadata
- [ ] Generated code identical (YAML vs JSON)
- [ ] Schema validation catches common errors
- [ ] Migration script preserves semantics 100%

### 5.2 Validation Integration

**Owner**: Shared (Library Quality core, CLI service)

**Library Quality Deliverables** (Phase 3):
```python
# tools/codegen/validators/syntax_validator.py
class SyntaxValidator:
    @staticmethod
    def validate(file: Path) -> ValidationResult:
        """Validate C++ syntax using Clang"""
        # ... implementation ...

# tools/codegen/validators/semantic_validator.py
class SemanticValidator:
    @staticmethod
    def validate(file: Path, svd: Path) -> ValidationResult:
        """Cross-reference with SVD"""
        # ... implementation ...
```

**CLI Deliverables** (Phase 2):
```python
# tools/codegen/cli/services/validation_service.py
class ValidationService:
    @staticmethod
    def validate_all(file: Path) -> ValidationResult:
        """Orchestrate all validators"""
        result = ValidationResult()
        result.add_check("syntax", SyntaxValidator.validate(file))
        result.add_check("semantics", SemanticValidator.validate(file, svd))
        # ... etc ...
        return result
```

**Integration Test**:
```python
# tests/integration/test_validation_integration.py
def test_cli_uses_library_quality_validators():
    """Verify CLI's ValidationService uses LQ validators"""
    from cli.services import ValidationService
    from validators import SyntaxValidator

    # CLI should delegate to LQ validators
    result = ValidationService.validate_all(test_file)
    assert isinstance(result.syntax, SyntaxValidator.Result)
```

**Acceptance Criteria**:
- [ ] CLI imports Library Quality validators (no duplication)
- [ ] Integration test passes
- [ ] CLI commands work: `alloy codegen validate`
- [ ] Library Quality tests cover all validators

### 5.3 Metadata Commands for Library Quality

**Owner**: CLI (Phase 1)

**Commands Library Quality Will Use**:
```bash
# Validate metadata during development
$ alloy metadata validate peripherals/gpio/stm32l4_gpio.yaml --strict

# Create new peripheral from template
$ alloy metadata create --template gpio --family stm32l4
✓ Created: peripherals/gpio/stm32l4_gpio.yaml
  Edit: vim peripherals/gpio/stm32l4_gpio.yaml

# Show what changed
$ alloy metadata diff stm32f4_gpio.yaml
```

**Value for Library Quality**:
- ✅ Phase 2 (Templates): Faster peripheral metadata creation
- ✅ Phase 3 (Testing): Validate metadata in CI/CD
- ✅ Phase 5 (Documentation): Show examples in guides

**Integration**:
```bash
# Library Quality workflow (Phase 2)
cd tools/codegen

# Create new GPIO for STM32L4
alloy metadata create --template gpio --family stm32l4

# Edit metadata
vim database/peripherals/gpio/stm32l4_gpio.yaml

# Validate
alloy metadata validate peripherals/gpio/stm32l4_gpio.yaml

# Generate code
alloy codegen generate gpio --platform stm32l4

# Preview changes
alloy codegen generate gpio --platform stm32l4 --dry-run --diff
```

### 5.4 Incremental Generation

**Owner**: CLI (Phase 6)

**Value for Library Quality**:
- **Template Development** (Phase 2): 10x faster iteration
- **Testing** (Phase 3): Quick test-fix-retest cycles
- **Optimization** (Phase 6): Measure startup code impact

**Usage**:
```bash
# Developing GPIO template
$ vim database/templates/peripheral/gpio.hpp.j2

# Regenerate only GPIO (instant feedback)
$ alloy codegen generate gpio --all-platforms --incremental
✓ stm32f4: Changed (0.5s)
✓ same70: Changed (0.6s)
✓ stm32g0: Changed (0.4s)
✓ stm32l4: Unchanged (skipped)
Total: 1.5s (vs 15s full regeneration)
```

**Estimated Timeline Reduction**:
- Library Quality Phase 2: -15% (10.5 days → 9 days)
- Library Quality Phase 3: -10% (10.5 days → 9.5 days)
- **Total**: ~3 days saved

### 5.5 Preview/Diff for CRTP Refactoring

**Owner**: CLI (Phase 6)

**Value for Library Quality**:
- **API Refactoring** (Phase 1): Confidence in CRTP changes
- **Binary Size** (Phase 6): Verify size reduction claims

**Usage**:
```bash
# Phase 1: Refactoring UartSimple to use CRTP
$ alloy codegen generate uart --api simple --dry-run --diff

Preview: Refactoring UartSimple → UartBase

--- src/hal/api/uart_simple.hpp (current: 12,006 bytes)
+++ src/hal/api/uart_simple.hpp (refactored: 2,048 bytes)

Code size: -83% (9,958 bytes saved)

@@ -10,30 +10,8 @@
 template<typename HardwarePolicy>
-class UartSimple {
+class UartSimple : public UartBase<UartSimple<HardwarePolicy>, HardwarePolicy> {
+    using Base = UartBase<UartSimple<HardwarePolicy>, HardwarePolicy>;

Apply refactoring? [y/N]:
```

---

## 6. Risk Mitigation

### Risk 1: Codegen Restructure Conflicts

**Problem**: Both specs modify `tools/codegen/` structure

**Impact**: HIGH - Could cause merge conflicts, broken imports

**Mitigation**:
1. ✅ **Library Quality Phase 4 completes FIRST** (week 1)
2. ✅ **Single PR** for codegen restructure (no parallel changes)
3. ✅ **CLI Phase 1** starts week 3 (after restructure merged)
4. ✅ **Integration tests** verify structure compatibility

**Acceptance Criteria**:
- [ ] Library Quality Phase 4 merged to main by week 1 end
- [ ] CLI Phase 1 rebases on restructured codebase
- [ ] All imports resolve correctly
- [ ] No duplicate directories or files

### Risk 2: YAML Migration Breaks Existing Workflows

**Problem**: Library Quality developers using JSON metadata

**Impact**: MEDIUM - Temporary productivity loss

**Mitigation**:
1. ✅ **Backward compatibility** for 2 months (weeks 2-10)
2. ✅ **Auto-detection**: Prefer `.yaml`, fall back to `.json`
3. ✅ **Migration script**: `alloy migrate json-to-yaml --all`
4. ✅ **Documentation**: Clear migration guide

**Timeline**:
- Week 2: YAML loader deployed (both formats work)
- Week 3: Library Quality starts using YAML
- Week 10: JSON marked deprecated
- Week 18: JSON support removed

### Risk 3: Validation Pipeline Duplication

**Problem**: Both specs implement validation logic

**Impact**: HIGH - Wasted effort, maintenance burden

**Mitigation**:
1. ✅ **Clear ownership**: Library Quality = core, CLI = wrapper
2. ✅ **Integration test**: Verify CLI uses LQ validators
3. ✅ **Code review**: Block duplicate validation logic in PRs
4. ✅ **Architecture doc**: Document validator API contract

**API Contract**:
```python
# Validator API (Library Quality owns)
class Validator:
    @staticmethod
    def validate(file: Path) -> ValidationResult:
        """Returns ValidationResult with passed/failed + details"""
        pass

# CLI MUST NOT reimplement validation logic
# CLI CAN wrap validators in service layer
```

### Risk 4: Timeline Slip

**Problem**: Parallel work may cause delays if dependencies unclear

**Impact**: MEDIUM - Could revert to sequential timeline

**Mitigation**:
1. ✅ **Weekly sync meetings** (track progress, identify blockers)
2. ✅ **Critical path tracking**: Monitor Library Quality Phase 4 → CLI Phase 0
3. ✅ **Buffer weeks**: Add 1 week buffer (week 13) for integration issues
4. ✅ **Checkpoint reviews**: Weeks 5, 9, 12 (validate integration)

**Early Warning Signals**:
- Week 1: Library Quality Phase 4 not on track
- Week 2: CLI Phase 0 blocked
- Week 5: Integration tests failing
- Week 9: Validation integration issues

---

## 7. Success Criteria

### Integration Success

**Week 2** (CLI Phase 0 complete):
- [ ] YAML loader works with Library Quality metadata
- [ ] Migration script converts all existing JSON → YAML
- [ ] Generated code identical (YAML vs JSON)
- [ ] Library Quality team trained on YAML format

**Week 5** (Foundation phases complete):
- [ ] CLI metadata commands work with Library Quality metadata
- [ ] Library Quality using CLI validation commands
- [ ] No duplicate code in validation pipeline
- [ ] Both teams can develop independently

**Week 9** (Validation integration complete):
- [ ] CLI's ValidationService uses LQ validators
- [ ] Integration tests passing
- [ ] `alloy codegen validate` command works
- [ ] No performance regression

**Week 12** (Final integration):
- [ ] All specs delivered
- [ ] Integration tests 100% passing
- [ ] Documentation cross-references correct
- [ ] Zero duplicate code
- [ ] Timeline: 12 weeks achieved (vs 21.5 sequential)

### Quality Metrics

**Code Duplication**:
- Baseline: Risk of duplicate validation (2x effort)
- Target: 0 duplicate validation logic
- Measured: Static analysis + code review

**Developer Experience**:
- Baseline: Separate CLIs for each workflow
- Target: Single unified CLI (`alloy` command)
- Measured: User testing + feedback

**Timeline Efficiency**:
- Baseline: 21.5 weeks sequential
- Target: 12 weeks parallel (40% reduction)
- Measured: Actual completion date

**Integration Quality**:
- Baseline: Potential conflicts and rework
- Target: Seamless integration, zero conflicts
- Measured: PR conflicts, rework hours

---

## 8. Deliverables

### Phase 0 (Weeks 1-2)
- [ ] Library Quality Phase 4 complete (codegen restructured)
- [ ] CLI Phase 0 complete (YAML migration)
- [ ] Integration document published (this doc)
- [ ] Metadata schema coordinated

### Phase 1 (Weeks 3-5)
- [ ] Both foundations complete (parallel)
- [ ] CLI metadata commands working
- [ ] Library Quality using YAML format
- [ ] Integration checkpoint passed

### Phase 2 (Weeks 6-9)
- [ ] Library Quality templates complete
- [ ] CLI validation service using LQ validators
- [ ] Integration tests passing
- [ ] Validation integration complete

### Phase 3 (Weeks 10-12)
- [ ] All specs complete
- [ ] Documentation cross-referenced
- [ ] Final integration testing passed
- [ ] Project ready for release

---

## 9. Contacts

**Library Quality Team**:
- Lead: TBD
- Reviewers: TBD

**Enhanced CLI Team**:
- Lead: TBD
- Reviewers: TBD

**Integration Coordinator**:
- Primary: TBD
- Backup: TBD

---

## Appendix: Quick Reference

### Command Mapping

| Workflow | Command | Owner | Uses |
|----------|---------|-------|------|
| Validate metadata | `alloy metadata validate` | CLI | LQ schemas |
| Create metadata | `alloy metadata create` | CLI | LQ templates |
| Generate code | `alloy codegen generate` | CLI | LQ validators |
| Validate code | `alloy codegen validate` | CLI | LQ validators |
| Run tests | `alloy codegen test` | CLI | LQ test infra |
| Preview changes | `alloy codegen generate --diff` | CLI | - |

### File Locations

| Component | Path | Owner |
|-----------|------|-------|
| Metadata database | `tools/codegen/database/` | Shared |
| YAML schemas | `tools/codegen/database/schemas/` | CLI |
| Peripheral templates | `tools/codegen/database/templates/peripheral/` | Library Quality |
| Project templates | `tools/codegen/database/templates/project/` | CLI |
| Core validators | `tools/codegen/validators/` | Library Quality |
| CLI services | `tools/codegen/cli/services/` | CLI |

---

**Document Version**: 1.0
**Last Updated**: 2025-01-17
**Next Review**: Week 5 checkpoint
