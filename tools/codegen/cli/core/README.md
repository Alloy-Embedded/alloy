# Alloy Codegen Core API

**Status**: Phase 4 Complete ✅
**Owner**: library-quality-improvements spec
**Consumed by**: CLI spec (enhance-cli-professional-tool)

This directory contains the core functionality for Alloy's code generation system.

---

## Directory Structure

```
cli/core/
├── __init__.py              # Core module exports
├── svd_parser.py            # SVD XML parsing
├── template_engine.py       # Jinja2 template rendering
├── schema_validator.py      # JSON/YAML schema validation
├── file_utils.py            # File I/O utilities
├── validators/              # Code validators (for CLI ValidationService)
│   ├── __init__.py
│   ├── syntax_validator.py     # Clang syntax checking
│   ├── semantic_validator.py   # SVD cross-reference validation
│   ├── compile_validator.py    # ARM GCC compilation
│   └── test_validator.py       # Test generation and execution
└── README.md                # This file
```

---

## Core Modules

### 1. SVD Parser (`svd_parser.py`)

Parses CMSIS-SVD files to extract peripheral/register definitions.

**Usage**:
```python
from cli.core import svd_parser

# Parse SVD file
svd = svd_parser.parse("path/to/STM32F401.svd")

# Access peripherals
gpio_a = svd.peripherals['GPIOA']
print(f"Base address: {gpio_a.base_address:#x}")

# Access registers
moder = gpio_a.registers['MODER']
print(f"Offset: {moder.offset:#x}")
```

**For CLI team**: Use this for metadata extraction commands.

---

### 2. Template Engine (`template_engine.py`)

Jinja2-based template rendering with custom filters for code generation.

**Usage**:
```python
from cli.core import TemplateEngine
from pathlib import Path

# Initialize engine
engine = TemplateEngine(template_dir=Path("tools/codegen/templates"))

# Render template
context = {
    'platform': {'name': 'STM32F4'},
    'gpio_ports': [{'name': 'A', 'pin_count': 16}]
}

output = engine.render("platform/gpio.hpp.j2", context)
```

**Custom Filters**:
- `sanitize` - Remove unsafe characters
- `format_hex` - Format as hex (0x1234)
- `cpp_type` - Convert to C++ type
- `to_pascal_case` - Convert to PascalCase
- `to_snake_case` - Convert to snake_case
- `to_upper_snake` - Convert to UPPER_SNAKE
- `parse_bit_range` - Parse bit field ranges
- `calculate_mask` - Calculate bit masks

**For CLI team**:
- This spec owns **peripheral templates** (gpio.hpp.j2, uart.hpp.j2, etc.)
- CLI spec will add **project templates** (blinky.cpp.j2, uart_logger.cpp.j2, etc.)
- Both use the same TemplateEngine

---

### 3. Schema Validator (`schema_validator.py`)

Validates metadata files against JSON schemas.

**Usage**:
```python
from cli.core import SchemaValidator, SchemaFormat
from pathlib import Path

# Initialize validator
validator = SchemaValidator(schema_dir=Path("tools/codegen/schemas"))

# Validate JSON file
result = validator.validate_file(
    file_path=Path("metadata/platforms/stm32f4.json"),
    schema_name="platform",
    format=SchemaFormat.JSON
)

if result.passed:
    print("✅ Validation passed")
else:
    print(f"❌ {result.message}")
    for error in result.errors:
        print(f"  - {error}")
```

**For CLI team**:
- Extend this for YAML validation (Phase 0)
- Add `SchemaFormat.YAML` support
- Create YAML-specific schemas

**Extension Point**:
```python
# CLI spec will add:
class YAMLSchemaValidator(SchemaValidator):
    def _load_yaml(self, file_path: Path) -> Dict:
        import yaml
        with open(file_path) as f:
            return yaml.safe_load(f)
```

---

### 4. File Utilities (`file_utils.py`)

Common file I/O operations.

**Usage**:
```python
from cli.core import (
    ensure_directory,
    write_text_file,
    find_files,
    file_hash
)
from pathlib import Path

# Create output directory
output_dir = ensure_directory(Path("build/generated"))

# Write generated code
write_text_file(
    path=output_dir / "gpio.hpp",
    content=generated_code,
    create_dirs=True
)

# Find all generated files
cpp_files = find_files(output_dir, "*.cpp", recursive=True)

# Verify file integrity
hash_value = file_hash(output_dir / "gpio.hpp")
```

**Functions**:
- `ensure_directory(path)` - Create dir if doesn't exist
- `clean_directory(path, pattern)` - Remove matching files
- `copy_file(src, dst)` - Copy file
- `find_files(dir, pattern)` - Find files matching pattern
- `read_text_file(path)` - Read text file
- `write_text_file(path, content)` - Write text file
- `file_hash(path)` - Calculate SHA256 hash
- `files_identical(file1, file2)` - Compare files by hash

---

## Validators Submodule

### Overview

The `validators/` submodule provides core validation infrastructure.

**Owned by**: library-quality-improvements spec
**Consumed by**: CLI ValidationService wrapper

### 1. Syntax Validator

Validates C++ syntax using Clang.

**Usage**:
```python
from cli.core.validators import SyntaxValidator

validator = SyntaxValidator(clang_path="clang++")

# Validate code string
code = """
#include <cstdint>
int main() { return 0; }
"""

result = validator.validate(code, std="c++23")
print(f"Syntax check: {result.passed} - {result.message}")

# Validate file
result = validator.validate_file(
    Path("src/hal/gpio.cpp"),
    std="c++23"
)
```

**For CLI team**: Wrap in ValidationService for `alloy validate syntax` command.

---

### 2. Semantic Validator

