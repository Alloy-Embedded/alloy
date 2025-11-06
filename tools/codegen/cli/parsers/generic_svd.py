"""
Generic SVD Parser

A powerful, reusable SVD parser that extracts all information from CMSIS-SVD files.
This parser automatically handles vendor detection, family classification, and
provides a clean interface for accessing device information.

Usage:
    parser = SVDParser(svd_path)
    device = parser.parse()

    print(f"Device: {device.name}")
    print(f"Vendor: {device.vendor}")
    print(f"Family: {device.family}")
    print(f"Interrupts: {len(device.interrupts)}")
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Any
import xml.etree.ElementTree as ET
import sys

# Add parent to path for imports
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.config import (
    normalize_vendor,
    normalize_name,
    detect_family,
    is_board_mcu
)
from cli.core.logger import logger


# ============================================================================
# DATA CLASSES
# ============================================================================

@dataclass
class Interrupt:
    """Represents an interrupt in the vector table"""
    name: str
    value: int
    description: Optional[str] = None

    def __lt__(self, other):
        """Sort by IRQ number"""
        return self.value < other.value


@dataclass
class Register:
    """Represents a peripheral register"""
    name: str
    offset: int
    size: int = 32
    reset_value: Optional[int] = None
    description: Optional[str] = None
    access: Optional[str] = None  # read-only, write-only, read-write


@dataclass
class Peripheral:
    """Represents a peripheral device"""
    name: str
    base_address: int
    description: Optional[str] = None
    group_name: Optional[str] = None
    registers: List[Register] = field(default_factory=list)
    interrupts: List[Interrupt] = field(default_factory=list)


@dataclass
class MemoryRegion:
    """Represents a memory region (Flash, RAM, etc.)"""
    name: str
    start: int
    size: int
    access: Optional[str] = None  # read, write, execute


@dataclass
class Pin:
    """Represents a physical pin"""
    name: str
    position: Optional[str] = None
    functions: List[str] = field(default_factory=list)


@dataclass
class SVDDevice:
    """
    Complete device information parsed from SVD.

    This is the main data structure returned by SVDParser.
    It contains all information about an MCU.
    """
    # Basic information
    name: str
    vendor: str
    vendor_normalized: str
    family: str
    description: Optional[str] = None
    version: Optional[str] = None

    # Architecture
    cpu_name: Optional[str] = None
    cpu_revision: Optional[str] = None
    cpu_endian: Optional[str] = None
    cpu_mpuPresent: Optional[bool] = None
    cpu_fpuPresent: Optional[bool] = None
    cpu_nvicPrioBits: Optional[int] = None

    # Memory
    address_unit_bits: int = 8
    width: int = 32
    memory_regions: List[MemoryRegion] = field(default_factory=list)

    # Peripherals and interrupts
    peripherals: Dict[str, Peripheral] = field(default_factory=dict)
    interrupts: List[Interrupt] = field(default_factory=list)

    # Pins (if available)
    pins: List[Pin] = field(default_factory=list)

    # Metadata
    is_board_mcu: bool = False
    svd_path: Optional[Path] = None

    def get_interrupt_by_name(self, name: str) -> Optional[Interrupt]:
        """Find interrupt by name"""
        for irq in self.interrupts:
            if irq.name == name:
                return irq
        return None

    def get_peripheral_by_name(self, name: str) -> Optional[Peripheral]:
        """Find peripheral by name"""
        return self.peripherals.get(name)

    def get_memory_region(self, name: str) -> Optional[MemoryRegion]:
        """Find memory region by name (e.g., 'FLASH', 'RAM')"""
        for region in self.memory_regions:
            if region.name.upper() == name.upper():
                return region
        return None


# ============================================================================
# SVD PARSER
# ============================================================================

class SVDParser:
    """
    Generic SVD parser that extracts all device information.

    This parser is designed to be:
    - Reusable across all generators
    - Vendor-agnostic
    - Comprehensive (extracts all useful information)
    - Robust (handles missing/malformed data gracefully)

    Example:
        parser = SVDParser(Path("STM32F103.svd"))
        device = parser.parse()

        if device:
            print(f"Device: {device.name}")
            print(f"Vendor: {device.vendor_normalized}")
            print(f"Family: {device.family}")
            print(f"CPU: {device.cpu_name}")
            print(f"Interrupts: {len(device.interrupts)}")
    """

    def __init__(self, svd_path: Path, auto_classify: bool = True):
        """
        Initialize parser.

        Args:
            svd_path: Path to SVD file
            auto_classify: If True, automatically detect vendor/family
        """
        self.svd_path = svd_path
        self.auto_classify = auto_classify
        self.tree: Optional[ET.ElementTree] = None
        self.root: Optional[ET.Element] = None

    def parse(self) -> Optional[SVDDevice]:
        """
        Parse the SVD file and return device information.

        Returns:
            SVDDevice object with all parsed information, or None if parsing failed
        """
        try:
            # Parse XML
            self.tree = ET.parse(self.svd_path)
            self.root = self.tree.getroot()

            # Extract basic device info
            device_name = self._get_text('name', required=True)
            if not device_name:
                logger.warning(f"No device name in {self.svd_path.name}")
                return None

            vendor_raw = self._get_text('vendor') or self._infer_vendor_from_path()
            vendor_normalized = normalize_vendor(vendor_raw)

            # Create device object
            device = SVDDevice(
                name=device_name,
                vendor=vendor_raw,
                vendor_normalized=vendor_normalized,
                family=detect_family(device_name, vendor_normalized) if self.auto_classify else device_name.lower(),
                description=self._get_text('description'),
                version=self._get_text('version'),
                svd_path=self.svd_path,
                is_board_mcu=is_board_mcu(device_name),
            )

            # Parse CPU information
            self._parse_cpu(device)

            # Parse memory layout
            self._parse_memory(device)

            # Parse peripherals and interrupts
            self._parse_peripherals(device)

            # Deduplicate and sort interrupts
            device.interrupts = self._deduplicate_interrupts(device.interrupts)

            logger.debug(
                f"Parsed {device.name}: {len(device.peripherals)} peripherals, "
                f"{len(device.interrupts)} interrupts"
            )

            return device

        except ET.ParseError as e:
            logger.error(f"XML parse error in {self.svd_path.name}: {e}")
            return None
        except Exception as e:
            logger.error(f"Failed to parse {self.svd_path.name}: {e}")
            return None

    # ========================================================================
    # PRIVATE PARSING METHODS
    # ========================================================================

    def _get_text(self, tag: str, parent: Optional[ET.Element] = None,
                  required: bool = False, default: str = None) -> Optional[str]:
        """Get text content of an XML element"""
        elem = (parent or self.root).find(tag)
        if elem is not None and elem.text:
            return elem.text.strip()

        if required:
            raise ValueError(f"Required tag '{tag}' not found")

        return default

    def _get_int(self, tag: str, parent: Optional[ET.Element] = None,
                 default: int = None) -> Optional[int]:
        """Get integer value from XML element (handles hex)"""
        text = self._get_text(tag, parent)
        if text is None:
            return default

        try:
            # Handle hex (0x prefix) and decimal
            if text.lower().startswith('0x'):
                return int(text, 16)
            else:
                return int(text, 0)  # Auto-detect base
        except ValueError:
            logger.debug(f"Failed to parse int from '{text}'")
            return default

    def _infer_vendor_from_path(self) -> str:
        """Infer vendor from SVD file path"""
        # SVD files are usually organized as: .../data/VendorName/.../device.svd
        current = self.svd_path.parent
        while current.name and current.name != 'data':
            if current.parent.name == 'data':
                return current.name
            current = current.parent
        return "Unknown"

    def _parse_cpu(self, device: SVDDevice):
        """Parse CPU information"""
        cpu = self.root.find('cpu')
        if cpu is None:
            return

        device.cpu_name = self._get_text('name', cpu)
        device.cpu_revision = self._get_text('revision', cpu)
        device.cpu_endian = self._get_text('endian', cpu)
        device.cpu_mpuPresent = self._get_text('mpuPresent', cpu) == 'true'
        device.cpu_fpuPresent = self._get_text('fpuPresent', cpu) == 'true'
        device.cpu_nvicPrioBits = self._get_int('nvicPrioBits', cpu)

    def _parse_memory(self, device: SVDDevice):
        """Parse memory regions"""
        # Try to find memory map in various locations
        # Some SVDs have addressUnitBits/width at device level
        device.address_unit_bits = self._get_int('addressUnitBits', default=8)
        device.width = self._get_int('width', default=32)

        # Look for memory regions (not standardized in SVD)
        # This is vendor-specific, so we'll try common patterns

        # Method 1: Some vendors define memory in peripherals
        for peripheral in self.root.findall('.//peripheral'):
            name = self._get_text('name', peripheral)
            if name and any(mem in name.upper() for mem in ['FLASH', 'SRAM', 'RAM']):
                base = self._get_int('baseAddress', peripheral)
                # Size is usually in description or not available
                if base is not None:
                    device.memory_regions.append(
                        MemoryRegion(name=name, start=base, size=0)
                    )

    def _parse_peripherals(self, device: SVDDevice):
        """Parse all peripherals and their interrupts"""
        seen_peripherals = set()

        for peripheral_elem in self.root.findall('.//peripheral'):
            name = self._get_text('name', peripheral_elem)
            if not name or name in seen_peripherals:
                continue

            seen_peripherals.add(name)

            # Basic peripheral info
            peripheral = Peripheral(
                name=name,
                base_address=self._get_int('baseAddress', peripheral_elem, default=0),
                description=self._get_text('description', peripheral_elem),
                group_name=self._get_text('groupName', peripheral_elem),
            )

            # Parse registers
            for reg_elem in peripheral_elem.findall('.//register'):
                register = self._parse_register(reg_elem)
                if register:
                    peripheral.registers.append(register)

            # Parse interrupts for this peripheral
            for irq_elem in peripheral_elem.findall('interrupt'):
                interrupt = self._parse_interrupt(irq_elem)
                if interrupt:
                    peripheral.interrupts.append(interrupt)
                    device.interrupts.append(interrupt)

            device.peripherals[name] = peripheral

    def _parse_register(self, reg_elem: ET.Element) -> Optional[Register]:
        """Parse a single register"""
        name = self._get_text('name', reg_elem)
        if not name:
            return None

        offset = self._get_int('addressOffset', reg_elem)
        if offset is None:
            return None

        return Register(
            name=name,
            offset=offset,
            size=self._get_int('size', reg_elem, default=32),
            reset_value=self._get_int('resetValue', reg_elem),
            description=self._get_text('description', reg_elem),
            access=self._get_text('access', reg_elem),
        )

    def _parse_interrupt(self, irq_elem: ET.Element) -> Optional[Interrupt]:
        """Parse a single interrupt"""
        name = self._get_text('name', irq_elem)
        value = self._get_int('value', irq_elem)

        if name is None or value is None:
            return None

        return Interrupt(
            name=name,
            value=value,
            description=self._get_text('description', irq_elem),
        )

    def _deduplicate_interrupts(self, interrupts: List[Interrupt]) -> List[Interrupt]:
        """Remove duplicate interrupts and sort by IRQ number"""
        seen = {}
        for irq in interrupts:
            if irq.name not in seen:
                seen[irq.name] = irq
            else:
                # Keep the one with better description
                if irq.description and not seen[irq.name].description:
                    seen[irq.name] = irq

        return sorted(seen.values(), key=lambda x: x.value)


# ============================================================================
# CONVENIENCE FUNCTIONS
# ============================================================================

def parse_svd(svd_path: Path, auto_classify: bool = True) -> Optional[SVDDevice]:
    """
    Convenience function to parse an SVD file.

    Args:
        svd_path: Path to SVD file
        auto_classify: Auto-detect vendor and family

    Returns:
        Parsed device or None if parsing failed
    """
    parser = SVDParser(svd_path, auto_classify)
    return parser.parse()


def find_svd_for_mcu(mcu_name: str, svd_dir: Path) -> Optional[Path]:
    """
    Find SVD file for a specific MCU.

    Args:
        mcu_name: MCU name (e.g., "STM32F103C8")
        svd_dir: Root SVD directory

    Returns:
        Path to SVD file or None if not found
    """
    mcu_upper = mcu_name.upper()

    # Search for matching SVD file
    for svd_file in svd_dir.rglob("*.svd"):
        try:
            tree = ET.parse(svd_file)
            root = tree.getroot()
            name_elem = root.find('name')

            if name_elem is not None:
                device_name = name_elem.text.strip().upper()
                # Check exact match or prefix match
                if device_name == mcu_upper or device_name.startswith(mcu_upper):
                    return svd_file
                if mcu_upper in device_name or device_name.startswith(mcu_upper.split('-')[0]):
                    return svd_file
        except:
            pass

    return None


# ============================================================================
# MAIN - FOR TESTING
# ============================================================================

def main():
    """Test the parser with command line arguments"""
    import argparse
    from cli.core.config import SVD_DIR
    from cli.core.logger import print_info, print_success, print_error

    parser = argparse.ArgumentParser(description='Test SVD parser')
    parser.add_argument('svd_file', help='SVD file to parse (relative to SVD_DIR or absolute)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    args = parser.parse_args()

    # Resolve path
    svd_path = Path(args.svd_file)
    if not svd_path.is_absolute():
        svd_path = SVD_DIR / svd_path

    if not svd_path.exists():
        print_error(f"SVD file not found: {svd_path}")
        return 1

    print_info(f"Parsing {svd_path.name}...")

    # Parse
    device = parse_svd(svd_path)

    if not device:
        print_error("Failed to parse SVD file")
        return 1

    # Display results
    print_success(f"Successfully parsed {device.name}")
    print()
    print(f"  Vendor:        {device.vendor} ({device.vendor_normalized})")
    print(f"  Family:        {device.family}")
    print(f"  Board MCU:     {device.is_board_mcu}")
    print(f"  Description:   {device.description or 'N/A'}")
    print()
    print(f"  CPU:           {device.cpu_name or 'N/A'}")
    print(f"  FPU:           {'Yes' if device.cpu_fpuPresent else 'No'}")
    print(f"  MPU:           {'Yes' if device.cpu_mpuPresent else 'No'}")
    print()
    print(f"  Peripherals:   {len(device.peripherals)}")
    print(f"  Interrupts:    {len(device.interrupts)}")
    print(f"  Memory Rgns:   {len(device.memory_regions)}")

    if args.verbose:
        print()
        print("Interrupts:")
        for irq in device.interrupts[:10]:  # Show first 10
            print(f"    {irq.value:3d}: {irq.name}")
        if len(device.interrupts) > 10:
            print(f"    ... and {len(device.interrupts) - 10} more")

        print()
        print("Peripherals:")
        for name, periph in list(device.peripherals.items())[:10]:
            print(f"    {name:20s} @ 0x{periph.base_address:08X}")
        if len(device.peripherals) > 10:
            print(f"    ... and {len(device.peripherals) - 10} more")

    return 0


if __name__ == '__main__':
    sys.exit(main())
