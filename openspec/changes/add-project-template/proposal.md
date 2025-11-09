# Proposal: Add Project Template Repository

## Summary
Create a template repository that uses Alloy as a Git submodule, providing a production-ready project structure with CMake build system, custom board support, multi-target capability, and developer tooling. This template enables users to start new Alloy-based projects with best practices built-in.

## Motivation
Currently, users must:
1. **Clone the entire Alloy framework** - Includes hundreds of generated files they don't need
2. **Manually configure CMake** - Complex toolchain setup, board selection
3. **Figure out project structure** - No guidance on organizing code
4. **Set up development tools** - VSCode, build scripts, debugging
5. **Create custom boards from scratch** - No template or examples

This creates a **high barrier to entry** for new users and **inconsistent project structures** across teams.

A template repository solves this by:
- **Clean separation** - User code separate from framework (via submodule)
- **Ready-to-use build system** - CMake configured, just add code
- **Best-practice structure** - Organized directories (src/, include/, tests/, boards/)
- **Custom board support** - Easy way to define new hardware
- **Multi-target builds** - Bootloader + application + tests in one repo
- **Developer experience** - VSCode integration, build scripts, examples
- **Easy updates** - `git submodule update` pulls latest Alloy

## Goals

### Core Features
1. **Git Submodule Integration**
   - Alloy as submodule in `external/alloy/`
   - Template tracks specific Alloy version (stable releases)
   - Easy to update: `git submodule update --remote`

2. **Multi-Target Build System**
   - Support multiple firmware images in one repo
   - Example: `bootloader/`, `application/`, `factory_test/`
   - Each target has own CMakeLists.txt
   - Shared code in `common/` library

3. **Custom Board Definitions**
   - Define boards in `boards/` directory
   - C++ header format: `boards/my_board/board.hpp`
   - Inherit from Alloy board or define from scratch
   - CMake auto-discovers custom boards

4. **Production-Ready Structure**
   ```
   my-project/
   ├── external/
   │   └── alloy/              # Git submodule
   ├── boards/
   │   └── custom_board/       # User custom boards
   │       ├── board.hpp
   │       └── linker.ld
   ├── common/                 # Shared library
   │   ├── include/
   │   └── src/
   ├── bootloader/             # Target 1
   │   ├── CMakeLists.txt
   │   └── main.cpp
   ├── application/            # Target 2
   │   ├── CMakeLists.txt
   │   └── main.cpp
   ├── tests/                  # Unit tests
   │   └── test_common.cpp
   ├── .vscode/                # VSCode config
   ├── scripts/                # Build/flash scripts
   ├── CMakeLists.txt          # Root CMake
   └── README.md
   ```

5. **Developer Tooling**
   - VSCode integration (tasks, launch configs, IntelliSense)
   - Build wrapper scripts (`./build.sh`, `./flash.sh`)
   - RTOS multi-task example included
   - Documentation and getting started guide

### Advanced Features (Future)
- CI/CD templates (GitHub Actions, GitLab CI)
- Code formatting (clang-format)
- Static analysis (clang-tidy)
- Memory map visualization tools
- OTA update support
- Production deployment scripts

## Non-Goals
1. **Not modifying Alloy core** - Template is separate, Alloy stays clean
2. **Not a monorepo** - Alloy and user code are separate repos
3. **Not board-agnostic** - Users still pick specific MCU/board
4. **Not supporting Arduino IDE** - CMake/CLI only
5. **Not a code generator** - Template is cloned/copied, not generated

