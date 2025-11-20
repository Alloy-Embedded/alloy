"""
Central configuration for code generation

This module provides all configuration constants used across the code generator,
including vendor mappings, family detection patterns, board MCU lists, etc.
"""

from pathlib import Path
from typing import Dict, List, Set
import re


# ============================================================================
# PATHS
# ============================================================================

CODEGEN_DIR = Path(__file__).parent.parent
REPO_ROOT = CODEGEN_DIR.parent
SVD_DIR = CODEGEN_DIR / "upstream" / "cmsis-svd-data" / "data"
HAL_VENDORS_DIR = REPO_ROOT / "src" / "hal" / "vendors"
DATABASE_DIR = CODEGEN_DIR / "database" / "families"


# ============================================================================
# BOARD CONFIGURATIONS
# ============================================================================

# MCUs that have board configurations in our project
BOARD_MCUS: List[str] = [
    "ATSAMD21G18A",   # arduino_zero
    "STM32F103",      # bluepill (family match)
    "ESP32",          # esp32_devkit
    "RP2040",         # rp_pico / rp2040_zero
    "ATSAME70Q21",    # same70_xpld
    "ATSAMV71Q21",    # samv71_xult
    "STM32F407",      # stm32f407vg (family match)
    "STM32F746",      # stm32f746disco (F746 compatible)
]


# ============================================================================
# VENDOR NAME MAPPINGS
# ============================================================================

# Comprehensive vendor name normalization mapping
# Maps various vendor name forms to their canonical short names
VENDOR_NAME_MAP: Dict[str, str] = {
    # STMicroelectronics
    "stmicroelectronics": "st",
    "stmicro": "st",
    "st microelectronics": "st",

    # Microchip / Atmel
    "atmel": "atmel",
    "microchip": "atmel",
    "microchip technology": "atmel",
    "microchip technology inc.": "atmel",
    "microchip technology inc": "atmel",

    # NXP / Freescale
    "nxp": "nxp",
    "nxp semiconductors": "nxp",
    "nxp semiconductors n.v.": "nxp",
    "nxp.com": "nxp",
    "freescale": "nxp",
    "freescale semiconductor": "nxp",
    "freescale semiconductor, inc.": "nxp",
    "freescale semiconductor inc.": "nxp",

    # Nordic Semiconductor
    "nordic": "nordic",
    "nordic semiconductor": "nordic",
    "nordic semi": "nordic",

    # Texas Instruments
    "texas instruments": "ti",
    "texasinstruments": "ti",
    "ti": "ti",

    # Silicon Labs
    "silicon labs": "silabs",
    "silicon laboratories": "silabs",
    "silicon laboratories, inc.": "silabs",
    "siliconlabs": "silabs",
    "silabs": "silabs",

    # Espressif
    "espressif": "espressif",
    "espressif systems": "espressif",
    "espressif systems (shanghai) co., ltd.": "espressif",
    "espressif systems shanghai co ltd": "espressif",
    "espressif community": "espressif",
    "espressif-community": "espressif",

    # Raspberry Pi
    "raspberry pi": "raspberrypi",
    "raspberry pi ltd": "raspberrypi",
    "raspberry pi ltd.": "raspberrypi",
    "raspberrypi": "raspberrypi",

    # Others
    "toshiba": "toshiba",
    "holtek": "holtek",
    "holtek semiconductor inc.": "holtek",
    "nuvoton": "nuvoton",
    "nuvoton technology corporation": "nuvoton",
    "cypress": "cypress",
    "cypress semiconductor": "cypress",
    "infineon": "infineon",
    "infineon technologies": "infineon",
    "infineon technologies ag": "infineon",
    "renesas": "renesas",
    "renesas electronics": "renesas",
    "renesas electronics corporation": "renesas",
    "gigadevice": "gigadevice",
    "gigadevice semiconductor inc.": "gigadevice",
    "sifive": "sifive",
    "sifive, inc.": "sifive",
    "fujitsu": "fujitsu",
    "fujitsu electronics inc.": "fujitsu",
    "spansion": "spansion",
    "canaan": "canaan",
    "canaan inc.": "canaan",
    "wch": "wch",
    "wch.cn": "wch",
}


