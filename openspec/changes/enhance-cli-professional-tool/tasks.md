# Tasks: Enhanced CLI - Professional Development Tool

**Change ID**: `enhance-cli-professional-tool`
**Status**: PROPOSED
**Last Updated**: 2025-01-17 (Consolidated with usability improvements)
**Duration**: 11.5 weeks (230 hours) - Updated from 8 weeks

---

## 🆕 Phase 0: YAML Migration (1 week, 20 hours)

### 0.1 YAML Infrastructure (6h) ✅ COMPLETED
- [x] Add PyYAML to dependencies
  - [x] Update `requirements.txt` with `PyYAML>=6.0`
  - [x] Update `pyproject.toml` dependencies
  - [x] Test PyYAML installation

- [x] Create YAML Database Loader
  - [x] Implement `tools/codegen/cli/loaders/yaml_loader.py`
  - [x] Support `yaml.safe_load()` (security)
  - [x] Handle multiline strings correctly
  - [x] Preserve comment metadata

- [x] Implement format auto-detection
  - [x] Detect `.json` vs `.yaml` extension
  - [x] Route to appropriate loader
  - [x] Support both formats simultaneously
  - [x] Add tests for auto-detection (in database_loader.py)

- [x] Write YAML schema validators
  - [x] Convert existing JSON schemas to YAML-compatible
  - [x] Add YAML-specific validation rules (mcu.schema.yaml, board.schema.yaml, peripheral.schema.yaml)
  - [x] Test schema validation

### 0.2 JSON→YAML Conversion (4h) ✅ COMPLETED
- [x] Create conversion script
  - [x] Script: `tools/codegen/scripts/migrate_json_to_yaml.py`
  - [x] Preserve structure and semantics
  - [x] Add comment placeholders for quirks
  - [x] Format multiline code snippets
  - [x] Validate output matches input semantically
  - [x] CLI with --all, --directory, --dry-run options
  - [x] Deep validation with detailed error messages

### 0.3 Database Migration (8h) ✅ COMPLETED
- [x] Migrate MCU metadata
  - [x] Create `database/mcus/stm32f4.yaml` with comprehensive examples
  - [x] Include STM32F401RE and STM32F407VG configurations
  - [x] Add inline comments for hardware quirks (I2C timing, UART baud rates, etc.)
  - [x] Add comments for peripheral limitations (USB, GPIO port quirks)

- [x] Migrate board metadata
  - [x] Create `database/boards/nucleo_f401re.yaml` with complete pinout
  - [x] Document pin conflicts in comments (LED/SPI1_SCK, USART2/ST-LINK, etc.)
  - [x] Document debugger connections (ST-LINK VCP)
  - [x] Add Arduino compatibility notes

- [x] Migrate peripheral and template metadata
  - [x] Create `database/peripherals/uart.yaml` with implementation status
  - [x] Create `database/templates/blinky.yaml` project template
  - [x] Document API levels (simple, fluent, expert)
  - [x] Add performance notes and quirks

- [x] Create database structure
  - [x] Created database/{mcus,boards,peripherals,templates} directories
  - [x] Created schema files in database/schema/
  - [x] Documented ownership boundaries (CLI owns schemas, Library Quality owns peripheral templates)

### 0.4 Documentation (2h) ✅ COMPLETED
- [x] Update metadata documentation
  - [x] Create comprehensive `docs/guides/YAML_METADATA_GUIDE.md` (200+ lines)
  - [x] Document YAML format advantages (25-30% smaller, inline comments, etc.)
  - [x] Provide MCU, board, peripheral, and template examples
  - [x] Add migration guide with script usage
  - [x] Document best practices (inline comments, multiline strings, quirks)
  - [x] Clarify ownership boundaries (CLI owns schemas, Library Quality owns peripheral templates)

---

## Phase 1: Foundation & Discovery (3 weeks, 52 hours) - 🆕 +12h

### 1.1 Database Schema Design (4h) ✅ COMPLETED
- [x] Define Pydantic models for MCU database
  - [x] MCU, MCUFamily, Memory, Package, Documentation models (mcu.py)
  - [x] Type-safe validation with Pydantic
  - [x] Helper methods (has_peripheral, get_peripheral_count, filter_mcus)
- [x] Define Pydantic models for board database
  - [x] Board, BoardInfo, MCUReference, ClockConfig, Pinout models (board.py)
  - [x] LED, Button, Debugger models
  - [x] Helper methods (has_led, get_led, has_button, get_button)
- [x] Define Pydantic models for peripheral database
  - [x] Peripheral, PeripheralImplementation, PeripheralTests models (peripheral.py)
  - [x] Enums for PeripheralType, ImplementationStatus, APILevel
  - [x] Helper methods (get_implementation, is_implemented_for, has_api_level)

