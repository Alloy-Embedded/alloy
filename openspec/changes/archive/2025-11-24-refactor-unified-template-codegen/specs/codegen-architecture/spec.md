## MODIFIED Requirements

### Requirement: Code Generation Architecture

The code generation system SHALL use template-based architecture instead of hardcoded string concatenation.

#### Scenario: Template-based register generation
- **GIVEN** SAME70 family metadata exists
- **WHEN** register generation is requested
- **THEN** Jinja2 template SHALL be rendered with metadata
- **AND** no Python string concatenation SHALL be used
- **AND** output SHALL be identical to manual implementation

#### Scenario: Consistent output across generators
- **GIVEN** multiple generators (registers, bitfields, peripherals)
- **WHEN** generating code for same family
- **THEN** all generators SHALL use same naming conventions
- **AND** all generators SHALL use same code style
- **AND** consistency SHALL be enforced by shared templates

#### Scenario: Metadata-driven generation
- **GIVEN** family configuration in JSON
- **WHEN** generation parameters change
- **THEN** only JSON metadata SHALL need modification
- **AND** no Python code SHALL need changes
- **AND** template logic SHALL remain unchanged

## ADDED Requirements

### Requirement: Three-Tier Metadata Hierarchy

The system SHALL organize metadata in vendor → family → peripheral hierarchy with inheritance.

#### Scenario: Vendor-level metadata
- **WHEN** creating vendor configuration for Atmel
- **THEN** file SHALL be `metadata/vendors/atmel.json`
- **AND** SHALL contain: architecture, endianness, naming conventions
- **AND** SHALL contain: SVD quirks database
- **AND** SHALL contain: family catalog with all supported families

#### Scenario: Family-level metadata
- **WHEN** creating family configuration for SAME70
- **THEN** file SHALL be `metadata/families/same70.json`
- **AND** SHALL inherit vendor-level settings
- **AND** SHALL override/extend with family-specific rules
- **AND** SHALL contain: register config, bitfield config, memory layout

#### Scenario: Peripheral-level metadata (optional)
- **WHEN** creating peripheral configuration for GPIO
- **THEN** file SHALL be `metadata/peripherals/gpio.json`
- **AND** SHALL contain peripheral-specific generation rules
- **AND** SHALL be referenced from family metadata
- **AND** SHALL support multiple implementations per family

#### Scenario: Metadata inheritance and override
- **GIVEN** vendor defines `register_case: "UPPER"`
- **AND** family overrides with `register_case: "PascalCase"`
- **WHEN** metadata is resolved
- **THEN** family value SHALL take precedence
- **AND** non-overridden vendor values SHALL be inherited
- **AND** final config SHALL be merged correctly

#### Scenario: Metadata validation
- **GIVEN** JSON Schema for each metadata level
- **WHEN** loading metadata file
- **THEN** schema validation SHALL run
- **AND** SHALL report errors with line numbers
- **AND** SHALL reject invalid metadata
- **AND** SHALL provide helpful error messages

### Requirement: UnifiedGenerator Orchestrator

The system SHALL provide single generator class orchestrating all code generation.

#### Scenario: Load metadata hierarchy
- **GIVEN** request to generate for SAME70 family
- **WHEN** UnifiedGenerator initializes
- **THEN** vendor metadata SHALL be loaded (vendors/atmel.json)
- **AND** family metadata SHALL be loaded (families/same70.json)
- **AND** configurations SHALL be merged with proper precedence
- **AND** final config SHALL be validated

#### Scenario: Render template with metadata
- **GIVEN** register structure template
- **AND** resolved SAME70 configuration
- **WHEN** rendering template
- **THEN** metadata SHALL be available as template variables
- **AND** custom filters SHALL be available (sanitize, format_hex, cpp_type)
- **AND** macros SHALL be available from common/macros.j2
- **AND** output SHALL be valid C++

#### Scenario: Atomic file writing
- **GIVEN** rendered template output
- **WHEN** writing to file
- **THEN** temporary file SHALL be created first
- **AND** content SHALL be written to temporary file
- **AND** temporary file SHALL be renamed atomically
- **AND** no partial writes SHALL occur on failure

#### Scenario: Dry-run mode
- **GIVEN** UnifiedGenerator with --dry-run flag
- **WHEN** generation is requested
- **THEN** all templates SHALL be rendered
- **AND** no files SHALL be written to disk
- **AND** output paths SHALL be printed
- **AND** validation errors SHALL be reported

