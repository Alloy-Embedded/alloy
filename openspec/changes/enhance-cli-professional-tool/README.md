# Enhanced CLI - Professional Development Tool

**Status**: 🟡 PROPOSED (Consolidated 2025-01-17)
**Priority**: HIGH
**Duration**: 11.5 weeks (~230 hours)
**Complexity**: HIGH

---

## Quick Summary

Transform Alloy's CLI into a **professional embedded development assistant** with comprehensive improvements:

### Original Vision (8 weeks)
- ✅ MCU/Board discovery system
- ✅ Interactive project initialization wizard
- ✅ Automated code validation pipeline
- ✅ Build/flash/debug integration
- ✅ Pinout visualization

### 🆕 Consolidated Improvements (+3.5 weeks)
- 🔥 **YAML metadata format** (25-30% smaller, inline comments)
- 👁️ **Preview/diff capability** (see changes before applying)
- ⚡ **Incremental generation** (10x faster iteration)
- ⚙️ **Configuration system** (`.alloy.yaml`)
- 🔍 **Enhanced metadata management** (validate, create, diff)

---

## Key Metrics

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Time to create project | 30 min | 2 min | **-93%** |
| Metadata file size | 8,500 lines | 6,400 lines | **-25%** |
| Syntax errors | 30% | <5% | **-83%** |
| Iteration speed | 5s | 0.5s | **10x faster** |
| Developer Experience | 5.0/10 | 8.6/10 | **+72%** |

---

## Timeline

| Phase | Duration | Key Deliverables |
|-------|----------|------------------|
| **Phase 0: YAML Migration** 🆕 | 1 week | YAML metadata, migration scripts |
| **Phase 1: Discovery** | 3 weeks 🆕 | MCU/board discovery + config system |
| **Phase 2: Validation** | 2 weeks | Automated validation pipeline |
| **Phase 3: Initialization** | 2 weeks | Interactive project wizard |
| **Phase 4: Build Integration** | 1 week | Unified build/flash/debug |
| **Phase 5: Documentation** | 1 week | Pinouts, datasheets, examples |
| **Phase 6: Polish** 🆕 | 1.5 weeks | Preview/diff, incremental gen, UX |
| **Total** | **11.5 weeks** | Professional-grade CLI |

---

## Files in This Proposal

- **`proposal.md`** - Complete proposal (1,937 lines, YAML examples)
- **`tasks.md`** - Implementation tasks with Phase 0
- **`CONSOLIDATED_IMPROVEMENTS.md`** - Integration guide for new features
- **`CLI_IMPROVEMENT_ANALYSIS.md`** - Original analysis (1,750 lines)
- **`README.md`** - This file (executive summary)

---

## Example: YAML vs JSON

### Before (JSON - 90+ lines, no comments)
```json
{
  "schema_version": "1.0",
  "family": {
    "id": "stm32f4",
    "vendor": "st",
    "display_name": "STM32F4 Series",
    "description": "High-performance ARM Cortex-M4 MCUs",
    "core": "Cortex-M4F",
    "features": ["FPU", "DSP Instructions"]
  },
  "mcus": [
    {
      "part_number": "STM32F401RET6",
      ...
    }
  ]
}
```

### After (YAML - 67 lines, with comments)
```yaml
schema_version: 1.0

family:
  id: stm32f4
  vendor: st
  display_name: STM32F4 Series

  # High-performance ARM Cortex-M4 MCUs with FPU and DSP
  # Target: Motor control, digital audio, sensor fusion
  description: |
    The STM32F4 series offers a balance of performance
    and power efficiency, featuring a Cortex-M4F core.

  core: Cortex-M4F
  features:
    - FPU              # Hardware floating-point unit
    - DSP Instructions # Digital signal processing

mcus:
  - part_number: STM32F401RET6
    peripherals:
      uart:
        count: 3
        # USART1/6: 5.25 Mbps max, USART2: 2.625 Mbps
        instances: [USART1, USART2, USART6]
    ...
```

**Benefits**: 25% smaller, inline docs, cleaner code snippets, no trailing commas!

---

## Example: New Commands

### Discovery Commands (Original)
```bash
# List STM32 MCUs with >= 512KB flash
alloy list mcus --vendor st --min-flash 512K

# Show detailed specs
alloy show mcu STM32F401RE
# Displays: cores, memory, peripherals, datasheet link

# Search by features
alloy search mcu "USB + Cortex-M4"
# Finds 23 matching MCUs
```

### 🆕 Metadata Management (New)
```bash
# Validate all metadata
alloy metadata validate

# Create new UART from template
alloy metadata create --template uart --name stm32l4_uart

# Preview what would change
alloy metadata diff same70.gpio
```

### 🆕 Preview/Diff (New)
```bash
# See changes before applying
alloy codegen generate --dry-run --diff

# Output:
# ✎ Modified: src/hal/vendors/arm/same70/gpio.hpp (+15, -3)
# + Created:  src/hal/vendors/arm/same70/i2c.hpp
#
# --- gpio.hpp (current)
# +++ gpio.hpp (generated)
# @@ -120,7 +120,10 @@
#      Result<void, ErrorCode> set() {
#          auto* port = get_port();
# -        port->SODR = pin_mask;
# +        port->SODR = pin_mask;  // Set Output Data Register
# +#ifdef ALLOY_GPIO_TEST_HOOK_SODR
# +        ALLOY_GPIO_TEST_HOOK_SODR();
# +#endif
#
# Apply changes? [y/N]
```

