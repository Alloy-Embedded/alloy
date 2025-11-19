"""
C++ code parser for extracting peripheral definitions.

Parses generated C++ headers to extract peripheral base addresses,
register offsets, and bitfield positions for cross-reference validation.
"""

import re
from pathlib import Path
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field


@dataclass
class CodeBitField:
    """Bitfield definition extracted from code."""
    name: str
    position: int
    width: Optional[int] = None
    line_number: int = 0


@dataclass
class CodeRegister:
    """Register definition extracted from code."""
    name: str
    offset: int
    line_number: int = 0
    fields: Dict[str, CodeBitField] = field(default_factory=dict)


@dataclass
class CodePeripheral:
    """Peripheral definition extracted from code."""
    name: str
    base_address: int
    line_number: int = 0
    registers: Dict[str, CodeRegister] = field(default_factory=dict)


class CodeParser:
    """
    Parser for generated C++ peripheral code.

    Extracts peripheral definitions using regex patterns.
    """

    def __init__(self):
        """Initialize code parser."""
        self.peripherals: Dict[str, CodePeripheral] = {}

        # Regex patterns for matching code constructs
        self._peripheral_pattern = re.compile(
            r'constexpr\s+std::uintptr_t\s+(\w+)_BASE\s*=\s*0x([0-9A-Fa-f]+)',
            re.MULTILINE
        )

        self._register_offset_pattern = re.compile(
            r'static\s+constexpr\s+std::size_t\s+(\w+)_OFFSET\s*=\s*0x([0-9A-Fa-f]+)',
            re.MULTILINE
        )

        self._bitfield_pos_pattern = re.compile(
            r'static\s+constexpr\s+std::uint(?:8|16|32|64)_t\s+(\w+)_POS\s*=\s*(\d+)',
            re.MULTILINE
        )

        self._bitfield_width_pattern = re.compile(
            r'static\s+constexpr\s+std::uint(?:8|16|32|64)_t\s+(\w+)_WIDTH\s*=\s*(\d+)',
            re.MULTILINE
        )

        # Pattern for namespace detection
        self._namespace_pattern = re.compile(
            r'namespace\s+(\w+)\s*\{',
            re.MULTILINE
        )

    def parse_file(self, header_path: Path) -> bool:
        """
        Parse C++ header file.

        Args:
            header_path: Path to C++ header file

        Returns:
            True if parsing succeeded
        """
        if not header_path.exists():
            return False

        try:
            content = header_path.read_text()
            lines = content.split('\n')

            # Parse peripheral base addresses
            self._parse_peripherals(content, lines)

            # Parse register offsets
            self._parse_registers(content, lines)

            # Parse bitfields
            self._parse_bitfields(content, lines)

            return True

        except Exception as e:
            print(f"Code parsing failed: {e}")
            return False

    def _parse_peripherals(self, content: str, lines: List[str]):
        """Parse peripheral base addresses."""
        for match in self._peripheral_pattern.finditer(content):
            name = match.group(1)
            address = int(match.group(2), 16)

            # Find line number
            line_num = self._find_line_number(content, match.start(), lines)

            # Extract peripheral name (remove _BASE suffix if present)
            peripheral_name = name
            if peripheral_name.endswith('_BASE'):
                peripheral_name = peripheral_name[:-5]

            if peripheral_name not in self.peripherals:
                self.peripherals[peripheral_name] = CodePeripheral(
                    name=peripheral_name,
                    base_address=address,
                    line_number=line_num
                )

    def _parse_registers(self, content: str, lines: List[str]):
        """Parse register offsets."""
        for match in self._register_offset_pattern.finditer(content):
            name = match.group(1)
            offset = int(match.group(2), 16)

            line_num = self._find_line_number(content, match.start(), lines)

            # Try to determine which peripheral this register belongs to
            # by looking at the name pattern (e.g., GPIO_MODER -> GPIO)
            peripheral_name = self._extract_peripheral_name(name)

            # Extract register name (remove _OFFSET suffix)
            register_name = name
            if register_name.endswith('_OFFSET'):
                register_name = register_name[:-7]

            # Create register
            register = CodeRegister(
                name=register_name,
                offset=offset,
                line_number=line_num
            )

            # Add to peripheral if we found one
            if peripheral_name and peripheral_name in self.peripherals:
                self.peripherals[peripheral_name].registers[register_name] = register

    def _parse_bitfields(self, content: str, lines: List[str]):
        """Parse bitfield positions and widths."""
        # Parse positions
        positions = {}
        for match in self._bitfield_pos_pattern.finditer(content):
            name = match.group(1)
            position = int(match.group(2))
            line_num = self._find_line_number(content, match.start(), lines)

            # Remove _POS suffix
            field_name = name[:-4] if name.endswith('_POS') else name

            positions[field_name] = (position, line_num)

        # Parse widths
        widths = {}
        for match in self._bitfield_width_pattern.finditer(content):
            name = match.group(1)
            width = int(match.group(2))

            # Remove _WIDTH suffix
            field_name = name[:-6] if name.endswith('_WIDTH') else name

            widths[field_name] = width

        # Combine positions and widths into bitfields
        for field_name, (position, line_num) in positions.items():
            width = widths.get(field_name)

            # Try to determine peripheral and register
            peripheral_name = self._extract_peripheral_name(field_name)
            register_name = self._extract_register_name(field_name, peripheral_name)

            bitfield = CodeBitField(
                name=field_name,
                position=position,
                width=width,
                line_number=line_num
            )

            # Add to register if we found one
            if (peripheral_name and peripheral_name in self.peripherals and
                register_name and register_name in self.peripherals[peripheral_name].registers):
                self.peripherals[peripheral_name].registers[register_name].fields[field_name] = bitfield

    def _find_line_number(self, content: str, pos: int, lines: List[str]) -> int:
        """Find line number for a position in content."""
        line_num = 1
        current_pos = 0

        for line in lines:
            if current_pos + len(line) >= pos:
                return line_num
            current_pos += len(line) + 1  # +1 for newline
            line_num += 1

        return line_num

    def _extract_peripheral_name(self, name: str) -> Optional[str]:
        """
        Extract peripheral name from a definition name.

        Examples:
        - GPIO_MODER -> GPIO
        - GPIOA_ODR -> GPIOA
        - TIM2_CR1 -> TIM2
        """
        # Common patterns: PERIPHERAL_REGISTER or PERIPHERAL_FIELD
        parts = name.split('_')
        if len(parts) >= 2:
            # Take first part as peripheral name
            return parts[0]

        return None

    def _extract_register_name(self, field_name: str, peripheral_name: Optional[str]) -> Optional[str]:
        """
        Extract register name from a field name.

        Examples:
        - GPIO_MODER_MODE0 -> MODER (if peripheral is GPIO)
        - GPIOA_ODR_OD5 -> ODR (if peripheral is GPIOA)
        """
        if not peripheral_name:
            return None

        # Remove peripheral prefix
        if field_name.startswith(peripheral_name + '_'):
            remainder = field_name[len(peripheral_name) + 1:]

            # Split remainder and take first part as register
            parts = remainder.split('_')
            if parts:
                return parts[0]

        return None

    def get_peripheral(self, name: str) -> Optional[CodePeripheral]:
        """Get peripheral by name."""
        return self.peripherals.get(name)

    def get_all_peripheral_names(self) -> List[str]:
        """Get list of all peripheral names."""
        return list(self.peripherals.keys())

    def find_register(self, peripheral_name: str, register_name: str) -> Optional[CodeRegister]:
        """Find a register in a peripheral."""
        peripheral = self.peripherals.get(peripheral_name)
        if peripheral:
            return peripheral.registers.get(register_name)
        return None

    def find_field(
        self,
        peripheral_name: str,
        register_name: str,
        field_name: str
    ) -> Optional[CodeBitField]:
        """Find a bitfield in a register."""
        register = self.find_register(peripheral_name, register_name)
        if register:
            return register.fields.get(field_name)
        return None

    def get_statistics(self) -> Dict[str, int]:
        """Get parsing statistics."""
        total_registers = sum(len(p.registers) for p in self.peripherals.values())
        total_fields = sum(
            len(r.fields)
            for p in self.peripherals.values()
            for r in p.registers.values()
        )

        return {
            'peripherals': len(self.peripherals),
            'registers': total_registers,
            'fields': total_fields
        }
