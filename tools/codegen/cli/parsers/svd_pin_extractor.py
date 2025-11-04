#!/usr/bin/env python3
"""
SVD Pin Extractor - Extract GPIO pin information from SVD files

This module parses CMSIS-SVD files to extract GPIO port and pin information,
including memory layout, peripheral availability, and device characteristics.
"""

import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field
import re
import json


@dataclass
class MCUPackageInfo:
    """Package-specific pin configuration"""
    package_name: str  # e.g., "LQFP48", "LQFP64"
    total_pins: int
    available_ports: Dict[str, List[int]]  # Port letter -> list of pin numbers

    def get_gpio_pin_count(self) -> int:
        """Count total GPIO pins"""
        return sum(len(pins) for pins in self.available_ports.values())


@dataclass
class MCUInfo:
    """Complete MCU information extracted from SVD"""
    device_name: str
    vendor: str
    family: str
    series: str

    # Memory
    flash_kb: int
    sram_kb: int

    # GPIO
    gpio_ports: List[str]  # ['A', 'B', 'C', 'D']
    max_pins_per_port: int = 16

    # Package variants
    packages: List[MCUPackageInfo] = field(default_factory=list)

    # Peripherals
    uart_count: int = 0
    i2c_count: int = 0
    spi_count: int = 0
    adc_count: int = 0
    adc_channels: int = 0
    timer_count: int = 0
    dma_channels: int = 0

    # Features
    has_usb: bool = False
    has_can: bool = False
    has_dac: bool = False
    has_rtc: bool = False