## Success Criteria
- [x] Template repository on GitHub with MIT license
- [x] Alloy integrated as Git submodule
- [x] Multi-target CMake build (bootloader + app)
- [x] Custom board example (derived from Alloy board)
- [x] RTOS multi-task example working
- [x] VSCode tasks for build/flash/debug
- [x] Documentation: README, STRUCTURE.md, CUSTOM_BOARDS.md
- [x] Works on all 5 platforms (STM32F1, STM32F4, ESP32, RP2040, SAMD21)
- [x] Build time <30 seconds for clean build
- [x] Users can create new project in <5 minutes

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Submodule complexity | Medium - users unfamiliar with submodules | Clear docs, scripts handle it automatically |
| CMake complexity | Medium - hard to extend | Well-documented CMakeLists, examples provided |
| Alloy version incompatibility | High - breaking changes in Alloy | Pin to stable release, document upgrade process |
| Custom board errors | Medium - hard to debug | Validation in CMake, example boards, docs |
| Large initial clone | Low - submodule adds download time | Document shallow clone option |
| Platform-specific issues | Medium - Windows/Mac/Linux differences | Test on all platforms, document requirements |

## Dependencies
- **Requires**: Alloy framework (as submodule)
- **Builds on**: CMake build system already in Alloy
- **Blocks**: User adoption (easier onboarding)
- **Enables**: Production projects, commercial use

## Timeline
- **Design & Spec**: 1 day (this proposal)
- **Template Structure**: 1 day (directories, initial CMake)
- **Multi-Target Build**: 1 day (bootloader + app example)
- **Custom Board Support**: 1 day (example + docs)
- **VSCode Integration**: 0.5 day (tasks, launch.json)
- **RTOS Example**: 0.5 day (multi-task demo)
- **Documentation**: 1 day (README, guides)
- **Testing**: 1 day (all platforms)
- **Total**: 6-7 days (1-1.5 weeks)

## Alternatives Considered

### 1. Monorepo (Alloy + User Code Together)
**Rejected**:
- **Tight coupling** - User code mixed with framework
- **Harder to update** - Can't easily pull Alloy updates
- **Namespace pollution** - User files mixed with generated files
- **Version control complexity** - Unclear what's user vs framework

**Why submodule**:
- Clean separation of concerns
- Easy to update Alloy (`git submodule update`)
- User repo stays small and focused
- Can fork Alloy if needed (submodule points to fork)

### 2. Copy Alloy Files Instead of Submodule
**Rejected**:
- **No updates** - Hard to get bug fixes and features
- **Large repo** - User repo contains entire Alloy
- **Merge conflicts** - Updating Alloy is manual nightmare

**Why submodule**:
- Always up-to-date with `git submodule update`
- User repo stays small
- Clear ownership (Alloy vs user code)

### 3. Single Target (No Bootloader/Multi-Firmware)
**Rejected**:
- **Limited use cases** - Production apps often need bootloader
- **Not scalable** - Hard to add factory test firmware later

**Why multi-target**:
- Common in production (bootloader + app)
- Easy to add targets later
- Shows best practices for complex projects

### 4. JSON/YAML for Board Config
**Rejected**:
- **Less flexible** - Can't use C++ code/macros
- **Parse complexity** - Need JSON parser in CMake
- **Type safety** - No compile-time validation

**Why C++ headers**:
- Compile-time validation
- Full C++ expressiveness
- Consistent with Alloy board definitions
- No additional dependencies

### 5. Include All Examples in Template
**Rejected**:
- **Bloat** - Users delete most examples anyway
- **Confusing** - Too many options for beginners

**Why minimal examples**:
- Just RTOS multi-task (most common use case)
- Users can reference Alloy's examples/ for more
- Keeps template clean and focused

### 6. No VSCode Integration (Editor Agnostic)
**Rejected**:
- **Worse DX** - Users spend hours configuring
- **VSCode is dominant** - Most embedded devs use it

**Why VSCode config**:
- Dramatically improves onboarding
- Other editors can ignore .vscode/
- Industry standard for embedded C++

## Architecture Overview