### 1.2 Database Population (8h)
- [ ] Extract MCU data from SVD files
  - [ ] Parse STM32F4 SVD files → JSON
  - [ ] Parse SAME70 SVD files → JSON
  - [ ] Parse STM32G0 SVD files → JSON
  - [ ] Extract peripheral counts and types
  - [ ] Extract memory sizes and addresses
- [ ] Parse board.hpp files to JSON
  - [ ] Convert nucleo_f401re board.hpp → JSON
  - [ ] Convert same70_xplained board.hpp → JSON
  - [ ] Extract pinout information
  - [ ] Extract peripheral pin mappings
  - [ ] Extract LED/button definitions
- [ ] Create peripheral metadata
  - [ ] List implemented peripherals (GPIO, UART, SPI, etc.)
  - [ ] Track implementation status per MCU family
  - [ ] Add API documentation links
- [ ] Add datasheet URLs
  - [ ] Collect STM32 datasheet links
  - [ ] Collect SAME70 datasheet links
  - [ ] Add reference manual links
  - [ ] Add errata sheet links
- [ ] Generate index files
  - [ ] Create MCU index by part number
  - [ ] Create board index by ID
  - [ ] Create peripheral index by type

### 1.2 Database Population (8h) - ⏭️ SKIPPED (examples already created in Phase 0)

### 1.3 MCU Service (8h) ✅ COMPLETED
- [x] Implement MCUService class (mcu_service.py)
  - [x] Database directory auto-detection
  - [x] LRU caching (@lru_cache) for loaded families
  - [x] Integration with DatabaseLoader (YAML/JSON auto-detection)
- [x] Implement `list()` method with comprehensive filtering
  - [x] Filter by vendor, family, min_flash, min_sram, with_peripheral
  - [x] Sort by part_number, flash, sram, freq
  - [x] Load from multiple families
- [x] Implement `show()` method
  - [x] Search across all families for part number
  - [x] Return MCU with full details
- [x] Implement `search()` method
  - [x] Parse complex queries ("USB + 512KB + Cortex-M4")
  - [x] AND logic for multiple tokens
  - [x] Search in part_number, core, tags, peripherals
  - [x] Memory size parsing (e.g., "512KB")
- [x] Implement `get_stats()` for database statistics

### 1.4 Board Service (6h) ✅ COMPLETED
- [x] Implement BoardService class (board_service.py)
  - [x] Database directory auto-detection
  - [x] LRU caching for loaded boards
  - [x] Integration with DatabaseLoader
- [x] Implement `list()` method with filtering
  - [x] Filter by vendor, mcu_family, has_led, has_button, tags
  - [x] Sort by display name
- [x] Implement `show()` method
  - [x] Load board by ID with full details
- [x] Implement `get_pinout()` method
  - [x] Return formatted pinout information
- [x] Implement `get_peripheral_pins()` method
  - [x] Get pin configurations for specific peripheral
- [x] Implement `find_compatible_boards()` method
  - [x] Find boards for specific MCU part number
- [x] Implement `get_stats()` for database statistics
- [ ] Implement `show()` method
  - [ ] Load single board by ID
  - [ ] Parse pinout data
  - [ ] Load associated MCU data
  - [ ] Format for display
- [ ] Implement pinout parsing
  - [ ] Parse connector definitions
  - [ ] Parse LED/button mappings
  - [ ] Parse peripheral pin options
  - [ ] Detect conflicts (shared pins)

### 1.5 CLI Commands (10h) ✅ COMPLETED
- [x] Set up Typer CLI framework
  - [x] Create main CLI app (cli/main.py) with Typer
  - [x] Set up command groups (list, show, search)
  - [x] Rich traceback for better errors
  - [x] Version command with styled output
- [x] Implement `alloy list mcus` (commands/list_cmd.py)
  - [x] Filter options: vendor, family, min-flash, min-sram, with-peripheral
  - [x] Sort by: part_number, flash, sram, freq
  - [x] Rich table with colors and emojis
  - [x] Status indicators (✅ production, 🔶 preview, etc.)
- [x] Implement `alloy list boards`
  - [x] Filter options: vendor, mcu-family, has-led, has-button, tag
  - [x] Rich table with features icons (💡 LED, 🔘 Button, 🔌 Debugger)
  - [x] Show MCU compatibility
- [x] Implement `alloy show mcu <name>` (commands/show_cmd.py)
  - [x] Load MCU details with full specs
  - [x] Rich panels and tables layout
  - [x] Display memory (Flash, SRAM, EEPROM)
  - [x] Display peripherals list
  - [x] Show documentation links (datasheet, reference manual, errata)
  - [x] Show compatible boards and tags
- [x] Implement `alloy show board <name>`
  - [x] Load board details with full configuration
  - [x] Display board specs (MCU, clock, features)
  - [x] Display LEDs and buttons with GPIO pins
  - [x] Show examples and tags
  - [x] --pinout option placeholder (Phase 5)
