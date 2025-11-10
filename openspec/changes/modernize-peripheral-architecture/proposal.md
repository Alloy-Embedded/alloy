# Modernize Peripheral Architecture

## Problem Statement

The current HAL architecture works but has several limitations compared to modern C++20/23 approaches:

1. **Poor Error Messages**: Template-heavy code produces cryptic compile errors (50+ lines of SFINAE failures)
2. **No Signal/Connection System**: Peripherals don't have explicit signal routing (like MODM's connect<> pattern)
3. **Limited DMA Integration**: DMA connections are implicit, not type-checked at compile-time
4. **Verbose Configuration**: No fluent API or designated initializers for complex setups
5. **Mixed Abstraction Levels**: Generic, platform, and vendor code not clearly separated
6. **Manual Pin Configuration**: No compile-time validation of alternate function compatibility

## Solution Overview

Modernize the HAL architecture using C++20/23 features while maintaining zero-overhead guarantees:

1. **Add C++20 Concepts** - Replace SFINAE with concepts for clear error messages
2. **Signal-based Connections** - Explicit peripheral-to-peripheral connections (GPIO→UART, ADC→DMA)
3. **Multi-level API** - Simple, Fluent, and Expert APIs for different use cases
4. **Compile-time Validation** - Use `consteval` for pin/signal compatibility checking
5. **Auto-generated Metadata** - Extend SVD codegen to produce signal routing tables
6. **Integrated DMA Configuration** - Type-safe DMA channel assignment and validation

## Goals

- ✅ **Maintain zero-overhead** - All configuration resolved at compile-time
- ✅ **Improve error messages** - 10x better than current template errors
- ✅ **Support 3 API levels** - Beginner, Common, Expert
- ✅ **Enable signal routing** - Explicit GPIO alternate function connections
- ✅ **Validate at compile-time** - Catch configuration errors before runtime
- ✅ **Keep incremental** - Can be adopted peripheral-by-peripheral

## Non-Goals

- ❌ No runtime overhead (no virtual functions, no dynamic allocation)
- ❌ Not changing existing peripheral implementations (only API layer)
- ❌ Not adding new peripheral types (GPIO, UART, etc. already exist)

## Success Criteria

1. **GPIO→UART connection**: Compile-time validated alternate function routing
2. **DMA channel assignment**: Type-checked DMA stream/channel allocation
3. **Error message quality**: Suggests compatible pins when connection fails
4. **Code size**: No increase in binary size vs current implementation
5. **Adoption**: Can migrate one peripheral at a time without breaking existing code

## Dependencies

- Requires C++20 compiler (already used in project)
- Builds on existing SVD codegen (`refactor-unified-template-codegen`)
- Extends current HAL interfaces (no breaking changes)

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Increased compile time | Medium | Use concepts sparingly, avoid deep template nesting |
| Complex implementation | High | Incremental phases, validate each step |
| Learning curve | Medium | Provide 3 API levels (simple → expert) |
| Backward compatibility | Low | New API coexists with old, migration is optional |

## Related Changes

- `refactor-unified-template-codegen` - Extends SVD codegen for signal metadata
- `add-advanced-hal-interfaces` - Adds new concept-based interfaces
- `add-project-template` - Updates examples to use new API

## Scope

This change includes:
- ✅ C++20 concept definitions for all peripherals
- ✅ Signal routing metadata generation from SVD
- ✅ Three API levels (Simple, Fluent, Expert)
- ✅ Compile-time pin/signal validation
- ✅ DMA connection type-checking
- ✅ Comprehensive error messages with suggestions
- ✅ Migration guide and examples

This change does NOT include:
- ❌ New peripheral implementations
- ❌ Runtime configuration
- ❌ Dynamic pin remapping
