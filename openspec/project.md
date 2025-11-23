# Project Context

## Purpose
MicroCore is a modern C++20/23 embedded systems framework for bare-metal development. It provides zero-overhead hardware abstractions with compile-time validation through C++20 concepts.

## Tech Stack
- C++20/23 (Concepts, Ranges, constexpr)
- CMake 3.25+
- Python 3.10+ (code generation)
- ARM GCC toolchain (embedded targets)
- Clang 14+ (host development)

## Project Conventions

### Code Style
- snake_case for functions, variables, files
- PascalCase for types, classes, concepts
- SCREAMING_SNAKE_CASE for constants
- Namespaces: `ucore::` (core framework)
- No virtual functions (zero-overhead principle)
- Extensive use of constexpr and compile-time evaluation

### Architecture Patterns
- Policy-Based Design (hardware policies via CRTP)
- Concept-Driven Interfaces (C++20 concepts)
- Zero-Overhead Abstractions (everything inlines to single instructions)
- Code Generation (SVD → C++ register definitions)
- 5-Layer Architecture: Generated → Hardware Policy → Platform → Board → Application

### Testing Strategy
- Unit tests with Catch2 v3
- Compile-time validation via static_assert
- Host platform for unit testing (mock hardware)
- Hardware-in-loop testing for actual boards

### Git Workflow
- Feature branches: `feature/description`
- OpenSpec for major changes
- Conventional commits
- PR required for main branch

## Domain Context
Embedded systems development targeting ARM Cortex-M (STM32, SAME70) and other microcontrollers. Focus on bare-metal, safety-critical, real-time systems.

## Important Constraints
- Zero runtime overhead (everything must inline)
- No heap allocation (static allocation only)
- No exceptions (Result<T,E> pattern instead)
- No RTTI
- Minimal binary size
- Fast compilation times

## External Dependencies
- arm-none-eabi-gcc (ARM toolchain)
- st-flash, openocd (flashing tools)
- Python libraries: lxml, jinja2, pyyaml (code generation)
