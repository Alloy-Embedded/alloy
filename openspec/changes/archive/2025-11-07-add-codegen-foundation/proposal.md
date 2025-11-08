## Why

To support hundreds of MCUs from different vendors, we need automated code generation from SVD files and databases. Manual implementation doesn't scale - frameworks like modm (3500+ MCUs) and Zephyr (1000+ boards) prove code generation is essential.

Without this infrastructure, adding each MCU requires weeks of manual work writing startup code, register definitions, vector tables, and linker scripts. With code generation, adding a new ARM MCU takes hours (download SVD + parse).

## What Changes

**Core Infrastructure:**
- Automated SVD downloader syncing CMSIS-Pack repository
- SVD parser converting XML → JSON database
- Code generator (Python + Jinja2) producing startup code, registers, linker scripts
- CMake integration making generation automatic and transparent
- Database structure organizing SVDs by vendor/family

**Deliverables:**
- `tools/codegen/sync_svd.py` - Downloads/updates SVDs from ARM CMSIS
- `tools/codegen/svd_parser.py` - Parses SVD XML → JSON
- `tools/codegen/generator.py` - Generates C++ from database
- `tools/codegen/database/` - SVD storage organized by vendor
- `tools/codegen/templates/` - Jinja2 templates for code generation
- `cmake/codegen.cmake` - Transparent CMake integration

## Impact

**Affected specs:**
- `codegen-foundation` (NEW) - SVD sync, parser, generator MVP
- `build-system` (MODIFIED) - CMake integration for code generation

**Affected code:**
- `tools/codegen/` - New directory with all codegen tooling
- `cmake/codegen.cmake` - New CMake module
- `build/generated/` - Generated code output (git-ignored)

**Enables:**
- Quick addition of ARM-based MCUs (STM32, nRF, LPC, SAM, etc.)
- Foundation for `add-codegen-multiarch` (RL78, ESP32 support)
- Scaling to 500+ MCUs by 2026
- Community contributions of new MCU support

## Dependencies

**Prerequisite changes:**
- `add-project-structure` ✅ Complete
- `add-core-error-handling` ✅ Complete
- `add-gpio-interface` ✅ Complete

**Blocked changes:**
- `add-codegen-multiarch` - Needs this foundation first
- `add-bluepill-hal` - Can use generated code from this
- All Phase 1 hardware implementations

**External dependencies:**
- Python 3.8+
- pip packages: `jinja2`, `lxml`, `requests`
- Git (for cloning CMSIS-Pack repo)
- Internet connection (for SVD sync)

## Risks

**Technical risks:**
| Risk | Mitigation |
|------|------------|
| SVD parsing complexity | Start with simple STM32F103 SVD, expand incrementally |
| Generator bugs produce invalid code | Validate generated code compiles before committing |
| CMSIS repo structure changes | Version-lock to known-good commit, document update process |
| Large repo size (SVDs) | Sync only needed vendors initially, use git submodule |

**Process risks:**
| Risk | Mitigation |
|------|------------|
| Over-engineering early | MVP first (1 SVD, 1 template), expand based on real needs |
| Templates hard to maintain | Comprehensive documentation, examples, tests |
| Manual database work for non-SVD MCUs | Accept as reality, document clearly |

## Success Criteria

**Phase 0 (This change):**
- [ ] SVD downloader fetches STM32 SVDs from CMSIS
- [ ] Parser converts STM32F103 SVD → valid JSON
- [ ] Generator produces compiling startup.cpp from JSON
- [ ] CMake integration works transparently
- [ ] Documentation complete

**Phase 1 (Future):**
- [ ] 10+ STM32 MCUs supported
- [ ] Blinky running on Blue Pill with generated code
- [ ] All templates implemented (vectors, registers, linker)

**Metrics:**
- Time to add new ARM MCU: < 2 hours
- Generated code size overhead: < 5% vs manual
- Build time impact: < 10% increase
- Lines of template code: < 500 (excluding vendor SVDs)
