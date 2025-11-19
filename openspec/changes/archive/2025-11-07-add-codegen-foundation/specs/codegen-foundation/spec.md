## ADDED Requirements

### Requirement: SVD Synchronization Tool

The system SHALL provide an automated tool to download and organize SVD files from CMSIS repositories.

#### Scenario: Initial SVD setup
- **WHEN** user runs `sync_svd.py --init`
- **THEN** CMSIS-SVD repository SHALL be cloned as git submodule
- **AND** organized directory structure SHALL be created in `database/svd/`
- **AND** symlinks SHALL organize SVDs by vendor (STM32/, nRF/, etc.)

#### Scenario: Update existing SVDs
- **WHEN** user runs `sync_svd.py --update`
- **THEN** git submodule SHALL be updated to latest commit
- **AND** new SVD files SHALL be linked to organized structure
- **AND** INDEX.md SHALL be regenerated with available SVDs

#### Scenario: Vendor filtering
- **WHEN** user runs `sync_svd.py --vendor STM32,nRF`
- **THEN** only specified vendors SHALL be linked
- **AND** other vendors SHALL be skipped
- **AND** disk space usage SHALL be minimized

#### Scenario: List available vendors
- **WHEN** user runs `sync_svd.py --list-vendors`
- **THEN** all available vendors SHALL be displayed
- **AND** number of MCUs per vendor SHALL be shown

#### Scenario: Dry-run mode
- **WHEN** user runs `sync_svd.py --dry-run --update`
- **THEN** actions SHALL be previewed without execution
- **AND** user SHALL see what would be changed

### Requirement: SVD Parser

The system SHALL convert SVD XML files to normalized JSON database format.

#### Scenario: Parse complete SVD
- **WHEN** `svd_parser.py --input STM32F103.svd --output stm32f1xx.json` is executed
- **THEN** device information (flash, RAM, vendor) SHALL be extracted
- **AND** all peripherals with registers SHALL be parsed
- **AND** interrupt vector table SHALL be extracted
- **AND** valid JSON SHALL be written to output file

#### Scenario: Merge into existing database
- **WHEN** `svd_parser.py --input STM32F103.svd --merge --output stm32f1xx.json` is executed
- **AND** output file already contains STM32F101
- **THEN** STM32F103 SHALL be added to mcus section
- **AND** existing entries SHALL NOT be modified
- **AND** family-level data SHALL be preserved

#### Scenario: Handle vendor quirks
- **WHEN** parsing STM32 SVD file
- **THEN** ST-specific naming SHALL be normalized
- **AND** derivedFrom inheritance SHALL be resolved
- **AND** missing peripheral bases SHALL be calculated
- **AND** consistent naming convention SHALL be applied

#### Scenario: Validation of parsed data
- **WHEN** parsing completes
- **THEN** required fields SHALL be verified present
- **AND** memory addresses SHALL be validated (non-zero, reasonable ranges)
- **AND** warning SHALL be emitted for suspicious values

### Requirement: JSON Database Schema

The system SHALL define structured JSON format for MCU descriptions.

#### Scenario: Database structure for family
- **WHEN** creating family database
- **THEN** top-level SHALL contain: family, architecture, vendor, mcus
- **AND** mcus SHALL be keyed by MCU name (e.g., "STM32F103C8")
- **AND** each MCU SHALL contain: flash, ram, peripherals, interrupts

#### Scenario: Memory layout specification
- **WHEN** specifying memory regions
- **THEN** flash SHALL include: size_kb, base_address, page_size_kb
- **AND** ram SHALL include: size_kb, base_address
- **AND** addresses SHALL be hex strings (e.g., "0x08000000")

#### Scenario: Peripheral definition
- **WHEN** defining peripheral (e.g., GPIO)
- **THEN** instances array SHALL list all instances (GPIOA, GPIOB, etc.)
- **AND** each instance SHALL have: name, base
- **AND** registers object SHALL map register name to offset and size
- **AND** common peripherals SHALL have consistent naming across families

