"""
Schema Validator

JSON/YAML schema validation for metadata files.

Owned by: library-quality-improvements spec
Extended by: CLI spec (for YAML schemas)
"""

import json
from pathlib import Path
from typing import Dict, Any, Optional
from dataclasses import dataclass
from enum import Enum

try:
    import jsonschema
    from jsonschema import validate, ValidationError
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False


class SchemaFormat(Enum):
    """Supported schema formats"""
    JSON = "json"
    YAML = "yaml"  # Will be implemented by CLI spec


@dataclass
class SchemaValidationResult:
    """Result of schema validation"""
    passed: bool
    message: str
    errors: list[str] = None
    file_path: Optional[Path] = None

    def __post_init__(self):
        if self.errors is None:
            self.errors = []


class SchemaValidator:
    """
    Core schema validator for metadata files.

    Validates JSON metadata against JSON schemas.
    The CLI spec will extend this to support YAML validation.
    """

    def __init__(self, schema_dir: Path):
        """
        Initialize schema validator.

        Args:
            schema_dir: Directory containing JSON schema files
        """
        self.schema_dir = schema_dir
        self.schema_cache: Dict[str, Dict] = {}

        if not HAS_JSONSCHEMA:
            raise ImportError(
                "jsonschema library not installed. "
                "Run: pip install jsonschema"
            )

    def load_schema(self, schema_name: str) -> Dict:
        """
        Load a JSON schema file.

        Args:
            schema_name: Name of schema (without .json extension)

        Returns:
            Schema dictionary

        Raises:
            FileNotFoundError: If schema file doesn't exist
        """
        # Check cache
        if schema_name in self.schema_cache:
            return self.schema_cache[schema_name]

        # Load from file
        schema_path = self.schema_dir / f"{schema_name}.json"

        if not schema_path.exists():
            raise FileNotFoundError(f"Schema not found: {schema_path}")

        with open(schema_path) as f:
            schema = json.load(f)

        # Cache it
        self.schema_cache[schema_name] = schema

        return schema

    def validate_file(
        self,
        file_path: Path,
        schema_name: str,
        format: SchemaFormat = SchemaFormat.JSON
    ) -> SchemaValidationResult:
        """
        Validate a metadata file against a schema.

        Args:
            file_path: Path to metadata file
            schema_name: Name of schema to validate against
            format: File format (JSON or YAML)

        Returns:
            SchemaValidationResult
        """
        if not file_path.exists():
            return SchemaValidationResult(
                passed=False,
                message=f"File not found: {file_path}",
                file_path=file_path
            )

        try:
            # Load schema
            schema = self.load_schema(schema_name)

            # Load data based on format
            if format == SchemaFormat.JSON:
                data = self._load_json(file_path)
            elif format == SchemaFormat.YAML:
                # This will be implemented by CLI spec
                return SchemaValidationResult(
                    passed=False,
                    message="YAML validation not yet implemented (will be added by CLI spec)",
                    file_path=file_path
                )
            else:
                return SchemaValidationResult(
                    passed=False,
                    message=f"Unsupported format: {format}",
                    file_path=file_path
                )

            # Validate
            validate(instance=data, schema=schema)

            return SchemaValidationResult(
                passed=True,
                message=f"Schema validation passed: {schema_name}",
                file_path=file_path
            )

        except ValidationError as e:
            return SchemaValidationResult(
                passed=False,
                message=f"Schema validation failed",
                errors=[str(e)],
                file_path=file_path
            )

        except json.JSONDecodeError as e:
            return SchemaValidationResult(
                passed=False,
                message=f"Invalid JSON",
                errors=[f"Line {e.lineno}: {e.msg}"],
                file_path=file_path
            )

        except Exception as e:
            return SchemaValidationResult(
                passed=False,
                message=f"Validation error: {e}",
                file_path=file_path
            )

    def validate_data(
        self,
        data: Dict[str, Any],
        schema_name: str
    ) -> SchemaValidationResult:
        """
        Validate data dictionary against a schema.

        Args:
            data: Data to validate
            schema_name: Schema name

        Returns:
            SchemaValidationResult
        """
        try:
            schema = self.load_schema(schema_name)
            validate(instance=data, schema=schema)

            return SchemaValidationResult(
                passed=True,
                message=f"Schema validation passed: {schema_name}"
            )

        except ValidationError as e:
            return SchemaValidationResult(
                passed=False,
                message="Schema validation failed",
                errors=[str(e)]
            )

    def _load_json(self, file_path: Path) -> Dict:
        """
        Load JSON file.

        Args:
            file_path: Path to JSON file

        Returns:
            Parsed JSON data
        """
        with open(file_path) as f:
            return json.load(f)


# Example usage
if __name__ == "__main__":
    # Example: validate platform metadata
    schema_dir = Path("tools/codegen/schemas")
    validator = SchemaValidator(schema_dir)

    # Validate a platform file
    platform_file = Path("tools/codegen/cli/generators/metadata/platforms/stm32f4.json")

    if platform_file.exists() and schema_dir.exists():
        result = validator.validate_file(platform_file, "platform")
        print(f"Validation: {result.passed} - {result.message}")

        if result.errors:
            for error in result.errors:
                print(f"  Error: {error}")
