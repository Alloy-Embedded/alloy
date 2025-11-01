# Tasks: Add Project Template Repository

## Phase 1: Repository Setup (Day 1)

### Task 1.1: Create Template Repository
**Estimated**: 1 hour
**Dependencies**: None
**Deliverable**: GitHub repository

Create template repo structure:
- [ ] Initialize Git repository
- [ ] Add MIT LICENSE
- [ ] Create .gitignore (build/, .vscode/ipch/, etc.)
- [ ] Create README.md skeleton
- [ ] Push to GitHub
- [ ] Enable "Template repository" in settings

**Files**:
- New: Repository on GitHub
- New: LICENSE, .gitignore, README.md

---

### Task 1.2: Add Alloy as Submodule
**Estimated**: 30 minutes
**Dependencies**: Task 1.1
**Deliverable**: Alloy submodule configured

- [ ] Add submodule: `git submodule add <alloy-url> external/alloy`
- [ ] Pin to stable version (main or v1.0.0)
- [ ] Create .gitmodules configuration
- [ ] Test clone with `--recursive`
- [ ] Commit submodule

**Validation**:
```bash
git clone --recursive <template-url>
test -f external/alloy/CMakeLists.txt
```

**Files**:
- New: .gitmodules
- New: external/alloy/ (submodule)

---

### Task 1.3: Create Directory Structure
**Estimated**: 1 hour
**Dependencies**: Task 1.2
**Deliverable**: All directories created

Create standard structure:
- [ ] `common/` (shared library)
- [ ] `bootloader/` (target 1)
- [ ] `application/` (target 2)
- [ ] `boards/` (custom boards)
- [ ] `tests/` (unit tests)
- [ ] `scripts/` (helper scripts)
- [ ] `docs/` (documentation)
- [ ] `.vscode/` (VSCode config)

**Files**:
- New: Directory structure

---

## Phase 2: CMake Build System (Day 2)

### Task 2.1: Root CMakeLists.txt
**Estimated**: 2 hours
**Dependencies**: Task 1.3
**Deliverable**: `CMakeLists.txt`

Create root CMake file:
- [ ] Project definition
- [ ] Include Alloy subdirectory
- [ ] Board and target selection variables
- [ ] Add custom boards to search path
- [ ] Include common library
- [ ] Include targets (bootloader, application)
- [ ] Testing support
- [ ] Configuration summary message

**Validation**:
```bash
cmake -B build -DBOARD=stm32f407vg
# Should configure without errors
```

**Files**:
- New: CMakeLists.txt

---

### Task 2.2: Common Library CMake
**Estimated**: 1 hour
**Dependencies**: Task 2.1
**Deliverable**: `common/CMakeLists.txt`

- [ ] Create STATIC library target
- [ ] Add placeholder source files
- [ ] Link against Alloy HAL
- [ ] Set include directories
- [ ] Compiler warnings

**Files**:
- New: common/CMakeLists.txt
- New: common/src/utils.cpp
- New: common/include/utils.h

---

### Task 2.3: Application CMakeLists
**Estimated**: 1.5 hours
**Dependencies**: Task 2.2
**Deliverable**: `application/CMakeLists.txt`

- [ ] Create executable target
- [ ] Link common + Alloy RTOS
- [ ] Apply board settings
- [ ] Generate outputs (bin, hex, map)
- [ ] Print size

**Files**:
- New: application/CMakeLists.txt
- New: application/main.cpp (RTOS example)

---

### Task 2.4: Bootloader CMakeLists
**Estimated**: 1 hour
**Dependencies**: Task 2.2
**Deliverable**: `bootloader/CMakeLists.txt`

- [ ] Create bootloader executable
- [ ] Custom linker script (offset 0x08000000)
- [ ] Link common + Alloy HAL (no RTOS)
- [ ] Generate outputs

**Files**:
- New: bootloader/CMakeLists.txt
- New: bootloader/main.cpp

---

### Task 2.5: Custom Board Discovery
**Estimated**: 2 hours
**Dependencies**: Task 2.1
**Deliverable**: Updated Alloy CMake or template CMake

Implement board search logic:
- [ ] Check `boards/` directory first
- [ ] Fall back to Alloy boards/
- [ ] Error if not found
- [ ] Print which board is used

**Validation**:
```bash
# Create custom board
mkdir -p boards/test_board
echo "..." > boards/test_board/board.hpp

# Should use custom board
cmake -B build -DBOARD=test_board | grep "Using custom board"
```