#### Scenario: Validation mode
- **GIVEN** UnifiedGenerator with --validate flag
- **WHEN** generation is requested
- **THEN** metadata SHALL be loaded and validated
- **AND** templates SHALL be rendered
- **AND** syntax SHALL be checked
- **AND** no files SHALL be written
- **AND** exit code SHALL indicate success/failure

#### Scenario: Progress reporting
- **GIVEN** UnifiedGenerator with --verbose flag
- **WHEN** generation runs
- **THEN** metadata loading SHALL be logged
- **AND** each template render SHALL be reported
- **AND** each file write SHALL be logged with path
- **AND** generation time SHALL be displayed

### Requirement: Register Template

The system SHALL provide Jinja2 template for generating register structures.

#### Scenario: Basic register structure
- **GIVEN** PIO peripheral with registers (PIO_PER, PIO_PDR, PIO_PSR)
- **WHEN** rendering register_struct.hpp.j2
- **THEN** struct SHALL be generated with correct layout
- **AND** each register SHALL have correct offset
- **AND** register types SHALL match SVD (uint32_t, etc.)
- **AND** struct size SHALL match peripheral size

#### Scenario: Register arrays
- **GIVEN** ABCDSR register as 2-element array
- **WHEN** rendering template
- **THEN** array SHALL be generated: `uint32_t ABCDSR[2];`
- **AND** array size SHALL match SVD dimension
- **AND** element offset SHALL be correct

#### Scenario: Register padding
- **GIVEN** PIO_PER at offset 0x0000 and PIO_PDR at offset 0x0004
- **WHEN** rendering template
- **THEN** no padding SHALL be inserted (consecutive)
- **GIVEN** register gap (0x0000 to 0x0010)
- **THEN** padding SHALL be inserted: `uint32_t _reserved0[3];`

#### Scenario: Overlapping registers
- **GIVEN** registers at same offset (different access modes)
- **WHEN** rendering template
- **THEN** union SHALL be generated
- **AND** all variants SHALL be accessible
- **AND** size SHALL match largest variant

#### Scenario: Register documentation
- **GIVEN** SVD description for register
- **WHEN** rendering template
- **THEN** C++ comment SHALL be generated with description
- **AND** register offset SHALL be documented
- **AND** access mode SHALL be documented (RO, WO, RW)

#### Scenario: Size validation
- **GIVEN** peripheral with defined size
- **WHEN** rendering template
- **THEN** static_assert SHALL be generated
- **AND** SHALL check sizeof(RegisterStruct) == expected_size
- **AND** SHALL fail compilation if size mismatch

#### Scenario: Configurable formatting
- **GIVEN** family metadata with `register_case: "PascalCase"`
- **WHEN** rendering template
- **THEN** register names SHALL be PascalCase
- **GIVEN** metadata with `register_case: "UPPER"`
- **THEN** register names SHALL be UPPERCASE

### Requirement: Bitfield Template

The system SHALL provide Jinja2 template for generating bitfield enums and helpers.

#### Scenario: Basic enum class
- **GIVEN** bitfield with values (e.g., GPIO peripheral function A/B/C/D)
- **WHEN** rendering bitfield_enum.hpp.j2
- **THEN** enum class SHALL be generated
- **AND** each value SHALL have correct numeric value
- **AND** enum SHALL be strongly typed

#### Scenario: Bitfield masks and shifts
- **GIVEN** bitfield at bits [7:4]
- **WHEN** rendering template
- **THEN** MASK constant SHALL be generated: `0xF0`
- **AND** SHIFT constant SHALL be generated: `4`
- **AND** constants SHALL be constexpr

#### Scenario: Bitfield helper functions
- **GIVEN** bitfield definition
- **WHEN** rendering template
- **THEN** set_field() function SHALL be generated
- **AND** get_field() function SHALL be generated
- **AND** clear_field() function SHALL be generated
- **AND** functions SHALL be constexpr

#### Scenario: Multi-bit field handling
- **GIVEN** field with multiple bits (not single bit flag)
- **WHEN** rendering template
- **THEN** read-modify-write helpers SHALL be generated
- **AND** range validation SHALL be included
- **AND** shift operations SHALL be correct

#### Scenario: Configurable enum style
- **GIVEN** family metadata with `enum_style: "UPPER_SNAKE"`
- **WHEN** rendering template
- **THEN** enum values SHALL be UPPER_SNAKE_CASE
- **GIVEN** metadata with `enum_style: "PascalCase"`
- **THEN** enum values SHALL be PascalCase

### Requirement: Platform Peripheral Templates

The system SHALL provide templates for GPIO, UART, SPI, I2C HAL generation.

