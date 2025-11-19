"""
Semantic validator - cross-references generated code with SVD files.

Validates that generated C++ code matches the hardware definitions in SVD files.
"""

import shutil
from pathlib import Path
from typing import Optional, List, Dict
import time

from .base import Validator, ValidationResult, ValidationStage
from .svd_parser import SVDParser
from .code_parser import CodeParser


class SemanticValidator(Validator):
    """Validates C++ code against SVD definitions."""

    def __init__(self, svd_path: Optional[Path] = None):
        super().__init__()
        self.stage = ValidationStage.SEMANTIC
        self.svd_path = svd_path
        self.svd_parser: Optional[SVDParser] = None

        # Load SVD if path provided
        if svd_path and svd_path.exists():
            self._load_svd(svd_path)

    def is_available(self) -> bool:
        """
        Check if semantic validation is available.

        Returns:
            True if SVD file is loaded
        """
        return self.svd_parser is not None and len(self.svd_parser.peripherals) > 0

    def _load_svd(self, svd_path: Path) -> bool:
        """Load SVD file."""
        self.svd_parser = SVDParser()
        return self.svd_parser.parse_file(svd_path)

    def validate(
        self,
        file_path: Path,
        svd_path: Optional[Path] = None,
        **options
    ) -> ValidationResult:
        """
        Validate C++ file against SVD definitions.

        Args:
            file_path: Path to C++ file
            svd_path: Path to SVD file (uses constructor path if None)
            **options: Additional options

        Returns:
            ValidationResult
        """
        result = ValidationResult(stage=self.stage)
        start_time = time.time()

        # Check file exists
        if not file_path.exists():
            result.add_error(f"File not found: {file_path}")
            return result

        # Load SVD if not already loaded
        if svd_path and svd_path != self.svd_path:
            if not self._load_svd(svd_path):
                result.add_error(
                    f"Failed to load SVD file: {svd_path}",
                    suggestion="Check SVD file format and path"
                )
                return result

        # Check if SVD is available
        if not self.is_available():
            result.add_error(
                "No SVD file loaded",
                suggestion="Provide SVD file path with --svd option"
            )
            return result

        # Parse C++ code
        code_parser = CodeParser()
        if not code_parser.parse_file(file_path):
            result.add_error(f"Failed to parse C++ file: {file_path}")
            return result

        # Cross-reference validation
        self._validate_peripherals(code_parser, result)
        self._validate_registers(code_parser, result)
        self._validate_bitfields(code_parser, result)

        # Add summary info
        if not result.has_errors():
            svd_stats = self.svd_parser.get_statistics()
            code_stats = code_parser.get_statistics()

            result.add_info(
                f"✓ Semantic validation passed "
                f"({code_stats['peripherals']} peripherals, "
                f"{code_stats['registers']} registers, "
                f"{code_stats['fields']} fields)"
            )

            result.metadata["svd_peripherals"] = svd_stats['peripherals']
            result.metadata["code_peripherals"] = code_stats['peripherals']
            result.metadata["svd_file"] = str(self.svd_path)

        # Record duration
        result.duration_ms = (time.time() - start_time) * 1000

        return result

    def _validate_peripherals(self, code_parser: CodeParser, result: ValidationResult):
        """Validate peripheral base addresses."""
        for periph_name, code_periph in code_parser.peripherals.items():
            svd_periph = self.svd_parser.get_peripheral(periph_name)

            if not svd_periph:
                result.add_warning(
                    f"Peripheral '{periph_name}' not found in SVD",
                    line=code_periph.line_number,
                    suggestion=f"Check if peripheral name matches SVD (found in code: {periph_name})"
                )
                continue

            # Check base address
            if code_periph.base_address != svd_periph.base_address:
                result.add_error(
                    f"Peripheral '{periph_name}' base address mismatch: "
                    f"code has 0x{code_periph.base_address:08X}, "
                    f"SVD defines 0x{svd_periph.base_address:08X}",
                    line=code_periph.line_number,
                    suggestion=f"Update base address to 0x{svd_periph.base_address:08X}"
                )

    def _validate_registers(self, code_parser: CodeParser, result: ValidationResult):
        """Validate register offsets."""
        for periph_name, code_periph in code_parser.peripherals.items():
            svd_periph = self.svd_parser.get_peripheral(periph_name)
            if not svd_periph:
                continue

            for reg_name, code_reg in code_periph.registers.items():
                svd_reg = svd_periph.registers.get(reg_name)

                if not svd_reg:
                    result.add_warning(
                        f"Register '{periph_name}::{reg_name}' not found in SVD",
                        line=code_reg.line_number,
                        suggestion=f"Check register name (found: {reg_name})"
                    )
                    continue

                # Check offset
                if code_reg.offset != svd_reg.offset:
                    result.add_error(
                        f"Register '{periph_name}::{reg_name}' offset mismatch: "
                        f"code has 0x{code_reg.offset:04X}, "
                        f"SVD defines 0x{svd_reg.offset:04X}",
                        line=code_reg.line_number,
                        suggestion=f"Update offset to 0x{svd_reg.offset:04X}"
                    )

    def _validate_bitfields(self, code_parser: CodeParser, result: ValidationResult):
        """Validate bitfield positions and widths."""
        for periph_name, code_periph in code_parser.peripherals.items():
            svd_periph = self.svd_parser.get_peripheral(periph_name)
            if not svd_periph:
                continue

            for reg_name, code_reg in code_periph.registers.items():
                svd_reg = svd_periph.registers.get(reg_name)
                if not svd_reg:
                    continue

                for field_name, code_field in code_reg.fields.items():
                    svd_field = svd_reg.fields.get(field_name)

                    if not svd_field:
                        result.add_warning(
                            f"Bitfield '{periph_name}::{reg_name}::{field_name}' not found in SVD",
                            line=code_field.line_number,
                            suggestion=f"Check field name (found: {field_name})"
                        )
                        continue

                    # Check position
                    if code_field.position != svd_field.bit_offset:
                        result.add_error(
                            f"Bitfield '{periph_name}::{reg_name}::{field_name}' position mismatch: "
                            f"code has bit {code_field.position}, "
                            f"SVD defines bit {svd_field.bit_offset}",
                            line=code_field.line_number,
                            suggestion=f"Update position to {svd_field.bit_offset}"
                        )

                    # Check width if available
                    if code_field.width and code_field.width != svd_field.bit_width:
                        result.add_error(
                            f"Bitfield '{periph_name}::{reg_name}::{field_name}' width mismatch: "
                            f"code has {code_field.width} bits, "
                            f"SVD defines {svd_field.bit_width} bits",
                            line=code_field.line_number,
                            suggestion=f"Update width to {svd_field.bit_width}"
                        )

    def set_svd_path(self, svd_path: Path) -> bool:
        """
        Set SVD file path and load it.

        Args:
            svd_path: Path to SVD file

        Returns:
            True if loaded successfully
        """
        self.svd_path = svd_path
        return self._load_svd(svd_path)

    def get_svd_info(self) -> Optional[Dict]:
        """
        Get SVD device information.

        Returns:
            Dict with device info or None
        """
        if not self.svd_parser or not self.svd_parser.device_info:
            return None

        info = self.svd_parser.device_info
        return {
            "name": info.name,
            "vendor": info.vendor,
            "version": info.version,
            "cpu": info.cpu,
            "description": info.description
        }

    def get_peripheral_list(self) -> List[str]:
        """
        Get list of peripherals from SVD.

        Returns:
            List of peripheral names
        """
        if not self.svd_parser:
            return []

        return self.svd_parser.get_all_peripheral_names()
