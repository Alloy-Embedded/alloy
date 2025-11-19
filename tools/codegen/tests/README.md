# Alloy CLI Tests

Comprehensive test suite for the Alloy CLI tool.

## Structure

```
tests/
├── conftest.py           # Shared pytest fixtures
├── unit/                 # Unit tests for individual components
│   └── test_services.py  # Tests for MetadataService, ConfigLoader
├── integration/          # Integration tests for CLI commands
│   └── test_cli_commands.py
└── fixtures/             # Test data and fixtures
```

## Running Tests

### All Tests
```bash
cd tools/codegen
pytest
```

### Unit Tests Only
```bash
pytest tests/unit/
```

### Integration Tests Only
```bash
pytest tests/integration/
```

### With Coverage
```bash
pytest --cov=cli --cov-report=html
```

### Specific Test
```bash
pytest tests/unit/test_services.py::TestMetadataService::test_validate_valid_mcu_yaml -v
```

## Test Markers

- `@pytest.mark.unit` - Unit tests (fast, no I/O)
- `@pytest.mark.integration` - Integration tests (CLI commands)
- `@pytest.mark.slow` - Slow running tests
- `@pytest.mark.requires_network` - Tests requiring network access

### Run Specific Markers
```bash
pytest -m unit          # Only unit tests
pytest -m integration   # Only integration tests
pytest -m "not slow"    # Skip slow tests
```

## Coverage Goals

- **Target**: 80% code coverage
- **Current**: Run `pytest --cov` to see current coverage
- **HTML Report**: Open `htmlcov/index.html` after running with `--cov-report=html`

## Writing Tests

### Unit Test Example
```python
import pytest

class TestMyService:
    def test_something(self):
        """Test description."""
        result = my_function()
        assert result == expected
```

### Using Fixtures
```python
def test_with_temp_dir(temp_dir):
    """Temp dir is auto-created and cleaned up."""
    test_file = temp_dir / "test.yaml"
    # ... test code ...
```

### Integration Test Example
```python
from typer.testing import CliRunner
from cli.main import app

runner = CliRunner()

def test_cli_command():
    result = runner.invoke(app, ["config", "show"])
    assert result.exit_code == 0
```

## Available Fixtures

- `temp_dir` - Temporary directory (auto-cleanup)
- `fixtures_dir` - Path to fixtures directory
- `sample_mcu_yaml` - Sample MCU metadata YAML file
- `sample_board_yaml` - Sample board metadata YAML file
- `sample_config_yaml` - Sample config YAML file
- `example_database` - Example MCU database dict
- `example_database_file` - Example database as JSON file

## Continuous Integration

Tests run automatically on:
- Push to main branch
- Pull requests
- Pre-commit hooks (if configured)

Minimum requirements:
- All tests must pass
- Coverage must be >= 80%
- No lint errors

## Troubleshooting

### Import Errors
Make sure you're in the `tools/codegen` directory when running tests.

### Coverage Too Low
```bash
# See which files need more tests
pytest --cov=cli --cov-report=term-missing
```

### Fixture Not Found
Check that the fixture is defined in `conftest.py` or imported properly.

### Tests Failing Locally
```bash
# Run with verbose output
pytest -vv

# Run specific failing test
pytest tests/unit/test_services.py::test_name -vv
```
