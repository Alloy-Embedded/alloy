"""
Metadata validation and management service.

Provides validation, creation, and diff functionality for metadata files.
"""

import yaml
import json
from pathlib import Path
from typing import Dict, Any, List, Optional, Tuple
from enum import Enum


class MetadataType(str, Enum):
    """Supported metadata types."""
    MCU = "mcu"
    BOARD = "board"
    PERIPHERAL = "peripheral"
    TEMPLATE = "template"


class ValidationSeverity(str, Enum):
    """Validation message severity."""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


class ValidationMessage:
    """Validation message with context."""

    def __init__(
        self,
        severity: ValidationSeverity,
        message: str,
        line: Optional[int] = None,
        column: Optional[int] = None,
        suggestion: Optional[str] = None
    ):
        self.severity = severity
        self.message = message
        self.line = line
        self.column = column
        self.suggestion = suggestion

    def __repr__(self):
        location = f":{self.line}" if self.line else ""
        return f"[{self.severity.value}]{location} {self.message}"


class ValidationResult:
    """Result of metadata validation."""

    def __init__(self):
        self.messages: List[ValidationMessage] = []
        self.valid = True

    def add_error(self, message: str, line: Optional[int] = None, suggestion: Optional[str] = None):
        """Add error message."""
        self.messages.append(ValidationMessage(
            ValidationSeverity.ERROR, message, line=line, suggestion=suggestion
        ))
        self.valid = False

    def add_warning(self, message: str, line: Optional[int] = None, suggestion: Optional[str] = None):
        """Add warning message."""
        self.messages.append(ValidationMessage(
            ValidationSeverity.WARNING, message, line=line, suggestion=suggestion
        ))

    def add_info(self, message: str, line: Optional[int] = None):
        """Add info message."""
        self.messages.append(ValidationMessage(
            ValidationSeverity.INFO, message, line=line
        ))

    def has_errors(self) -> bool:
        """Check if validation has errors."""
        return not self.valid

    def error_count(self) -> int:
        """Count error messages."""
        return sum(1 for m in self.messages if m.severity == ValidationSeverity.ERROR)

    def warning_count(self) -> int:
        """Count warning messages."""
        return sum(1 for m in self.messages if m.severity == ValidationSeverity.WARNING)


