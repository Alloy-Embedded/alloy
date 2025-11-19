"""
Board data models using Pydantic.

Provides type-safe models for board metadata.
"""

from typing import List, Optional, Dict, Any
from pydantic import BaseModel, Field


class BoardInfo(BaseModel):
    """Board information."""
    id: str = Field(..., description="Board identifier")
    display_name: str = Field(..., description="Human-readable name")
    vendor: str = Field(..., description="Vendor name")
    url: Optional[str] = None
    description: Optional[str] = None


class MCUReference(BaseModel):
    """Reference to MCU used on board."""
    part_number: str = Field(..., description="MCU part number")
    family: str = Field(..., description="MCU family identifier")


class ClockConfig(BaseModel):
    """Clock configuration for board."""
    system_freq_hz: int = Field(..., ge=1, description="System clock frequency")
    xtal_freq_hz: int = Field(0, ge=0, description="External crystal frequency")
    has_pll: bool = Field(False, description="Has PLL")


class LED(BaseModel):
    """LED configuration."""
    name: str = Field(..., description="LED name")
    color: Optional[str] = None
    gpio: str = Field(..., description="GPIO pin")
    active: str = Field(..., description="Active level (high/low)")
    description: Optional[str] = None


class Button(BaseModel):
    """Button configuration."""
    name: str = Field(..., description="Button name")
    gpio: str = Field(..., description="GPIO pin")
    active: str = Field(..., description="Active level (high/low)")
    description: Optional[str] = None


class Debugger(BaseModel):
    """Debugger configuration."""
    type: str = Field(..., description="Debugger type")
    uart: Optional[Dict[str, str]] = None


class Pinout(BaseModel):
    """Board pinout configuration."""
    leds: List[LED] = Field(default_factory=list)
    buttons: List[Button] = Field(default_factory=list)
    debugger: Optional[Debugger] = None


class PeripheralPin(BaseModel):
    """Peripheral pin configuration."""
    model_config = {"extra": "allow"}


class Board(BaseModel):
    """Board configuration model."""
    schema_version: str = Field(..., description="Schema version")
    board: BoardInfo
    mcu: MCUReference
    clock: ClockConfig
    pinout: Pinout
    peripherals: Dict[str, Any] = Field(default_factory=dict)
    connectors: Dict[str, Any] = Field(default_factory=dict)
    examples: List[str] = Field(default_factory=list)
    status: str = Field("supported", description="Support status")
    tags: List[str] = Field(default_factory=list)

    @property
    def id(self) -> str:
        """Board ID."""
        return self.board.id

    @property
    def name(self) -> str:
        """Board display name."""
        return self.board.display_name

    def has_led(self) -> bool:
        """Check if board has LED."""
        return len(self.pinout.leds) > 0

    def get_led(self, name: Optional[str] = None) -> Optional[LED]:
        """Get LED by name, or first LED if name not specified."""
        if not self.pinout.leds:
            return None

        if name is None:
            return self.pinout.leds[0]

        for led in self.pinout.leds:
            if led.name == name:
                return led

        return None

    def has_button(self) -> bool:
        """Check if board has button."""
        return len(self.pinout.buttons) > 0

    def get_button(self, name: Optional[str] = None) -> Optional[Button]:
        """Get button by name, or first button if name not specified."""
        if not self.pinout.buttons:
            return None

        if name is None:
            return self.pinout.buttons[0]

        for button in self.pinout.buttons:
            if button.name == name:
                return button

        return None

    @classmethod
    def from_yaml(cls, data: Dict[str, Any]) -> "Board":
        """Create from YAML data."""
        return cls(**data)