Cross-references generated code against SVD files.

**Usage**:
```python
from cli.core.validators import SemanticValidator

validator = SemanticValidator()

result = validator.validate(
    generated_file=Path("src/hal/vendors/st/stm32f4/gpio.hpp"),
    svd_file=Path("tools/codegen/svd/STM32F401.svd")
)

if result.passed:
    print("✅ Addresses and offsets match SVD")
else:
    print(f"❌ {result.message}")
```

**Checks**:
- Peripheral base addresses match SVD
- Register offsets match SVD
- Bit field positions match SVD

**For CLI team**: Wrap in ValidationService for `alloy validate semantic` command.

---

### 3. Compile Validator

Compiles generated code with ARM GCC.

**Usage**:
```python
from cli.core.validators import CompileValidator
from pathlib import Path

validator = CompileValidator(gcc_path="arm-none-eabi-gcc")

result = validator.validate(
    source_files=[Path("src/hal/gpio.cpp")],
    target="STM32F401xE",
    include_dirs=[Path("src/hal/api"), Path("src/core")]
)

if result.passed:
    print("✅ Compilation successful")
else:
    print(f"❌ {result.message}")
```

**For CLI team**: Wrap in ValidationService for `alloy validate compile` command.

---

### 4. Test Validator

Generates and runs Catch2 unit tests.

**Usage**:
```python
from cli.core.validators import TestValidator

validator = TestValidator()

metadata = {
    'platform': {'name': 'STM32F4'},
    'gpio_ports': [{'name': 'A', 'pin_count': 16}]
}

result = validator.validate('gpio', metadata)
print(f"Test generation: {result.passed} - {result.message}")
```

**Generates**:
- Catch2 test cases
- Compile-time validation tests
- Runtime behavior tests

**For CLI team**: Wrap in ValidationService for `alloy validate test` command.

---

## CLI Integration Guide

### ValidationService Wrapper (CLI Spec Owns)

The CLI spec will create a ValidationService that wraps these validators:

```python
# tools/codegen/cli/services/validation_service.py (CLI spec creates this)

from cli.core.validators import (
    SyntaxValidator,
    SemanticValidator,
    CompileValidator,
    TestValidator
)

class ValidationService:
    """
    CLI wrapper around core validators.
    Owned by: enhance-cli-professional-tool spec
    """

    def __init__(self):
        self.syntax = SyntaxValidator()
        self.semantic = SemanticValidator()
        self.compile = CompileValidator()
        self.test = TestValidator()

    def validate_all(self, files: List[Path], strict: bool = False):
        """Run all validators (CLI command: alloy metadata validate)"""
        results = []

        # Syntax check
        for file in files:
            result = self.syntax.validate_file(file)
            results.append(('syntax', file, result))

        # Semantic check (if SVD available)
        # ...

        # Compilation check
        result = self.compile.validate(files, target=...)
        results.append(('compile', None, result))

        return results
```

### CLI Commands Using Core API

**Metadata Commands** (CLI spec):
```python
# alloy metadata list
from cli.core import svd_parser

def cmd_metadata_list():
    svds = find_svd_files()
    for svd_path in svds:
        svd = svd_parser.parse(svd_path)
        print(f"Platform: {svd.device_name}")

# alloy metadata validate
from cli.services import ValidationService

def cmd_metadata_validate():
    service = ValidationService()
    results = service.validate_all(generated_files)
```

**Template Commands** (CLI spec):
```python
# alloy new <template>
from cli.core import TemplateEngine

def cmd_new_project(template_name: str):
    engine = TemplateEngine(Path("tools/codegen/templates"))

    # Use project templates (CLI owns these)
    output = engine.render(f"projects/{template_name}.cpp.j2", context)
    write_text_file(Path("src/main.cpp"), output)
```

---

## Testing

Run tests to verify core modules work:

```bash
# Test imports
python3 -c "from cli.core import TemplateEngine, SchemaValidator; print('✅ Imports OK')"

# Test validators
python3 -c "from cli.core.validators import SyntaxValidator, SemanticValidator; print('✅ Validators OK')"

# Run unit tests
cd tools/codegen
pytest tests/ -v -k "core"
```

---

## Coordination Checkpoints

### ✅ Checkpoint 1: Validator Interfaces Stable
**Status**: Complete
**Deliverable**: All 4 validators implemented with stable APIs
**CLI Action**: Review validator interfaces, provide feedback

### ✅ Checkpoint 2: Template Engine API Documented
**Status**: Complete
**Deliverable**: TemplateEngine class with custom filters documented
**CLI Action**: Plan project template integration

### ✅ Checkpoint 3: Schema Validator Extension Point
**Status**: Complete
**Deliverable**: SchemaValidator with extension point for YAML
**CLI Action**: Implement YAML support in Phase 0

---

## Next Steps for CLI Team

Now that Phase 4 is complete, CLI can start **Phase 0 (YAML Migration)**:

1. ✅ **Add PyYAML**: `pip install pyyaml` to requirements.txt
2. ✅ **Extend SchemaValidator**: Add YAML loading support
3. ✅ **Create YAML schemas**: Define schemas for platforms, boards, etc.
4. ✅ **Implement metadata commands**: Use svd_parser to extract metadata
5. ✅ **Create ValidationService**: Wrap core validators
6. ✅ **Add project templates**: Create templates/projects/ directory

---

## Questions?

- See `openspec/changes/library-quality-improvements/CLI_PREREQUISITE.md` for prerequisite details
- See `openspec/changes/INTEGRATION_LIBRARY_CLI.md` for coordination plan
- Contact library quality team for API questions

**Phase 4 is now complete! CLI development can proceed! 🚀**