- [x] Implement `alloy search mcu <query>` (commands/search_cmd.py)
  - [x] Parse query string with AND logic ("USB + 512KB")
  - [x] Call MCUService.search() with intelligent matching
  - [x] Display results table with rank
  - [x] Show search summary and hint

### 1.6 Configuration System (6h) ✅ COMPLETED
- [x] Create `.alloy.yaml` schema
  - [x] Define YAML schema (database/schema/config.schema.yaml)
  - [x] Document all configuration sections (general, paths, discovery, build, validation, metadata, project)
  - [x] Support environment variable expansion (${ALLOY_*:-default})
  - [x] Document configuration hierarchy (CLI > env > project > user > system > defaults)
- [x] Implement Pydantic config models
  - [x] AlloyConfig root model (cli/models/config.py)
  - [x] GeneralConfig, PathsConfig, DiscoveryConfig models
  - [x] BuildConfig, ValidationConfig, MetadataConfig models
  - [x] ProjectConfig model for project-specific settings
  - [x] merge() method for hierarchical config merging
  - [x] to_yaml_dict() for clean YAML export
- [x] Implement ConfigLoader class (cli/loaders/config_loader.py)
  - [x] Support multiple config locations (system, user, project)
  - [x] Hierarchical loading with proper precedence
  - [x] Environment variable support (ALLOY_* prefix)
  - [x] CLI override support
  - [x] LRU caching for performance
  - [x] save() method for writing configs
  - [x] find_project_config() to search up directory tree
  - [x] Global config singleton (get_config, set_config)
- [x] Configuration hierarchy implementation
  - [x] 1. CLI arguments (highest priority)
  - [x] 2. Environment variables (ALLOY_*)
  - [x] 3. Project config (.alloy.yaml in current dir)
  - [x] 4. User config (~/.config/alloy/config.yaml)
  - [x] 5. System config (/etc/alloy/config.yaml)
  - [x] 6. Defaults (lowest priority)
- [x] Environment variable overrides
  - [x] ALLOY_VERBOSE, ALLOY_OUTPUT_FORMAT mapping
  - [x] ALLOY_DATABASE_PATH, ALLOY_TEMPLATES_PATH mapping
  - [x] ALLOY_DEFAULT_VENDOR, ALLOY_BUILD_TYPE mapping
  - [x] Type conversion (string → bool, int)
  - [x] Full mapping documentation
- [x] CLI config commands (cli/commands/config_cmd.py)
  - [x] `alloy config show` - Display merged or scoped config
  - [x] `alloy config show --scope user/project/system/all`
  - [x] `alloy config show --section paths` - Show specific section
  - [x] `alloy config set <key> <value>` - Set config value
  - [x] `alloy config set --scope user/project` - Scope selection
  - [x] `alloy config unset <key>` - Reset to default
  - [x] `alloy config edit` - Open in editor ($EDITOR)
  - [x] `alloy config init` - Create new config file
  - [x] `alloy config init --force` - Overwrite existing
  - [x] `alloy config path` - Show config file locations
  - [x] Rich formatting with tables and syntax highlighting
- [x] Create example config files
  - [x] Project config example (examples/.alloy.yaml)
  - [x] User config example (examples/user-config.yaml)
  - [x] Full configuration documentation
- [x] Create configuration guide
  - [x] Comprehensive guide (docs/guides/CONFIGURATION_GUIDE.md)
  - [x] Document all config options with examples
  - [x] Document configuration hierarchy
  - [x] Document environment variable mapping
  - [x] Common workflows (init project, set preferences, etc.)
  - [x] Team configuration best practices
  - [x] Troubleshooting section
- [x] Integration with CLI
  - [x] Register config command group in main.py
  - [x] Export ConfigLoader in loaders/__init__.py
  - [x] Export config models in models/__init__.py
  - [x] Update help text to reference config system

### 1.7 Enhanced Metadata Commands (6h)
- [ ] Implement `alloy metadata validate`
  - [ ] Validate YAML/JSON syntax
  - [ ] Validate against schema
  - [ ] Check for required fields
  - [ ] Report errors with line numbers
  - [ ] --strict mode for additional checks
- [ ] Implement `alloy metadata create`
  - [ ] --template option (mcu, board, peripheral)
  - [ ] Interactive prompts for required fields
  - [ ] Generate valid YAML with comments
  - [ ] Auto-format output
- [ ] Implement `alloy metadata diff`
  - [ ] Compare two metadata files
  - [ ] Show added/removed/modified fields
  - [ ] Colored diff output
  - [ ] Support for YAML and JSON
- [ ] Better error messages
  - [ ] Suggest fixes for common errors
  - [ ] Link to documentation
  - [ ] Show examples of correct format

### 1.8 Testing (4h)
- [ ] Set up pytest framework
  - [ ] Install pytest and pytest-cov
  - [ ] Configure pytest.ini
  - [ ] Set up test directory structure
