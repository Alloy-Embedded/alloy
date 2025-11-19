"""
SVD (System View Description) parser for ARM CMSIS-SVD files.

Parses SVD XML files to extract peripheral, register, and bitfield definitions
for cross-reference validation with generated code.
"""

import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field


@dataclass
class BitField:
    """Represents a bitfield within a register."""
    name: str
    bit_offset: int
    bit_width: int
    access: str = "read-write"
    description: Optional[str] = None

    @property
    def bit_range(self) -> tuple:
        """Get bit range as (msb, lsb)."""
        return (self.bit_offset + self.bit_width - 1, self.bit_offset)


@dataclass
class Register:
    """Represents a peripheral register."""
    name: str
    offset: int
    size: int = 32
    access: str = "read-write"
    reset_value: int = 0
    description: Optional[str] = None
    fields: Dict[str, BitField] = field(default_factory=dict)

    def add_field(self, field: BitField):
        """Add a bitfield to this register."""
        self.fields[field.name] = field


@dataclass
class Peripheral:
    """Represents a peripheral device."""
    name: str
    base_address: int
    description: Optional[str] = None
    registers: Dict[str, Register] = field(default_factory=dict)
    group_name: Optional[str] = None

    def add_register(self, register: Register):
        """Add a register to this peripheral."""
        self.registers[register.name] = register

    def get_register_address(self, register_name: str) -> Optional[int]:
        """Get absolute address of a register."""
        if register_name in self.registers:
            return self.base_address + self.registers[register_name].offset
        return None


@dataclass
class DeviceInfo:
    """Device information from SVD."""
    name: str
    vendor: Optional[str] = None
    version: Optional[str] = None
    description: Optional[str] = None
    cpu: Optional[str] = None
    address_unit_bits: int = 8
    width: int = 32


class SVDParser:
    """
    Parser for ARM CMSIS-SVD files.

    Parses SVD XML to extract peripheral, register, and bitfield definitions.
    """

    def __init__(self):
        """Initialize SVD parser."""
        self.device_info: Optional[DeviceInfo] = None
        self.peripherals: Dict[str, Peripheral] = {}

    def parse_file(self, svd_path: Path) -> bool:
        """
        Parse SVD file.

        Args:
            svd_path: Path to SVD XML file

        Returns:
            True if parsing succeeded
        """
        if not svd_path.exists():
            return False

        try:
            tree = ET.parse(svd_path)
            root = tree.getroot()

            # Parse device info
            self._parse_device_info(root)

            # Parse peripherals
            peripherals_elem = root.find('peripherals')
            if peripherals_elem is not None:
                for peripheral_elem in peripherals_elem.findall('peripheral'):
                    peripheral = self._parse_peripheral(peripheral_elem)
                    if peripheral:
                        self.peripherals[peripheral.name] = peripheral

            return True

        except ET.ParseError as e:
            print(f"SVD parse error: {e}")
            return False
        except Exception as e:
            print(f"SVD parsing failed: {e}")
            return False

    def _parse_device_info(self, root: ET.Element):
        """Parse device information."""
        name = self._get_text(root, 'name', 'Unknown')
        vendor = self._get_text(root, 'vendor')
        version = self._get_text(root, 'version')
        description = self._get_text(root, 'description')

        cpu_elem = root.find('cpu')
        cpu = None
        if cpu_elem is not None:
            cpu = self._get_text(cpu_elem, 'name')

        self.device_info = DeviceInfo(
            name=name,
            vendor=vendor,
            version=version,
            description=description,
            cpu=cpu
        )

    def _parse_peripheral(self, elem: ET.Element) -> Optional[Peripheral]:
        """Parse a peripheral element."""
        name = self._get_text(elem, 'name')
        if not name:
            return None

        # Get base address
        base_address_str = self._get_text(elem, 'baseAddress')
        if not base_address_str:
            return None

        base_address = self._parse_int(base_address_str)
        description = self._get_text(elem, 'description')
        group_name = self._get_text(elem, 'groupName')

        peripheral = Peripheral(
            name=name,
            base_address=base_address,
            description=description,
            group_name=group_name
        )

        # Parse registers
        registers_elem = elem.find('registers')
        if registers_elem is not None:
            for register_elem in registers_elem.findall('register'):
                register = self._parse_register(register_elem)
                if register:
                    peripheral.add_register(register)

        return peripheral

    def _parse_register(self, elem: ET.Element) -> Optional[Register]:
        """Parse a register element."""
        name = self._get_text(elem, 'name')
        if not name:
            return None

        offset_str = self._get_text(elem, 'addressOffset')
        if not offset_str:
            return None

        offset = self._parse_int(offset_str)
        size = self._parse_int(self._get_text(elem, 'size', '32'))
        access = self._get_text(elem, 'access', 'read-write')
        reset_value = self._parse_int(self._get_text(elem, 'resetValue', '0'))
        description = self._get_text(elem, 'description')

        register = Register(
            name=name,
            offset=offset,
            size=size,
            access=access,
            reset_value=reset_value,
            description=description
        )

        # Parse fields (bitfields)
        fields_elem = elem.find('fields')
        if fields_elem is not None:
            for field_elem in fields_elem.findall('field'):
                field = self._parse_field(field_elem)
                if field:
                    register.add_field(field)

        return register

    def _parse_field(self, elem: ET.Element) -> Optional[BitField]:
        """Parse a bitfield element."""
        name = self._get_text(elem, 'name')
        if not name:
            return None

        # Bitfield can be specified as bitOffset+bitWidth or bitRange
        bit_offset_str = self._get_text(elem, 'bitOffset')
        bit_width_str = self._get_text(elem, 'bitWidth')
        bit_range_str = self._get_text(elem, 'bitRange')

        if bit_offset_str and bit_width_str:
            bit_offset = self._parse_int(bit_offset_str)
            bit_width = self._parse_int(bit_width_str)
        elif bit_range_str:
            # Parse format: [msb:lsb]
            bit_range_str = bit_range_str.strip('[]')
            msb, lsb = map(int, bit_range_str.split(':'))
            bit_offset = lsb
            bit_width = msb - lsb + 1
        else:
            return None

        access = self._get_text(elem, 'access', 'read-write')
        description = self._get_text(elem, 'description')

        return BitField(
            name=name,
            bit_offset=bit_offset,
            bit_width=bit_width,
            access=access,
            description=description
        )

    def _get_text(self, elem: ET.Element, tag: str, default: str = None) -> Optional[str]:
        """Get text content of a child element."""
        child = elem.find(tag)
        if child is not None and child.text:
            return child.text.strip()
        return default

    def _parse_int(self, value: str) -> int:
        """Parse integer from string (supports hex with 0x prefix)."""
        if not value:
            return 0

        value = value.strip()

        # Handle hex values
        if value.startswith('0x') or value.startswith('0X'):
            return int(value, 16)

        # Handle binary values
        if value.startswith('0b') or value.startswith('0B'):
            return int(value, 2)

        # Decimal
        return int(value)

    def get_peripheral(self, name: str) -> Optional[Peripheral]:
        """Get peripheral by name."""
        return self.peripherals.get(name)

    def get_all_peripheral_names(self) -> List[str]:
        """Get list of all peripheral names."""
        return list(self.peripherals.keys())

    def find_register(self, peripheral_name: str, register_name: str) -> Optional[Register]:
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
    ) -> Optional[BitField]:
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
