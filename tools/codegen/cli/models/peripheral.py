"""
Peripheral data models using Pydantic.

Provides type-safe models for peripheral metadata.
"""

from typing import List, Optional, Dict, Any
from pydantic import BaseModel, Field
from enum import Enum


class PeripheralType(str, Enum):
    """Peripheral type enumeration."""
    COMMUNICATION = "communication"
    ANALOG = "analog"
    DIGITAL = "digital"
    TIMER = "timer"
    SYSTEM = "system"


class ImplementationStatus(str, Enum):
    """Implementation status."""
    IMPLEMENTED = "implemented"
    IN_PROGRESS = "in_progress"
    PLANNED = "planned"
    NOT_SUPPORTED = "not_supported"


class APILevel(str, Enum):
    """API level enumeration."""
    SIMPLE = "simple"
    FLUENT = "fluent"
    EXPERT = "expert"


class PeripheralTests(BaseModel):
    """Peripheral test information."""
    unit_tests: bool = False
    hardware_tests: bool = False
    coverage_percent: int = Field(0, ge=0, le=100)


class PeripheralQuirk(BaseModel):
    """Hardware quirk or limitation."""
    description: str
    impact: Optional[str] = None
    workaround: Optional[str] = None


class PeripheralImplementation(BaseModel):
    """Peripheral implementation for a specific family."""
    family: str = Field(..., description="MCU family identifier")
    status: ImplementationStatus
    api_levels: List[APILevel] = Field(default_factory=list)
    features: List[str] = Field(default_factory=list)
    limitations: List[str] = Field(default_factory=list)
    template: Optional[str] = None
    tests: Optional[PeripheralTests] = None
    quirks: List[PeripheralQuirk] = Field(default_factory=list)


class PeripheralInfo(BaseModel):
    """Peripheral information."""
    id: str = Field(..., description="Peripheral identifier")
    display_name: str = Field(..., description="Human-readable name")
    type: PeripheralType
    description: Optional[str] = None


class PeripheralDocumentation(BaseModel):
    """Peripheral documentation."""
    api_reference: Optional[str] = None
    examples: List[str] = Field(default_factory=list)
    tutorials: List[str] = Field(default_factory=list)


class Peripheral(BaseModel):
    """Peripheral metadata model."""
    schema_version: str = Field(..., description="Schema version")
    peripheral: PeripheralInfo
    implementations: List[PeripheralImplementation] = Field(default_factory=list)
    documentation: Optional[PeripheralDocumentation] = None
    usage_examples: Dict[str, str] = Field(default_factory=dict)
    performance: Dict[str, Any] = Field(default_factory=dict)

    def get_implementation(self, family: str) -> Optional[PeripheralImplementation]:
        """Get implementation for specific family."""
        for impl in self.implementations:
            if impl.family == family:
                return impl
        return None

    def is_implemented_for(self, family: str) -> bool:
        """Check if peripheral is implemented for family."""
        impl = self.get_implementation(family)
        return impl is not None and impl.status == ImplementationStatus.IMPLEMENTED

    def has_api_level(self, family: str, api_level: APILevel) -> bool:
        """Check if specific API level is available for family."""
        impl = self.get_implementation(family)
        if impl is None:
            return False
        return api_level in impl.api_levels

    @classmethod
    def from_yaml(cls, data: Dict[str, Any]) -> "Peripheral":
        """Create from YAML data."""
        return cls(**data)