class SVDPinExtractor:
    """Extract pin and peripheral information from SVD files"""

    def __init__(self, svd_path: Path, verbose: bool = False):
        self.svd_path = svd_path
        self.verbose = verbose

        # Parse SVD
        self.tree = ET.parse(svd_path)
        self.root = self.tree.getroot()

    def extract(self) -> MCUInfo:
        """Extract all MCU information from SVD"""

        # Basic device info
        device_name = self._get_text('.//name')  # name is direct child of device/root
        vendor = self._get_text('.//vendor', 'Unknown')

        if self.verbose:
            print(f"Extracting info from {device_name}...")

        # Detect family and series
        family, series = self._detect_family_series(device_name)

        # Memory information
        flash_kb, sram_kb = self._extract_memory()

        # GPIO ports
        gpio_ports = self._extract_gpio_ports()

        # Peripherals
        peripherals = self._extract_peripherals()

        mcu_info = MCUInfo(
            device_name=device_name,
            vendor=vendor,
            family=family,
            series=series,
            flash_kb=flash_kb,
            sram_kb=sram_kb,
            gpio_ports=gpio_ports,
            **peripherals
        )

        # Try to infer package information
        mcu_info.packages = self._infer_packages(mcu_info)

        return mcu_info

    def _get_text(self, xpath: str, default: str = '') -> str:
        """Get text from XML element"""
        elem = self.root.find(xpath)
        return elem.text.strip() if elem is not None and elem.text else default

    def _get_int(self, xpath: str, default: int = 0) -> int:
        """Get integer from XML element"""
        text = self._get_text(xpath)
        if not text:
            return default
        try:
            # Handle hex values
            if text.startswith('0x') or text.startswith('0X'):
                return int(text, 16)
            return int(text)
        except ValueError:
            return default

    def _detect_family_series(self, device_name: str) -> Tuple[str, str]:
        """Detect MCU family and series from device name"""
        name_lower = device_name.lower()

        # STM32 patterns
        if name_lower.startswith('stm32'):
            # STM32F103xx -> family=stm32f1, series=stm32f103
            match = re.match(r'stm32([a-z]\d+)([a-z]\d+)?', name_lower)
            if match:
                family = f"stm32{match.group(1)}"
                series = device_name[:9] if len(device_name) >= 9 else device_name
                return family, series
            return "stm32", device_name

        # ESP32 patterns
        elif name_lower.startswith('esp'):
            match = re.match(r'esp(\d+[a-z]?\d*)', name_lower.replace('-', ''))
            if match:
                family = f"esp{match.group(1)}"
                return family, device_name
            return "esp32", device_name

        # Generic
        return "unknown", device_name

    def _extract_memory(self) -> Tuple[int, int]:
        """Extract flash and SRAM sizes from SVD"""
        flash_kb = 0
        sram_kb = 0

        # Look for memory elements
        for memory in self.root.findall('.//memory'):
            name = self._get_text_from_elem(memory, 'name', '').upper()
            size = self._get_int_from_elem(memory, 'size', 0)

            if 'FLASH' in name or 'IROM' in name:
                flash_kb = size // 1024
            elif 'RAM' in name or 'SRAM' in name or 'IRAM' in name:
                sram_kb = size // 1024

        # Fallback: try to infer from device name
        if flash_kb == 0:
            flash_kb = self._infer_flash_from_name(self._get_text('.//device/name'))

        return flash_kb, sram_kb

    def _infer_flash_from_name(self, device_name: str) -> int:
        """Infer flash size from device name (e.g., STM32F103C8 = 64KB)"""
        name_upper = device_name.upper()

        # STM32 flash size codes
        flash_codes = {
            '4': 16,   # 16KB
            '6': 32,   # 32KB
            '8': 64,   # 64KB
            'B': 128,  # 128KB
            'C': 256,  # 256KB
            'D': 384,  # 384KB
            'E': 512,  # 512KB
            'F': 768,  # 768KB
            'G': 1024, # 1MB
        }

        # Extract flash code (usually 2nd to last character)
        if len(name_upper) >= 2:
            flash_code = name_upper[-2]
            if flash_code in flash_codes:
                return flash_codes[flash_code]

        return 0

    def _extract_gpio_ports(self) -> List[str]:
        """Extract GPIO port names from peripherals"""
        ports = []

        for peripheral in self.root.findall('.//peripheral'):
            name = self._get_text_from_elem(peripheral, 'name', '')
            group = self._get_text_from_elem(peripheral, 'groupName', '')

            # Look for GPIO peripherals
            if 'GPIO' in group or name.startswith('GPIO'):
                # Extract port letter (GPIOA -> A)
                match = re.search(r'GPIO([A-Z])', name)
                if match:
                    port_letter = match.group(1)
                    if port_letter not in ports:
                        ports.append(port_letter)

        return sorted(ports)

    def _extract_peripherals(self) -> Dict:
        """Count peripherals from SVD"""
        peripherals = {
            'uart_count': 0,
            'i2c_count': 0,
            'spi_count': 0,
            'adc_count': 0,
            'adc_channels': 0,
            'timer_count': 0,
            'dma_channels': 0,
            'has_usb': False,
            'has_can': False,
            'has_dac': False,
            'has_rtc': False,
        }

        seen_peripherals = set()

        for peripheral in self.root.findall('.//peripheral'):
            name = self._get_text_from_elem(peripheral, 'name', '').upper()

            # Skip duplicates
            if name in seen_peripherals:
                continue
            seen_peripherals.add(name)

            # Note: We DO count derived peripherals because they represent
            # separate hardware instances (e.g., I2C2 is derivedFrom I2C1 but is a real peripheral)

            # Count UART/USART
            if 'USART' in name or 'UART' in name:
                peripherals['uart_count'] += 1

            # Count I2C
            elif 'I2C' in name:
                peripherals['i2c_count'] += 1

            # Count SPI
            elif 'SPI' in name:
                peripherals['spi_count'] += 1

            # Count ADC
            elif name.startswith('ADC'):
                peripherals['adc_count'] += 1
                # Try to count channels from registers
                channels = self._count_adc_channels(peripheral)
                if channels > peripherals['adc_channels']:
                    peripherals['adc_channels'] = channels

            # Count timers
            elif 'TIM' in name and not 'STIM' in name:  # Exclude STIMU
                peripherals['timer_count'] += 1

            # Check for USB
            elif 'USB' in name:
                peripherals['has_usb'] = True

            # Check for CAN
            elif 'CAN' in name:
                peripherals['has_can'] = True

            # Check for DAC
            elif 'DAC' in name:
                peripherals['has_dac'] = True

            # Check for RTC
            elif 'RTC' in name:
                peripherals['has_rtc'] = True

        # Count DMA channels
        for peripheral in self.root.findall('.//peripheral'):
            name = self._get_text_from_elem(peripheral, 'name', '').upper()
            if 'DMA' in name and 'Channel' in name:
                peripherals['dma_channels'] += 1

        return peripherals

    def _count_adc_channels(self, adc_peripheral: ET.Element) -> int:
        """Count ADC channels from registers"""
        channels = 0

        # Look for SQR (sequence) registers which indicate channels
        for register in adc_peripheral.findall('.//register'):
            reg_name = self._get_text_from_elem(register, 'name', '')
            if 'SQR' in reg_name or 'SMPR' in reg_name:
                # Count channel fields
                for field in register.findall('.//field'):
                    field_name = self._get_text_from_elem(field, 'name', '')
                    # Extract channel number (e.g., SQ1, SMP10)
                    match = re.search(r'(\d+)', field_name)
                    if match:
                        ch_num = int(match.group(1))
                        if ch_num > channels:
                            channels = ch_num

        return min(channels + 1, 18)  # Most STM32 have max 18 channels

    def _infer_packages(self, mcu_info: MCUInfo) -> List[MCUPackageInfo]:
        """Infer package information from device name and GPIO ports"""
        packages = []
        device_name = mcu_info.device_name.upper()

        # STM32 package detection from device name
        # Format: STM32F103C8T6 -> C = LQFP48
        package_codes = {
            'C': ('LQFP48', 48),    # Medium pin count
            'R': ('LQFP64', 64),    # Medium-high pin count
            'V': ('LQFP100', 100),  # High pin count
            'Z': ('LQFP144', 144),  # Very high pin count
            'T': ('LQFP36', 36),    # Low pin count
        }

        # Extract package code (usually 6th character for STM32)
        package_code = None
        if device_name.startswith('STM32') and len(device_name) >= 11:
            package_code = device_name[10]

        if package_code and package_code in package_codes:
            package_name, total_pins = package_codes[package_code]

            # Determine available pins per port based on package
            available_ports = {}

            for port in mcu_info.gpio_ports:
                if package_name == 'LQFP48':
                    # LQFP48 limitations
                    if port in ['A', 'B']:
                        available_ports[port] = list(range(16))  # PA0-PA15, PB0-PB15
                    elif port == 'C':
                        available_ports[port] = [13, 14, 15]  # PC13-PC15 only
                    elif port == 'D':
                        available_ports[port] = [0, 1]  # PD0-PD1 only (OSC)

                elif package_name == 'LQFP64':
                    # LQFP64 has more pins
                    if port in ['A', 'B', 'C', 'D']:
                        available_ports[port] = list(range(16))  # Full ports
                    elif port == 'E':
                        available_ports[port] = list(range(16))  # PE available

                elif package_name in ['LQFP100', 'LQFP144']:
                    # Larger packages have all pins
                    available_ports[port] = list(range(16))

                else:
                    # Default: assume all pins available
                    available_ports[port] = list(range(16))

            package = MCUPackageInfo(
                package_name=package_name,
                total_pins=total_pins,
                available_ports=available_ports
            )
            packages.append(package)

        # If no package detected, create default based on GPIO ports
        if not packages:
            available_ports = {port: list(range(16)) for port in mcu_info.gpio_ports}
            package = MCUPackageInfo(
                package_name="Unknown",
                total_pins=0,
                available_ports=available_ports
            )
            packages.append(package)

        return packages

    def _get_text_from_elem(self, elem: ET.Element, tag: str, default: str = '') -> str:
        """Get text from child element"""
        child = elem.find(tag)
        return child.text.strip() if child is not None and child.text else default

    def _get_int_from_elem(self, elem: ET.Element, tag: str, default: int = 0) -> int:
        """Get integer from child element"""
        text = self._get_text_from_elem(elem, tag)
        if not text:
            return default
        try:
            if text.startswith('0x') or text.startswith('0X'):
                return int(text, 16)
            return int(text)
        except ValueError:
            return default