- [ ] Write service unit tests
  - [ ] Test MCUService.list() with filters
  - [ ] Test MCUService.show() with valid/invalid IDs
  - [ ] Test MCUService.search() with queries
  - [ ] Test BoardService.list()
  - [ ] Test BoardService.show()
- [ ] Write database validation tests
  - [ ] Test JSON schema validation
  - [ ] Test data integrity (no broken links)
  - [ ] Test index consistency
- [ ] Write CLI integration tests
  - [ ] Test `alloy list mcus` output
  - [ ] Test `alloy show mcu` output
  - [ ] Test error handling
- [ ] Achieve 80% coverage
  - [ ] Run pytest --cov
  - [ ] Identify uncovered code
  - [ ] Add missing tests

**Phase 1 Deliverables**:
- ✅ MCU/board database (YAML files) - Phase 0
- ✅ MCU and Board services (Python) - Phase 1.3, 1.4
- ✅ 5 CLI discovery commands (list mcus, list boards, show mcu, show board, search mcu) - Phase 1.5
- ✅ Configuration system (.alloy.yaml with hierarchy) - Phase 1.6
- ✅ 6 config commands (show, set, unset, edit, init, path) - Phase 1.6
- ⏳ 3 metadata commands (validate, create, diff) - Phase 1.7 (pending)
- ⏳ Test suite (80% coverage) - Phase 1.8 (pending)

**Phase 1 Progress**: 34h/52h completed (65%)

---

## Phase 2: Validation Pipeline (2 weeks, 40 hours)

### 2.1 Syntax Validator (8h)
- [ ] Design SyntaxValidator class
  - [ ] Define validation interface
  - [ ] Define result data structure
- [ ] Implement Clang integration
  - [ ] Check clang++ availability
  - [ ] Build clang command with flags
  - [ ] Add include path resolution
- [ ] Parse Clang output
  - [ ] Parse stderr for errors
  - [ ] Extract error messages
  - [ ] Extract line numbers and context
  - [ ] Categorize errors (syntax, semantic)
- [ ] Create rich error reports
  - [ ] Format errors with Rich
  - [ ] Show code snippets
  - [ ] Highlight error locations
- [ ] Test syntax validator
  - [ ] Test with valid C++ files
  - [ ] Test with syntax errors
  - [ ] Test with missing includes
  - [ ] Test with C++23 features

### 2.2 Semantic Validator (12h)
- [ ] Design SemanticValidator class
  - [ ] Define validation interface
  - [ ] Define SVD cross-reference checks
- [ ] Implement SVD parser
  - [ ] Parse SVD XML structure
  - [ ] Extract peripheral definitions
  - [ ] Extract register offsets
  - [ ] Extract bitfield positions/widths
  - [ ] Build lookup indexes
- [ ] Implement code parser
  - [ ] Parse generated C++ headers
  - [ ] Extract peripheral base addresses
  - [ ] Extract register offsets
  - [ ] Extract bitfield definitions
  - [ ] Build code structure
- [ ] Implement cross-reference validation
  - [ ] Check peripheral base addresses match SVD
  - [ ] Check register offsets match SVD
  - [ ] Check bitfield positions match SVD
  - [ ] Check bitfield widths match SVD
  - [ ] Check peripheral existence in SVD
- [ ] Create detailed error reports
  - [ ] Report mismatched addresses (show both values)
  - [ ] Report missing peripherals
  - [ ] Report incorrect offsets
  - [ ] Suggest fixes
- [ ] Test semantic validator
  - [ ] Test with correct generated code
  - [ ] Test with incorrect addresses
  - [ ] Test with missing peripherals
  - [ ] Test with multiple SVD files

### 2.3 Compile Validator (8h)
- [ ] Design CompileValidator class
  - [ ] Define validation interface
  - [ ] Define test program templates
- [ ] Implement test program generator
  - [ ] Create minimal main() template
  - [ ] Instantiate peripheral templates
  - [ ] Include generated headers
  - [ ] Add compilation flags
- [ ] Integrate ARM GCC
  - [ ] Check arm-none-eabi-gcc availability
  - [ ] Build compilation command
  - [ ] Add MCU-specific flags (cortex-m4, etc.)
  - [ ] Configure include paths
- [ ] Parse GCC output
  - [ ] Parse stderr for errors
  - [ ] Extract error messages
  - [ ] Extract warnings
  - [ ] Get object file size
- [ ] Create compilation reports
  - [ ] Show compilation status
  - [ ] Display errors with context
  - [ ] Show object file size
  - [ ] Show compiler version
- [ ] Test compile validator
  - [ ] Test with valid generated code
  - [ ] Test with compilation errors
  - [ ] Test with different MCU targets
  - [ ] Test with missing dependencies

### 2.4 Test Generator (8h)
- [ ] Design TestGenerator class
  - [ ] Define test template structure
  - [ ] Define test categories
