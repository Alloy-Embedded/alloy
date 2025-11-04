# Alloy CLI Internal Structure

This directory contains the internal implementation of the Alloy CLI.

## Directory Structure

```
cli/
├── commands/           # CLI command implementations
│   ├── codegen.py     # Code generation command
│   ├── status.py      # Status report command
│   └── vendors.py     # Vendor listing command
├── core/              # Core utilities
│   ├── logger.py      # Colored logging with emojis
│   └── version.py     # Version information
├── generators/        # Generic code generators
│   ├── generate_all.py
│   ├── generate_mcu_status.py
│   ├── generate_support_matrix.py
│   └── generator.py
├── parsers/           # SVD parsers and utilities
│   ├── list_svds.py
│   ├── svd_discovery.py
│   ├── svd_parser.py
│   └── svd_pin_extractor.py
└── vendors/           # Vendor-specific modules
    └── st/            # STMicroelectronics
        ├── generate_all_st_pins.py
        ├── generate_pin_functions.py
        ├── generate_pins_from_svd.py
        ├── generate_stm32_pins.py
        ├── stm32f1_pin_functions.py
        ├── stm32f4_pin_functions.py
        └── stm32f7_pin_functions.py
```

## Module Responsibilities

### commands/
Contains the implementation of CLI commands. Each command:
- Defines argument parsing via `setup_parser(parser)`
- Implements execution logic via `execute(args)`
- Returns 0 on success, non-zero on failure

### core/
Core utilities used across the CLI:
- **logger.py**: Provides colored logging functions with emojis
  - `print_success()`, `print_error()`, `print_warning()`, `print_info()`
  - `print_header()` for section headers
  - ANSI color codes and emoji icons
- **version.py**: Version information (currently v1.0.0)

### generators/
Generic code generators that can work with multiple vendors:
- **generate_all.py**: Batch code generator from JSON databases
- **generate_mcu_status.py**: Implementation status report generator
- **generate_support_matrix.py**: Support matrix generator
- **generator.py**: Core code generator using Jinja2 templates

### parsers/
SVD file parsers and discovery utilities:
- **svd_discovery.py**: Multi-source SVD file discovery with merge policy
- **svd_parser.py**: CMSIS-SVD XML parser to JSON
- **svd_pin_extractor.py**: Extract pin information from SVD files
- **list_svds.py**: List available SVD files

### vendors/
Vendor-specific code generators organized by manufacturer:

#### vendors/st/ (STMicroelectronics)
- **generate_all_st_pins.py**: Main entry point for ST code generation
- **generate_pin_functions.py**: Generate pin function headers
- **generate_pins_from_svd.py**: Generate pin definitions from SVD
- **generate_stm32_pins.py**: STM32-specific pin generator
- **stm32fX_pin_functions.py**: Pin function databases for each family

## Adding New Vendors

To add support for a new vendor (e.g., Atmel):

1. **Create vendor directory**:
   ```bash
   mkdir -p cli/vendors/atmel
   touch cli/vendors/atmel/__init__.py
   ```

2. **Create generator**:
   ```python
   # cli/vendors/atmel/generate_atmel_pins.py
   def main():
       """Generate code for Atmel MCUs"""
       # Implementation here
       return 0
   ```

3. **Update codegen command**:
   ```python
   # cli/commands/codegen.py
   def generate_atmel(args):
       from cli.vendors.atmel.generate_atmel_pins import main
       return main()
   ```

4. **Update vendors list**:
   ```python
   # cli/commands/vendors.py
   VENDORS = {
       'atmel': {
           'name': 'Atmel/Microchip',
           'status': '✅ Supported',
           'families': [...]
       }
   }
   ```

## Import Conventions

All imports use absolute imports from the `cli` package:

```python
# From commands/
from cli.core.logger import print_success
from cli.vendors.st.generate_all_st_pins import main

# From vendors/st/
from cli.parsers.svd_discovery import discover_all_svds
from cli.vendors.st.stm32f4_pin_functions import get_pin_functions

# From generators/
from cli.parsers.svd_discovery import discover_all_svds
```

## Path Resolution

When referencing repository paths, use:

```python
from pathlib import Path

# In cli/vendors/st/ (3 levels deep)
REPO_ROOT = Path(__file__).parent.parent.parent.parent

# In cli/generators/ (2 levels deep)
REPO_ROOT = Path(__file__).parent.parent.parent.parent

# In cli/parsers/ (2 levels deep)
CODEGEN_DIR = Path(__file__).parent.parent.parent
```

This ensures all modules correctly resolve paths to:
- `src/hal/vendors/` - Generated code output
- `upstream/cmsis-svd-data/` - SVD files
- `database/` - JSON databases

## Testing

Test the CLI after making changes:

```bash
# Test all commands
./alloy --version
./alloy vendors
./alloy status
./alloy codegen --dry-run

# Test specific vendor
./alloy codegen --vendor st --dry-run
```

## Code Style

- Use docstrings for all modules and functions
- Use type hints where appropriate
- Follow PEP 8 naming conventions
- Use colored logging from `cli.core.logger`
- Return 0 on success, non-zero on failure

## Version History

- **v1.0.0** (2025-11-03) - Initial CLI release
  - Unified command-line interface
  - STMicroelectronics support
  - Status reporting
  - Vendor listing
