"""
Service layer for Alloy CLI.

Business logic for MCU discovery, board management, and project operations.
"""

from .mcu_service import MCUService
from .board_service import BoardService
from .metadata_service import MetadataService, MetadataType, ValidationResult
from .pin_recommendation import (
    PinRecommendationEngine,
    PinInfo,
    PinFunction,
    PinRecommendation,
    PinConfiguration,
    PinConflict,
    ConflictType,
    create_stm32_pin_database,
)

__all__ = [
    "MCUService",
    "BoardService",
    "MetadataService",
    "MetadataType",
    "ValidationResult",
    "PinRecommendationEngine",
    "PinInfo",
    "PinFunction",
    "PinRecommendation",
    "PinConfiguration",
    "PinConflict",
    "ConflictType",
    "create_stm32_pin_database",
]