- [ ] Create Catch2 test templates
  - [ ] Template for peripheral base addresses
  - [ ] Template for register offsets
  - [ ] Template for bitfield positions
  - [ ] Template for compile-time checks
- [ ] Implement test generation
  - [ ] Parse generated code structure
  - [ ] Generate base address tests
  - [ ] Generate register offset tests
  - [ ] Generate bitfield tests
  - [ ] Generate static assertion tests
- [ ] Integrate with Catch2
  - [ ] Create test runner
  - [ ] Configure Catch2 build
  - [ ] Add test discovery
- [ ] Test the test generator
  - [ ] Generate tests for GPIO
  - [ ] Generate tests for UART
  - [ ] Verify tests pass with correct code
  - [ ] Verify tests fail with incorrect code

### 2.5 Validation CLI (4h)
- [ ] Implement `alloy codegen validate <file>`
  - [ ] Run syntax validation
  - [ ] Run semantic validation
  - [ ] Run compilation validation
  - [ ] Run generated tests
  - [ ] Display progress bar
  - [ ] Show summary results
- [ ] Implement `alloy codegen validate --all`
  - [ ] Find all generated files
  - [ ] Validate each file
  - [ ] Show progress (N/M files)
  - [ ] Generate summary report
  - [ ] Save results to JSON
- [ ] Add validation options
  - [ ] `--stage <syntax|semantic|compile|test>` (run specific stage)
  - [ ] `--verbose` (show detailed output)
  - [ ] `--json` (output as JSON)
  - [ ] `--fix` (auto-fix issues if possible)
- [ ] Create rich output
  - [ ] Use Rich progress bars
  - [ ] Use Rich panels for results
  - [ ] Color-code pass/fail
  - [ ] Show timing information

**Phase 2 Deliverables**:
- ✅ 4-stage validation pipeline
- ✅ Automated test generation
- ✅ CLI validation commands
- ✅ CI/CD integration ready

---

## Phase 3: Interactive Initialization (2 weeks, 48 hours)

### 3.1 Project Templates (8h)
- [ ] Design template structure
  - [ ] Define template metadata schema
  - [ ] Define template file layout
  - [ ] Define variable substitution rules
- [ ] Create blinky template
  - [ ] Create CMakeLists.txt template
  - [ ] Create src/main.cpp (LED blink)
  - [ ] Create board configuration
  - [ ] Add template metadata
- [ ] Create uart_logger template
  - [ ] Create CMakeLists.txt template
  - [ ] Create src/main.cpp (UART echo)
  - [ ] Configure UART peripheral
  - [ ] Add example logger code
- [ ] Create rtos_tasks template
  - [ ] Create CMakeLists.txt template
  - [ ] Create src/main.cpp (2 tasks)
  - [ ] Configure RTOS
  - [ ] Add task examples
- [ ] Create template registry
  - [ ] List available templates
  - [ ] Template metadata (description, difficulty)
  - [ ] Template dependencies

### 3.2 Wizard Framework (8h)
- [ ] Set up InquirerPy
  - [ ] Install InquirerPy
  - [ ] Configure theme
  - [ ] Test interactive prompts
- [ ] Implement board selection wizard
  - [ ] List available boards
  - [ ] Show board specs preview
  - [ ] Filter by criteria
  - [ ] Confirm selection
- [ ] Implement peripheral selection wizard
  - [ ] List available peripherals for board
  - [ ] Multi-select checkboxes
  - [ ] Show peripheral descriptions
  - [ ] Confirm selections
- [ ] Implement pin configuration wizard
  - [ ] For each selected peripheral:
    - [ ] Show available pin options
    - [ ] Highlight conflicts
    - [ ] Recommend optimal choice
    - [ ] Allow manual override
- [ ] Implement project settings wizard
  - [ ] Project name input
  - [ ] Build system choice (CMake/Meson)
  - [ ] Optimization level
  - [ ] Debug settings

### 3.3 Smart Pin Recommendation (12h)
- [ ] Implement conflict detection
  - [ ] Track assigned pins
  - [ ] Detect shared pins (LED, button, ST-LINK)
  - [ ] Check alternate function conflicts
  - [ ] Validate pin availability
- [ ] Implement recommendation engine
  - [ ] Score pin options (0-100)
  - [ ] Prefer recommended pins from board JSON
  - [ ] Avoid conflicts
  - [ ] Prefer proximity (SPI pins together)
  - [ ] Consider signal integrity
- [ ] Implement alternate suggestions
  - [ ] Find all valid pin combinations
  - [ ] Rank by score
  - [ ] Show top 3 alternatives
  - [ ] Explain trade-offs
- [ ] Create visual pin display
  - [ ] Show board outline (ASCII art)
  - [ ] Highlight available pins (green)
  - [ ] Highlight conflicts (red)
  - [ ] Show selected pins (blue)
  - [ ] Add pin labels
