"""
Data models for Alloy CLI.

Pydantic models for type-safe database operations.
"""

from .mcu import MCU, MCUFamily, Memory, Package, PeripheralSet, Documentation
from .board import Board, BoardInfo, MCUReference, ClockConfig, Pinout, LED, Button
from .peripheral import Peripheral, PeripheralImplementation, PeripheralTests

__all__ = [
    # MCU models
    "MCU",
    "MCUFamily",
    "Memory",
    "Package",
    "PeripheralSet",
    "Documentation",
    # Board models
    "Board",
    "BoardInfo",
    "MCUReference",
    "ClockConfig",
    "Pinout",
    "LED",
    "Button",
    # Peripheral models
    "Peripheral",
    "PeripheralImplementation",
    "PeripheralTests",
]