def extract_mcu_info_from_svd(svd_path: Path, verbose: bool = False) -> MCUInfo:
    """Convenience function to extract MCU info from SVD file"""
    extractor = SVDPinExtractor(svd_path, verbose=verbose)
    return extractor.extract()


def export_to_json(mcu_info: MCUInfo, output_path: Path):
    """Export MCU info to JSON format"""
    data = {
        "device_name": mcu_info.device_name,
        "vendor": mcu_info.vendor,
        "family": mcu_info.family,
        "series": mcu_info.series,
        "memory": {
            "flash_kb": mcu_info.flash_kb,
            "sram_kb": mcu_info.sram_kb
        },
        "gpio": {
            "ports": mcu_info.gpio_ports,
            "max_pins_per_port": mcu_info.max_pins_per_port
        },
        "packages": [
            {
                "name": pkg.package_name,
                "total_pins": pkg.total_pins,
                "gpio_pins": pkg.get_gpio_pin_count(),
                "available_ports": pkg.available_ports
            }
            for pkg in mcu_info.packages
        ],
        "peripherals": {
            "uart_count": mcu_info.uart_count,
            "i2c_count": mcu_info.i2c_count,
            "spi_count": mcu_info.spi_count,
            "adc_count": mcu_info.adc_count,
            "adc_channels": mcu_info.adc_channels,
            "timer_count": mcu_info.timer_count,
            "dma_channels": mcu_info.dma_channels
        },
        "features": {
            "has_usb": mcu_info.has_usb,
            "has_can": mcu_info.has_can,
            "has_dac": mcu_info.has_dac,
            "has_rtc": mcu_info.has_rtc
        }
    }

    with open(output_path, 'w') as f:
        json.dump(data, f, indent=2)


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: python svd_pin_extractor.py <svd_file> [output.json]")
        sys.exit(1)

    svd_file = Path(sys.argv[1])
    output_file = Path(sys.argv[2]) if len(sys.argv) > 2 else None

    print(f"Extracting MCU info from {svd_file.name}...")
    mcu_info = extract_mcu_info_from_svd(svd_file, verbose=True)

    print(f"\n✓ Extracted info for {mcu_info.device_name}")
    print(f"  Vendor: {mcu_info.vendor}")
    print(f"  Family: {mcu_info.family}")
    print(f"  Flash: {mcu_info.flash_kb}KB, RAM: {mcu_info.sram_kb}KB")
    print(f"  GPIO Ports: {', '.join(mcu_info.gpio_ports)}")
    print(f"  Peripherals: {mcu_info.uart_count} UART, {mcu_info.i2c_count} I2C, {mcu_info.spi_count} SPI")

    for pkg in mcu_info.packages:
        print(f"  Package: {pkg.package_name} ({pkg.total_pins} pins, {pkg.get_gpio_pin_count()} GPIO)")

    if output_file:
        export_to_json(mcu_info, output_file)
        print(f"\n✓ Exported to {output_file}")
