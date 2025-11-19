## Why

The Alloy project needs a well-organized directory structure and CMake configuration to support C++20 development for embedded systems. This is the foundation for all future development.

## What Changes

- Create standardized directory structure for HAL, core, drivers, platform, examples, and tests
- Set up CMake 3.25+ with C++20 support (no modules, headers only)
- Configure toolchain files for arm-none-eabi and host compilation
- Establish naming conventions (snake_case for files, ALLOY_ prefix for CMake variables)
- Create initial .gitignore for build artifacts

## Impact

- Affected specs: project-structure (new capability)
- Affected code: Root directory structure, CMakeLists.txt files
- **Foundation for all future development** - must be done first