class MetadataService:
    """Service for metadata validation and management."""

    def __init__(self, database_path: Optional[Path] = None):
        """
        Initialize metadata service.

        Args:
            database_path: Path to database directory (default: ./database)
        """
        self.database_path = database_path or Path("./database")
        self.schema_path = self.database_path / "schema"

    def validate_file(
        self,
        file_path: Path,
        strict: bool = False,
        metadata_type: Optional[MetadataType] = None
    ) -> ValidationResult:
        """
        Validate a metadata file.

        Args:
            file_path: Path to metadata file
            strict: Enable strict validation mode
            metadata_type: Type of metadata (auto-detected if None)

        Returns:
            ValidationResult with messages
        """
        result = ValidationResult()

        # Check file exists
        if not file_path.exists():
            result.add_error(f"File not found: {file_path}")
            return result

        # Parse file
        try:
            data, file_format = self._load_file(file_path)
        except Exception as e:
            result.add_error(f"Failed to parse file: {e}", suggestion="Check YAML/JSON syntax")
            return result

        # Auto-detect metadata type if not specified
        if metadata_type is None:
            metadata_type = self._detect_metadata_type(file_path, data)

        # Validate syntax (already done by parser)
        result.add_info(f"✓ Syntax valid ({file_format.upper()})")

        # Validate structure
        self._validate_structure(data, metadata_type, result, strict)

        # Validate required fields
        self._validate_required_fields(data, metadata_type, result)

        # Validate field types
        self._validate_field_types(data, metadata_type, result)

        # Strict mode checks
        if strict:
            self._validate_strict(data, metadata_type, result)

        return result

    def create_template(
        self,
        metadata_type: MetadataType,
        output_path: Optional[Path] = None,
        format: str = "yaml"
    ) -> Path:
        """
        Create a new metadata file from template.

        Args:
            metadata_type: Type of metadata to create
            output_path: Where to save (default: generate name)
            format: Output format (yaml or json)

        Returns:
            Path to created file
        """
        # Get template
        template = self._get_template(metadata_type)

        # Determine output path
        if output_path is None:
            suffix = ".yaml" if format == "yaml" else ".json"
            output_path = Path(f"new_{metadata_type.value}{suffix}")

        # Write file
        with open(output_path, 'w', encoding='utf-8') as f:
            if format == "yaml":
                yaml.safe_dump(template, f, default_flow_style=False, sort_keys=False)
            else:
                json.dump(template, f, indent=2)

        return output_path

    def diff_files(
        self,
        file1: Path,
        file2: Path
    ) -> Dict[str, Any]:
        """
        Compare two metadata files.

        Args:
            file1: First file
            file2: Second file

        Returns:
            Dict with added, removed, modified keys
        """
        # Load files
        data1, _ = self._load_file(file1)
        data2, _ = self._load_file(file2)

        # Compare
        return self._deep_diff(data1, data2)

    # Private methods

    def _load_file(self, file_path: Path) -> Tuple[Dict[str, Any], str]:
        """Load YAML or JSON file."""
        with open(file_path, 'r', encoding='utf-8') as f:
            if file_path.suffix in ['.yaml', '.yml']:
                return yaml.safe_load(f), "yaml"
            else:
                return json.load(f), "json"

    def _detect_metadata_type(self, file_path: Path, data: Dict[str, Any]) -> MetadataType:
        """Auto-detect metadata type from path or content."""
        # Check path
        if "mcu" in str(file_path).lower():
            return MetadataType.MCU
        if "board" in str(file_path).lower():
            return MetadataType.BOARD
        if "peripheral" in str(file_path).lower():
            return MetadataType.PERIPHERAL
        if "template" in str(file_path).lower():
            return MetadataType.TEMPLATE

        # Check content
        if "family" in data and "mcus" in data:
            return MetadataType.MCU
        if "board" in data and "mcu" in data and "pinout" in data:
            return MetadataType.BOARD
        if "peripheral_type" in data or "implementations" in data:
            return MetadataType.PERIPHERAL

        # Default to MCU
        return MetadataType.MCU

    def _validate_structure(
        self,
        data: Dict[str, Any],
        metadata_type: MetadataType,
        result: ValidationResult,
        strict: bool
    ):
        """Validate overall structure."""
        # Check schema_version
        if "schema_version" not in data:
            result.add_warning(
                "Missing 'schema_version' field",
                suggestion="Add: schema_version: '1.0'"
            )

        # Type-specific validation
        if metadata_type == MetadataType.MCU:
            if "family" not in data:
                result.add_error("MCU metadata must have 'family' section")
            if "mcus" not in data:
                result.add_error("MCU metadata must have 'mcus' array")

        elif metadata_type == MetadataType.BOARD:
            if "board" not in data:
                result.add_error("Board metadata must have 'board' section")
            if "mcu" not in data:
                result.add_error("Board metadata must have 'mcu' section")
            if "pinout" not in data:
                result.add_error("Board metadata must have 'pinout' section")

        elif metadata_type == MetadataType.PERIPHERAL:
            if "peripheral_type" not in data:
                result.add_error("Peripheral metadata must have 'peripheral_type'")

    def _validate_required_fields(
        self,
        data: Dict[str, Any],
        metadata_type: MetadataType,
        result: ValidationResult
    ):
        """Validate required fields."""
        if metadata_type == MetadataType.MCU:
            family = data.get("family", {})
            required = ["id", "vendor", "display_name"]
            for field in required:
                if field not in family:
                    result.add_error(f"Missing required field in family: {field}")

            # Check MCUs
            mcus = data.get("mcus", [])
            if not mcus:
                result.add_warning("No MCUs defined")

            for i, mcu in enumerate(mcus):
                mcu_required = ["part_number", "core", "max_freq_mhz", "memory"]
                for field in mcu_required:
                    if field not in mcu:
                        result.add_error(f"MCU[{i}] missing required field: {field}")

        elif metadata_type == MetadataType.BOARD:
            board = data.get("board", {})
            board_required = ["id", "name", "vendor"]
            for field in board_required:
                if field not in board:
                    result.add_error(f"Missing required field in board: {field}")

            mcu = data.get("mcu", {})
            mcu_required = ["part_number", "family"]
            for field in mcu_required:
                if field not in mcu:
                    result.add_error(f"Missing required field in mcu: {field}")

    def _validate_field_types(
        self,
        data: Dict[str, Any],
        metadata_type: MetadataType,
        result: ValidationResult
    ):
        """Validate field types."""
        if metadata_type == MetadataType.MCU:
            mcus = data.get("mcus", [])
            for i, mcu in enumerate(mcus):
                # Check integer fields
                if "max_freq_mhz" in mcu and not isinstance(mcu["max_freq_mhz"], int):
                    result.add_error(f"MCU[{i}].max_freq_mhz must be integer")

                # Check memory
                if "memory" in mcu:
                    mem = mcu["memory"]
                    for field in ["flash_kb", "sram_kb"]:
                        if field in mem and not isinstance(mem[field], int):
                            result.add_error(f"MCU[{i}].memory.{field} must be integer")

    def _validate_strict(
        self,
        data: Dict[str, Any],
        metadata_type: MetadataType,
        result: ValidationResult
    ):
        """Strict mode validation."""
        # Check for empty strings
        self._check_empty_strings(data, result, path="")

        # Check for TODO/FIXME comments
        yaml_str = yaml.dump(data)
        if "TODO" in yaml_str or "FIXME" in yaml_str:
            result.add_warning("File contains TODO/FIXME comments")

        # Check for deprecated fields
        if "deprecated" in data and data["deprecated"]:
            result.add_info("Metadata marked as deprecated")

    def _check_empty_strings(self, obj: Any, result: ValidationResult, path: str):
        """Recursively check for empty strings."""
        if isinstance(obj, dict):
            for key, value in obj.items():
                new_path = f"{path}.{key}" if path else key
                if isinstance(value, str) and value.strip() == "":
                    result.add_warning(f"Empty string at: {new_path}")
                else:
                    self._check_empty_strings(value, result, new_path)
        elif isinstance(obj, list):
            for i, item in enumerate(obj):
                new_path = f"{path}[{i}]"
                self._check_empty_strings(item, result, new_path)

    def _get_template(self, metadata_type: MetadataType) -> Dict[str, Any]:
        """Get template for metadata type."""
        if metadata_type == MetadataType.MCU:
            return {
                "schema_version": "1.0",
                "family": {
                    "id": "new_family",
                    "vendor": "vendor_name",
                    "display_name": "New MCU Family",
                    "description": "Description of the MCU family"
                },
                "mcus": [
                    {
                        "part_number": "MCU_PART_NUMBER",
                        "display_name": "MCU Display Name",
                        "core": "Cortex-M4F",
                        "max_freq_mhz": 100,
                        "memory": {
                            "flash_kb": 512,
                            "sram_kb": 128,
                            "eeprom_kb": 0
                        },
                        "package": {
                            "type": "LQFP",
                            "pins": 64
                        },
                        "peripherals": {},
                        "status": "production",
                        "tags": []
                    }
                ]
            }

        elif metadata_type == MetadataType.BOARD:
            return {
                "schema_version": "1.0",
                "board": {
                    "id": "new_board",
                    "name": "New Development Board",
                    "vendor": "Vendor Name",
                    "url": "https://example.com"
                },
                "mcu": {
                    "part_number": "MCU_PART_NUMBER",
                    "family": "mcu_family"
                },
                "clock": {
                    "system_freq_hz": 100000000,
                    "xtal_freq_hz": 8000000
                },
                "pinout": {
                    "leds": [],
                    "buttons": [],
                    "debugger": None
                },
                "status": "supported",
                "tags": []
            }

        elif metadata_type == MetadataType.PERIPHERAL:
            return {
                "schema_version": "1.0",
                "peripheral_type": "new_peripheral",
                "display_name": "New Peripheral",
                "description": "Description of the peripheral",
                "implementations": {}
            }

        else:  # TEMPLATE
            return {
                "schema_version": "1.0",
                "template": {
                    "id": "new_template",
                    "name": "New Project Template",
                    "description": "Template description"
                },
                "files": []
            }

    def _deep_diff(
        self,
        obj1: Any,
        obj2: Any,
        path: str = ""
    ) -> Dict[str, List[str]]:
        """Deep comparison of two objects."""
        result = {
            "added": [],
            "removed": [],
            "modified": []
        }

        if isinstance(obj1, dict) and isinstance(obj2, dict):
            # Check added/removed keys
            keys1 = set(obj1.keys())
            keys2 = set(obj2.keys())

            for key in keys2 - keys1:
                result["added"].append(f"{path}.{key}" if path else key)

            for key in keys1 - keys2:
                result["removed"].append(f"{path}.{key}" if path else key)

            # Check modified keys
            for key in keys1 & keys2:
                new_path = f"{path}.{key}" if path else key
                if obj1[key] != obj2[key]:
                    if isinstance(obj1[key], (dict, list)) and isinstance(obj2[key], (dict, list)):
                        # Recurse for nested structures
                        nested = self._deep_diff(obj1[key], obj2[key], new_path)
                        result["added"].extend(nested["added"])
                        result["removed"].extend(nested["removed"])
                        result["modified"].extend(nested["modified"])
                    else:
                        result["modified"].append(new_path)

        elif isinstance(obj1, list) and isinstance(obj2, list):
            if len(obj1) != len(obj2):
                result["modified"].append(f"{path} (length: {len(obj1)} → {len(obj2)})")

        elif obj1 != obj2:
            result["modified"].append(path or "root")

        return result