#### Scenario: GPIO template
- **GIVEN** GPIO peripheral metadata for SAME70
- **WHEN** rendering gpio.hpp.j2
- **THEN** GpioPin template class SHALL be generated
- **AND** set/clear/toggle/read operations SHALL be generated
- **AND** peripheral function configuration SHALL be generated
- **AND** interrupt configuration SHALL be generated

#### Scenario: UART template
- **GIVEN** UART peripheral metadata
- **WHEN** rendering uart.hpp.j2
- **THEN** UartPort class SHALL be generated
- **AND** configure() method SHALL be generated
- **AND** send/receive methods SHALL be generated
- **AND** error handling SHALL be included

#### Scenario: SPI template
- **GIVEN** SPI peripheral metadata
- **WHEN** rendering spi.hpp.j2
- **THEN** SpiPort class SHALL be generated
- **AND** configure() with mode/speed SHALL be generated
- **AND** transfer methods SHALL be generated
- **AND** chip select handling SHALL be included

#### Scenario: I2C template
- **GIVEN** I2C (TWI) peripheral metadata
- **WHEN** rendering i2c.hpp.j2
- **THEN** I2cPort class SHALL be generated
- **AND** configure() with speed SHALL be generated
- **AND** read/write methods SHALL be generated
- **AND** multi-byte transfers SHALL be supported

#### Scenario: Error handling generation
- **GIVEN** peripheral template
- **WHEN** rendering operations
- **THEN** Result<T,ErrorCode> return types SHALL be generated
- **AND** error conditions SHALL be checked
- **AND** appropriate ErrorCode SHALL be returned

#### Scenario: Interrupt configuration
- **GIVEN** peripheral with interrupt support
- **WHEN** rendering template
- **THEN** interrupt enable/disable SHALL be generated
- **AND** callback registration SHALL be generated
- **AND** interrupt vector SHALL be configured

### Requirement: Startup Code Template

The system SHALL provide template for Cortex-M startup code generation.

#### Scenario: Vector table generation
- **GIVEN** MCU with N interrupt vectors
- **WHEN** rendering cortex_m_startup.cpp.j2
- **THEN** vector table array SHALL be generated
- **AND** all N vectors SHALL be populated
- **AND** weak default handlers SHALL be defined
- **AND** vector table SHALL be in correct section

#### Scenario: Data initialization
- **GIVEN** MCU with Flash and RAM regions
- **WHEN** rendering startup template
- **THEN** Reset_Handler SHALL copy .data from Flash to RAM
- **AND** copy loop SHALL use correct addresses
- **AND** .bss section SHALL be zeroed

#### Scenario: C++ initialization
- **GIVEN** C++ project
- **WHEN** rendering startup template
- **THEN** __init_array SHALL be called
- **AND** static constructors SHALL run before main()
- **AND** constructor order SHALL be preserved

#### Scenario: Hardware initialization
- **GIVEN** family metadata with FPU enabled
- **WHEN** rendering startup template
- **THEN** FPU initialization SHALL be included
- **GIVEN** metadata with cache enabled
- **THEN** cache initialization SHALL be included
- **GIVEN** metadata with MPU
- **THEN** MPU configuration SHALL be included

#### Scenario: Stack configuration
- **GIVEN** family metadata with stack_size: 8192
- **WHEN** rendering startup template
- **THEN** stack SHALL be allocated with correct size
- **AND** stack pointer SHALL be initialized
- **AND** stack overflow detection SHALL be enabled if supported

### Requirement: Linker Script Template

The system SHALL provide template for Cortex-M linker script generation.

#### Scenario: Memory region definition
- **GIVEN** family metadata with Flash at 0x00400000, 512KB
- **AND** RAM at 0x20000000, 128KB
- **WHEN** rendering cortex_m.ld.j2
- **THEN** MEMORY block SHALL define FLASH region
- **AND** MEMORY block SHALL define RAM region
- **AND** addresses and sizes SHALL match metadata

#### Scenario: Section placement
- **GIVEN** linker template
- **WHEN** rendering
- **THEN** .text SHALL be placed in FLASH
- **AND** .data SHALL be in RAM with LMA in FLASH
- **AND** .bss SHALL be in RAM
- **AND** .stack SHALL be in RAM

#### Scenario: Symbol generation
- **GIVEN** linker template
- **WHEN** rendering
- **THEN** _etext, _sdata, _edata symbols SHALL be defined
- **AND** _sbss, _ebss symbols SHALL be defined
- **AND** _estack symbol SHALL be defined
- **AND** symbols SHALL be used in startup code

#### Scenario: Configurable sections
- **GIVEN** family metadata with DTCM region
- **WHEN** rendering template
- **THEN** DTCM memory region SHALL be defined
- **AND** .dtcm section SHALL be created
- **AND** fast data placement option SHALL be available