- [ ] Test recommendation engine
  - [ ] Test with single peripheral
  - [ ] Test with multiple peripherals
  - [ ] Test with conflicting requests
  - [ ] Test with all pins assigned

### 3.4 Project Generator (12h)
- [ ] Implement ProjectGenerator class
  - [ ] Define project structure
  - [ ] Implement template rendering
  - [ ] Implement file creation
- [ ] Generate directory structure
  - [ ] Create src/, include/, boards/, cmake/
  - [ ] Copy board files
  - [ ] Copy toolchain files
  - [ ] Create build/ directory
- [ ] Generate CMakeLists.txt
  - [ ] Render from template
  - [ ] Substitute variables (board, MCU, etc.)
  - [ ] Add peripheral libraries
  - [ ] Configure compiler flags
  - [ ] Add custom targets (flash, size)
- [ ] Generate src/main.cpp
  - [ ] Render from template
  - [ ] Include peripheral headers
  - [ ] Add peripheral initialization
  - [ ] Add template-specific code
  - [ ] Add comments
- [ ] Generate board configuration
  - [ ] Copy board.hpp
  - [ ] Generate peripheral configurations
  - [ ] Add pin definitions
  - [ ] Add clock configuration
- [ ] Generate peripheral drivers
  - [ ] Run code generation for selected peripherals
  - [ ] Validate generated code
  - [ ] Add to CMake build
- [ ] Create .gitignore
  - [ ] Ignore build/
  - [ ] Ignore IDE files
  - [ ] Ignore generated binaries

### 3.5 CLI Commands (4h)
- [ ] Implement `alloy init`
  - [ ] Run interactive wizard
  - [ ] Collect user inputs
  - [ ] Call ProjectGenerator
  - [ ] Show progress
  - [ ] Display success message with next steps
- [ ] Implement `alloy init --template <name>`
  - [ ] Skip template selection
  - [ ] Run board wizard
  - [ ] Generate project
- [ ] Implement `alloy init --board <name> --template <name>`
  - [ ] Skip all wizards
  - [ ] Use defaults
  - [ ] Generate project immediately
- [ ] Implement `alloy config peripheral add`
  - [ ] Run in existing project
  - [ ] Select peripheral
  - [ ] Configure pins
  - [ ] Regenerate code
  - [ ] Update CMakeLists.txt

### 3.6 Testing (4h)
- [ ] Write wizard unit tests
  - [ ] Test board selection
  - [ ] Test peripheral selection
  - [ ] Test pin configuration
  - [ ] Mock InquirerPy prompts
- [ ] Write template validation tests
  - [ ] Validate template metadata
  - [ ] Validate template syntax
  - [ ] Test variable substitution
- [ ] Write integration tests
  - [ ] Test full init workflow
  - [ ] Verify project structure
  - [ ] Compile generated project
  - [ ] Flash to board (if available)
- [ ] Write end-to-end tests
  - [ ] Test blinky template → compile → flash
  - [ ] Test uart template → compile
  - [ ] Test rtos template → compile

**Phase 3 Deliverables**:
- ✅ Interactive wizard
- ✅ 3 project templates (blinky, uart, rtos)
- ✅ Smart pin recommendation engine
- ✅ Project generator
- ✅ 4 new CLI commands

---

## Phase 4: Build Integration (1 week, 24 hours)

### 4.1 Build Service (8h)
- [ ] Design BuildService class
  - [ ] Define build abstraction interface
  - [ ] Support CMake and Meson
- [ ] Implement build system detection
  - [ ] Check for CMakeLists.txt
  - [ ] Check for meson.build
  - [ ] Validate project structure
- [ ] Implement CMake integration
  - [ ] Run cmake configure
  - [ ] Run cmake build
  - [ ] Parse cmake output
  - [ ] Extract build errors
- [ ] Implement Meson integration (optional)
  - [ ] Run meson setup
  - [ ] Run meson compile
  - [ ] Parse meson output
  - [ ] Extract build errors
- [ ] Add progress tracking
  - [ ] Parse build progress (N/M files)
  - [ ] Show Rich progress bar
  - [ ] Estimate time remaining
- [ ] Add build caching
  - [ ] Detect incremental builds
  - [ ] Skip unnecessary rebuilds
  - [ ] Show cache hit rate

### 4.2 Flash Integration (8h)
- [ ] Design FlashService class
  - [ ] Define flash interface
  - [ ] Support multiple programmers
- [ ] Implement OpenOCD integration
  - [ ] Check openocd availability
  - [ ] Load board-specific config
  - [ ] Run flash command
  - [ ] Parse openocd output
  - [ ] Detect flash errors
- [ ] Implement ST-Link support
  - [ ] Detect ST-Link programmer
  - [ ] Configure for board
  - [ ] Flash binary
  - [ ] Verify flash
