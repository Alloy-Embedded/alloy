"""
Semantic Validator

Cross-references generated code against SVD files to ensure correctness.

Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

from pathlib import Path
from dataclasses import dataclass
from typing import Dict, List, Optional
import re
import xml.etree.ElementTree as ET

from .syntax_validator import ValidationLevel, ValidationResult


class SemanticValidator:
    """
    Validates peripheral addresses, register offsets match SVD.

    Ensures generated code matches hardware specifications.
    """

    def __init__(self):
        """Initialize semantic validator."""
        self.svd_cache: Dict[Path, ET.Element] = {}

    def validate(self, generated_file: Path, svd_file: Path) -> ValidationResult:
        """
        Cross-reference generated code against SVD.

        Args:
            generated_file: Path to generated C++ file
            svd_file: Path to SVD XML file

        Returns:
            ValidationResult with semantic validation results
        """
        if not generated_file.exists():
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message=f"Generated file not found: {generated_file}",
                file_path=generated_file
            )

        if not svd_file.exists():
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message=f"SVD file not found: {svd_file}",
                file_path=svd_file
            )

        try:
            # Parse SVD file
            svd_data = self._parse_svd(svd_file)

            # Parse generated code
            generated_code = generated_file.read_text()

            # Validate peripheral base addresses
            errors = []
            warnings = []

            # Check peripheral addresses
            for peripheral in svd_data.get('peripherals', []):
                name = peripheral.get('name')
                address = peripheral.get('baseAddress')

                if address:
                    # Look for this address in generated code
                    if not self._find_address_in_code(generated_code, address):
                        warnings.append(
                            f"Peripheral {name} base address {address:#x} not found in generated code"
                        )

            # Check register offsets
            for peripheral in svd_data.get('peripherals', []):
                for register in peripheral.get('registers', []):
                    reg_name = register.get('name')
                    offset = register.get('addressOffset')

                    if offset is not None:
                        # Look for this offset in generated code
                        if not self._find_offset_in_code(generated_code, offset):
                            warnings.append(
                                f"Register {reg_name} offset {offset:#x} not found in generated code"
                            )

            if errors:
                return ValidationResult(
                    passed=False,
                    level=ValidationLevel.ERROR,
                    message=f"Semantic errors:\n" + "\n".join(errors),
                    file_path=generated_file
                )
            elif warnings:
                return ValidationResult(
                    passed=True,
                    level=ValidationLevel.WARNING,
                    message=f"Semantic warnings:\n" + "\n".join(warnings),
                    file_path=generated_file
                )
            else:
                return ValidationResult(
                    passed=True,
                    level=ValidationLevel.INFO,
                    message="Semantic validation passed",
                    file_path=generated_file
                )

        except Exception as e:
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message=f"Semantic validation failed: {e}",
                file_path=generated_file
            )

    def _parse_svd(self, svd_file: Path) -> Dict:
        """
        Parse SVD file and extract peripheral/register information.

        Args:
            svd_file: Path to SVD file

        Returns:
            Dictionary with SVD data
        """
        # Check cache
        if svd_file in self.svd_cache:
            root = self.svd_cache[svd_file]
        else:
            tree = ET.parse(svd_file)
            root = tree.getroot()
            self.svd_cache[svd_file] = root

        peripherals = []

        # Find all peripherals
        for peripheral_elem in root.findall('.//peripheral'):
            name_elem = peripheral_elem.find('name')
            base_addr_elem = peripheral_elem.find('baseAddress')

            if name_elem is not None and base_addr_elem is not None:
                peripheral_data = {
                    'name': name_elem.text,
                    'baseAddress': int(base_addr_elem.text, 0),
                    'registers': []
                }

                # Find registers
                registers_elem = peripheral_elem.find('registers')
                if registers_elem is not None:
                    for register_elem in registers_elem.findall('register'):
                        reg_name_elem = register_elem.find('name')
                        offset_elem = register_elem.find('addressOffset')

                        if reg_name_elem is not None and offset_elem is not None:
                            peripheral_data['registers'].append({
                                'name': reg_name_elem.text,
                                'addressOffset': int(offset_elem.text, 0)
                            })

                peripherals.append(peripheral_data)

        return {'peripherals': peripherals}

    def _find_address_in_code(self, code: str, address: int) -> bool:
        """
        Check if an address appears in generated code.

        Args:
            code: Generated C++ code
            address: Address to search for

        Returns:
            True if address found
        """
        # Look for hex representation (0x40020000)
        hex_addr = f"0x{address:08X}"
        hex_addr_lower = f"0x{address:08x}"

        return hex_addr in code or hex_addr_lower in code

    def _find_offset_in_code(self, code: str, offset: int) -> bool:
        """
        Check if an offset appears in generated code.

        Args:
            code: Generated C++ code
            offset: Offset to search for

        Returns:
            True if offset found
        """
        # Look for hex representation
        hex_offset = f"0x{offset:02X}"
        hex_offset_lower = f"0x{offset:02x}"

        return hex_offset in code or hex_offset_lower in code


# Example usage
if __name__ == "__main__":
    validator = SemanticValidator()

    # Example: validate GPIO generated code against STM32F4 SVD
    generated = Path("src/hal/vendors/st/stm32f4/gpio.hpp")
    svd = Path("tools/codegen/svd/upstream/STMicro/STM32F401.svd")

    if generated.exists() and svd.exists():
        result = validator.validate(generated, svd)
        print(f"Semantic validation: {result.passed} - {result.message}")
