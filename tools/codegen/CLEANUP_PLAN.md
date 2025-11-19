# CLI Cleanup Plan - Keeping Both CLIs Separate

**Date**: 2025-11-19
**Strategy**: Remove only duplicates/obsolete files, keep both CLIs functional

## Files to Remove

### 1. Old Tests (tests/_old/)
- [x] tests/_old/test_generator.py
- [x] tests/_old/test_svd_parser.py
**Reason**: Old test files, replaced by new test suite

### 2. Archived Code (archive/)
- [x] archive/old_platform_templates/*.j2 (10 files)
- [x] archive/old_generators/generate_startup.py
**Reason**: Archived/deprecated code, no longer used

### 3. Duplicate Validators (cli/core/validators/)
- [x] cli/core/validators/ (entire directory)
**Reason**: Duplicates of cli/validators/ (new implementation is better)

### 4. Python Cache Files
- [x] **pycache**/ directories
- [x] .pytest_cache/
- [x] *.pyc files
**Reason**: Build artifacts, should be gitignored

## Files to KEEP (Critical)

### Code Generation (Old CLI)
- ✅ codegen.py - Main entry point for code generation
- ✅ cli/vendors/ - Vendor-specific generators (ST, Atmel, etc.)
- ✅ cli/generators/ - Code generation logic
- ✅ cli/parsers/ - SVD parsers
- ✅ cli/core/svd_parser.py - Core SVD parsing
- ✅ cli/core/template_engine.py - Jinja2 templates (old)
- ✅ cli/commands/codegen.py - Codegen command wrapper
- ✅ cli/commands/status.py - Status command
- ✅ cli/commands/vendors.py - Vendors command
- ✅ cli/commands/clean.py - Clean command

### Project Development (New CLI)
- ✅ cli/main.py - New CLI entry point
- ✅ cli/commands/ - New CLI commands (list, show, search, init, build, docs, etc.)
- ✅ cli/services/ - New services (MCU, Board, Build, Flash, etc.)
- ✅ cli/models/ - Pydantic models
- ✅ cli/validators/ - New validators (better than old ones)
- ✅ cli/generators/template_engine.py - New template engine
- ✅ cli/generators/project_generator.py - Project generator
- ✅ tests/unit/test_phase*.py - New test suite

## Documentation Updates

### 1. Create CLI_USAGE.md
Explain the two CLIs and their purposes:
- Old CLI (codegen.py) - Code generation from SVD
- New CLI (alloy) - Project development

### 2. Update .gitignore
Add Python cache patterns if not already there

## Cleanup Commands

```bash
# Remove old tests
rm -rf tests/_old/

# Remove archived code
rm -rf archive/

# Remove duplicate validators
rm -rf cli/core/validators/

# Remove Python cache
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null
find . -type d -name ".pytest_cache" -exec rm -rf {} + 2>/dev/null
find . -type f -name "*.pyc" -delete

# Remove empty __init__.py in old locations (if any)
```

## Post-Cleanup Verification

1. ✅ Old CLI still works: `python3 codegen.py --help`
2. ✅ New CLI still works: `alloy --help`
3. ✅ Code generation works: `python3 codegen.py generate --startup`
4. ✅ Project init works: `alloy init --help`
5. ✅ Tests pass: `pytest tests/unit/test_phase*.py`

## Success Criteria

- [x] Both CLIs remain functional
- [x] No code generation capability lost
- [x] No project development capability lost
- [x] Reduced repository size (remove cache/obsolete files)
- [x] Clear documentation on CLI separation