#### Scenario: Interrupt vector table
- **WHEN** defining interrupts
- **THEN** count field SHALL specify total number of vectors
- **AND** vectors array SHALL list important interrupts
- **AND** each vector SHALL have: number, name
- **AND** ARM standard vectors (0-15) SHALL always be present

### Requirement: Code Generator Core

The system SHALL generate C++ source code from JSON databases using templates.

#### Scenario: Generate startup code
- **WHEN** `generator.py --mcu STM32F103C8` is executed
- **THEN** JSON database SHALL be loaded for STM32F103C8
- **AND** startup template SHALL be rendered with MCU data
- **AND** `build/generated/STM32F103C8/startup.cpp` SHALL be written
- **AND** file SHALL contain valid C++ code

#### Scenario: Template rendering
- **WHEN** rendering Jinja2 template
- **THEN** MCU data SHALL be available as template variables
- **AND** macros from common/macros.j2 SHALL be available
- **AND** file header from common/header.j2 SHALL be included
- **AND** generated code SHALL be formatted and readable

#### Scenario: Output file organization
- **WHEN** generating code for MCU
- **THEN** output SHALL go to `build/generated/{MCU_NAME}/`
- **AND** directory SHALL be created if missing
- **AND** existing files SHALL be overwritten
- **AND** generation timestamp SHALL be recorded

#### Scenario: Validation mode
- **WHEN** generator runs with `--validate` flag
- **THEN** templates SHALL be rendered
- **AND** no files SHALL be written to disk
- **AND** errors SHALL be reported if rendering fails
- **AND** exit code SHALL indicate success or failure

#### Scenario: Verbose output
- **WHEN** generator runs with `--verbose` flag
- **THEN** database loading SHALL be logged
- **AND** each template rendered SHALL be reported
- **AND** each file written SHALL be logged with path
- **AND** generation time SHALL be displayed

### Requirement: Jinja2 Templates

The system SHALL provide templates for generating startup code, register definitions, vector tables, and linker scripts.

#### Scenario: Startup template for Cortex-M
- **WHEN** rendering startup template
- **THEN** Reset_Handler SHALL copy .data from Flash to RAM
- **AND** Reset_Handler SHALL zero .bss section
- **AND** C++ static constructors SHALL be called (__init_array)
- **AND** main() SHALL be called
- **AND** infinite loop with WFI SHALL follow main return

#### Scenario: Common header template
- **WHEN** any file is generated
- **THEN** header SHALL include auto-generated warning
- **AND** header SHALL include generation timestamp
- **AND** header SHALL include source database/template info
- **AND** header SHALL include DO NOT EDIT message

#### Scenario: Template macros
- **WHEN** templates use common macros
- **THEN** hex() filter SHALL format addresses as 0x-prefixed hex
- **AND** macros for peripheral loops SHALL be available
- **AND** macros for register formatting SHALL be available

### Requirement: CMake Integration

The system SHALL integrate code generation transparently into CMake build process.

#### Scenario: Automatic generation on configure
- **WHEN** CMake configures project with ALLOY_MCU set
- **THEN** codegen.cmake SHALL be loaded
- **AND** alloy_generate_code() SHALL be called
- **AND** code generation SHALL occur if needed
- **AND** generated files SHALL be added to build

#### Scenario: Incremental generation
- **WHEN** generated code already exists and is up-to-date
- **THEN** generation SHALL be skipped
- **AND** marker file (.generated) SHALL be checked
- **AND** database modification time SHALL be compared
- **AND** CMake configure SHALL complete quickly (< 100ms overhead)

#### Scenario: Regeneration trigger
- **WHEN** database JSON is newer than marker file
- **THEN** code generation SHALL run again
- **AND** old generated files SHALL be overwritten
- **AND** new marker file SHALL be created
- **AND** user SHALL see "Regenerating code for {MCU}" message

#### Scenario: Generation failure handling
- **WHEN** generator.py fails (non-zero exit code)
- **THEN** CMake SHALL terminate with FATAL_ERROR
- **AND** error message from generator SHALL be displayed
- **AND** build SHALL not proceed
- **AND** user SHALL get clear instructions to fix issue

