# Tasks: Memory Analysis and Optimization Tools

## 1. CMake Infrastructure (6 tasks)

- [ ] 1.1 Create `cmake/memory_analysis.cmake` with `alloy_add_memory_analysis()` function
- [ ] 1.2 Add `ALLOY_MINIMAL_BUILD` option to root CMakeLists.txt
- [ ] 1.3 Implement compilation flags for minimal build mode (-Os, -flto, -ffunction-sections)
- [ ] 1.4 Implement linker flags for minimal build mode (--gc-sections, --print-memory-usage)
- [ ] 1.5 Add board-specific memory size variables (ALLOY_RAM_SIZE, ALLOY_FLASH_SIZE)
- [ ] 1.6 Create `memory-report` custom target that invokes size/objdump/analyzer

## 2. Python Memory Analyzer (8 tasks)

- [ ] 2.1 Create `tools/analyze_memory.py` skeleton with argparse setup
- [ ] 2.2 Implement `LinkerMapParser` class with section parsing
- [ ] 2.3 Implement symbol parsing from linker map files
- [ ] 2.4 Add support for ARM (arm-none-eabi-ld) map format
- [ ] 2.5 Add support for RL78 (rl78-elf-ld) map format
- [ ] 2.6 Add support for Xtensa (xtensa-esp32-elf-ld) map format
- [ ] 2.7 Implement `MemoryReport` class with text report generation
- [ ] 2.8 Add JSON output option for machine-readable results

## 3. Static Assertion Infrastructure (4 tasks)

- [ ] 3.1 Create `src/core/memory.hpp` header
- [ ] 3.2 Implement `ALLOY_ASSERT_MAX_SIZE` macro
- [ ] 3.3 Implement `ALLOY_ASSERT_ZERO_OVERHEAD` macro
- [ ] 3.4 Implement `ALLOY_ASSERT_ALIGNMENT` macro

## 4. Testing (7 tasks)

- [ ] 4.1 Create `tools/test/fixtures/` directory
- [ ] 4.2 Add example ARM linker map file to fixtures
- [ ] 4.3 Add example RL78 linker map file to fixtures
- [ ] 4.4 Create `tools/test/test_analyze_memory.py` with pytest tests
- [ ] 4.5 Test ARM map parsing
- [ ] 4.6 Test RL78 map parsing
- [ ] 4.7 Test report generation with memory budgets

## 5. CI Integration (5 tasks)

- [ ] 5.1 Create `.github/workflows/memory-check.yml` workflow
- [ ] 5.2 Configure workflow to build example for each target (host, ARM, RL78)
- [ ] 5.3 Run memory-report target and capture output as artifact
- [ ] 5.4 Add step to compare memory usage vs. baseline (previous commit)
- [ ] 5.5 Configure bot to comment memory diff on PRs

## 6. Documentation (4 tasks)

- [ ] 6.1 Create memory budget documentation template (add to CONTRIBUTING.md)
- [ ] 6.2 Document ALLOY_MINIMAL_BUILD option in BUILD.md
- [ ] 6.3 Document memory-report target usage in BUILD.md
- [ ] 6.4 Add examples of static assertions to coding guidelines

## 7. Validation (6 tasks)

- [ ] 7.1 Build host example with memory-report, verify output format
- [ ] 7.2 Build ARM example with memory-report, verify ARM map parsing
- [ ] 7.3 Build with ALLOY_MINIMAL_BUILD=ON, verify size reduction ≥15%
- [ ] 7.4 Add static assertions to dummy test module, verify compile-time errors
- [ ] 7.5 Run CI workflow, verify memory tracking works
- [ ] 7.6 Validate memory budget compliance for Phase 0 modules

---

**Total Tasks**: 40
**Estimated Effort**: 2-3 days
**Dependencies**: `add-project-structure` must be complete