### Repository Structure
```
alloy-project-template/          # GitHub template repo
├── .github/
│   └── workflows/
│       └── build.yml            # CI (future)
│
├── .vscode/                     # VSCode integration
│   ├── tasks.json               # Build/Flash/Clean tasks
│   ├── launch.json              # Debug configurations
│   ├── settings.json            # IntelliSense, formatting
│   └── extensions.json          # Recommended extensions
│
├── external/
│   └── alloy/                   # Git submodule → alloy repo
│       ├── src/
│       ├── boards/
│       ├── CMakeLists.txt
│       └── ...
│
├── boards/                      # User custom boards
│   └── my_custom_board/
│       ├── board.hpp            # Board definition
│       ├── linker.ld            # Linker script (optional)
│       └── README.md            # Board documentation
│
├── common/                      # Shared library (used by all targets)
│   ├── include/
│   │   └── config.h             # Project-wide config
│   ├── src/
│   │   └── utils.cpp            # Shared utilities
│   └── CMakeLists.txt           # Library definition
│
├── bootloader/                  # Target 1: Bootloader firmware
│   ├── main.cpp                 # Bootloader entry point
│   └── CMakeLists.txt           # Bootloader build config
│
├── application/                 # Target 2: Main application
│   ├── main.cpp                 # RTOS multi-task example
│   ├── tasks/                   # Task implementations
│   │   ├── sensor_task.cpp
│   │   └── display_task.cpp
│   └── CMakeLists.txt           # Application build config
│
├── tests/                       # Unit tests (host platform)
│   ├── test_utils.cpp
│   └── CMakeLists.txt
│
├── scripts/                     # Helper scripts
│   ├── build.sh                 # Wrapper for cmake build
│   ├── flash.sh                 # Flash firmware to device
│   ├── clean.sh                 # Clean build artifacts
│   └── setup-submodule.sh       # Initialize Alloy submodule
│
├── docs/                        # Project documentation
│   ├── STRUCTURE.md             # Explain directory layout
│   ├── CUSTOM_BOARDS.md         # How to create custom boards
│   ├── BUILD.md                 # Build system guide
│   └── DEPLOYMENT.md            # Production deployment
│
├── .gitignore                   # Ignore build/, .vscode/ipch/, etc.
├── .gitmodules                  # Submodule configuration
├── CMakeLists.txt               # Root CMake (includes Alloy + targets)
├── README.md                    # Getting started guide
└── LICENSE                      # MIT license
```

### CMake Integration

#### Root CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyAlloyProject CXX C ASM)

# Include Alloy framework
add_subdirectory(external/alloy)

# Set board (can be from Alloy or custom)
set(BOARD "my_custom_board" CACHE STRING "Target board")

# Add custom boards directory to search path
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/boards")

# Common library (shared code)
add_subdirectory(common)

# Targets
add_subdirectory(bootloader)
add_subdirectory(application)

# Tests (host platform only)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

#### Custom Board Definition
```cpp
// boards/my_custom_board/board.hpp
#ifndef MY_CUSTOM_BOARD_HPP
#define MY_CUSTOM_BOARD_HPP

// Inherit from Alloy board (if similar hardware)
#include "external/alloy/boards/stm32f407vg/board.hpp"

namespace Board {
    // Override name
    inline constexpr const char* name = "My Custom STM32F407 Board";

    // Add custom pins
    using CustomLed = alloy::hal::stm32f4::GpioPin<48>;  // PC0

    // Override initialization if needed
    inline void initialize() {
        // Call base initialization
        ::Board::initialize();  // From stm32f407vg

        // Custom init
        CustomLed led;
        led.configure(alloy::hal::PinMode::Output);
    }
}

#endif
```

### Build System Flow
```
User runs: ./scripts/build.sh application stm32f407vg
    ↓
Script calls: cmake -B build -DBOARD=stm32f407vg -DTARGET=application
    ↓
CMake configures:
  1. Finds board in boards/ or external/alloy/boards/
  2. Includes Alloy CMakeLists.txt
  3. Sets up toolchain (ARM, Xtensa, etc.)
  4. Builds common library
  5. Builds selected target (application)
    ↓
Output: build/application/application.elf
    ↓
User runs: ./scripts/flash.sh application stm32f407vg
    ↓
Script flashes via OpenOCD/pyOCD/esptool
```

