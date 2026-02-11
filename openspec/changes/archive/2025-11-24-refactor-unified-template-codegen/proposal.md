## Why

The current code generation system has grown organically with hardcoded Python string concatenation in each generator (`generate_registers.py`, `generate_bitfields.py`, `generate_gpio.py`). Each generator contains 600+ lines of hardcoded C++ string building, making maintenance difficult and consistency impossible.

The GPIO generator already demonstrates that template-based generation (Jinja2 + JSON metadata) produces cleaner code (361 lines vs 562 manual lines), better consistency, and faster development. This same pattern should be used for ALL code generation - registers, bitfields, peripherals, startup code, and linker scripts.

Without this refactoring, adding new MCU families requires duplicating hardcoded logic across multiple generators. With unified template architecture, adding a new family means creating a single JSON file.

## What Changes

**Core Architecture:**
- Unified metadata system (vendor → family → peripheral hierarchy)
- Template-based generation replacing all hardcoded string concatenation
- Single UnifiedGenerator class orchestrating all code generation
- JSON schemas for validation and IDE support
- Gradual migration path preserving existing functionality

**Deliverables:**
- `tools/codegen/templates/` - Jinja2 templates for registers, bitfields, peripherals, startup, linker
- `tools/codegen/cli/generators/metadata/` - Three-tier JSON metadata structure
- `tools/codegen/cli/generators/unified_generator.py` - Central orchestrator
- `tools/codegen/schemas/` - JSON Schema definitions for metadata validation
- `tools/codegen/custom-svd/` - Multi-source SVD repository (migrated from enhance-svd-mcu-generation)
- `tools/codegen/cli/parsers/svd_discovery.py` - SVD discovery with merge policy
- `tools/codegen/cli/parsers/list_svds.py` - CLI tool for listing available SVDs
- `tools/codegen/cli/generators/generate_support_matrix.py` - Auto-generated MCU support matrix
- Migration guides and comprehensive documentation

## Impact

**Affected specs:**
- `codegen-foundation` (MODIFIED) - Add template-based architecture
- `codegen-architecture` (NEW) - Unified template system specification

**Affected code:**
- `tools/codegen/cli/generators/platform/generate_registers.py` - Migrate to templates
- `tools/codegen/cli/generators/platform/generate_bitfields.py` - Migrate to templates
- `tools/codegen/cli/generators/platform/generate_gpio.py` - Already uses templates, enhance
- `tools/codegen/cli/generators/unified_generator.py` - NEW orchestrator
- `tools/codegen/templates/` - NEW template library
- `tools/codegen/cli/generators/metadata/` - NEW metadata hierarchy

**Benefits:**
- 75% reduction in generator code (600+ lines → 150 lines template)
- 90% faster new family support (weeks → hours)
- Perfect consistency across all generated code
- Zero code duplication
- Single source of truth (JSON metadata)
- IDE support via JSON Schema
- Multi-source SVD support (upstream + custom community contributions)
- Auto-generated MCU support matrix with peripheral counts
- C++20 compile-time pin validation with concepts (future integration)
- 783+ SVD files discovered from CMSIS repository

## Dependencies

**Prerequisite changes:**
- `add-codegen-foundation` ✅ Complete (SVD parser, basic generator)
- `add-gpio-interface` ✅ Complete (GPIO abstractions)

**Blocked changes:**
- None - this is an internal refactoring with gradual migration

**External dependencies:**
- Python 3.8+
- Jinja2 3.0+ (already required)
- jsonschema 4.0+ (for metadata validation)
- Existing SVD database

## Risks

**Technical risks:**
| Risk | Mitigation |
|------|------------|
| Breaking existing generators | Parallel systems, side-by-side validation before deprecation |
| Template complexity | Start simple, comprehensive examples, macro library |
| Metadata explosion | Three-tier hierarchy reuses common configs, inheritance |
| Migration bugs | Automated tests comparing old vs new output byte-for-byte |

**Process risks:**
| Risk | Mitigation |
|------|------------|
| Learning curve for templates | Detailed documentation, migration guide, examples |
| Incomplete migration | Phase-by-phase plan with clear milestones |
| Performance regression | Benchmark before/after, optimize if needed |

## Success Criteria

**Phase 1 - Foundation (Week 1-2):**
- [ ] MetadataLoader class validates and loads JSON
- [ ] TemplateEngine wrapper configured
- [ ] JSON schemas for all metadata types
- [ ] Documentation complete

**Phase 2 - Register Migration (Week 3-4):**
- [ ] `register_struct.hpp.j2` template complete
- [ ] `generate_registers.py` migrated to use template
- [ ] SAME70 registers generated match byte-for-byte
- [ ] All register tests passing

**Phase 3 - Bitfield Migration (Week 5-6):**
- [ ] `bitfield_enum.hpp.j2` template complete
- [ ] `generate_bitfields.py` migrated
- [ ] Output validated against manual version
- [ ] All bitfield tests passing

**Phase 4 - Platform Peripherals (Week 7-9):**
- [ ] Templates for UART, SPI, I2C
- [ ] GPIO template enhanced
- [ ] All platform HAL generated
- [ ] Tests passing on all boards

**Phase 5 - Startup & Linker (Week 10-11):**
- [ ] `startup.cpp.j2` template
- [ ] Linker script templates
- [ ] All boards boot successfully
- [ ] Memory layout verified

**Phase 6 - Deprecation (Week 12):**
- [ ] Old generators marked deprecated
- [ ] Migration complete for all families
- [ ] Documentation updated
- [ ] Performance benchmarks validated

**Metrics:**
- Lines of code in generators: 600+ → 150 per generator
- Time to add new MCU family: 2 weeks → 4 hours
- Code duplication: ~2400 lines → 0 lines
- Test coverage: >95% for all generated code
- Build time: No regression (< 5% change)

## Migration from enhance-svd-mcu-generation

This spec absorbs and supersedes the `enhance-svd-mcu-generation` change, integrating its completed features:

**Migrated Features (✅ Complete):**
1. **Custom SVD Repository** - Multi-source SVD discovery system
   - `tools/codegen/custom-svd/` infrastructure
   - Merge policy with priority rules (custom > upstream)
   - 783 SVDs discovered from CMSIS repository

2. **SVD Discovery System** - CLI tools for SVD management
   - `svd_discovery.py` - Multi-source detection with conflict resolution
   - `list_svds.py` - CLI for listing available SVD files by vendor

3. **Support Matrix Generator** - Auto-generated documentation
   - `generate_support_matrix.py` - Scans generated code
   - Markdown table generation with peripheral counts
   - Vendor/family grouping with status badges

**Pending Integration (🚧 Phase 13):**
1. **C++20 Pin Validation** - Compile-time pin checking
   - Add `ValidPin<Pin>` concepts to pin templates
   - Generate constexpr pin lookup tables
   - Compile error for invalid pins on specific MCU variants

2. **MCU Traits System** - Compile-time MCU metadata
   - Generate traits for flash/RAM/package info
   - Peripheral availability flags (UART count, I2C count, etc.)
   - Feature flags (HAS_USB, HAS_CAN, HAS_DAC)

**Rationale for Merge:**
- Both specs target the same codebase (`tools/codegen/`)
- 60-70% feature overlap (template-based generation, metadata system)
- The unified template architecture provides better foundation for pin generation
- Reduces maintenance burden and spec fragmentation
- `enhance-svd` is 81% complete, features can be preserved via migration
