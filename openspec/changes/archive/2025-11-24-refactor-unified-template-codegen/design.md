# Unified Template-Based Code Generation Architecture

## Overview

This document describes the architectural design for refactoring Alloy's code generation system from hardcoded Python string concatenation to a unified template-based architecture using Jinja2 templates and hierarchical JSON metadata.

## Problem Statement

Current state:
- Each generator (`generate_registers.py`, `generate_bitfields.py`, etc.) contains 600+ lines of hardcoded C++ string building
- Code duplication across generators (~2400 lines total)
- Inconsistent output formats
- Difficult to maintain and extend
- Adding new MCU family requires modifying multiple Python files

## Solution Architecture

### Three-Tier Metadata Hierarchy

```
Vendor Level (vendors/atmel.json)
    ↓ inherits common settings
Family Level (families/same70.json)
    ↓ inherits + overrides
Peripheral Level (peripherals/gpio.json) [optional]
```

**Vendor metadata** defines:
- Common architectural settings (endianness, pointer size)
- Naming conventions across all families
- SVD quirks database
- Code style preferences
- Family catalog

**Family metadata** defines:
- Register generation rules
- Bitfield generation config
- SVD bug fixes for this family
- Per-peripheral configurations
- Memory layout
- Startup configuration

**Peripheral metadata** (optional) defines:
- Peripheral-specific generation rules
- Operation mappings
- Pin configurations

### Template Structure

```
tools/codegen/templates/
├── common/
│   ├── header.j2              # File header boilerplate
│   ├── macros.j2              # Reusable Jinja2 macros
│   └── filters.py             # Custom Jinja2 filters
├── registers/
│   ├── register_struct.hpp.j2 # Register structure template
│   └── namespace.hpp.j2       # Namespace wrapper template
├── bitfields/
│   ├── bitfield_enum.hpp.j2   # Enum class template
│   └── helpers.hpp.j2         # Bitfield helper functions
├── platform/
│   ├── gpio.hpp.j2            # GPIO HAL template
│   ├── uart.hpp.j2            # UART HAL template
│   ├── spi.hpp.j2             # SPI HAL template
│   └── i2c.hpp.j2             # I2C HAL template
├── startup/
│   └── cortex_m_startup.cpp.j2 # Startup code template
└── linker/
    └── cortex_m.ld.j2         # Linker script template
```

### UnifiedGenerator Class

Central orchestrator that:
1. Loads and validates metadata (vendor → family → peripheral hierarchy)
2. Resolves configuration (inherits + overrides)
3. Renders templates with merged metadata
4. Writes output files atomically
5. Validates generated code

```python
class UnifiedGenerator:
    def __init__(self, metadata_dir: Path, template_dir: Path):
        self.metadata_loader = MetadataLoader(metadata_dir)
        self.template_engine = TemplateEngine(template_dir)
    
    def generate(self, family: str, mcu: str, target: str) -> None:
        # Load and merge metadata hierarchy
        config = self.metadata_loader.resolve_config(family, mcu)
        
        # Render template
        output = self.template_engine.render(target, config)
        
        # Write atomically
        self.write_output(output, config.output_path)
```

### Custom Jinja2 Filters

```python
# Custom filters for code generation
def sanitize(name: str) -> str:
    """Sanitize identifier for C++"""
    
def format_hex(value: int, width: int = 8) -> str:
    """Format as 0x-prefixed hex"""
    
def cpp_type(svd_type: str) -> str:
    """Map SVD type to C++ type"""
```

### Metadata Validation

All metadata validated against JSON Schema:
- `schemas/vendor.schema.json`
- `schemas/family.schema.json`
- `schemas/peripheral.schema.json`

Validation happens at load time, catching errors before generation.

## Migration Strategy

### Phase 1: Foundation (Week 1-2)
- Create metadata loader with schema validation
- Create template engine wrapper
- Write comprehensive tests
- Document metadata format

### Phase 2: Registers (Week 3-4)
- Create register templates
- Migrate `generate_registers.py`
- Validate output byte-for-byte
- Run all tests

### Phase 3: Bitfields (Week 5-6)
- Create bitfield templates
- Migrate `generate_bitfields.py`
- Validate output
- Run all tests

### Phase 4: Platform Peripherals (Week 7-9)
- Create peripheral templates (GPIO, UART, SPI, I2C)
- Migrate all platform generators
- Test on all boards
- Validate functionality

### Phase 5: Startup & Linker (Week 10-11)
- Create startup/linker templates
- Migrate generators
- Boot test on all boards
- Validate memory layout

### Phase 6: Cleanup (Week 12)
- Remove legacy code
- Update documentation
- Final validation
- Performance benchmarks

### Parallel Systems During Migration

Run old and new generators side-by-side:
```bash
# Old generator (legacy)
python generate_registers_legacy.py --mcu SAME70

# New generator
python generate_registers.py --mcu SAME70

# Compare outputs
diff -r build/generated/old/ build/generated/new/
```

Only deprecate old generator after:
1. Byte-for-byte output match
2. All tests passing
3. Performance validated
4. Documentation updated

## Benefits Quantification

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Generator LOC | 600+ per generator | 150 per generator | 75% reduction |
| Time to add MCU family | 2 weeks | 4 hours | 90% faster |
| Code duplication | ~2400 lines | 0 lines | 100% elimination |
| Consistency issues | Frequent | Zero | Perfect consistency |
| Template reusability | 0% | 100% | Infinite reuse |

## Testing Strategy

### Unit Tests
- Metadata loading and validation
- Template rendering
- Custom filters
- Error handling

### Integration Tests
- End-to-end generation
- Multiple families (SAME70, STM32F1, STM32F4)
- All peripheral types
- Startup and linker scripts

### Regression Tests
- Compare old vs new output byte-for-byte
- Ensure no functional changes
- Validate all existing tests still pass

### Validation Tests
- Generated code compiles
- Generated code runs on hardware
- Memory layout correct
- Boot sequence successful

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Breaking existing code | Parallel systems, gradual migration, automated comparison |
| Template bugs | Comprehensive tests, examples, validation mode |
| Metadata errors | JSON Schema validation, clear error messages |
| Performance regression | Benchmarks before/after, optimization if needed |
| Learning curve | Detailed docs, migration guide, examples |

## Success Criteria

- [ ] All generators migrated to templates
- [ ] Zero hardcoded C++ string concatenation
- [ ] >95% test coverage
- [ ] All boards boot successfully
- [ ] Build time impact < 5%
- [ ] 75% code reduction achieved
- [ ] Documentation complete
- [ ] Legacy code removed
