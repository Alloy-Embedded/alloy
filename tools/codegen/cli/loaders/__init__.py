"""
Database loaders for Alloy CLI.

This module provides loaders for different database formats (JSON, YAML)
with automatic format detection.
"""

from .yaml_loader import YAMLDatabaseLoader
from .database_loader import DatabaseLoader, auto_detect_loader

__all__ = [
    "YAMLDatabaseLoader",
    "DatabaseLoader",
    "auto_detect_loader",
]
