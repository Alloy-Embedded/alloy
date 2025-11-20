"""
Centralized path management for code generation

This module provides consistent path resolution for all generators,
ensuring files are always created in the correct vendor/family/mcu structure.

NOTE: This module now uses the centralized config.py for all settings.
"""

from pathlib import Path
from typing import Optional
import sys

# Add parent to path for imports
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from core.config import (
    REPO_ROOT,
    HAL_VENDORS_DIR,
    SVD_DIR,
    normalize_vendor as _normalize_vendor,
    normalize_name as _normalize_name,
    detect_family as _detect_family
)


def get_mcu_output_dir(vendor: str, family: str, mcu: Optional[str] = None) -> Path:
    """
    Get the output directory for a specific MCU following the standard structure.

    Args:
        vendor: Vendor name (e.g., "st", "microchip", "raspberrypi", "espressif")
        family: Family name (e.g., "stm32f1", "samd21", "rp2040", "esp32")
        mcu: Optional MCU name (e.g., "stm32f103c8", "atsamd21g18a")
             If None, returns family directory

    Returns:
        Path object for: src/hal/vendors/{vendor}/{family}/{mcu}/
        or src/hal/vendors/{vendor}/{family}/ if mcu is None

    Examples:
        >>> get_mcu_output_dir("st", "stm32f1", "stm32f103c8")
        PosixPath('src/hal/vendors/st/stm32f1/stm32f103c8')

        >>> get_mcu_output_dir("raspberrypi", "rp2040")
        PosixPath('src/hal/vendors/raspberrypi/rp2040')
    """
    vendor_lower = vendor.lower()
    family_lower = family.lower()

    if mcu:
        mcu_lower = mcu.lower()
        return HAL_VENDORS_DIR / vendor_lower / family_lower / mcu_lower
    else:
        return HAL_VENDORS_DIR / vendor_lower / family_lower


def get_vendor_dir(vendor: str) -> Path:
    """
    Get the vendor directory.

    Args:
        vendor: Vendor name (e.g., "st", "microchip")

    Returns:
        Path object for: src/hal/vendors/{vendor}/
    """
    return HAL_VENDORS_DIR / vendor.lower()


def get_family_dir(vendor: str, family: str) -> Path:
    """
    Get the family directory.

    Args:
        vendor: Vendor name
        family: Family name

    Returns:
        Path object for: src/hal/vendors/{vendor}/{family}/
    """
    return HAL_VENDORS_DIR / vendor.lower() / family.lower()


def get_generated_output_dir(vendor: str, family: str, mcu: Optional[str] = None) -> Path:
    """
    Get the /generated/ subdirectory for auto-generated code.

    This directory contains all auto-generated code (registers, bitfields, etc.)
    that should not be manually edited.

    Args:
        vendor: Vendor name (e.g., "st", "microchip", "raspberrypi")
        family: Family name (e.g., "stm32f1", "samd21", "rp2040")
        mcu: Optional MCU name for MCU-specific generated code

    Returns:
        Path object for: src/hal/vendors/{vendor}/{family}/generated/
        or src/hal/vendors/{vendor}/{family}/{mcu}/generated/

    Examples:
        >>> get_generated_output_dir("st", "stm32f4")
        PosixPath('src/hal/vendors/st/stm32f4/generated')

        >>> get_generated_output_dir("atmel", "same70", "atsame70q21b")
        PosixPath('src/hal/vendors/atmel/same70/atsame70q21b/generated')
    """
    base_dir = get_mcu_output_dir(vendor, family, mcu)
    return base_dir / "generated"


def normalize_vendor_name(vendor: str) -> str:
    """
    Normalize vendor names to standard form.

    NOTE: This now uses the centralized config.normalize_vendor()

    Args:
        vendor: Raw vendor name from SVD or other source

    Returns:
        Normalized vendor name

    Examples:
        >>> normalize_vendor_name("STMicroelectronics")
        'st'
        >>> normalize_vendor_name("Silicon Labs")
        'silabs'
    """
    return _normalize_vendor(vendor)


def normalize_family_name(family: str, vendor: str = None) -> str:
    """
    Normalize family names to standard form.

    NOTE: This now uses config.normalize_name()

    Args:
        family: Raw family name
        vendor: Vendor name (for vendor-specific rules, optional)

    Returns:
        Normalized family name

    Examples:
        >>> normalize_family_name("STM32F1", "st")
        'stm32f1'
        >>> normalize_family_name("SAMD21", "microchip")
        'samd21'
    """
    return _normalize_name(family)


def normalize_mcu_name(mcu: str) -> str:
    """
    Normalize MCU names to standard form.

    NOTE: This now uses config.normalize_name()

    Args:
        mcu: Raw MCU name

    Returns:
        Normalized MCU name (lowercase)

    Examples:
        >>> normalize_mcu_name("STM32F103C8")
        'stm32f103c8'
        >>> normalize_mcu_name("ATSAMD21G18A")
        'atsamd21g18a'
    """
    return _normalize_name(mcu)


def ensure_dir(path: Path) -> Path:
    """
    Ensure a directory exists, creating it if necessary.

    Args:
        path: Directory path

    Returns:
        The same path (for chaining)
    """
    path.mkdir(parents=True, exist_ok=True)
    return path
