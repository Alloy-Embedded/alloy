"""
MCU data models using Pydantic.

Provides type-safe models for MCU metadata.
"""

from typing import List, Optional, Dict, Any
from pydantic import BaseModel, Field, HttpUrl


class Memory(BaseModel):
    """MCU memory configuration."""
    flash_kb: int = Field(..., ge=0, description="Flash memory in KB")
    sram_kb: int = Field(..., ge=0, description="SRAM memory in KB")
    eeprom_kb: int = Field(0, ge=0, description="EEPROM memory in KB")

    @property
    def flash_bytes(self) -> int:
        """Flash memory in bytes."""
        return self.flash_kb * 1024

    @property
    def sram_bytes(self) -> int:
        """SRAM memory in bytes."""
        return self.sram_kb * 1024


class Package(BaseModel):
    """MCU package information."""
    type: str = Field(..., description="Package type (e.g., LQFP64)")
    pins: int = Field(..., ge=1, description="Total number of pins")
    io_pins: int = Field(0, ge=0, description="Number of I/O pins")


class Documentation(BaseModel):
    """MCU documentation links."""
    datasheet: Optional[str] = None
    reference_manual: Optional[str] = None
    errata: Optional[str] = None
    programming_manual: Optional[str] = None
    svd_file: Optional[str] = None


class PeripheralSet(BaseModel):
    """Set of peripherals for an MCU."""
    # Allow any peripheral type with flexible structure
    model_config = {"extra": "allow"}


class MCU(BaseModel):
    """MCU configuration model."""
    part_number: str = Field(..., description="Official MCU part number")
    display_name: Optional[str] = None
    core: str = Field(..., description="CPU core (e.g., Cortex-M4F)")
    max_freq_mhz: int = Field(..., ge=1, description="Maximum frequency in MHz")

    memory: Memory
    package: Package
    peripherals: Dict[str, Any] = Field(default_factory=dict)
    documentation: Optional[Documentation] = None

    boards: List[str] = Field(default_factory=list, description="Board IDs using this MCU")
    status: str = Field("production", description="Production status")
    tags: List[str] = Field(default_factory=list, description="Tags for categorization")

    @property
    def name(self) -> str:
        """Convenient name property."""
        return self.display_name or self.part_number

    def has_peripheral(self, peripheral_type: str) -> bool:
        """Check if MCU has a specific peripheral type."""
        return peripheral_type in self.peripherals

    def get_peripheral_count(self, peripheral_type: str) -> int:
        """Get count of specific peripheral."""
        if peripheral_type not in self.peripherals:
            return 0

        peripheral = self.peripherals[peripheral_type]
        if isinstance(peripheral, dict):
            return peripheral.get("count", 0)

        return 0


class MCUFamily(BaseModel):
    """MCU family model."""
    id: str = Field(..., description="Family identifier")
    vendor: str = Field(..., description="Vendor identifier")
    display_name: str = Field(..., description="Human-readable name")
    description: Optional[str] = None
    core: str = Field(..., description="CPU core type")
    features: List[str] = Field(default_factory=list)

    mcus: List[MCU] = Field(default_factory=list)

    def get_mcu(self, part_number: str) -> Optional[MCU]:
        """Get MCU by part number."""
        for mcu in self.mcus:
            if mcu.part_number == part_number:
                return mcu
        return None

    def filter_mcus(
        self,
        min_flash: Optional[int] = None,
        min_sram: Optional[int] = None,
        with_peripheral: Optional[str] = None
    ) -> List[MCU]:
        """Filter MCUs by criteria."""
        filtered = self.mcus

        if min_flash is not None:
            filtered = [m for m in filtered if m.memory.flash_kb >= min_flash]

        if min_sram is not None:
            filtered = [m for m in filtered if m.memory.sram_kb >= min_sram]

        if with_peripheral is not None:
            filtered = [m for m in filtered if m.has_peripheral(with_peripheral)]

        return filtered


class MCUDatabase(BaseModel):
    """Complete MCU database model."""
    schema_version: str = Field(..., description="Schema version")
    family: MCUFamily

    @classmethod
    def from_yaml(cls, data: Dict[str, Any]) -> "MCUDatabase":
        """Create from YAML data."""
        return cls(**data)
