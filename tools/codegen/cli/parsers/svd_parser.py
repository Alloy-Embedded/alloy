#!/usr/bin/env python3
"""
SVD Parser

Parses CMSIS-SVD XML files and converts them to Alloy JSON database format.

Usage:
    python svd_parser.py --input STM32F103.svd --output stm32f1xx.json
    python svd_parser.py --input STM32F103.svd --output stm32f1xx.json --merge
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, Any, List, Optional
import xml.etree.ElementTree as ET

class Colors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    INFO = '\033[96m'
    ENDC = '\033[0m'

def print_success(msg: str):
    print(f"{Colors.OKGREEN}✓{Colors.ENDC} {msg}")

def print_info(msg: str):
    print(f"{Colors.INFO}→{Colors.ENDC} {msg}")

def print_warning(msg: str):
    print(f"{Colors.WARNING}⚠{Colors.ENDC} {msg}")

def print_error(msg: str):
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}")


class SVDParser:
    """Parser for CMSIS-SVD XML files"""

    def __init__(self, svd_path: Path, verbose: bool = False):
        self.svd_path = svd_path
        self.verbose = verbose

        # Load and parse XML
        if not svd_path.exists():
            print_error(f"SVD file not found: {svd_path}")
            sys.exit(1)

        try:
            self.tree = ET.parse(svd_path)
            self.root = self.tree.getroot()
        except ET.ParseError as e:
            print_error(f"Failed to parse SVD XML: {e}")
            sys.exit(1)

    def parse(self) -> Dict[str, Any]:
        """Parse SVD file and return database dict"""
        print_info(f"Parsing {self.svd_path.name}...")

        # Extract information
        device_name = self._get_device_name()
        print_info(f"Device: {device_name}")

        mcu_config = {
            "flash": self._parse_flash(),
            "ram": self._parse_ram(),
            "peripherals": self._parse_peripherals(),
            "interrupts": self._parse_interrupts()
        }

        # Detect architecture and vendor
        cpu = self.root.find('.//cpu')
        architecture = self._detect_architecture(cpu)
        vendor = self._get_text('.//vendor', default='Unknown')

        # Create database structure
        database = {
            "family": self._extract_family(device_name),
            "architecture": architecture,
            "vendor": vendor,
            "mcus": {
                device_name: mcu_config
            }
        }

        print_success(f"Parsed {device_name} successfully")
        return database

    def _get_device_name(self) -> str:
        """Extract device name from SVD"""
        name = self._get_text('.//name')
        if not name:
            print_error("Could not find device name in SVD")
            sys.exit(1)
        return name

    def _get_int(self, xpath: str, default: int = 0) -> int:
        """Get integer value from element"""
        text = self._get_text(xpath)
        if not text:
            return default

        # Handle hex (0x...) and decimal
        try:
            if text.startswith('0x') or text.startswith('0X'):
                return int(text, 16)
            else:
                return int(text)
        except ValueError:
            return default

    def _extract_family(self, device_name: str) -> str:
        """Extract family name from device name"""
        # STM32F103 -> STM32F1
        # nRF52832 -> nRF52
        if device_name.startswith('STM32'):
            return device_name[:7]  # STM32F1
        elif device_name.startswith('nRF'):
            return device_name[:5]  # nRF52
        else:
            return device_name.split(' ')[0]

    def _detect_architecture(self, cpu: Optional[ET.Element]) -> str:
        """Detect CPU architecture from SVD"""
        if cpu is None:
            return "arm-cortex-m3"  # Default

        name = cpu.find('name')
        if name is not None and name.text:
            cpu_name = name.text.strip().lower()
            if 'cm0+' in cpu_name or 'cm0plus' in cpu_name:
                return "arm-cortex-m0plus"
            elif 'cm0' in cpu_name:
                return "arm-cortex-m0"
            elif 'cm3' in cpu_name:
                return "arm-cortex-m3"
            elif 'cm4' in cpu_name:
                # Check if has FPU
                fpu_present = cpu.find('fpuPresent')
                if fpu_present is not None and fpu_present.text == 'true':
                    return "arm-cortex-m4f"
                return "arm-cortex-m4"
            elif 'cm7' in cpu_name:
                return "arm-cortex-m7"
            elif 'cm33' in cpu_name:
                return "arm-cortex-m33"

        return "arm-cortex-m3"

    def _parse_flash(self) -> Dict[str, Any]:
        """Parse flash memory information"""
        # Try to find flash in memory map
        for mem in self.root.findall('.//memory'):
            name = self._get_text('.//name', parent=mem)
            if 'FLASH' in name.upper() or 'IROM' in name.upper():
                size = self._get_int('.//size', parent=mem)
                start = self._get_int('.//start', parent=mem)

                if size > 0 and start > 0:
                    return {
                        "size_kb": size // 1024,
                        "base_address": f"0x{start:08X}",
                        "page_size_kb": 1  # Default, can be refined
                    }

        # Fallback: Common STM32 values
        print_warning("Flash info not found in SVD, using defaults")
        return {
            "size_kb": 64,
            "base_address": "0x08000000",
            "page_size_kb": 1
        }

    def _parse_ram(self) -> Dict[str, Any]:
        """Parse RAM memory information"""
        # Try to find RAM in memory map
        for mem in self.root.findall('.//memory'):
            name = self._get_text('.//name', parent=mem)
            if 'RAM' in name.upper() or 'SRAM' in name.upper() or 'IRAM' in name.upper():
                size = self._get_int('.//size', parent=mem)
                start = self._get_int('.//start', parent=mem)

                if size > 0 and start > 0:
                    return {
                        "size_kb": size // 1024,
                        "base_address": f"0x{start:08X}"
                    }

        # Fallback
        print_warning("RAM info not found in SVD, using defaults")
        return {
            "size_kb": 20,
            "base_address": "0x20000000"
        }

    def _get_text(self, xpath: str, parent: Optional[ET.Element] = None, default: str = '') -> str:
        """Get text with optional parent element"""
        elem = (parent or self.root).find(xpath)
        if elem is not None and elem.text:
            return elem.text.strip()
        return default

    def _get_int(self, xpath: str, parent: Optional[ET.Element] = None, default: int = 0) -> int:
        """Get integer with optional parent element"""
        text = self._get_text(xpath, parent)
        if not text:
            return default

        try:
            if text.startswith('0x') or text.startswith('0X'):
                return int(text, 16)
            else:
                return int(text)
        except ValueError:
            return default

    def _parse_peripherals(self) -> Dict[str, Any]:
        """Parse peripheral definitions"""
        peripherals = {}

        for periph in self.root.findall('.//peripheral'):
            name = self._get_text('.//name', parent=periph)
            base_addr = self._get_int('.//baseAddress', parent=periph)

            if not name or base_addr == 0:
                continue

            # Determine peripheral type (GPIO, USART, etc.)
            periph_type = self._classify_peripheral(name)

            if periph_type not in peripherals:
                peripherals[periph_type] = {
                    "instances": [],
                    "registers": {}
                }

            # Add instance
            instance = {
                "name": name,
                "base": f"0x{base_addr:08X}"
            }

            # Try to find IRQ number
            irq = periph.find('.//interrupt/value')
            if irq is not None and irq.text:
                try:
                    instance["irq"] = int(irq.text)
                except ValueError:
                    pass

            peripherals[periph_type]["instances"].append(instance)

            # Parse registers (if not already parsed for this type)
            if not peripherals[periph_type]["registers"]:
                registers = self._parse_registers(periph)
                if registers:
                    peripherals[periph_type]["registers"] = registers

                # Parse bit fields
                bit_fields = self._parse_bit_fields(periph)
                if bit_fields:
                    peripherals[periph_type]["bits"] = bit_fields

        if self.verbose:
            print_info(f"Found {len(peripherals)} peripheral types")

        return peripherals

    def _classify_peripheral(self, name: str) -> str:
        """Classify peripheral by name

        Supports multiple vendor naming conventions:
        - STM32: GPIO, USART, TIM, etc.
        - Nordic: GPIOTE, UARTE, TWIM/TWIS, TIMER
        - Atmel: PORT, SERCOM, TC/TCC
        - NXP: TPM, PIT, LPTMR
        - Espressif: TIMG, LEDC, TWAI
        """
        name_upper = name.upper()

        # GPIO variants
        if 'GPIO' in name_upper or 'PORT' in name_upper and 'REPORT' not in name_upper:
            return 'GPIO'

        # UART/USART variants (including UARTE from Nordic)
        elif 'USART' in name_upper or 'UART' in name_upper:
            return 'USART'

        # SPI variants (including SPIM/SPIS from Nordic, QSPI)
        elif 'SPI' in name_upper:
            return 'SPI'

        # I2C variants (I2C, TWI, TWIM/TWIS from Nordic)
        elif 'I2C' in name_upper or 'I²C' in name_upper or 'TWI' in name_upper:
            return 'I2C'

        # Serial Communication (Atmel SERCOM can be UART/SPI/I2C)
        elif 'SERCOM' in name_upper:
            return 'SERCOM'

        # Timer variants (TIM, TIMER, TC, TCC, TPM, PIT, LPTMR, TIMG)
        elif (('TIM' in name_upper and 'SYSTICK' not in name_upper and 'OPTIM' not in name_upper) or
              'TIMER' in name_upper or
              name_upper.startswith('TC') and len(name_upper) <= 4 or  # TC3, TCC0, etc
              name_upper.startswith('TCC') and len(name_upper) <= 5 or
              'TPM' in name_upper or
              'PIT' in name_upper and 'SPIT' not in name_upper or
              'LPTMR' in name_upper):
            return 'TIM'

        # ADC variants (ADC, SAADC from Nordic, SARADC from ESP32)
        elif 'ADC' in name_upper:
            return 'ADC'

        # DAC
        elif 'DAC' in name_upper:
            return 'DAC'

        # DMA variants (DMA, DMAC from Atmel, DMAMUX)
        elif 'DMA' in name_upper:
            return 'DMA'

        # PWM/LED Controller (LEDC from ESP32)
        elif 'PWM' in name_upper or 'LEDC' in name_upper:
            return 'PWM'

        # CAN variants (CAN, TWAI from ESP32)
        elif 'CAN' in name_upper or 'TWAI' in name_upper:
            return 'CAN'

        # USB
        elif 'USB' in name_upper:
            return 'USB'

        # Ethernet
        elif 'ETH' in name_upper or 'EMAC' in name_upper:
            return 'ETH'

        # Clock/Reset Control (RCC, CLOCK, MCG, SYSCTRL, SYSTEM)
        elif 'RCC' in name_upper or ('CLOCK' in name_upper and 'UNCLOCK' not in name_upper) or 'MCG' in name_upper:
            return 'RCC'

        # Power Management
        elif 'PWR' in name_upper or 'POWER' in name_upper:
            return 'PWR'

        # Real-Time Clock
        elif 'RTC' in name_upper:
            return 'RTC'

        # Watchdog (WDT, WWDG, IWDG)
        elif 'WDG' in name_upper or 'WDT' in name_upper:
            return 'WDG'

        # SDIO/SD/MMC
        elif 'SDIO' in name_upper or 'SDMMC' in name_upper:
            return 'SDIO'

        # Flash Controller
        elif 'FLASH' in name_upper or 'FTFA' in name_upper or 'NVMCTRL' in name_upper:
            return 'FLASH'

        # I2S (Audio)
        elif 'I2S' in name_upper:
            return 'I2S'

        # Cryptography (AES, SHA, RSA, etc.)
        elif any(crypto in name_upper for crypto in ['AES', 'SHA', 'RSA', 'HMAC', 'CRYP', 'CRYPTO']):
            return 'CRYPTO'

        # Random Number Generator
        elif 'RNG' in name_upper or 'TRNG' in name_upper:
            return 'RNG'

        # Comparator
        elif 'COMP' in name_upper and 'LPCOMP' not in name_upper:
            return 'COMP'

        # Radio (Nordic, ESP32)
        elif 'RADIO' in name_upper or 'BLE' in name_upper:
            return 'RADIO'

        else:
            # Return first word (e.g., EXTI, AFIO, NVIC, etc.)
            return name.split('_')[0].split('[')[0]

    def _parse_registers(self, periph: ET.Element) -> Dict[str, Any]:
        """Parse register definitions for a peripheral"""
        registers = {}

        for reg in periph.findall('.//register'):
            name = self._get_text('.//name', parent=reg)
            offset = self._get_int('.//addressOffset', parent=reg)
            size = self._get_int('.//size', parent=reg, default=32)

            if not name:
                continue

            registers[name] = {
                "offset": f"0x{offset:02X}",
                "size": size
            }

            # Add description if available
            desc = self._get_text('.//description', parent=reg)
            if desc:
                registers[name]["description"] = desc

        return registers

    def _parse_bit_fields(self, periph: ET.Element) -> Dict[str, Any]:
        """Parse bit field definitions for a peripheral"""
        bit_fields = {}

        for reg in periph.findall('.//register'):
            reg_name = self._get_text('.//name', parent=reg)
            if not reg_name:
                continue

            fields = reg.find('.//fields')
            if fields is None:
                continue

            reg_bits = {}
            for field in fields.findall('.//field'):
                field_name = self._get_text('.//name', parent=field)
                if not field_name:
                    continue

                # Get bit offset
                bit_offset = self._get_int('.//bitOffset', parent=field, default=-1)
                lsb = self._get_int('.//lsb', parent=field, default=-1)

                # Prefer bitOffset, fallback to lsb
                if bit_offset >= 0:
                    bit_pos = bit_offset
                elif lsb >= 0:
                    bit_pos = lsb
                else:
                    continue

                # Get bit width
                bit_width = self._get_int('.//bitWidth', parent=field, default=-1)
                msb = self._get_int('.//msb', parent=field, default=-1)

                # Calculate width
                if bit_width > 0:
                    width = bit_width
                elif msb >= 0 and lsb >= 0:
                    width = msb - lsb + 1
                else:
                    width = 1  # Default: single bit

                # Get description
                desc = self._get_text('.//description', parent=field)
                if not desc:
                    desc = field_name

                # Store bit field
                bit_info = {
                    "bit": bit_pos,
                    "description": desc
                }

                # Only add width if > 1
                if width > 1:
                    bit_info["width"] = width

                reg_bits[field_name] = bit_info

            # Only add register if it has fields
            if reg_bits:
                bit_fields[reg_name] = reg_bits

        return bit_fields

    def _parse_interrupts(self) -> Dict[str, Any]:
        """Parse interrupt vector table"""
        vectors = []

        # ARM Cortex-M standard vectors (always present)
        standard_vectors = [
            {"number": 0, "name": "Initial_SP"},
            {"number": 1, "name": "Reset_Handler"},
            {"number": 2, "name": "NMI_Handler"},
            {"number": 3, "name": "HardFault_Handler"},
            {"number": 4, "name": "MemManage_Handler"},
            {"number": 5, "name": "BusFault_Handler"},
            {"number": 6, "name": "UsageFault_Handler"},
            {"number": 11, "name": "SVC_Handler"},
            {"number": 12, "name": "DebugMon_Handler"},
            {"number": 14, "name": "PendSV_Handler"},
            {"number": 15, "name": "SysTick_Handler"}
        ]
        vectors.extend(standard_vectors)

        # Device-specific interrupts (use dict to avoid duplicates)
        device_interrupts = {}  # number -> name
        for periph in self.root.findall('.//peripheral'):
            for interrupt in periph.findall('.//interrupt'):
                name = self._get_text('.//name', parent=interrupt)
                value = self._get_int('.//value', parent=interrupt)

                if name and value >= 0:
                    # ARM Cortex-M: external interrupts start at 16
                    vector_num = value + 16
                    # Keep first occurrence of each vector number
                    if vector_num not in device_interrupts:
                        device_interrupts[vector_num] = f"{name}_IRQHandler"

        # Add device interrupts to vectors list
        for number, name in sorted(device_interrupts.items()):
            vectors.append({
                "number": number,
                "name": name
            })

        # Sort by number and get max
        vectors.sort(key=lambda x: x['number'])
        max_vector = max(v['number'] for v in vectors) if vectors else 16

        return {
            "count": max_vector + 1,
            "vectors": vectors
        }


def merge_databases(existing: Dict[str, Any], new: Dict[str, Any]) -> Dict[str, Any]:
    """Merge new MCU into existing database"""
    print_info("Merging into existing database...")

    # Keep existing family-level info
    result = existing.copy()

    # Add new MCUs
    new_mcu_name = list(new["mcus"].keys())[0]
    if new_mcu_name in result["mcus"]:
        print_warning(f"Overwriting existing MCU: {new_mcu_name}")

    result["mcus"][new_mcu_name] = new["mcus"][new_mcu_name]

    print_success(f"Merged {new_mcu_name}, total MCUs: {len(result['mcus'])}")
    return result


def main():
    parser = argparse.ArgumentParser(
        description="Parse CMSIS-SVD files to Alloy JSON database",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument('--input', '-i', required=True, type=Path,
                        help='Input SVD file')
    parser.add_argument('--output', '-o', required=True, type=Path,
                        help='Output JSON file')
    parser.add_argument('--merge', action='store_true',
                        help='Merge into existing database file')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    args = parser.parse_args()

    # Check input file
    if not args.input.exists():
        print_error(f"Input file not found: {args.input}")
        return 1

    # Parse SVD
    svd_parser = SVDParser(args.input, verbose=args.verbose)
    database = svd_parser.parse()

    # Merge if requested
    if args.merge and args.output.exists():
        try:
            with open(args.output) as f:
                existing = json.load(f)
            database = merge_databases(existing, database)
        except Exception as e:
            print_error(f"Failed to load existing database: {e}")
            return 1

    # Write output
    try:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, 'w') as f:
            json.dump(database, f, indent=2)
        print_success(f"Database written to {args.output}")
    except Exception as e:
        print_error(f"Failed to write output: {e}")
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
