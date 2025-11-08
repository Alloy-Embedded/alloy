# Spec: codegen-foundation

## Purpose
Provides automated code generation infrastructure for ARM Cortex-M microcontrollers from SVD files, enabling rapid MCU support with minimal manual work.

## Requirements

### Requirement: SVD Infrastructure
The system SHALL provide automated SVD file management

#### Scenario: Downloading SVD files
- **WHEN** User runs `sync_svd.py --init`
- **THEN** CMSIS-SVD repository is cloned and SVD files are organized by vendor

**REQ-CODEGEN-001**: The system SHALL download SVD files from CMSIS-Pack repository
- **Rationale**: Official source for ARM MCU register definitions
- **Implementation**: Git submodule + Python sync script

**REQ-CODEGEN-002**: The system SHALL organize SVDs by vendor
- **Rationale**: Easier navigation and selective sync
- **Implementation**: Symlink structure by vendor name

### Requirement: SVD Parser
The system SHALL parse SVD XML files into JSON database format

**REQ-CODEGEN-003**: The system SHALL parse device information (name, vendor, flash, RAM)
- **Rationale**: Essential MCU metadata
- **Implementation**: XML ElementTree parser

**REQ-CODEGEN-004**: The system SHALL parse peripherals and registers
- **Rationale**: Register access code generation
- **Implementation**: Peripheral and register extraction

**REQ-CODEGEN-005**: The system SHALL parse interrupt vectors
- **Rationale**: Vector table generation
- **Implementation**: Interrupt mapping extraction

### Requirement: Code Generator
The system SHALL generate C++ code from JSON database

**REQ-CODEGEN-006**: The system SHALL generate startup code
- **Rationale**: MCU initialization and vector table
- **Implementation**: Jinja2 template rendering

**REQ-CODEGEN-007**: The system SHALL generate with timestamp and source tracking
- **Rationale**: Traceability and regeneration detection
- **Implementation**: File headers with metadata

### Requirement: CMake Integration
The system SHALL integrate transparently with CMake build system

**REQ-CODEGEN-008**: The system SHALL auto-detect database files for MCU
- **Rationale**: Zero-configuration build
- **Implementation**: Database file pattern matching

**REQ-CODEGEN-009**: The system SHALL regenerate only when database changes
- **Rationale**: Fast incremental builds
- **Implementation**: Timestamp marker files

### Non-Functional Requirements

**REQ-CODEGEN-NF-001**: The system SHALL generate code in < 5 seconds
- **Rationale**: Fast development iteration
- **Implementation**: Optimized parser and template rendering
- **Measured**: 124ms (40x faster than target)

**REQ-CODEGEN-NF-002**: The system SHALL validate all databases
- **Rationale**: Prevent invalid code generation
- **Implementation**: JSON schema validation

**REQ-CODEGEN-NF-003**: The system SHALL have comprehensive test coverage
- **Rationale**: Reliable code generation
- **Implementation**: 38 Python tests (100% passing)

## Implementation

### Files
- `tools/codegen/sync_svd.py` - SVD downloader (10 commands)
- `tools/codegen/svd_parser.py` - XML → JSON parser
- `tools/codegen/generator.py` - Code generator core
- `tools/codegen/database/` - 510 MCU databases
- `tools/codegen/templates/` - Jinja2 templates
- `cmake/codegen.cmake` - CMake integration

### API Surface
```bash
# Download SVDs
python tools/codegen/sync_svd.py --init
python tools/codegen/sync_svd.py --update --vendor STM32

# Parse SVD to JSON
python tools/codegen/svd_parser.py --input STM32F103.svd --output stm32f103.json

# Generate code
python tools/codegen/generator.py --mcu STM32F103C8 --output build/generated/
```

### Usage Examples
```cmake
# CMake automatically generates code
alloy_generate_code(
    MCU STM32F103C8
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
)
```

## Testing
- 38 Python tests (pytest)
- 100% test pass rate
- Integration tests for full pipeline
- Database schema validation

## Performance
- Generation time: 124ms (target: <5000ms)
- 510 MCU databases supported
- Zero CMake overhead for unchanged databases

## Dependencies
- Python 3.8+
- pip: jinja2, lxml, requests
- Git (for SVD sync)
- CMake 3.20+

## References
- CMSIS-SVD specification
- Implementation: `tools/codegen/`
- Documentation: 2169 lines across 4 guides