**Files**:
- Modified: CMakeLists.txt (or Alloy's CMake)

---

## Phase 3: Custom Boards (Day 3)

### Task 3.1: Example Custom Board (Inherit)
**Estimated**: 2 hours
**Dependencies**: Phase 2
**Deliverable**: `boards/custom_stm32f407/`

Create example custom board:
- [ ] Inherit from stm32f407vg
- [ ] Override board name
- [ ] Define custom pins
- [ ] Extend initialize()
- [ ] Add README

**Files**:
- New: boards/custom_stm32f407/board.hpp
- New: boards/custom_stm32f407/README.md

---

### Task 3.2: Example Custom Board (From Scratch)
**Estimated**: 3 hours
**Dependencies**: Task 3.1
**Deliverable**: `boards/completely_custom/`

Create from-scratch example:
- [ ] Define all pins
- [ ] Full initialize() implementation
- [ ] Custom linker script
- [ ] Documentation

**Files**:
- New: boards/completely_custom/board.hpp
- New: boards/completely_custom/linker.ld
- New: boards/completely_custom/README.md

---

### Task 3.3: Custom Board Documentation
**Estimated**: 2 hours
**Dependencies**: Tasks 3.1, 3.2
**Deliverable**: `docs/CUSTOM_BOARDS.md`

Write comprehensive guide:
- [ ] When to inherit vs from-scratch
- [ ] Pin mapping guide
- [ ] Linker script customization
- [ ] Testing custom boards
- [ ] Common pitfalls

**Files**:
- New: docs/CUSTOM_BOARDS.md

---

## Phase 4: Helper Scripts (Day 4)

### Task 4.1: Setup Script
**Estimated**: 1 hour
**Dependencies**: Task 1.2
**Deliverable**: `scripts/setup-submodule.sh`

- [ ] Check if submodule initialized
- [ ] Run `git submodule update --init --recursive`
- [ ] Print Alloy version
- [ ] Success message

**Files**:
- New: scripts/setup-submodule.sh

---

### Task 4.2: Build Script
**Estimated**: 2 hours
**Dependencies**: Phase 2
**Deliverable**: `scripts/build.sh`

- [ ] Parse arguments (target, board, build_type)
- [ ] Create build directory
- [ ] Run CMake configure
- [ ] Run CMake build
- [ ] Print success with binary path

**Validation**:
```bash
./scripts/build.sh application stm32f407vg
test -f build/application/application.elf
```

**Files**:
- New: scripts/build.sh

---

### Task 4.3: Flash Script
**Estimated**: 2 hours
**Dependencies**: Task 4.2
**Deliverable**: `scripts/flash.sh`

Support multiple platforms:
- [ ] OpenOCD for STM32
- [ ] esptool for ESP32
- [ ] picotool for RP2040
- [ ] Error handling

**Files**:
- New: scripts/flash.sh

---

### Task 4.4: Clean Script
**Estimated**: 30 minutes
**Dependencies**: None
**Deliverable**: `scripts/clean.sh`

- [ ] Remove build/ directory
- [ ] Remove .vscode/ipch/
- [ ] Confirmation prompt

**Files**:
- New: scripts/clean.sh

---

## Phase 5: VSCode Integration (Day 5)

### Task 5.1: tasks.json
**Estimated**: 1.5 hours
**Dependencies**: Phase 4
**Deliverable**: `.vscode/tasks.json`

Create tasks:
- [ ] Build Application
- [ ] Build Bootloader
- [ ] Flash Application
- [ ] Flash Bootloader
- [ ] Clean Build
- [ ] Build All

**Files**:
- New: .vscode/tasks.json

---

### Task 5.2: launch.json
**Estimated**: 2 hours
**Dependencies**: Phase 4
**Deliverable**: `.vscode/launch.json`

Create debug configs:
- [ ] OpenOCD (STM32)
- [ ] PyOCD (alternative)
- [ ] GDB (ESP32)
- [ ] Picoprobe (RP2040)
- [ ] Pre-launch task (build)

**Files**:
- New: .vscode/launch.json

---

### Task 5.3: settings.json
**Estimated**: 1 hour
**Dependencies**: None
**Deliverable**: `.vscode/settings.json`

- [ ] IntelliSense include paths
- [ ] C++ standard (C++20)
- [ ] CMake configuration
- [ ] File associations

**Files**:
- New: .vscode/settings.json

---

### Task 5.4: extensions.json
**Estimated**: 30 minutes
**Dependencies**: None
**Deliverable**: `.vscode/extensions.json`

Recommend extensions:
- [ ] C/C++ (Microsoft)
- [ ] CMake Tools
- [ ] Cortex-Debug
- [ ] GitLens

**Files**:
- New: .vscode/extensions.json

---

## Phase 6: RTOS Example (Day 6)

### Task 6.1: Multi-Task Application
**Estimated**: 3 hours
**Dependencies**: Phase 2
**Deliverable**: `application/` with RTOS example

Implement RTOS demo:
- [ ] Sensor task (high priority)
- [ ] Display task (normal priority)
- [ ] Communication task (normal priority)
- [ ] Idle task (low priority)
- [ ] Message queue between tasks
- [ ] Mutex for shared SPI
- [ ] Event flags for notifications

**Files**:
- Modified: application/main.cpp
- New: application/tasks/sensor_task.cpp
- New: application/tasks/display_task.cpp
- New: application/tasks/comm_task.cpp

---

### Task 6.2: Task Documentation
**Estimated**: 1 hour
**Dependencies**: Task 6.1
**Deliverable**: `application/README.md`

Document example:
- [ ] Task descriptions
- [ ] Priority rationale
- [ ] IPC usage
- [ ] How to customize

**Files**:
- New: application/README.md

---

## Phase 7: Documentation (Day 7)

### Task 7.1: Main README
**Estimated**: 2 hours
**Dependencies**: All implementation
**Deliverable**: README.md

Write comprehensive README:
- [ ] Project description
- [ ] Quick start (clone, build, flash)
- [ ] Prerequisites
- [ ] Project structure overview
- [ ] How to add targets
- [ ] How to add custom boards
- [ ] Links to detailed docs

**Files**:
- Modified: README.md

---

### Task 7.2: STRUCTURE.md
**Estimated**: 1.5 hours
**Dependencies**: None
**Deliverable**: `docs/STRUCTURE.md`

Explain architecture:
- [ ] Directory layout
- [ ] Build system
- [ ] Target separation
- [ ] Shared library usage
- [ ] When to modify what

**Files**:
- New: docs/STRUCTURE.md

---

### Task 7.3: BUILD.md
**Estimated**: 1.5 hours
**Dependencies**: Phase 4
**Deliverable**: `docs/BUILD.md`

Build system guide:
- [ ] CMake options
- [ ] Toolchain setup
- [ ] Cross-compilation
- [ ] Build types (Debug/Release)
- [ ] Troubleshooting

**Files**:
- New: docs/BUILD.md

---

### Task 7.4: DEPLOYMENT.md
**Estimated**: 1 hour
**Dependencies**: None
**Deliverable**: `docs/DEPLOYMENT.md`

Production deployment:
- [ ] Release builds
- [ ] Bootloader + app flashing
- [ ] Version management
- [ ] Field updates

**Files**:
- New: docs/DEPLOYMENT.md

---

## Phase 8: Testing & Validation (Day 8)

### Task 8.1: Test All Platforms
**Estimated**: 4 hours
**Dependencies**: All implementation
**Deliverable**: Validation report

Test template on all platforms:
- [ ] STM32F1 (Blue Pill)
- [ ] STM32F4 (Discovery)
- [ ] ESP32 (DevKit)
- [ ] RP2040 (Pico)
- [ ] SAMD21 (Arduino Zero)

For each platform:
- [ ] Clone template
- [ ] Build bootloader
- [ ] Build application
- [ ] Flash and run
- [ ] Verify RTOS example works

**Files**:
- New: docs/VALIDATION.md

---

### Task 8.2: Test Custom Board Workflow
**Estimated**: 2 hours
**Dependencies**: Phase 3
**Deliverable**: Custom board test results

- [ ] Create new custom board from template
- [ ] Build with custom board
- [ ] Verify CMake finds it
- [ ] Document any issues

---

### Task 8.3: VSCode Workflow Test
**Estimated**: 1 hour
**Dependencies**: Phase 5
**Deliverable**: VSCode test results

- [ ] Open project in VSCode
- [ ] Run build task
- [ ] Run flash task
- [ ] Start debug session
- [ ] Verify IntelliSense works

---

## Summary

**Total Estimated Time**: ~42 hours (7-8 working days, ~1.5 weeks)

**Phases**:
1. Repository Setup (Day 1): 2.5 hours
2. CMake Build System (Day 2): 7.5 hours
3. Custom Boards (Day 3): 7 hours
4. Helper Scripts (Day 4): 5.5 hours
5. VSCode Integration (Day 5): 5 hours
6. RTOS Example (Day 6): 4 hours
7. Documentation (Day 7): 6 hours
8. Testing & Validation (Day 8): 7 hours

**Critical Path**:
1. Repository Setup → CMake → Scripts → Testing

**Parallelization Opportunities**:
- VSCode integration (Phase 5) can be done in parallel with Scripts (Phase 4)
- Documentation (Phase 7) can start once structure is finalized
- RTOS example (Phase 6) can be done alongside CMake work

**Deliverables Checklist**:
- [ ] GitHub template repository
- [ ] Alloy as Git submodule
- [ ] Multi-target CMake build (bootloader + app)
- [ ] Common shared library
- [ ] 2 example custom boards
- [ ] Helper scripts (build, flash, clean)
- [ ] VSCode full integration
- [ ] RTOS multi-task example
- [ ] Comprehensive documentation (README, guides)
- [ ] Tested on all 5 platforms
- [ ] Quick start (<5 minutes for new users)