#### Scenario: Python dependency check
- **WHEN** CMake runs on system without Python 3.8+
- **THEN** find_package(Python3 REQUIRED) SHALL fail
- **AND** clear error message SHALL tell user to install Python
- **AND** minimum Python version SHALL be specified

### Requirement: Database Validation

The system SHALL validate JSON databases against schema before use.

#### Scenario: Schema validation
- **WHEN** `validate_database.py database/families/*.json` is run
- **THEN** each JSON file SHALL be parsed
- **AND** schema SHALL be checked against database_schema.json
- **AND** required fields SHALL be verified present
- **AND** data types SHALL be validated

#### Scenario: Address validation
- **WHEN** validating memory addresses
- **THEN** flash base SHALL be checked for typical ranges (0x0000000-0x20000000)
- **AND** RAM base SHALL be checked for typical ranges (0x20000000+)
- **AND** peripheral bases SHALL be non-zero
- **AND** warnings SHALL be emitted for suspicious values

#### Scenario: Consistency checks
- **WHEN** validating database
- **THEN** interrupt numbers SHALL not have duplicates
- **AND** peripheral instances SHALL have unique names
- **AND** register offsets SHALL be reasonable (< 0x1000 typically)
- **AND** family/architecture SHALL match MCU architecture

### Requirement: Testing Infrastructure

The system SHALL include comprehensive tests for parser, generator, and integration.

#### Scenario: SVD parser unit tests
- **WHEN** `pytest tools/codegen/tests/test_svd_parser.py` is run
- **THEN** fixture SVD SHALL be parsed
- **AND** device info extraction SHALL be tested
- **AND** peripheral parsing SHALL be tested
- **AND** interrupt parsing SHALL be tested
- **AND** all tests SHALL pass

#### Scenario: Generator unit tests
- **WHEN** `pytest tools/codegen/tests/test_generator.py` is run
- **THEN** template rendering SHALL be tested with fixture data
- **AND** file writing SHALL be tested
- **AND** database loading SHALL be tested
- **AND** error handling SHALL be tested

#### Scenario: Integration test
- **WHEN** integration test runs
- **THEN** complete pipeline SHALL execute (sync → parse → generate)
- **AND** generated code SHALL be compiled with arm-none-eabi-g++
- **AND** compilation SHALL succeed
- **AND** expected symbols SHALL be present in output

### Requirement: Documentation

The system SHALL provide comprehensive documentation for all tools and workflows.

#### Scenario: Tool usage documentation
- **WHEN** user reads tools/codegen/README.md
- **THEN** overview of code generation SHALL be explained
- **AND** each tool (sync, parse, generate) SHALL be documented
- **AND** example workflows SHALL be provided
- **AND** troubleshooting guide SHALL be included

#### Scenario: Adding new MCU tutorial
- **WHEN** user follows "Adding new ARM MCU" tutorial
- **THEN** step-by-step instructions SHALL be provided
- **AND** user SHALL successfully add STM32F446 as example
- **AND** verification steps SHALL be included
- **AND** expected output SHALL be shown

#### Scenario: Database schema documentation
- **WHEN** user reads database schema docs
- **THEN** all required and optional fields SHALL be documented
- **AND** example JSON snippets SHALL be provided
- **AND** vendor-specific quirks SHALL be explained
- **AND** validation rules SHALL be documented

## MODIFIED Requirements

### Requirement: Build System Configuration

The build system SHALL be extended to support automatic code generation.

#### Scenario: ALLOY_MCU variable
- **WHEN** user sets ALLOY_MCU in CMakeLists.txt
- **THEN** CMake SHALL load appropriate database
- **AND** code generation SHALL be triggered if needed
- **AND** generated files SHALL be added to build
- **MODIFIED:** Previously MCU was specified via board file only

#### Scenario: Build output structure
- **WHEN** building project
- **THEN** build/generated/{MCU}/ directory SHALL exist
- **AND** generated files SHALL be available to compiler
- **AND** include paths SHALL include generated directory
- **MODIFIED:** New generated code directory structure

## REMOVED Requirements

None. This is a new capability with no removed requirements.