### Requirement: Metadata Validation and Schemas

The system SHALL validate all metadata against JSON Schema before use.

#### Scenario: Vendor schema validation
- **GIVEN** vendors/atmel.json metadata file
- **WHEN** loading with MetadataLoader
- **THEN** vendor.schema.json SHALL be applied
- **AND** required fields SHALL be checked
- **AND** type validation SHALL be enforced
- **AND** errors SHALL reference JSON path

#### Scenario: Family schema validation
- **GIVEN** families/same70.json metadata file
- **WHEN** loading with MetadataLoader
- **THEN** family.schema.json SHALL be applied
- **AND** all required sections SHALL be validated
- **AND** enum values SHALL be checked
- **AND** cross-references SHALL be validated

#### Scenario: Schema error reporting
- **GIVEN** invalid metadata (wrong type)
- **WHEN** validation runs
- **THEN** error message SHALL include file path
- **AND** error message SHALL include JSON path
- **AND** error message SHALL explain expected format
- **AND** line number SHALL be provided if possible

#### Scenario: IDE schema support
- **GIVEN** JSON Schema files in schemas/
- **WHEN** editing metadata in VS Code
- **THEN** autocomplete SHALL work
- **AND** validation SHALL run in real-time
- **AND** documentation SHALL appear on hover
- **AND** schema SHALL be referenced in JSON files

### Requirement: Migration Path

The system SHALL support gradual migration from old generators to new template system.

#### Scenario: Parallel generator execution
- **GIVEN** old generator generate_registers_legacy.py
- **AND** new generator generate_registers.py
- **WHEN** both generators run
- **THEN** outputs SHALL be in separate directories
- **AND** automated comparison SHALL be available
- **AND** byte-for-byte diff SHALL be performed

#### Scenario: Backward compatibility
- **GIVEN** existing build scripts
- **WHEN** new generator is deployed
- **THEN** command-line interface SHALL remain compatible
- **AND** all existing flags SHALL work
- **AND** output paths SHALL be configurable
- **AND** no build system changes SHALL be required

#### Scenario: Gradual deprecation
- **GIVEN** old generator still in use
- **WHEN** new generator is validated
- **THEN** deprecation warning SHALL be added
- **AND** migration deadline SHALL be communicated
- **AND** documentation SHALL guide migration
- **AFTER** validation period
- **THEN** old generator SHALL be removed

### Requirement: Custom Jinja2 Filters

The system SHALL provide custom filters for code generation.

#### Scenario: Identifier sanitization
- **GIVEN** SVD name with invalid characters "GPIO-A"
- **WHEN** applying sanitize filter
- **THEN** output SHALL be "GPIO_A"
- **AND** SHALL be valid C++ identifier

#### Scenario: Hex formatting
- **GIVEN** address value 0x400E0E00
- **WHEN** applying format_hex filter
- **THEN** output SHALL be "0x400E0E00"
- **AND** width SHALL be configurable
- **AND** prefix "0x" SHALL be included

#### Scenario: Type mapping
- **GIVEN** SVD type "uint32_t"
- **WHEN** applying cpp_type filter
- **THEN** output SHALL be "uint32_t"
- **GIVEN** SVD type "int"
- **THEN** output SHALL be "int32_t"

#### Scenario: Case conversion
- **GIVEN** name "gpio_control"
- **WHEN** applying to_pascal_case filter
- **THEN** output SHALL be "GpioControl"
- **WHEN** applying to_upper_snake filter
- **THEN** output SHALL be "GPIO_CONTROL"

### Requirement: Performance and Efficiency

The code generation system SHALL maintain build performance.

#### Scenario: Build time impact
- **GIVEN** project with 10 MCU families
- **WHEN** full code generation runs
- **THEN** total time SHALL be < 5% increase vs manual
- **AND** parallel generation SHALL be supported
- **AND** caching SHALL avoid redundant work

#### Scenario: Incremental generation
- **GIVEN** previously generated code
- **AND** metadata has not changed
- **WHEN** build is run
- **THEN** generation SHALL be skipped
- **AND** timestamps SHALL be checked
- **AND** build SHALL use cached output

#### Scenario: Memory usage
- **GIVEN** large SVD database
- **WHEN** generation runs
- **THEN** peak memory SHALL be < 500MB
- **AND** streaming parsing SHALL be used
- **AND** metadata SHALL not all be loaded at once

## REMOVED Requirements

None - this is a refactoring that adds capabilities without removing existing functionality.

## RENAMED Requirements

None - no requirements are being renamed.