# ============================================================================
# FAMILY DETECTION PATTERNS
# ============================================================================

# Regex patterns for detecting MCU families
# Order matters - more specific patterns should come first
FAMILY_PATTERNS: List[tuple[re.Pattern, str, str]] = [
    # STM32 families - Very specific patterns
    (re.compile(r'stm32([a-z])(\d)', re.IGNORECASE), r'stm32\1\2', 'st'),
    (re.compile(r'stm32([a-z]\d+)', re.IGNORECASE), r'stm32\1', 'st'),

    # Microchip SAM families
    (re.compile(r'at(sam[edv]\d+)', re.IGNORECASE), r'\1', 'atmel'),
    (re.compile(r'(sam[edv]\d+)', re.IGNORECASE), r'\1', 'atmel'),

    # Nordic nRF families
    (re.compile(r'nrf(\d{2})', re.IGNORECASE), r'nrf\1', 'nordic'),

    # ESP32 families
    (re.compile(r'esp32[-_]?c3', re.IGNORECASE), r'esp32_c3', 'espressif'),
    (re.compile(r'esp32[-_]?c2', re.IGNORECASE), r'esp32_c2', 'espressif'),
    (re.compile(r'esp32[-_]?c6', re.IGNORECASE), r'esp32_c6', 'espressif'),
    (re.compile(r'esp32[-_]?s2', re.IGNORECASE), r'esp32_s2', 'espressif'),
    (re.compile(r'esp32[-_]?s3', re.IGNORECASE), r'esp32_s3', 'espressif'),
    (re.compile(r'esp32[-_]?h2', re.IGNORECASE), r'esp32_h2', 'espressif'),
    (re.compile(r'esp32[-_]?p4', re.IGNORECASE), r'esp32_p4', 'espressif'),
    (re.compile(r'esp32', re.IGNORECASE), r'esp32', 'espressif'),

    # Raspberry Pi
    (re.compile(r'rp(2040|2350)', re.IGNORECASE), r'rp\1', 'raspberrypi'),

    # NXP LPC families
    (re.compile(r'lpc(\d{2,4})', re.IGNORECASE), r'lpc\1', 'nxp'),

    # NXP Kinetis families
    (re.compile(r'mk(\d{2})', re.IGNORECASE), r'kinetis_k\1', 'nxp'),

    # TI MSP families
    (re.compile(r'msp(\d{3,4})', re.IGNORECASE), r'msp\1', 'ti'),

    # TI TM4C families
    (re.compile(r'tm4c(\d+)', re.IGNORECASE), r'tm4c\1', 'ti'),

    # Silicon Labs EFM/EFR families
    (re.compile(r'efm32([a-z]+\d+)', re.IGNORECASE), r'efm32\1', 'silabs'),
    (re.compile(r'efr32([a-z]+\d+)', re.IGNORECASE), r'efr32\1', 'silabs'),
]


# ============================================================================
# FILE TEMPLATES
# ============================================================================

# Files that should be generated for each MCU
GENERATED_FILES: Set[str] = {
    "startup.cpp",        # Startup code with vector table
    "peripherals.hpp",    # Peripheral definitions
    "pins.hpp",           # Pin definitions
    "gpio.hpp",           # GPIO configuration
    "hardware.hpp",       # Hardware abstractions
    "traits.hpp",         # Type traits for hardware
    "pin_functions.hpp",  # Pin function mappings
}

# Files that should be generated at family level
FAMILY_LEVEL_FILES: Set[str] = {
    "gpio_hal.hpp",       # GPIO HAL
    "clock.hpp",          # Clock configuration
    "delay.hpp",          # Delay functions
    "systick.hpp",        # SysTick timer
}


# ============================================================================
# GENERATION SETTINGS
# ============================================================================

# Whether to generate files for all MCUs or only board MCUs
GENERATE_ALL_MCUS: bool = False

# Whether to enable verbose logging by default
VERBOSE_DEFAULT: bool = False