### 🆕 Incremental Generation (New)
```bash
# Only regenerate changed files (10x faster!)
alloy codegen generate --incremental

# Skipping same70.uart (unchanged)
# Skipping stm32f4.gpio (unchanged)
# ✓ Generated same70.gpio (metadata changed)
#
# Completed in 0.5s (vs 5s for full generation)
```

### 🆕 Configuration (New)
```yaml
# .alloy.yaml (project root)
version: 1.0

codegen:
  auto_format: true
  validate: true
  incremental: true  # Fast iteration!

defaults:
  board: nucleo_f401re
  template: blinky

families:
  same70: true
  stm32f4: true
  stm32g0: false  # Disabled temporarily
```

---

## What's New (2025-01-17 Consolidation)

### Added Features

1. **YAML Metadata Format** (Phase 0)
   - 25-30% smaller files
   - Inline comments for hardware quirks
   - Clean multiline code snippets
   - Better git diffs

2. **Preview/Diff** (Phase 6)
   - `--dry-run --diff` flag
   - See exact changes before applying
   - Colorized unified diff
   - Prevents destructive changes

3. **Incremental Generation** (Phase 6)
   - Checksum-based change detection
   - Skip unchanged metadata
   - 10x faster iteration (5s → 0.5s)

4. **Configuration System** (Phase 1)
   - `.alloy.yaml` for settings
   - Hierarchy: CLI > env > project > user > defaults
   - Customizable paths, formatting, defaults

5. **Enhanced Metadata Commands** (Phase 1)
   - `metadata validate [--strict]`
   - `metadata create --template <type>`
   - `metadata diff <name>`

### Updated Timeline
- **Original**: 8 weeks (176 hours)
- **New**: 11.5 weeks (230 hours)
- **Added**: Phase 0 (YAML) + enhancements to Phase 1, 6

### Updated Goals
- **G6**: YAML Migration (MUST HAVE)
- **G7**: Preview & Incremental (MUST HAVE)
- **G8**: Configuration (SHOULD HAVE)

---

## Why These Changes?

### Problem Identified

Original proposal used JSON for all metadata. However:

- ❌ **604-line GPIO file** (verbose)
- ❌ **No comments** → hardware quirks undocumented
- ❌ **Code snippets** require `\n` escaping
- ❌ **30% of edits** cause trailing comma errors
- ❌ **Difficult merges** in git

### Solution: YAML + Usability Improvements

- ✅ **25-30% smaller** files
- ✅ **Inline comments** preserve tribal knowledge
- ✅ **Clean code** (multiline strings, no escaping)
- ✅ **<5% error rate** (vs 30% with JSON)
- ✅ **Better git diffs** and merge resolution

Plus preview/diff and incremental generation for **10x faster iteration**!

---

## Dependencies

### New
- `PyYAML >= 6.0` - YAML parsing

### Existing
- `typer >= 0.9.0` - Modern CLI framework
- `rich >= 13.0.0` - Terminal formatting
- `jsonschema >= 4.0.0` - Schema validation
- `cmsis-svd >= 0.4.0` - SVD parsing

All are standard Python packages for embedded development.

---

## Success Criteria

- [ ] YAML metadata 25-30% smaller than JSON
- [ ] Metadata syntax error rate <5%
- [ ] Preview shows exact changes (100% accuracy)
- [ ] Incremental generation 5-10x faster
- [ ] Config system working with hierarchy
- [ ] All tests passing (>90% coverage)
- [ ] User can create project in <2 minutes
- [ ] Developer Experience Score >= 8.0/10

---

## Approval Checklist

- [ ] Architecture review completed
- [ ] YAML format agreed upon
- [ ] Timeline acceptable (11.5 weeks)
- [ ] Testing strategy sufficient
- [ ] Documentation plan adequate
- [ ] Risks identified and mitigated
- [ ] ROI justified (+72% DX improvement)

---

## Next Steps

1. **Review** consolidated proposal and improvements
2. **Approve** or request changes
3. **Begin Phase 0** (YAML migration)
4. **Weekly check-ins** for progress tracking
5. **User testing** after each phase
6. **Release** after all validation passes

---

## Related Documents

- **Analysis**: `CLI_IMPROVEMENT_ANALYSIS.md` (comprehensive analysis)
- **Integration**: `CONSOLIDATED_IMPROVEMENTS.md` (detailed integration guide)
- **Current CLI**: `tools/codegen/codegen.py` (800 lines)
- **Metadata**: `tools/codegen/cli/generators/metadata/` (45 files to migrate)

---

## Questions?

For details on specific aspects:

1. **Why YAML?** → See `CLI_IMPROVEMENT_ANALYSIS.md` (JSON vs YAML comparison)
2. **How to integrate?** → See `CONSOLIDATED_IMPROVEMENTS.md` (step-by-step guide)
3. **Full proposal** → See `proposal.md` (1,937 lines, complete spec)
4. **Implementation** → See `tasks.md` (120+ tasks with Phase 0)

---

**Created**: 2025-01-17
**Status**: Awaiting Approval
**Consolidated By**: Development Team
