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
from .build_service import (
    BuildService,
    BuildSystem,
    BuildStatus,
    BuildResult,
    BuildError,
    BuildProgress,
    SizeInfo,
)
from .flash_service import (
    FlashService,
    FlashTool,
    FlashStatus,
    FlashResult,
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
    "BuildService",
    "BuildSystem",
    "BuildStatus",
    "BuildResult",
    "BuildError",
    "BuildProgress",
    "SizeInfo",
    "FlashService",
    "FlashTool",
    "FlashStatus",
    "FlashResult",
]