- [ ] Implement J-Link support (optional)
  - [ ] Detect J-Link programmer
  - [ ] Configure for MCU
  - [ ] Flash binary
- [ ] Add progress reporting
  - [ ] Show flash progress
  - [ ] Show verify progress
  - [ ] Estimate time remaining
- [ ] Add error handling
  - [ ] Detect connection failures
  - [ ] Detect flash failures
  - [ ] Suggest troubleshooting steps

### 4.3 CLI Commands (6h)
- [ ] Implement `alloy build configure`
  - [ ] Detect build system
  - [ ] Run configuration
  - [ ] Show configuration summary
  - [ ] Detect errors
- [ ] Implement `alloy build compile`
  - [ ] Detect build system
  - [ ] Run compilation
  - [ ] Show progress bar
  - [ ] Show build summary (size, time)
  - [ ] Handle errors gracefully
- [ ] Implement `alloy build flash`
  - [ ] Compile if needed
  - [ ] Detect programmer
  - [ ] Flash binary
  - [ ] Verify flash
  - [ ] Show success message
- [ ] Implement `alloy build size`
  - [ ] Run size analysis
  - [ ] Show memory usage (text, data, bss)
  - [ ] Show flash/RAM usage percentages
  - [ ] Warn if over limits
- [ ] Implement `alloy build clean`
  - [ ] Remove build directory
  - [ ] Remove generated files
  - [ ] Show cleaned files

### 4.4 Testing (2h)
- [ ] Write build service unit tests
  - [ ] Test build system detection
  - [ ] Test CMake integration (mocked)
  - [ ] Test Meson integration (mocked)
  - [ ] Test error parsing
- [ ] Write flash service unit tests
  - [ ] Test OpenOCD integration (mocked)
  - [ ] Test ST-Link integration (mocked)
  - [ ] Test error handling
- [ ] Write integration tests
  - [ ] Test full build workflow
  - [ ] Test incremental builds
  - [ ] Test clean builds
- [ ] Write CLI tests
  - [ ] Test `alloy build compile`
  - [ ] Test `alloy build flash` (mocked)
  - [ ] Test `alloy build size`

**Phase 4 Deliverables**:
- ✅ Build abstraction layer (CMake + Meson)
- ✅ Flash integration (OpenOCD + ST-Link)
- ✅ 5 build commands
- ✅ Tests

---

## Phase 5: Documentation & Pinouts (1 week, 24 hours)

### 5.1 Pinout Renderer (12h)
- [ ] Design ASCII art pinout format
  - [ ] Define board outline
  - [ ] Define pin layout
  - [ ] Define color scheme
- [ ] Implement PinoutRenderer class
  - [ ] Parse board JSON pinout
  - [ ] Render connector layout
  - [ ] Render pin labels
  - [ ] Render pin functions
  - [ ] Add color highlighting
- [ ] Implement interactive features
  - [ ] Pin search/filter
  - [ ] Highlight peripheral pins
  - [ ] Show alternate functions
  - [ ] Toggle between views (physical/logical)
- [ ] Create pinout templates
  - [ ] Arduino Uno layout
  - [ ] Nucleo-64 layout
  - [ ] Xplained Pro layout
- [ ] Add pinout for all boards
  - [ ] nucleo_f401re pinout
  - [ ] same70_xplained pinout
  - [ ] nucleo_g071rb pinout
- [ ] Test pinout renderer
  - [ ] Test ASCII art generation
  - [ ] Test color output
  - [ ] Test interactive features
  - [ ] Test with different terminals

### 5.2 Documentation Service (6h)
- [ ] Create datasheet URL database
  - [ ] Collect all MCU datasheets
  - [ ] Collect reference manuals
  - [ ] Collect errata sheets
  - [ ] Collect application notes
- [ ] Implement DocumentationService class
  - [ ] Load documentation database
  - [ ] Search by MCU/peripheral
  - [ ] Open URLs in browser
- [ ] Generate API documentation
  - [ ] Extract API from headers (Doxygen)
  - [ ] Generate HTML docs
  - [ ] Host locally or online
  - [ ] Create index
- [ ] Add example code browser
  - [ ] List available examples
  - [ ] Show example source
  - [ ] Explain example purpose
  - [ ] Link to relevant docs

### 5.3 CLI Commands (4h)
- [ ] Implement `alloy show pinout <board>`
  - [ ] Load board pinout
  - [ ] Render ASCII art
  - [ ] Display in terminal
  - [ ] Add interactive mode
- [ ] Implement `alloy docs datasheet <mcu>`
  - [ ] Find datasheet URL
  - [ ] Open in browser
  - [ ] Show quick specs in terminal
- [ ] Implement `alloy docs api <peripheral>`
  - [ ] Find API documentation
  - [ ] Open in browser
  - [ ] Show quick reference in terminal
- [ ] Implement `alloy examples list`
  - [ ] List available examples
  - [ ] Filter by board/peripheral
  - [ ] Show example descriptions