# Whether to enable manifest tracking by default
MANIFEST_ENABLED: bool = True

# Manifest file name
MANIFEST_FILENAME: str = ".generated_manifest.json"


# ============================================================================
# HELPER FUNCTIONS
# ============================================================================

def normalize_name(name: str) -> str:
    """
    Normalize any name to a safe directory/file name.

    Converts to lowercase, removes special characters, replaces spaces
    with underscores, removes common domain suffixes.

    Args:
        name: Raw name to normalize

    Returns:
        Normalized name suitable for filesystem use

    Examples:
        >>> normalize_name("STM32F103")
        'stm32f103'
        >>> normalize_name("Example Vendor")
        'example_vendor'
        >>> normalize_name("NXP.com")
        'nxp'
    """
    # Convert to lowercase
    name = name.lower()

    # Remove common domain suffixes
    name = re.sub(r'\.(com|org|net|io|cn)$', '', name)

    # Replace spaces, dots, and other special chars with underscore
    name = re.sub(r'[^a-z0-9]+', '_', name)

    # Remove leading/trailing underscores
    name = name.strip('_')

    # Collapse multiple underscores
    name = re.sub(r'_+', '_', name)

    return name


def normalize_vendor(vendor: str) -> str:
    """
    Normalize vendor name to canonical form.

    Args:
        vendor: Raw vendor name from SVD or other source

    Returns:
        Canonical vendor name

    Examples:
        >>> normalize_vendor("STMicroelectronics")
        'st'
        >>> normalize_vendor("Microchip Technology Inc.")
        'atmel'
    """
    vendor_lower = vendor.lower().strip()

    # Try exact match first
    if vendor_lower in VENDOR_NAME_MAP:
        return VENDOR_NAME_MAP[vendor_lower]

    # Try partial match (check if vendor_lower contains any key)
    for key, normalized in VENDOR_NAME_MAP.items():
        if key in vendor_lower or vendor_lower in key:
            return normalized

    # Fall back to normalized form
    return normalize_name(vendor)


def detect_family(device_name: str, vendor: str = None) -> str:
    """
    Detect MCU family from device name.

    Args:
        device_name: Device/MCU name
        vendor: Optional vendor hint for better matching

    Returns:
        Detected family name

    Examples:
        >>> detect_family("STM32F103C8")
        'stm32f1'
        >>> detect_family("ATSAMD21G18A")
        'samd21'
        >>> detect_family("nRF52840")
        'nrf52'
    """
    device_lower = device_name.lower()
    vendor_norm = normalize_vendor(vendor) if vendor else None

    # Try patterns
    for pattern, replacement, pattern_vendor in FAMILY_PATTERNS:
        # If vendor is specified, only use patterns for that vendor
        if vendor_norm and pattern_vendor != vendor_norm:
            continue

        match = pattern.search(device_lower)
        if match:
            if '\\' in replacement:
                # Use match.expand() to handle backreferences like \1, \2
                result = match.expand(replacement)
                return result.lower()
            else:
                # Direct replacement
                return replacement.lower()

    # Fallback: use first alphanumeric segment
    parts = re.split(r'[^a-z0-9]+', device_lower)
    return parts[0] if parts else 'unknown'


def is_board_mcu(device_name: str) -> bool:
    """
    Check if device is configured as a board MCU.

    Args:
        device_name: Device/MCU name

    Returns:
        True if device is in BOARD_MCUS list
    """
    device_upper = device_name.upper()

    for board_mcu in BOARD_MCUS:
        if board_mcu in device_upper or device_upper.startswith(board_mcu):
            return True

    return False


def get_expected_files(mcu_dir: Path, is_family_dir: bool = False) -> Set[str]:
    """
    Get set of expected generated files for a directory.

    Args:
        mcu_dir: MCU or family directory
        is_family_dir: True if this is a family-level directory

    Returns:
        Set of expected file names
    """
    if is_family_dir:
        return FAMILY_LEVEL_FILES.copy()
    else:
        return GENERATED_FILES.copy()