## Example Usage

### Quick Start (New User)
```bash
# 1. Create new project from template
gh repo create my-firmware --template alloy-project-template
cd my-firmware

# 2. Initialize Alloy submodule
./scripts/setup-submodule.sh

# 3. Build application for STM32F407
./scripts/build.sh application stm32f407vg

# 4. Flash to device
./scripts/flash.sh application stm32f407vg

# Done! RTOS multi-task example is running
```

### Creating Custom Board
```bash
# 1. Copy example board
cp -r boards/my_custom_board boards/production_board

# 2. Edit board.hpp
vim boards/production_board/board.hpp

# 3. Build with custom board
./scripts/build.sh application production_board

# 4. Test on hardware
./scripts/flash.sh application production_board
```

### Adding New Target
```bash
# 1. Create new target directory
mkdir factory_test
cp application/CMakeLists.txt factory_test/

# 2. Write factory test code
vim factory_test/main.cpp

# 3. Add to root CMakeLists.txt
echo "add_subdirectory(factory_test)" >> CMakeLists.txt

# 4. Build new target
./scripts/build.sh factory_test stm32f407vg
```

## Key Design Decisions

### 1. Git Submodule vs Alternatives
**Chosen: Git Submodule**

Pros:
- Clean separation of user code and framework
- Easy updates (`git submodule update --remote`)
- Users can fork Alloy if needed (point submodule to fork)
- Small user repo (Alloy not duplicated)

Cons:
- Slightly more complex (`git clone --recursive`)
- Submodule tracking can be confusing for Git beginners

Mitigation:
- Provide setup script that handles submodule
- Clear documentation with examples
- VSCode tasks can auto-init submodule

### 2. Multi-Target by Default
**Chosen: Multiple targets (bootloader + application)**

Rationale:
- Production projects often need bootloader
- Shows best practices for complex projects
- Easy to ignore bootloader if not needed (just don't build it)

Alternative considered: Single target
- Simpler but less realistic
- Users would need to restructure for production

### 3. C++ Headers for Boards
**Chosen: C++ board.hpp**

Rationale:
- Compile-time validation (static_assert)
- Full C++ expressiveness
- Consistent with Alloy's board definitions
- No additional dependencies (no JSON parser)

Alternative considered: JSON config
- Less flexible
- Requires JSON parser in CMake
- No type safety

### 4. VSCode as Primary IDE
**Chosen: Include .vscode/ directory**

Rationale:
- VSCode is industry standard for embedded C++
- Dramatically improves developer experience
- Other editors can ignore .vscode/
- Easy to customize

Alternative considered: Editor agnostic
- Users spend hours configuring
- Inconsistent experience across team

## Documentation Strategy

### README.md (Quick Start)
- Clone template
- Initialize submodule
- Build first example
- Flash to device
- Next steps

### STRUCTURE.md (Architecture)
- Directory layout explanation
- Build system overview
- Where to add code
- Naming conventions

### CUSTOM_BOARDS.md (Board Guide)
- How to create custom board
- Inherit vs from-scratch
- Linker script customization
- Pin mapping tools

### BUILD.md (Build System)
- CMake configuration options
- Cross-compilation setup
- Toolchain installation
- Troubleshooting

## Future Enhancements
1. **GitHub template features** - "Use this template" button
2. **CI/CD workflows** - Auto-build on push, release artifacts
3. **Code formatting** - clang-format integration
4. **Static analysis** - clang-tidy checks
5. **Memory visualization** - Analyze .map files
6. **OTA updates** - Bootloader + app OTA example
7. **Multi-core** - ESP32 dual-core template

## References
- Git Submodules: https://git-scm.com/book/en/v2/Git-Tools-Submodules
- CMake Subdirectories: https://cmake.org/cmake/help/latest/command/add_subdirectory.html
- VSCode C++ Tasks: https://code.visualstudio.com/docs/cpp/cpp-debug
- GitHub Templates: https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-template-repository