- [ ] Implement `alloy examples show <name>`
  - [ ] Display example source
  - [ ] Show usage instructions
  - [ ] Link to full documentation

### 5.4 Testing (2h)
- [ ] Write pinout renderer tests
  - [ ] Test ASCII art generation
  - [ ] Test color codes
  - [ ] Test layout accuracy
- [ ] Write documentation service tests
  - [ ] Test URL lookup
  - [ ] Test browser integration (mocked)
  - [ ] Test API doc generation
- [ ] Write CLI tests
  - [ ] Test `alloy show pinout`
  - [ ] Test `alloy docs datasheet`
  - [ ] Test `alloy examples list`

**Phase 5 Deliverables**:
- ✅ ASCII art pinout display
- ✅ Documentation integration (datasheets, API docs)
- ✅ Example code browser
- ✅ 5 documentation commands

---

## Phase 6: Polish & Optimization (Optional, 16 hours)

### 6.1 Performance Optimization (6h)
- [ ] Profile CLI performance
  - [ ] Measure command latency
  - [ ] Identify slow operations
  - [ ] Measure database load time
- [ ] Implement database indexing
  - [ ] Create hash-based indexes
  - [ ] Create search indexes
  - [ ] Test index performance
- [ ] Implement lazy loading
  - [ ] Load summary data only
  - [ ] Load details on demand
  - [ ] Cache frequently accessed data
- [ ] Implement caching
  - [ ] Cache database queries
  - [ ] Cache SVD parsing
  - [ ] Cache validation results
  - [ ] Add cache invalidation
- [ ] Optimize validation pipeline
  - [ ] Parallelize validation stages
  - [ ] Cache compilation results
  - [ ] Skip unchanged files

### 6.2 Error Messages (4h)
- [ ] Improve error messages
  - [ ] Make errors actionable
  - [ ] Add suggestions
  - [ ] Show context
  - [ ] Use friendly language
- [ ] Add troubleshooting hints
  - [ ] Detect common issues
  - [ ] Suggest fixes
  - [ ] Link to documentation
- [ ] Implement recovery suggestions
  - [ ] Offer auto-fix options
  - [ ] Suggest alternative actions
  - [ ] Show related commands
- [ ] Test error messages
  - [ ] Trigger all error paths
  - [ ] Verify message quality
  - [ ] User test error UX

### 6.3 Documentation (4h)
- [ ] Write CLI usage guide
  - [ ] Getting started tutorial
  - [ ] Command reference
  - [ ] Common workflows
  - [ ] Troubleshooting
- [ ] Create video tutorials
  - [ ] Quick start (5 min)
  - [ ] Project initialization (10 min)
  - [ ] Advanced features (15 min)
- [ ] Write migration guide
  - [ ] From old codegen.py
  - [ ] Step-by-step instructions
  - [ ] Common pitfalls
- [ ] Update README
  - [ ] Add CLI section
  - [ ] Add screenshots
  - [ ] Add examples

### 6.4 Final Testing (2h)
- [ ] Run end-to-end tests
  - [ ] Full workflow tests
  - [ ] Multi-board tests
  - [ ] Edge case tests
- [ ] User acceptance testing
  - [ ] Test with real users
  - [ ] Collect feedback
  - [ ] Fix critical issues
- [ ] Performance benchmarks
  - [ ] Measure command latency
  - [ ] Measure validation speed
  - [ ] Measure build time
- [ ] CI/CD validation
  - [ ] Test on Ubuntu
  - [ ] Test on macOS
  - [ ] Test on Windows

**Phase 6 Deliverables**:
- ✅ Performance optimizations
- ✅ Improved error messages
- ✅ Complete documentation
- ✅ Video tutorials

---

## Summary

**Total Tasks**: 150+
**Total Estimated Time**: 176 hours (8 weeks)
**Complexity**: HIGH
**Priority**: HIGH

**Critical Path**:
1. Phase 1 (Foundation) → Phase 2 (Validation)
2. Phase 2 (Validation) → Phase 3 (Initialization)
3. Phase 3 (Initialization) → Phase 4 (Build)
4. Phase 4 (Build) → Phase 5 (Docs)
5. Phase 5 (Docs) → Phase 6 (Polish)

**Parallelization Opportunities**:
- Phase 2 and Phase 3 can partially overlap (test generator can wait)
- Phase 5 can start before Phase 4 completes
- Database population (1.2) can happen in parallel with service development (1.3-1.4)

**Risk Mitigation**:
- Validate after each phase before proceeding
- Get user feedback early (after Phase 1)
- Keep backward compatibility throughout
- Maintain test coverage >80% at all times

**Success Criteria**:
- All functional requirements met (FR1-FR5)
- All non-functional requirements met (NFR1-NFR4)
- 80% test coverage achieved
- User satisfaction surveys positive
- Zero critical bugs in production
